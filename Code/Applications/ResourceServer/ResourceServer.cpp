#include "ResourceServer.h"
#include "EngineTools/Resource/ResourceCompiler.h"
#include "EngineTools/Resource/ResourceCompilerNetworkMessages.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/Resource/ResourceCompilerRegistry.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Base/Resource/ResourceProviders/ResourceNetworkMessages.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/Profiling.h"

#include "Game/_Module/GameModule.h"
#include "Engine/_Module/EngineModule.h"
#include "Base/_Module/BaseModule.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceServer::ResourceServer( TypeSystem::TypeRegistry const& typeRegistry )
        : m_context( typeRegistry )
    {}

    bool ResourceServer::Initialize( FileSystem::Path const& iniFilePath )
    {
        if ( !m_context.Initialize( iniFilePath ) )
        {
            return false;
        }

        // File System
        //-------------------------------------------------------------------------

        m_fileSystemWatcher.StartWatching( m_context.GetSourceDataDirectory() );

        // Create Workers
        //-------------------------------------------------------------------------

        int32_t const numWorkersToCreate = true ? Threading::GetProcessorInfo().m_numPhysicalCores - 1 : 1;
        m_workers.reserve( numWorkersToCreate );
        for ( int32_t i = 0; i < numWorkersToCreate; i++ )
        {
            m_workers.emplace_back( m_context, i + 1 );
        }

        // Packaging
        //-------------------------------------------------------------------------

        RefreshAvailableMapList();

        // Tools
        //-------------------------------------------------------------------------

        m_pDataFileResaver = EE::New<DataFileResaver>( m_context.m_typeRegistry, m_context.GetSourceDataDirectory() );

        return true;
    }

    void ResourceServer::Shutdown()
    {
        m_context.m_isExiting = true;

        // Complete all scheduled requests
        //-------------------------------------------------------------------------

        m_context.m_taskSystem.WaitForAll();
        UpdateWorkers();

        // Requests
        //-------------------------------------------------------------------------

        m_pendingRequests.clear();

        for ( auto pRequest : m_requests )
        {
            EE::Delete( pRequest );
        }
        m_requests.clear();

        // Tools
        //-------------------------------------------------------------------------

        EE::Delete( m_pDataFileResaver );

        // Unregister File Watcher
        //-------------------------------------------------------------------------

        if ( m_fileSystemWatcher.IsWatching() )
        {
            m_fileSystemWatcher.StopWatching();
        }

        //-------------------------------------------------------------------------

        m_context.Shutdown();
    }

    //-------------------------------------------------------------------------

    void ResourceServer::Update()
    {
        UpdateNetwork();
        UpdateResaveOfDataFiles();
        UpdateFileSystemWatcher();
        UpdatePackaging();
        UpdateRecompilationBlockers();

        ProcessPendingRequests();
        UpdateWorkers();
        CleanupCompletedRequests();

        m_context.m_networkServer.SendOutgoingMessages();
    }

    bool ResourceServer::IsBusy() const
    {
        if ( IsPackaging() || !m_pendingRequests.empty() )
        {
            return true;
        }

        for ( auto const& worker : m_workers )
        {
            if ( worker.IsBusy() )
            {
                return true;
            }
        }

        return false;
    }

    TInlineVector<ResourceServerWorker const*, 32> ResourceServer::GetWorkers() const
    {
        TInlineVector<ResourceServerWorker const*, 32> workers;
        for ( auto const& w : m_workers )
        {
            workers.emplace_back( &w );
        }
        return workers;
    }

    TInlineVector<Network::Server::ClientInfo const*, 32> ResourceServer::GetConnectedClients() const
    {
        TInlineVector<Network::Server::ClientInfo const*, 32> clients;
        for ( auto const& ci : m_context.m_networkServer.GetConnectedClients() )
        {
            for ( uint64_t clientID : m_context.m_nonWorkerClientIDs )
            {
                if ( ci.m_ID == clientID )
                {
                    clients.emplace_back( &ci );
                    break;
                }
            }
        }
        return clients;
    }

    //-------------------------------------------------------------------------

    bool ResourceServer::DeleteCompiledResourceDatabase()
    {
        EE_ASSERT( !IsBusy() );
        m_context.m_compileResourceDB.ResetDatabase();
        return true;
    }

    // Updates
    //-------------------------------------------------------------------------

    void ResourceServer::UpdateNetwork()
    {
        EE_PROFILE_FUNCTION();

        // Process received messages
        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE( "Receive Messages" );

            auto ProcessIncomingMessages = [this] ( Network::Message const& message )
            {
                int32_t const messageID = message.GetMessageID();

                if ( messageID == (int32_t) NetworkMessageID::RequestResource )
                {
                    uint64_t const clientID = message.GetClientID();
                    NetworkResourceRequest networkRequest = message.GetPayload<NetworkResourceRequest>();
                    for ( ResourceID const& requestedResourceID : networkRequest.m_resourceIDs )
                    {
                        TryAddPendingRequest( clientID, RequestOrigin::Network, requestedResourceID );
                    }
                }

                //-------------------------------------------------------------------------

                if ( messageID == (int32_t) NetworkMessageID::AddRecompilationBlocker )
                {
                    uint64_t const clientID = message.GetClientID();
                    NetworkResourceRecompilationBlockerRequest networkRequest = message.GetPayload<NetworkResourceRecompilationBlockerRequest>();
                    if ( networkRequest.m_ID.IsValid() && networkRequest.m_path.IsValid() && networkRequest.m_timeToBlock > 0 )
                    {
                        AddRecompilationBlocker( networkRequest.m_ID, networkRequest.m_path, networkRequest.m_timeToBlock, String( String::CtorSprintf(), "Network Request: ", clientID ) );
                    }
                }

                //-------------------------------------------------------------------------

                if ( messageID == (int32_t) NetworkMessageID::RemoveRecompilationBlocker )
                {
                    uint64_t const clientID = message.GetClientID();
                    NetworkResourceRecompilationBlockerRequest networkRequest = message.GetPayload<NetworkResourceRecompilationBlockerRequest>();
                    if ( networkRequest.m_ID.IsValid() )
                    {
                        RemoveRecompilationBlocker( networkRequest.m_ID );
                    }
                }

                //-------------------------------------------------------------------------

                if ( messageID == (int32_t) NetworkResourceWorkerMessageID::ID )
                {
                    EE_PROFILE_SCOPE_RESOURCE( "Worker Connected" );

                    uint64_t const clientID = message.GetClientID();
                    NetworkResourceCompilerID networkID = message.GetPayload<NetworkResourceCompilerID>();
                    for ( auto& worker : m_workers )
                    {
                        if ( worker.m_workerID == networkID.m_workerID )
                        {
                            worker.m_networkClientID = clientID;
                            break;
                        }
                    }
                }

                //-------------------------------------------------------------------------

                if ( messageID == (int32_t) NetworkResourceWorkerMessageID::CompileRequestHeartbeat )
                {
                    uint64_t const clientID = message.GetClientID();
                    for ( auto& worker : m_workers )
                    {
                        if ( worker.m_networkClientID == clientID )
                        {
                            worker.HandleHeartbeatMessage( message.GetPayload<NetworkResourceCompilerRequest>() );
                        }
                    }
                }

                //-------------------------------------------------------------------------

                if ( messageID == (int32_t) NetworkResourceWorkerMessageID::CompileRequestComplete )
                {
                    uint64_t const clientID = message.GetClientID();
                    for ( auto& worker : m_workers )
                    {
                        if ( worker.m_networkClientID == clientID )
                        {
                            worker.HandleCompletionMessage( message.GetPayload<NetworkResourceCompilerResponse>() );
                        }
                    }
                }
            };

            m_context.m_networkServer.Update( ProcessIncomingMessages );
        }

        // Update client lists
        //-------------------------------------------------------------------------

        m_context.m_nonWorkerClientIDs.clear();
        for ( int32_t i = 0; i < m_context.m_networkServer.GetNumConnectedClients(); i++ )
        {
            m_context.m_nonWorkerClientIDs.emplace_back( m_context.m_networkServer.GetConnectedClientInfo( i ).m_ID );
        }

        for ( auto const& worker : m_workers )
        {
            m_context.m_nonWorkerClientIDs.erase_first_unsorted( worker.m_networkClientID );
        }
    }

    void ResourceServer::UpdateFileSystemWatcher()
    {
        EE_PROFILE_FUNCTION();

        if ( !m_fileSystemWatcher.IsWatching() )
        {
            return;
        }

        if ( m_fileSystemWatcher.Update() )
        {
            for ( FileSystem::Watcher::Event const& fsEvent : m_fileSystemWatcher.GetFileSystemChangeEvents() )
            {
                if ( fsEvent.IsDirectoryEvent() )
                {
                    continue;
                }

                //-------------------------------------------------------------------------

                EE_ASSERT( fsEvent.m_path.IsValid() && fsEvent.m_path.IsFilePath() );

                DataPath modifiedFileDataPath = DataPath( fsEvent.m_path, m_context.m_pSettings->m_sourceDataDirectoryPath );
                if ( !modifiedFileDataPath.IsValid() )
                {
                    return;
                }

                // Check if this is a resource ID, if so then just notify everyone that something has changed
                FileSystem::Extension const extension = modifiedFileDataPath.GetExtension();
                if ( !extension.empty() && ResourceTypeID::IsValidResourceTypeIdentifierString( extension ) )
                {
                    ResourceID const resourceID( modifiedFileDataPath );
                    if ( resourceID.IsValid() )
                    {
                        // If this is a resource, then recompile it
                        //-------------------------------------------------------------------------

                        TypeSystem::ResourceInfo const* pResourceInfo = m_context.m_typeRegistry.GetResourceInfo( resourceID.GetResourceTypeID() );
                        if ( pResourceInfo != nullptr )
                        {
                            if ( !IsResourceRecompilationBlocked( resourceID ) )
                            {
                                TryAddPendingRequest( RequestOrigin::FileWatcher, resourceID, "External file system change detected!" );
                            }
                            else
                            {
                                VectorEmplaceBackUnique( m_blockedRecompilationRequests, resourceID );
                            }
                        }

                        // Recompile any dependent resources
                        //-------------------------------------------------------------------------

                        TVector<ResourceID> dependentResources;
                        if ( m_context.m_compileResourceDB.GetResourcesThatDependsOnPath( modifiedFileDataPath, dependentResources ) )
                        {
                            for ( ResourceID const& dependentResourceID : dependentResources )
                            {
                                if ( !IsResourceRecompilationBlocked( dependentResourceID ) )
                                {
                                    TryAddPendingRequest( RequestOrigin::FileWatcher, dependentResourceID, "External file system change detected!" );
                                }
                                else
                                {
                                    VectorEmplaceBackUnique( m_blockedRecompilationRequests, dependentResourceID );
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Requests And Tasks
    //-------------------------------------------------------------------------

    Request* ResourceServer::TryAddPendingRequest( uint64_t clientID, RequestOrigin origin, ResourceID const& resourceID, String const& extraInfo )
    {
        // For any automatic requests, ensure that we dont register multiple of these simultaneously
        if ( origin == RequestOrigin::FileWatcher )
        {
            for ( Request const* pExistingRequest : m_pendingRequests )
            {
                if ( pExistingRequest->m_resourceID == resourceID && pExistingRequest->m_origin == origin )
                {
                    return nullptr;
                }
            }
        }

        return m_pendingRequests.emplace_back( EE::New<Request>( m_context, clientID, origin, resourceID, extraInfo ) );
    }

    void ResourceServer::ProcessPendingRequests()
    {
        EE_PROFILE_FUNCTION();

        auto FindTaskForRequest = [this] ( Request const* pRequest ) -> WorkerTask*
        {
            for ( ResourceServerWorker& worker : m_workers )
            {
                if ( WorkerTask* pTask = worker.TryFindTaskMatchingRequest( pRequest ) )
                {
                    return pTask;
                }
            }

            return nullptr;
        };

        //-------------------------------------------------------------------------

        bool requestsUpdated = false;

        for ( uint32_t i = 0; i < m_pendingRequests.size(); i++ )
        {
            Request* pRequest = m_pendingRequests[i];

            // Add to overall request list
            m_requests.emplace_back( pRequest );
            requestsUpdated = true;

            // Dont schedule invalid requests
            if ( pRequest->m_status == RequestStatus::Failed )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            // Do we already have a task for this request?
            if ( WorkerTask* pTask = FindTaskForRequest( pRequest ) )
            {
                pTask->AddRequest( pRequest );
            }
            else // Schedule a new task
            {
                ScheduleWorkerTask( pRequest );
            }
        }

        if ( requestsUpdated )
        {
            m_requestsUpdatedEvent.Execute();
        }

        m_pendingRequests.clear();
    }

    void ResourceServer::ScheduleWorkerTask( Request* pRequest )
    {
        // Currently a dumb sequential scheduling...
        // TODO: better load balancing!!!
        bool const isForcedCompilation = pRequest->m_origin == RequestOrigin::ManualCompileForced;
        auto pTask = m_workers[m_workerEnqueueIndex].CreateTask( pRequest->m_resourceID, pRequest->IsPackagingRequest(), isForcedCompilation );
        m_workerEnqueueIndex = ( m_workerEnqueueIndex + 1 ) % int32_t( m_workers.size() );
        pTask->AddRequest( pRequest );
    }

    void ResourceServer::UpdateWorkers()
    {
        for ( auto& worker : m_workers )
        {
            worker.Update();
        }
    }

    void ResourceServer::CleanupCompletedRequests()
    {
        EE_PROFILE_FUNCTION();

        if ( !m_cleanupRequested )
        {
            return;
        }

        bool requestsUpdated = false;

        for ( int32_t i = int32_t( m_requests.size() ) - 1; i >= 0; i-- )
        {
            if ( m_requests[i]->IsComplete() )
            {
                EE::Delete( m_requests[i] );
                m_requests.erase( m_requests.begin() + i );
                requestsUpdated = true;
            }
        }

        if ( requestsUpdated )
        {
            m_requestsUpdatedEvent.Execute();
        }

        m_cleanupRequested = false;
    }

    // Packaging
    //-------------------------------------------------------------------------

    void ResourceServer::RefreshAvailableMapList()
    {
        m_allMaps.clear();

        TVector<FileSystem::Path> results;
        if ( FileSystem::GetDirectoryContents( m_context.GetSourceDataDirectory(), results, FileSystem::DirectoryReaderOutput::OnlyFiles, FileSystem::DirectoryReaderMode::Recursive, { "map" } ) )
        {
            for ( auto const& foundMapPath : results )
            {
                m_allMaps.emplace_back( ResourceID( DataPath( foundMapPath, m_context.GetSourceDataDirectory() ) ) );
            }
        }
    }

    void ResourceServer::AddMapToPackagingList( ResourceID mapResourceID )
    {
        EE_ASSERT( mapResourceID.GetResourceTypeID() == EntityModel::EntityMapDescriptor::GetStaticResourceTypeID() );
        VectorEmplaceBackUnique( m_mapsToBePackaged, mapResourceID );
    }

    void ResourceServer::RemoveMapFromPackagingList( ResourceID mapResourceID )
    {
        EE_ASSERT( mapResourceID.GetResourceTypeID() == EntityModel::EntityMapDescriptor::GetStaticResourceTypeID() );
        m_mapsToBePackaged.erase_first_unsorted( mapResourceID );
    }

    bool ResourceServer::CanStartPackaging() const
    {
        return ( m_packagingStage == PackagingStage::None || m_packagingStage == PackagingStage::Complete ) && !m_mapsToBePackaged.empty();
    }

    void ResourceServer::StartPackaging()
    {
        EE_ASSERT( CanStartPackaging() );
        m_context.m_taskSystem.ScheduleTask( &m_packagingTask );
        m_packagingStage = PackagingStage::Preparing;
    }

    float ResourceServer::GetPackagingProgress() const
    {
        switch ( m_packagingStage )
        {
            case PackagingStage::None:
            {
                return 1.0f;
            }
            break;

            case PackagingStage::Preparing:
            {
                return 0.1f;
            }
            break;

            case PackagingStage::Packaging:
            {
                float numComplete = 0.0f;
                for ( auto pRequest : m_packagingRequests )
                {
                    if ( pRequest->IsComplete() )
                    {
                        numComplete++;
                    }
                }

                float const percentageComplete = numComplete / m_packagingRequests.size();
                return 0.05f + ( 0.95f * percentageComplete );
            }
            break;

            case PackagingStage::Complete:
            {
                return 1.0f;
            }
            break;
        }

        return 0.0f;
    }

    void ResourceServer::UpdatePackaging()
    {
        EE_PROFILE_FUNCTION();

        if ( m_packagingStage == PackagingStage::Preparing )
        {
            if ( m_packagingTask.GetIsComplete() )
            {
                for ( auto const& resourceID : m_packagingRuntimeDependencies )
                {
                    m_packagingRequests.emplace_back( TryAddPendingRequest( RequestOrigin::Package, resourceID ) );
                }

                m_packagingStage = PackagingStage::Packaging;
            }
        }
        else if ( m_packagingStage == PackagingStage::Packaging )
        {
            bool isComplete = true;

            for ( auto pRequest : m_packagingRequests )
            {
                if ( !pRequest->IsComplete() )
                {
                    isComplete = false;
                    break;
                }
            }

            if ( isComplete )
            {
                m_packagingRequests.clear();
                m_packagingStage = PackagingStage::Complete;
            }
        }

    }

    void ResourceServer::RunPackagingTask()
    {
        BaseModule baseModule;
        for ( auto pResourcePtr : baseModule.GetModuleResources() )
        {
            m_packagingRuntimeDependencies.emplace_back( pResourcePtr->GetResourceID() );
        }

        EngineModule engineModule;
        for ( auto pResourcePtr : engineModule.GetModuleResources() )
        {
            m_packagingRuntimeDependencies.emplace_back( pResourcePtr->GetResourceID() );
        }

        GameModule gameModule;
        for ( auto pResourcePtr : gameModule.GetModuleResources() )
        {
            m_packagingRuntimeDependencies.emplace_back( pResourcePtr->GetResourceID() );
        }

        //-------------------------------------------------------------------------

        for ( auto const& mapID : m_mapsToBePackaged )
        {
            if ( !mapID.IsValid() )
            {
                continue;
            }

            EnqueueResourceForPackaging( mapID );
        }
    }

    void ResourceServer::EnqueueResourceForPackaging( ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );

        if ( m_context.m_isExiting )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto pCompiler = m_context.m_pCompilerRegistry->GetCompilerForResourceType( resourceID.GetResourceTypeID() );
        if ( pCompiler != nullptr )
        {
            // Add resource for packaging
            VectorEmplaceBackUnique( m_packagingRuntimeDependencies, resourceID );

            // Read the descriptor
            FileSystem::Path const descriptorFilePath = resourceID.GetFileSystemPath( m_context.GetSourceDataDirectory() );
            if ( !descriptorFilePath.Exists() )
            {
                return;
            }

            Log log( LogCategory::Resource, "Resource Server - Read Descriptor" );
            ResourceDescriptor* pDescriptor = ResourceDescriptor::TryReadFromFile( m_context.m_typeRegistry, log, descriptorFilePath );
            if ( pDescriptor == nullptr )
            {
                return;
            }

            // Get sub-resource name
            String subResourceName;
            if ( resourceID.IsSubResourceID() )
            {
                subResourceName = resourceID.GetSubResourceName();
            }

            // Get all runtime install dependencies
            TVector<ResourceID> referencedResources;
            pDescriptor->GetInstallDependencies( m_context.m_typeRegistry, m_context.GetSourceDataDirectory(), subResourceName, referencedResources );

            // Recursively enqueue all referenced resources
            for ( auto const& referenceResourceID : referencedResources )
            {
                EnqueueResourceForPackaging( referenceResourceID );
            }

            // Free the descriptor
            EE::Delete( pDescriptor );
        }
    }

    // Misc Tools
    //-------------------------------------------------------------------------

    void ResourceServer::RequestResaveOfDataFiles()
    {
        EE_ASSERT( m_pDataFileResaver != nullptr );
        StartResaveOfDataFiles();
    }

    bool ResourceServer::IsResavingDataFiles() const
    {
        EE_ASSERT( m_pDataFileResaver != nullptr );
        return m_pDataFileResaver->IsResaving();
    }

    float ResourceServer::GetDataFileResaveProgress() const
    {
        EE_ASSERT( m_pDataFileResaver != nullptr );
        return m_pDataFileResaver->GetProgress().ToFloat();
    }

    void ResourceServer::StartResaveOfDataFiles()
    {
        EE_ASSERT( m_pDataFileResaver != nullptr );
        EE_ASSERT( !IsResavingDataFiles() );
        m_fileSystemWatcher.StopWatching();
        m_pDataFileResaver->BeginResave();
    }

    void ResourceServer::EndResaveOfDataFiles()
    {
        EE_ASSERT( IsResavingDataFiles() );
        m_pDataFileResaver->EndResave();
        m_fileSystemWatcher.StartWatching( m_context.GetSourceDataDirectory() );
    }

    void ResourceServer::UpdateResaveOfDataFiles()
    {
        EE_PROFILE_FUNCTION();

        if ( !m_pDataFileResaver->IsResaving() )
        {
            return;
        }

        m_pDataFileResaver->UpdateResave( 10 );

        if ( m_pDataFileResaver->GetProgress() == 1.0f )
        {
            EndResaveOfDataFiles();
        }
    }

    // Recompilation Blocking
    //-------------------------------------------------------------------------

    bool ResourceServer::IsResourceRecompilationBlocked( ResourceID const& ID ) const
    {
        for ( auto const& blocker : m_recompilationBlockers )
        {
            if ( ID.GetDataPath() == blocker.m_path )
            {
                return true;
            }

            if ( ID.GetDataPath().IsUnder( blocker.m_path ) )
            {
                return true;
            }
        }

        return false;
    }

    void ResourceServer::AddRecompilationBlocker( UUID const& blockerID, DataPath const& path, Seconds timeToBlock, String const& optionalSourceID )
    {
        EE_ASSERT( blockerID.IsValid() );
        EE_ASSERT( path.IsValid() );
        EE_ASSERT( timeToBlock >= 0 );

        for ( auto& blocker : m_recompilationBlockers )
        {
            if ( blocker.m_ID == blockerID )
            {
                EE_LOG_WARNING( LogCategory::Resource, "Resource Server", "Requested a duplicate recompilation block! ID already registered!" );
                return;
            }
        }

        auto& blocker = m_recompilationBlockers.emplace_back();
        blocker.m_ID = blockerID;
        blocker.m_path = path;
        blocker.m_timer.Start( timeToBlock );
        blocker.m_sourceID = optionalSourceID;
    }

    void ResourceServer::RefreshRecompilationBlocker( UUID const& blockerID, Seconds timeToBlock )
    {
        for ( auto& blocker : m_recompilationBlockers )
        {
            if ( blocker.m_ID == blockerID )
            {
                blocker.m_timer.Start( timeToBlock );
                return;
            }
        }
    }

    void ResourceServer::RemoveRecompilationBlocker( UUID const& blockerID )
    {
        for ( int32_t i = 0; i < (int32_t) m_recompilationBlockers.size(); i++ )
        {
            if ( m_recompilationBlockers[i].m_ID == blockerID )
            {
                m_recompilationBlockers.erase_unsorted( m_recompilationBlockers.begin() + i );
                return;
            }
        }
    }

    void ResourceServer::UpdateRecompilationBlockers()
    {
        EE_PROFILE_FUNCTION();

        for ( int32_t i = 0; i < (int32_t) m_recompilationBlockers.size(); i++ )
        {
            if ( m_recompilationBlockers[i].m_timer.Update() )
            {
                m_recompilationBlockers.erase_unsorted( m_recompilationBlockers.begin() + i );
                i--;
            }
        }

        for ( int32_t i = 0; i < (int32_t) m_blockedRecompilationRequests.size(); i++ )
        {
            ResourceID const& resourceID = m_blockedRecompilationRequests[i];
            if ( !IsResourceRecompilationBlocked( resourceID ) )
            {
                TryAddPendingRequest( RequestOrigin::UnblockedRecompilationRequest, resourceID, "Previously blocked recompilation request!" );
                m_blockedRecompilationRequests.erase_unsorted( m_blockedRecompilationRequests.begin() + i );
                i--;
            }
        }
    }
}