#include "ResourceSystem.h"
#include "ResourceProvider.h"
#include "ResourceRequest.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceSystem::ResourceSystem( TaskSystem& taskSystem )
        : m_taskSystem( taskSystem )
        , m_asyncProcessingTask( [this] ( TaskSetPartition range, uint32_t threadnum ) { ProcessResourceRequests(); } )
    {}

    ResourceSystem::~ResourceSystem()
    {
        EE_ASSERT( m_pResourceProvider == nullptr && !IsBusy() && m_resourceRecords.empty() );
    }

    ResourceGlobalSettings const& ResourceSystem::GetSettings() const
    {
        return m_pResourceProvider->GetSettings();
    }

    void ResourceSystem::Initialize( ResourceProvider* pResourceProvider )
    {
        EE_ASSERT( pResourceProvider != nullptr && pResourceProvider->IsReady() );
        m_pResourceProvider = pResourceProvider;
    }

    void ResourceSystem::Shutdown()
    {
        WaitForAllRequestsToComplete();
        m_pResourceProvider = nullptr;
    }

    bool ResourceSystem::IsBusy() const
    {
        if ( m_isAsyncTaskRunning )
        {
            return true;
        }

        if ( !m_activeRequests.empty() )
        {
            return true;
        }

        if ( !m_pendingRequests.empty() )
        {
            return true;
        }

        return false;
    }

    //-------------------------------------------------------------------------

    void ResourceSystem::GetUsersForResource( ResourceRecord const* pResourceRecord, TInlineVector<ResourceRequesterID, 20>& userIDs ) const
    {
        EE_ASSERT( pResourceRecord != nullptr );
        Threading::RecursiveScopeLock lock( m_accessLock );

        for ( auto const& requesterID : pResourceRecord->m_references )
        {
            // Internal user i.e. install dependency
            if ( requesterID.IsInstallDependencyRequest() )
            {
                uint32_t const resourcePathID( requesterID.GetInstallDependencyResourcePathID() );
                auto const dependentRecordIter = m_resourceRecords.find_as( resourcePathID );
                EE_ASSERT( dependentRecordIter != m_resourceRecords.end() );

                ResourceRecord* pFoundDependentRecord = dependentRecordIter->second;
                GetUsersForResource( pFoundDependentRecord, userIDs );
            }
            else // Actual external user
            {
                // Skip manual requests
                if ( requesterID.IsManualRequest() )
                {
                    continue;
                }

                // Add unique users to the list
                VectorEmplaceBackUnique( userIDs, requesterID );
            }
        }
    }

    void ResourceSystem::GetDependentResourcesForResource( ResourceRecord const* pResourceRecord, TInlineVector<ResourceID, 20>& dependentResources ) const
    {
        EE_ASSERT( pResourceRecord != nullptr );
        Threading::RecursiveScopeLock lock( m_accessLock );

        for ( auto const& requesterID : pResourceRecord->m_references )
        {
            if ( requesterID.IsInstallDependencyRequest() )
            {
                uint32_t const resourcePathID( requesterID.GetInstallDependencyResourcePathID() );
                auto const dependentRecordIter = m_resourceRecords.find_as( resourcePathID );
                EE_ASSERT( dependentRecordIter != m_resourceRecords.end() );

                ResourceRecord* pFoundDependentRecord = dependentRecordIter->second;
                dependentResources.emplace_back( pFoundDependentRecord->m_resourceID );
                GetDependentResourcesForResource( pFoundDependentRecord, dependentResources );
            }
        }
    }

    //-------------------------------------------------------------------------

    void ResourceSystem::RegisterResourceLoader( ResourceLoader* pLoader )
    {
        auto& loadableTypes = pLoader->GetLoadableTypes();
        for ( auto& type : loadableTypes )
        {
            auto loaderIter = m_resourceLoaders.find( type );
            EE_ASSERT( loaderIter == m_resourceLoaders.end() ); // Catch duplicate registrations
            m_resourceLoaders[type] = pLoader;
        }
    }

    void ResourceSystem::UnregisterResourceLoader( ResourceLoader* pLoader )
    {
        auto& loadableTypes = pLoader->GetLoadableTypes();
        for ( auto& type : loadableTypes )
        {
            auto loaderIter = m_resourceLoaders.find( type );
            EE_ASSERT( loaderIter != m_resourceLoaders.end() ); // Catch invalid unregistrations
            m_resourceLoaders.erase( loaderIter );
        }
    }

    //-------------------------------------------------------------------------

    ResourceRecord* ResourceSystem::FindOrCreateResourceRecord( ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );
        Threading::RecursiveScopeLock lock( m_accessLock );

        ResourceRecord* pRecord = nullptr;
        auto const recordIter = m_resourceRecords.find( resourceID );
        if ( recordIter == m_resourceRecords.end() )
        {
            pRecord = EE::New<ResourceRecord>( resourceID );
            m_resourceRecords[resourceID] = pRecord;
        }
        else
        {
            pRecord = recordIter->second;
        }

        return pRecord;
    }

    ResourceRecord* ResourceSystem::FindExistingResourceRecord( ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );
        Threading::RecursiveScopeLock lock( m_accessLock );

        auto const recordIter = m_resourceRecords.find( resourceID );
        EE_ASSERT( recordIter != m_resourceRecords.end() );
        return recordIter->second;
    }

    void ResourceSystem::LoadResource( ResourcePtr& resourcePtr, ResourceRequesterID const& requesterID )
    {
        Threading::RecursiveScopeLock lock( m_accessLock );

        // Immediately update the resource ptr
        auto pRecord = FindOrCreateResourceRecord( resourcePtr.GetResourceID() );
        resourcePtr.m_pResourceRecord = pRecord;

        //-------------------------------------------------------------------------

        if ( !pRecord->HasReferences() )
        {
            AddPendingRequest( PendingRequest( PendingRequest::Type::Load, pRecord, requesterID ) );
        }

        pRecord->AddReference( requesterID );
    }

    void ResourceSystem::UnloadResource( ResourcePtr& resourcePtr, ResourceRequesterID const& requesterID )
    {
        Threading::RecursiveScopeLock lock( m_accessLock );

        // Immediately update the resource ptr
        resourcePtr.m_pResourceRecord = nullptr;

        //-------------------------------------------------------------------------

        auto pRecord = FindExistingResourceRecord( resourcePtr.GetResourceID() );
        pRecord->RemoveReference( requesterID );

        if ( !pRecord->HasReferences() )
        {
            AddPendingRequest( PendingRequest( PendingRequest::Type::Unload, pRecord, requesterID ) );
        }
    }

    void ResourceSystem::AddPendingRequest( PendingRequest&& request )
    {
        Threading::RecursiveScopeLock lock( m_accessLock );

        // Try find a pending request for this resource ID
        auto predicate = [] ( PendingRequest const& request, ResourceID const& resourceID ) { return request.m_pRecord->GetResourceID() == resourceID; };
        int32_t const foundIdx = VectorFindIndex( m_pendingRequests, request.m_pRecord->GetResourceID(), predicate );

        // If we dont have a request for this resource ID create one
        if ( foundIdx == InvalidIndex )
        {
            m_pendingRequests.emplace_back( eastl::move( request ) );
        }
        else // Overwrite exiting request - we deal with whether we have to register a task or not in the update
        {
            m_pendingRequests[foundIdx] = request;
        }
    }

    ResourceRequest* ResourceSystem::TryFindActiveRequest( ResourceRecord const* pResourceRecord ) const
    {
        EE_ASSERT( pResourceRecord != nullptr );
        EE_ASSERT( !m_isAsyncTaskRunning );

        Threading::RecursiveScopeLock lock( m_accessLock );
        auto predicate = [] ( ResourceRequest const* pRequest, ResourceRecord const* pResourceRecord ) { return pRequest->GetResourceRecord() == pResourceRecord; };
        int32_t const foundIdx = VectorFindIndex( m_activeRequests, pResourceRecord, predicate );

        if ( foundIdx != InvalidIndex )
        {
            return m_activeRequests[foundIdx];
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------

    void ResourceSystem::UpdateResourceProvider()
    {
        Threading::RecursiveScopeLock lock( m_accessLock );
        m_pResourceProvider->Update();

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        for ( auto const& updatedResourceID : m_pResourceProvider->GetExternallyUpdatedResources() )
        {
            RequestResourceHotReload( updatedResourceID );
        }
        #endif
    }

    void ResourceSystem::Update( bool waitForAsyncTask )
    {
        EE_PROFILE_FUNCTION_RESOURCE();
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_pResourceProvider != nullptr );

        // Update resource provider
        //-------------------------------------------------------------------------
        // This will also update the hot-reload data

        UpdateResourceProvider();

        // Wait for async task to complete
        //-------------------------------------------------------------------------

        // If we've specified to wait for the async-task to complete, block
        if ( waitForAsyncTask )
        {
            m_taskSystem.WaitForTask( &m_asyncProcessingTask );
        }

        if ( m_isAsyncTaskRunning && !m_asyncProcessingTask.GetIsComplete() )
        {
            return;
        }

        m_isAsyncTaskRunning = false;

        // Process and Update requests
        //-------------------------------------------------------------------------

        {
            Threading::RecursiveScopeLock lock( m_accessLock );

            for ( auto& pendingRequest : m_pendingRequests )
            {
                // Get existing active request
                auto pActiveRequest = TryFindActiveRequest( pendingRequest.m_pRecord );

                // Load request
                if ( pendingRequest.m_type == PendingRequest::Type::Load )
                {
                    if ( pActiveRequest != nullptr )
                    {
                        if ( pActiveRequest->IsUnloadRequest() )
                        {
                            pActiveRequest->SwitchToLoadTask();
                        }
                    }
                    else if ( pendingRequest.m_pRecord->IsLoaded() ) // Can occur due to multiple requests for the same resource in the same frame
                    {
                        // Do Nothing
                    }
                    else // Create new request
                    {
                        auto loaderIter = m_resourceLoaders.find( pendingRequest.m_pRecord->GetResourceTypeID() );
                        EE_ASSERT( loaderIter != m_resourceLoaders.end() );
                        m_activeRequests.emplace_back( EE::New<ResourceRequest>( pendingRequest.m_requesterID, ResourceRequest::Type::Load, pendingRequest.m_pRecord, loaderIter->second ) );
                    }
                }
                else // Unload request
                {
                    if ( pActiveRequest != nullptr )
                    {
                        if ( pActiveRequest->IsLoadRequest() )
                        {
                            pActiveRequest->SwitchToUnloadTask();
                        }
                    }
                    else if ( pendingRequest.m_pRecord->IsUnloaded() ) // Can occur due to multiple requests for the same resource in the same frame
                    {
                        if ( !pendingRequest.m_pRecord->HasReferences() )
                        {
                            auto recordIter = m_resourceRecords.find( pendingRequest.m_pRecord->m_resourceID );
                            EE_ASSERT( recordIter != m_resourceRecords.end() );
                            EE_ASSERT( recordIter->second == pendingRequest.m_pRecord );

                            EE::Delete( pendingRequest.m_pRecord );
                            m_resourceRecords.erase( recordIter );
                        }
                    }
                    else // Create new request
                    {
                        auto loaderIter = m_resourceLoaders.find( pendingRequest.m_pRecord->GetResourceTypeID() );
                        EE_ASSERT( loaderIter != m_resourceLoaders.end() );
                        m_activeRequests.emplace_back( EE::New<ResourceRequest>( pendingRequest.m_requesterID, ResourceRequest::Type::Unload, pendingRequest.m_pRecord, loaderIter->second ) );
                    }
                }
            }

            m_pendingRequests.clear();

            // Process completed requests
            //-------------------------------------------------------------------------

            for ( auto pCompletedRequest : m_completedRequests )
            {
                ResourceID const resourceID = pCompletedRequest->GetResourceID();
                EE_ASSERT( pCompletedRequest->IsComplete() );

                #if EE_DEVELOPMENT_TOOLS
                m_history.emplace_back( CompletedRequestLog( pCompletedRequest->IsLoadRequest() ? PendingRequest::Type::Load : PendingRequest::Type::Unload, resourceID ) );
                #endif

                if ( pCompletedRequest->IsUnloadRequest() )
                {
                    // Check if we can remove the record, we may have had a load request for it in the meantime
                    if ( !pCompletedRequest->GetResourceRecord()->HasReferences() )
                    {
                        auto recordIter = m_resourceRecords.find( resourceID );
                        EE_ASSERT( recordIter != m_resourceRecords.end() );
                        EE_ASSERT( recordIter->second == pCompletedRequest->GetResourceRecord() );

                        EE::Delete( recordIter->second );
                        m_resourceRecords.erase( recordIter );
                    }
                }

                // Delete request
                EE::Delete( pCompletedRequest );
            }

            m_completedRequests.clear();
        }

        // Kick off new async task
        //-------------------------------------------------------------------------

        if ( !m_activeRequests.empty() )
        {
            m_taskSystem.ScheduleTask( &m_asyncProcessingTask );
            m_isAsyncTaskRunning = true;
        }
    }

    void ResourceSystem::WaitForAllRequestsToComplete()
    {
        while ( IsBusy() )
        {
            Update();
        }
    }

    void ResourceSystem::ProcessResourceRequests()
    {
        EE_PROFILE_FUNCTION_RESOURCE();

        //-------------------------------------------------------------------------

        // We dont have to worry about this loop even if the m_activeRequests array is modified from another thread since we only access the array in 2 places and both use locks
        for ( int32_t i = (int32_t) m_activeRequests.size() - 1; i >= 0; i-- )
        {
            ResourceRequest::RequestContext context;
            context.m_createRawRequestRequestFunction = [this] ( ResourceRequest* pRequest ) { m_pResourceProvider->RequestRawResource( pRequest ); };
            context.m_cancelRawRequestRequestFunction = [this] ( ResourceRequest* pRequest ) { m_pResourceProvider->CancelRequest( pRequest ); };
            context.m_loadResourceFunction = [this] ( ResourceRequesterID const& requesterID, ResourcePtr& resourcePtr ) { LoadResource( resourcePtr, requesterID ); };
            context.m_unloadResourceFunction = [this] ( ResourceRequesterID const& requesterID, ResourcePtr& resourcePtr ) { UnloadResource( resourcePtr, requesterID ); };

            //-------------------------------------------------------------------------

            bool isRequestComplete = false;

            ResourceRequest* pRequest = m_activeRequests[i];
            if ( pRequest->IsActive() )
            {
                isRequestComplete = pRequest->Update( context );
            }
            else
            {
                isRequestComplete = true;
            }

            //-------------------------------------------------------------------------

            if ( isRequestComplete )
            {
                // We need to process and remove completed requests at the next update stage since unload task may have queued unload requests which refer to the request's allocated memory
                m_completedRequests.emplace_back( pRequest );
                m_activeRequests.erase_unsorted( m_activeRequests.begin() + i );
            }
        }
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void ResourceSystem::RequestResourceHotReload( ResourceID const& resourceID )
    {
        Threading::RecursiveScopeLock lock( m_accessLock );

        // If the resource is not currently in use then just early-out
        auto const recordIter = m_resourceRecords.find( resourceID );
        if ( recordIter == m_resourceRecords.end() )
        {
            return;
        }

        // Generate a list of users for this resource
        ResourceRecord* pRecord = recordIter->second;
        GetUsersForResource( pRecord, m_usersThatRequireReload );

        // Get the list of dependent resources that we also need to reload
        TInlineVector<ResourceID, 20> dependentResources;
        GetDependentResourcesForResource( pRecord, dependentResources );
        for ( auto const& dependentResourceID : dependentResources )
        {
            RequestResourceHotReload( dependentResourceID );
        }

        // Add to list of resources to be reloaded
        VectorEmplaceBackUnique( m_externallyUpdatedResources, resourceID );
    }

    void ResourceSystem::ClearHotReloadRequests()
    {
        Threading::RecursiveScopeLock lock( m_accessLock );
        m_usersThatRequireReload.clear(); 
        m_externallyUpdatedResources.clear();
    }
    #endif
}