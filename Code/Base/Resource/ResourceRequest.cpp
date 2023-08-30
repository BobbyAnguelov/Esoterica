#include "ResourceRequest.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Profiling.h"
#include "Base/Threading/Threading.h"


//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceRequest::ResourceRequest( ResourceRequesterID const& requesterID, Type type, ResourceRecord* pRecord, ResourceLoader* pResourceLoader )
        : m_requesterID( requesterID )
        , m_pResourceRecord( pRecord )
        , m_pResourceLoader( pResourceLoader )
        , m_type( type )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_pResourceRecord != nullptr && m_pResourceRecord->IsValid() );
        EE_ASSERT( m_pResourceLoader != nullptr );
        EE_ASSERT( m_type != Type::Invalid );
        EE_ASSERT( !m_pResourceRecord->IsLoading() && !m_pResourceRecord->IsUnloading() );

        if ( m_type == Type::Load )
        {
            EE_ASSERT( m_pResourceRecord->IsUnloaded() || m_pResourceRecord->HasLoadingFailed() );
            m_stage = Stage::RequestRawResource;
            m_pResourceRecord->SetLoadingStatus( LoadingStatus::Loading );
        }
        else // Unload
        {
            if ( m_pResourceRecord->HasLoadingFailed() )
            {
                m_stage = Stage::UnloadFailedResource;
            }
            else if ( m_pResourceRecord->IsLoaded() )
            {
                m_stage = Stage::UninstallResource;
                m_pResourceRecord->SetLoadingStatus( LoadingStatus::Unloading );
            }
            else // Why are you unloading an already unloaded resource?!
            {
                EE_UNREACHABLE_CODE();
            }
        }
    }

    void ResourceRequest::OnRawResourceRequestComplete( String const& filePath )
    {
        // Raw resource failed to load
        if ( filePath.empty() )
        {
            EE_LOG_ERROR( "Resource", "Resource Request", "Failed to find/compile resource file (%s)", m_pResourceRecord->GetResourceID().c_str() );
            m_stage = ResourceRequest::Stage::Complete;
            m_pResourceRecord->SetLoadingStatus( LoadingStatus::Failed );
        }
        else // Continue the load operation
        {
            m_rawResourcePath = filePath;
            m_stage = ResourceRequest::Stage::LoadResource;
        }
    }

    void ResourceRequest::SwitchToLoadTask()
    {
        EE_ASSERT( m_type == Type::Unload );
        EE_ASSERT( !m_isReloadRequest );

        m_type = Type::Load;
        m_pResourceRecord->SetLoadingStatus( LoadingStatus::Loading );

        //-------------------------------------------------------------------------

        switch ( m_stage )
        {
            case Stage::Complete:
            {
                m_stage = Stage::RequestRawResource;
            }
            break;

            case Stage::UninstallResource:
            {
                m_stage = Stage::Complete;
                m_pResourceRecord->SetLoadingStatus( LoadingStatus::Loaded );
            }
            break;

            case Stage::CancelWaitForLoadDependencies:
            {
                m_stage = Stage::WaitForLoadDependencies;
            }
            break;

            case Stage::UnloadResource:
            {
                m_stage = Stage::InstallResource;
            }
            break;

            default:
            EE_HALT();
            break;
        }
    }

    void ResourceRequest::SwitchToUnloadTask()
    {
        EE_ASSERT( m_type == Type::Load );
        EE_ASSERT( !m_pResourceRecord->HasReferences() );

        m_type = Type::Unload;
        m_pResourceRecord->SetLoadingStatus( LoadingStatus::Unloading );

        //-------------------------------------------------------------------------

        switch ( m_stage )
        {
            case Stage::WaitForRawResourceRequest:
            {
                m_stage = Stage::CancelRawResourceRequest;
            }
            break;

            case Stage::LoadResource:
            {
                m_stage = Stage::Complete;
                m_pResourceRecord->SetLoadingStatus( LoadingStatus::Unloaded );
            }
            break;

            case Stage::WaitForLoadDependencies:
            {
                m_stage = Stage::CancelWaitForLoadDependencies;
            }
            break;

            case Stage::InstallResource:
            case Stage::WaitForInstallResource:
            {
                m_stage = Stage::UnloadResource;
            }
            break;

            case Stage::Complete:
            {
                m_stage = Stage::UninstallResource;
            }
            break;

            default:
            EE_HALT();
            break;
        }
    }

    //-------------------------------------------------------------------------

    bool ResourceRequest::Update( RequestContext& requestContext )
    {
        // Update loading
        //-------------------------------------------------------------------------

        switch ( m_stage )
        {
            case ResourceRequest::Stage::RequestRawResource:
            {
                RequestRawResource( requestContext );
            }
            break;

            case ResourceRequest::Stage::WaitForRawResourceRequest:
            {
                // Do Nothing
                EE_PROFILE_SCOPE_RESOURCE( "Wait For Raw Resource Request" );
            }
            break;

            case ResourceRequest::Stage::LoadResource:
            {
                LoadResource( requestContext );
            }
            break;

            case ResourceRequest::Stage::WaitForLoadDependencies:
            {
                WaitForLoadDependencies( requestContext );
            }
            break;

            case ResourceRequest::Stage::InstallResource:
            {
                InstallResource( requestContext );
            }
            break;

            case ResourceRequest::Stage::WaitForInstallResource:
            {
                WaitForInstallResource( requestContext );
            }
            break;

            //-------------------------------------------------------------------------

            case ResourceRequest::Stage::UninstallResource:
            {
                // Execute all unload operations immediately
                UninstallResource( requestContext );
                UnloadResource( requestContext );
            }
            break;

            case ResourceRequest::Stage::UnloadResource:
            {
                UnloadResource( requestContext );
            }
            break;

            case ResourceRequest::Stage::UnloadFailedResource:
            {
                UnloadFailedResource( requestContext );
            }
            break;

            case ResourceRequest::Stage::CancelWaitForLoadDependencies:
            {
                // Execute all unload operations immediately
                m_pendingInstallDependencies.clear();
                m_installDependencies.clear();
                m_stage = Stage::UnloadResource;
                UnloadResource( requestContext );
            }
            break;

            case ResourceRequest::Stage::CancelRawResourceRequest:
            {
                CancelRawRequestRequest( requestContext );
            }
            break;

            default:
            {
                EE_UNREACHABLE_CODE();
            }
            break;
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( IsComplete() )
        {
            EE_ASSERT( m_pResourceRecord->IsLoaded() || m_pResourceRecord->IsUnloaded() || m_pResourceRecord->HasLoadingFailed() );
        }
        #endif

        return IsComplete();
    }

    //-------------------------------------------------------------------------

    void ResourceRequest::RequestRawResource( RequestContext& requestContext )
    {
        EE_PROFILE_FUNCTION_RESOURCE();
        m_stage = ResourceRequest::Stage::WaitForRawResourceRequest;
        requestContext.m_createRawRequestRequestFunction( this );
    }

    void ResourceRequest::LoadResource( RequestContext& requestContext )
    {
        EE_PROFILE_FUNCTION_RESOURCE();
        EE_ASSERT( m_stage == ResourceRequest::Stage::LoadResource );
        EE_ASSERT( m_rawResourcePath.IsValid() );

        // Read file
        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_IO( "Read File" );
            EE_PROFILE_TAG( "filename", m_rawResourcePath.GetFilename().c_str() );

            #if EE_DEVELOPMENT_TOOLS
            ScopedTimer<PlatformClock> timer( m_pResourceRecord->m_fileReadTime );
            #endif

            if ( !FileSystem::LoadFile( m_rawResourcePath, m_rawResourceData ) )
            {
                EE_LOG_ERROR( "Resource", "Resource Request", "Failed to load resource file (%s)", m_pResourceRecord->GetResourceID().c_str() );
                m_stage = ResourceRequest::Stage::Complete;
                m_pResourceRecord->SetLoadingStatus( LoadingStatus::Failed );
                return;
            }
        }

        // Load resource
        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_RESOURCE( "Load Resource" );

            #if EE_DEVELOPMENT_TOOLS
            char resTypeID[5];
            m_pResourceRecord->GetResourceTypeID().GetString( resTypeID );
            EE_PROFILE_TAG( "Loader", resTypeID );
            #endif

            // Load the resource
            EE_ASSERT( !m_rawResourceData.empty() );

            #if EE_DEVELOPMENT_TOOLS
            ScopedTimer<PlatformClock> timer( m_pResourceRecord->m_loadTime );
            #endif

            if ( !m_pResourceLoader->Load( GetResourceID(), m_rawResourceData, m_pResourceRecord ) )
            {
                EE_LOG_ERROR( "Resource", "Resource Request", "Failed to load compiled resource data (%s)", m_pResourceRecord->GetResourceID().c_str() );
                m_pResourceRecord->SetLoadingStatus( LoadingStatus::Failed );
                m_pResourceLoader->Unload( GetResourceID(), m_pResourceRecord );
                m_stage = ResourceRequest::Stage::Complete;
                return;
            }

            // Release raw data
            m_rawResourceData.clear();
        }

        // Load dependencies
        //-------------------------------------------------------------------------

        // Create the resource ptrs for the install dependencies and request their load
        // These resource ptrs are temporary and will be clear upon completion of the request
        ResourceRequesterID const installDependencyRequesterID( m_pResourceRecord->GetResourceID() );
        uint32_t const numInstallDependencies = (uint32_t) m_pResourceRecord->m_installDependencyResourceIDs.size();
        m_pendingInstallDependencies.resize( numInstallDependencies );
        for ( uint32_t i = 0; i < numInstallDependencies; i++ )
        {
            // Do not use the requester ID for install dependencies! Since they are not explicitly loaded by a specific user!
            // Instead we create a ResourceRequesterID from the depending resource's resourceID
            m_pendingInstallDependencies[i] = ResourcePtr( m_pResourceRecord->m_installDependencyResourceIDs[i] );
            requestContext.m_loadResourceFunction( installDependencyRequesterID, m_pendingInstallDependencies[i] );
        }

        m_stage = ResourceRequest::Stage::WaitForLoadDependencies;

        #if EE_DEVELOPMENT_TOOLS
        m_pResourceRecord->m_waitForDependenciesTime = 0.0f;
        m_stageTimer.Start();
        #endif
    }

    void ResourceRequest::WaitForLoadDependencies( RequestContext& requestContext )
    {
        EE_PROFILE_FUNCTION_RESOURCE();
        EE_ASSERT( m_stage == ResourceRequest::Stage::WaitForLoadDependencies );

        enum class InstallStatus
        {
            Loading,
            ShouldProceed,
            ShouldFail,
        };

        // Check if all dependencies are finished installing
        auto status = InstallStatus::ShouldProceed;

        for ( size_t i = 0; i < m_pendingInstallDependencies.size(); i++ )
        {
            if ( m_pendingInstallDependencies[i].HasLoadingFailed() )
            {
                if ( !m_pResourceLoader->CanProceedWithFailedInstallDependency() )
                {
                    EE_LOG_ERROR( "Resource", "Resource Request", "Failed to load install dependency: %s", m_pendingInstallDependencies[i].GetResourceID().ToString().c_str() );
                    status = InstallStatus::ShouldFail;
                    break;
                }
            }

            // If it's loaded, move it to the loaded list and continue iterating
            if ( m_pendingInstallDependencies[i].IsLoaded() || m_pendingInstallDependencies[i].HasLoadingFailed() )
            {
                m_installDependencies.emplace_back( m_pendingInstallDependencies[i] );
                m_pendingInstallDependencies.erase_unsorted( m_pendingInstallDependencies.begin() + i );
                i--;
            }
            else
            {
                status = InstallStatus::Loading;
                break;
            }
        }

        // If dependency has failed, the resource has failed to load so immediately unload and set status to failed
        if ( status == InstallStatus::ShouldFail )
        {
            EE_LOG_ERROR( "Resource", "Resource Request", "Failed to load resource file due to failed dependency (%s)", m_pResourceRecord->GetResourceID().c_str() );

            // Do not use the user ID for install dependencies! Since they are not explicitly loaded by a specific user!
            // Instead we create a ResourceRequesterID from the depending resource's resourceID
            ResourceRequesterID const installDependencyRequesterID( m_pResourceRecord->GetResourceID() );

            // Unload all install dependencies
            for ( auto& pendingDependency : m_pendingInstallDependencies )
            {
                requestContext.m_unloadResourceFunction( installDependencyRequesterID, pendingDependency );
            }

            for ( auto& dependency : m_installDependencies )
            {
                requestContext.m_unloadResourceFunction( installDependencyRequesterID, dependency );
            }

            m_pendingInstallDependencies.clear();
            m_installDependencies.clear();
            m_pResourceRecord->SetLoadingStatus( LoadingStatus::Failed );
            m_pResourceLoader->Unload( GetResourceID(), m_pResourceRecord );
            m_stage = ResourceRequest::Stage::Complete;
        }
        // Install runtime resource
        else if ( status == InstallStatus::ShouldProceed )
        {
            EE_ASSERT( m_pendingInstallDependencies.empty() );
            m_stage = ResourceRequest::Stage::InstallResource;

            #if EE_DEVELOPMENT_TOOLS
            m_pResourceRecord->m_waitForDependenciesTime = m_stageTimer.GetElapsedTimeMilliseconds();
            m_stageTimer.Start();
            #endif
        }
    }

    void ResourceRequest::InstallResource( RequestContext& requestContext )
    {
        EE_PROFILE_FUNCTION_RESOURCE();
        EE_ASSERT( m_stage == ResourceRequest::Stage::InstallResource );
        EE_ASSERT( m_pendingInstallDependencies.empty() );

        {
            #if EE_DEVELOPMENT_TOOLS
            ScopedTimer<PlatformClock> timer( m_pResourceRecord->m_installTime );
            #endif

            InstallResult const result = m_pResourceLoader->Install( GetResourceID(), m_pResourceRecord, m_installDependencies );
            switch ( result )
            {
                // Finished installing the resource
                case InstallResult::Succeeded :
                {
                    EE_ASSERT( m_pResourceRecord->GetResourceData() != nullptr );
                    m_installDependencies.clear();
                    m_pResourceRecord->SetLoadingStatus( LoadingStatus::Loaded );
                    m_stage = ResourceRequest::Stage::Complete;

                    #if EE_DEVELOPMENT_TOOLS
                    m_pResourceRecord->m_installTime = m_stageTimer.GetElapsedTimeMilliseconds();
                    #endif
                }
                break;

                // Wait for install to complete
                case InstallResult::InProgress:
                {
                    EE_ASSERT( m_pResourceRecord->GetResourceData() != nullptr );
                    m_installDependencies.clear();
                    m_stage = ResourceRequest::Stage::WaitForInstallResource;
                }
                break;

                // Install operation failed, unload resource and set status to failed
                case InstallResult::Failed:
                {
                    EE_LOG_ERROR( "Resource", "Resource Request", "Failed to install resource (%s)", m_pResourceRecord->GetResourceID().c_str() );

                    m_stage = ResourceRequest::Stage::UnloadResource;
                    UnloadResource( requestContext );
                    m_pResourceRecord->SetLoadingStatus( LoadingStatus::Failed );
                    m_stage = ResourceRequest::Stage::Complete;
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }
        }
    }

    void ResourceRequest::WaitForInstallResource( RequestContext& requestContext )
    {
        EE_PROFILE_FUNCTION_RESOURCE();
        EE_ASSERT( m_stage == ResourceRequest::Stage::WaitForInstallResource );
        EE_ASSERT( m_pendingInstallDependencies.empty() );
        EE_ASSERT( m_pResourceRecord->GetResourceData() != nullptr );

        InstallResult const result = m_pResourceLoader->UpdateInstall( GetResourceID(), m_pResourceRecord );
        switch ( result )
        {
            // Finished installing the resource
            case InstallResult::Succeeded:
            {
                m_pResourceRecord->SetLoadingStatus( LoadingStatus::Loaded );
                m_stage = ResourceRequest::Stage::Complete;
            }
            break;

            // Wait for install to complete
            case InstallResult::InProgress:
            {
                // Do Nothing
            }
            break;

            // Install operation failed, unload resource and set status to failed
            case InstallResult::Failed:
            {
                EE_LOG_ERROR( "Resource", "Resource Request", "Failed to install resource (%s)", m_pResourceRecord->GetResourceID().c_str() );

                m_stage = ResourceRequest::Stage::UnloadResource;
                UnloadResource( requestContext );
                m_pResourceRecord->SetLoadingStatus( LoadingStatus::Failed );
                m_stage = ResourceRequest::Stage::Complete;
            }
            break;

            default:
            {
                EE_UNREACHABLE_CODE();
            }
            break;
        }
    }

    //-------------------------------------------------------------------------

    void ResourceRequest::UninstallResource( RequestContext& requestContext )
    {
        EE_ASSERT( m_stage == ResourceRequest::Stage::UninstallResource );
        m_pResourceLoader->Uninstall( GetResourceID(), m_pResourceRecord );
        m_stage = ResourceRequest::Stage::UnloadResource;
    }

    void ResourceRequest::UnloadResource( RequestContext& requestContext )
    {
        EE_ASSERT( m_stage == ResourceRequest::Stage::UnloadResource );

        // Unload dependencies
        //-------------------------------------------------------------------------

        // Create the resource ptrs for the install dependencies and request the unload
        // These resource ptrs are temporary and will be cleared upon completion of the request
        ResourceRequesterID const installDependencyRequesterID( m_pResourceRecord->GetResourceID() );
        uint32_t const numInstallDependencies = (uint32_t) m_pResourceRecord->m_installDependencyResourceIDs.size();
        m_pendingInstallDependencies.resize( numInstallDependencies );
        for ( uint32_t i = 0; i < numInstallDependencies; i++ )
        {
            // Do not use the user ID for install dependencies! Since they are not explicitly loaded by a specific user!
            // Instead we create a ResourceRequesterID from the depending resource's resourceID
            m_pendingInstallDependencies[i] = ResourcePtr( m_pResourceRecord->m_installDependencyResourceIDs[i] );
            requestContext.m_unloadResourceFunction( installDependencyRequesterID, m_pendingInstallDependencies[i] );
        }

        // Unload resource
        //-------------------------------------------------------------------------

        EE_ASSERT( m_pResourceRecord->IsUnloading() );
        m_pResourceLoader->Unload( GetResourceID(), m_pResourceRecord );
        m_pResourceRecord->SetLoadingStatus( LoadingStatus::Unloaded );

        m_stage = ResourceRequest::Stage::Complete;

        if ( m_isReloadRequest )
        {
            // Clear the flag here, in case we re-used this request again
            m_isReloadRequest = false;
            SwitchToLoadTask();
        }
    }

    void ResourceRequest::UnloadFailedResource( RequestContext& requestContext )
    {
        EE_ASSERT( m_stage == ResourceRequest::Stage::UnloadFailedResource );
        EE_ASSERT( m_pResourceRecord->HasLoadingFailed() );

        m_pResourceLoader->Unload( GetResourceID(), m_pResourceRecord );
        m_pResourceRecord->SetLoadingStatus( LoadingStatus::Unloaded );
        m_stage = ResourceRequest::Stage::Complete;

        if ( m_isReloadRequest )
        {
            // Clear the flag here, in case we re-used this request again
            m_isReloadRequest = false;
            SwitchToLoadTask();
        }
    }

    //-------------------------------------------------------------------------

    void ResourceRequest::CancelRawRequestRequest( RequestContext& requestContext )
    {
        EE_ASSERT( m_stage == ResourceRequest::Stage::CancelRawResourceRequest );
        requestContext.m_cancelRawRequestRequestFunction( this );
        m_pResourceRecord->SetLoadingStatus( LoadingStatus::Unloaded );
        m_stage = ResourceRequest::Stage::Complete;
    }
}