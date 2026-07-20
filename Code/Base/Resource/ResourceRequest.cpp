#include "ResourceRequest.h"
#include "Base/Profiling.h"
#include "Base/Threading/Threading.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    #if EE_DEVELOPMENT_TOOLS
    char const* GetResourceLoadStageName( ResourceLoadStage stage )
    {
        constexpr static char const* const stageNames[] =
        {
            "RequestRawResource",
            "WaitForRawResourceRequest",
            "LoadResource",
            "WaitForInstallDependencies",
            "InstallResource",
            "UninstallResource",
            "UnloadResource",
            "Complete",
        };

        EE_ASSERT( stage != ResourceLoadStage::None );
        return stageNames[(uint8_t) stage];
    }
    #endif

    //-------------------------------------------------------------------------

    ResourceRequest::ResourceRequest( RequestType type, ResourceRecord* pRecord, ResourceLoader* pResourceLoader )
        : m_pResourceRecord( pRecord )
        , m_pResourceLoader( pResourceLoader )
        , m_requestType( type )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_pResourceRecord != nullptr && m_pResourceRecord->IsValid() );
        EE_ASSERT( m_pResourceLoader != nullptr );
        EE_ASSERT( m_requestType != RequestType::Invalid );
        EE_ASSERT( !m_pResourceRecord->IsLoading() && !m_pResourceRecord->IsUnloading() );

        if ( m_requestType == RequestType::Load )
        {
            EE_ASSERT( m_pResourceRecord->IsUnloaded() || m_pResourceRecord->HasLoadingFailed() );
            SetStage( ResourceLoadStage::RequestRawResource );
            m_pResourceRecord->SetLoadingStatus( LoadingStatus::Loading );
        }
        else // Unload
        {
            if ( m_pResourceRecord->HasLoadingFailed() )
            {
                SetStage( ResourceLoadStage::Complete );
            }
            else if ( m_pResourceRecord->IsLoaded() )
            {
                SetStage( ResourceLoadStage::UninstallResource );
                m_pResourceRecord->SetLoadingStatus( LoadingStatus::Unloading );
            }
            else // Why are you unloading an already unloaded resource?!
            {
                EE_UNREACHABLE_CODE();
            }
        }
    }

    ResourceID const& ResourceRequest::GetResourceID() const
    {
        return m_pResourceRecord->GetResourceID();
    }

    ResourceTypeID ResourceRequest::GetResourceTypeID() const
    {
        return m_pResourceRecord->GetResourceTypeID();
    }

    LoadingStatus ResourceRequest::GetLoadingStatus() const
    {
        return m_pResourceRecord->GetLoadingStatus();
    }

    void ResourceRequest::LogError( char const* pFormat, ... )
    {
        #if EE_DEVELOPMENT_TOOLS
        va_list args;
        va_start( args, pFormat );

        InlineString msg;
        msg.sprintf_va_list( pFormat, args );

        m_pResourceRecord->m_errorLog.append( msg.c_str() );
        EE_LOG_ERROR( LogCategory::Resource, "Resource Request", msg.c_str() );

        va_end( args );
        #endif
    }

    void ResourceRequest::SetStage( ResourceLoadStage newStage )
    {
        ResourceLoadStage const currentStage = m_stage;
        EE_ASSERT( newStage != currentStage );

        #if EE_DEVELOPMENT_TOOLS
        if ( currentStage != ResourceLoadStage::None )
        {
            m_pResourceRecord->m_stageDurations[(uint8_t) currentStage] = m_stageTimer.GetElapsedTimeMilliseconds();
        }
        #endif

        //-------------------------------------------------------------------------

        m_stage = newStage;

        if ( newStage == ResourceLoadStage::Complete )
        {
            if ( m_requestType == RequestType::Load )
            {
                if ( m_hasLoadFailed )
                {
                    m_pResourceRecord->SetLoadingStatus( LoadingStatus::Failed );
                }
                else
                {
                    m_pResourceRecord->SetLoadingStatus( LoadingStatus::Loaded );
                }
            }
            else
            {
                m_pResourceRecord->SetLoadingStatus( LoadingStatus::Unloaded );
            }
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_pResourceRecord->m_loadStage = newStage;

        if ( currentStage != ResourceLoadStage::None )
        {
            m_stageTimer.Start();
        }
        #endif
    }

    void ResourceRequest::SetLoadFailed()
    {
        m_hasLoadFailed = true;
        m_pResourceRecord->SetLoadingStatus( LoadingStatus::Failed );
    }

    void ResourceRequest::RequestSwitchToLoadTask()
    {
        EE_ASSERT( IsUnloadRequest() );

        if ( m_requestType == RequestType::Load )
        {
            m_switchRequestType = SwitchRequestType::None;
        }
        else // We're an unload task
        {
            m_switchRequestType = SwitchRequestType::SwitchToLoad;
        }
    }

    void ResourceRequest::RequestSwitchToUnloadTask()
    {
        EE_ASSERT( IsLoadRequest() );

        if ( m_requestType == RequestType::Unload )
        {
            m_switchRequestType = SwitchRequestType::None;
        }
        else // We're a load task
        {
            m_switchRequestType = SwitchRequestType::SwitchToUnload;
        }
    }

    bool ResourceRequest::ProcessSwitchRequestType( RequestContext& requestContext )
    {
        bool const hadRequest = m_switchRequestType != SwitchRequestType::None;

        if ( m_switchRequestType == SwitchRequestType::SwitchToLoad )
        {
            EE_ASSERT( m_requestType == RequestType::Unload );

            m_requestType = RequestType::Load;
            m_pResourceRecord->SetLoadingStatus( LoadingStatus::Loading );

            //-------------------------------------------------------------------------

            // Note: The stage is the current stage that was completed
            switch ( m_stage )
            {
                case ResourceLoadStage::Complete:
                case ResourceLoadStage::UnloadResource:
                {
                    SetStage( ResourceLoadStage::RequestRawResource );
                }
                break;

                case ResourceLoadStage::UninstallResource:
                {
                    RequestInstallDependencies( requestContext );
                }
                break;

                default:
                EE_HALT();
                break;
            }
        }
        else if ( m_switchRequestType == SwitchRequestType::SwitchToUnload )
        {
            EE_ASSERT( m_requestType == RequestType::Load );
            EE_ASSERT( !m_pResourceRecord->HasReferences() );

            m_requestType = RequestType::Unload;
            m_pResourceRecord->SetLoadingStatus( LoadingStatus::Unloading );

            //-------------------------------------------------------------------------

            // Note: The stage is the current stage that was completed
            switch ( m_stage )
            {
                case ResourceLoadStage::WaitForRawResourceRequest:
                {
                    requestContext.m_cancelRawRequestRequestFunction( this );
                    SetStage( ResourceLoadStage::Complete );
                }
                break;

                case ResourceLoadStage::LoadResource:
                {
                    SetStage( ResourceLoadStage::UnloadResource );
                }
                break;

                case ResourceLoadStage::WaitForInstallDependencies:
                {
                    m_pendingInstallDependencies.clear();
                    m_installDependencies.clear();
                    UnloadInstallDependencies( requestContext );
                    SetStage( ResourceLoadStage::UnloadResource );
                }
                break;

                case ResourceLoadStage::InstallResource:
                case ResourceLoadStage::Complete:
                {
                    SetStage( ResourceLoadStage::UninstallResource );
                }
                break;

                case ResourceLoadStage::UninstallResource:
                case ResourceLoadStage::UnloadResource:
                {
                    EE_ASSERT( m_hasLoadFailed );
                }
                break;

                case ResourceLoadStage::RequestRawResource:
                case ResourceLoadStage::None:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }
        }

        m_switchRequestType = SwitchRequestType::None;
        return hadRequest;
    }

    //-------------------------------------------------------------------------

    bool ResourceRequest::Update( RequestContext& requestContext )
    {
        // Update loading
        //-------------------------------------------------------------------------

        switch ( m_stage )
        {
            case ResourceLoadStage::RequestRawResource:
            {
                RequestRawResource( requestContext );
            }
            break;

            case ResourceLoadStage::WaitForRawResourceRequest:
            {
                WaitForRawResourceRequest( requestContext );
            }
            break;

            case ResourceLoadStage::LoadResource:
            {
                LoadResource( requestContext );
            }
            break;

            case ResourceLoadStage::WaitForInstallDependencies:
            {
                WaitForInstallDependencies( requestContext );
            }
            break;

            case ResourceLoadStage::InstallResource:
            {
                InstallResource( requestContext );
            }
            break;

            //-------------------------------------------------------------------------

            case ResourceLoadStage::UninstallResource:
            {
                UninstallResource( requestContext );
            }
            break;

            case ResourceLoadStage::UnloadResource:
            {
                UnloadResource( requestContext );
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

    // Load
    //-------------------------------------------------------------------------

    void ResourceRequest::RequestRawResource( RequestContext& requestContext )
    {
        EE_PROFILE_FUNCTION_RESOURCE();
        SetStage( ResourceLoadStage::WaitForRawResourceRequest );
        requestContext.m_createRawRequestRequestFunction( this );
    }

    void ResourceRequest::WaitForRawResourceRequest( RequestContext& requestContext )
    {
        EE_PROFILE_SCOPE_RESOURCE( "Wait For Raw Resource Request" );

        // This is okay to be called while waiting
        ProcessSwitchRequestType( requestContext );
    }

    void ResourceRequest::OnRawResourceRequestComplete( String const& filePath, String const& log )
    {
        // Raw resource failed to load
        if ( filePath.empty() )
        {
            LogError( "Failed to find/compile resource file (%s) - %s", m_pResourceRecord->GetResourceID().c_str(), log.c_str() );
            SetLoadFailed();
            SetStage( ResourceLoadStage::Complete );

            #if EE_DEVELOPMENT_TOOLS
            m_pResourceRecord->m_compilationLog = log;
            #endif
        }
        else // Continue the load operation
        {
            m_rawResourcePath = filePath;
            SetStage( ResourceLoadStage::LoadResource );
        }
    }

    void ResourceRequest::LoadResource( RequestContext& requestContext )
    {
        EE_PROFILE_FUNCTION_RESOURCE();
        EE_ASSERT( m_stage == ResourceLoadStage::LoadResource );
        EE_ASSERT( m_rawResourcePath.IsValid() );

        EE_PROFILE_SCOPE_RESOURCE( "Load Resource" );
        EE_PROFILE_TAG( "filename", m_rawResourcePath.GetFilename().c_str() );

        #if EE_DEVELOPMENT_TOOLS
        TInlineString<9> const resTypeID = m_pResourceRecord->GetResourceTypeID().ToString();
        EE_PROFILE_TAG( "Loader", resTypeID.c_str() );
        #endif

        // Load resource
        //-------------------------------------------------------------------------

        LoadResult const loadResult = m_pResourceLoader->Load( GetResourceID(), m_rawResourcePath, m_pResourceRecord );

        // Still loading
        if ( loadResult == LoadResult::InProgress )
        {
            return;
        }

        // Check if we need to switch type
        if ( ProcessSwitchRequestType( requestContext ) )
        {
            return;
        }

        if ( loadResult == LoadResult::Failed )
        {
            LogError( "Failed to load compiled resource data (%s)", m_pResourceRecord->GetResourceID().c_str() );
            SetLoadFailed();
            SetStage( ResourceLoadStage::UnloadResource );
            return;
        }

        // Request install dependencies
        //-------------------------------------------------------------------------

        RequestInstallDependencies( requestContext );
    }

    void ResourceRequest::RequestInstallDependencies( RequestContext& requestContext )
    {
        // Create the resource ptrs for the install dependencies and request their load
        // These resource ptrs are temporary and will be clear upon completion of the request
        ResourceRequesterID const installDependencyRequesterID = ResourceRequesterID::InstallDependencyRequest( m_pResourceRecord->GetResourceID() );
        uint32_t const numInstallDependencies = (uint32_t) m_pResourceRecord->m_installDependencyResourceIDs.size();
        if ( numInstallDependencies > 0 )
        {
            m_pendingInstallDependencies.resize( numInstallDependencies );
            for ( uint32_t i = 0; i < numInstallDependencies; i++ )
            {
                // Do not use the requester ID for install dependencies! Since they are not explicitly loaded by a specific user!
                // Instead we create a ResourceRequesterID from the depending resource's resourceID
                m_pendingInstallDependencies[i] = ResourcePtr( m_pResourceRecord->m_installDependencyResourceIDs[i] );
                requestContext.m_loadResourceFunction( installDependencyRequesterID, m_pendingInstallDependencies[i] );
            }

            SetStage( ResourceLoadStage::WaitForInstallDependencies );
        }
        else // Nothing to wait for
        {
            SetStage( ResourceLoadStage::InstallResource );
        }
    }

    void ResourceRequest::WaitForInstallDependencies( RequestContext& requestContext )
    {
        EE_PROFILE_FUNCTION_RESOURCE();
        EE_ASSERT( m_stage == ResourceLoadStage::WaitForInstallDependencies );

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
                    LogError( "Failed to load install dependency: %s", m_pendingInstallDependencies[i].GetResourceID().c_str() );
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

        // If we are still loading, nothing more to do
        if ( status == InstallStatus::Loading )
        {
            return;
        }

        // Check if we need to switch type
        if ( ProcessSwitchRequestType( requestContext ) )
        {
            return;
        }

        // If dependency has failed, the resource has failed
        if ( status == InstallStatus::ShouldFail )
        {
            ResourceID const& resourceID = m_pResourceRecord->GetResourceID();

            // Do not use the user ID for install dependencies! Since they are not explicitly loaded by a specific user!
            // Instead we create a ResourceRequesterID from the depending resource's resourceID
            ResourceRequesterID const installDependencyRequesterID = ResourceRequesterID::InstallDependencyRequest( resourceID );

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

            SetLoadFailed();
            SetStage( ResourceLoadStage::UnloadResource );
        }
        // Install runtime resource
        else if ( status == InstallStatus::ShouldProceed )
        {
            EE_ASSERT( m_pendingInstallDependencies.empty() );
            SetStage( ResourceLoadStage::InstallResource );
        }
    }

    void ResourceRequest::InstallResource( RequestContext& requestContext )
    {
        EE_PROFILE_FUNCTION_RESOURCE();
        EE_ASSERT( m_stage == ResourceLoadStage::InstallResource );
        EE_ASSERT( m_pendingInstallDependencies.empty() );

        LoadResult const loadResult = m_pResourceLoader->Install( GetResourceID(), m_installDependencies, m_pResourceRecord );

        // Still installing
        if ( loadResult == LoadResult::InProgress )
        {
            return;
        }

        // Check if we need to switch type
        if ( ProcessSwitchRequestType( requestContext ) )
        {
            return;
        }

        // Install complete
        if ( loadResult == LoadResult::Complete )
        {
            m_pResourceLoader->OnInstallComplete( GetResourceID(), m_pResourceRecord );

            EE_ASSERT( m_pResourceRecord->GetResourceData() != nullptr );
            m_installDependencies.clear();
            SetStage( ResourceLoadStage::Complete );
            return;
        }

        // Install operation failed, uninstall and unload resource and set status to failed
        if( loadResult == LoadResult::Failed )
        {
            LogError( "Failed to install resource (%s)", m_pResourceRecord->GetResourceID().c_str() );
            SetLoadFailed();
            SetStage( ResourceLoadStage::UninstallResource );
            return;
        }
    }

    // Unload
    //-------------------------------------------------------------------------

    void ResourceRequest::UninstallResource( RequestContext& requestContext )
    {
        EE_PROFILE_FUNCTION_RESOURCE();
        EE_ASSERT( m_stage == ResourceLoadStage::UninstallResource );
        EE_ASSERT( m_pResourceRecord->IsUnloading() || m_hasLoadFailed );

        //-------------------------------------------------------------------------

        UnloadResult const unloadResult = m_pResourceLoader->Uninstall( GetResourceID(), m_pResourceRecord );

        // Still uninstalling
        if ( unloadResult == UnloadResult::InProgress )
        {
            return;
        }

        // Check if we need to switch type
        if ( ProcessSwitchRequestType( requestContext ) )
        {
            return;
        }

        UnloadInstallDependencies( requestContext );
        SetStage( ResourceLoadStage::UnloadResource );
    }

    void ResourceRequest::UnloadInstallDependencies( RequestContext& requestContext )
    {
        // Create the resource ptrs for the install dependencies and request the unload
        // These resource ptrs are temporary and will be cleared upon completion of the request
        ResourceRequesterID const installDependencyRequesterID = ResourceRequesterID::InstallDependencyRequest( m_pResourceRecord->GetResourceID() );
        uint32_t const numInstallDependencies = (uint32_t) m_pResourceRecord->m_installDependencyResourceIDs.size();
        m_pendingInstallDependencies.resize( numInstallDependencies );
        for ( uint32_t i = 0; i < numInstallDependencies; i++ )
        {
            // Do not use the user ID for install dependencies! Since they are not explicitly loaded by a specific user!
            // Instead we create a ResourceRequesterID from the depending resource's resourceID
            m_pendingInstallDependencies[i] = ResourcePtr( m_pResourceRecord->m_installDependencyResourceIDs[i] );
            requestContext.m_unloadResourceFunction( installDependencyRequesterID, m_pendingInstallDependencies[i] );
        }
    }

    void ResourceRequest::UnloadResource( RequestContext& requestContext )
    {
        EE_PROFILE_FUNCTION_RESOURCE();
        EE_ASSERT( m_stage == ResourceLoadStage::UnloadResource );
        EE_ASSERT( m_pResourceRecord->IsUnloading() || m_hasLoadFailed );

        // Unload resource
        //-------------------------------------------------------------------------

        UnloadResult const unloadResult = m_pResourceLoader->Unload( GetResourceID(), m_pResourceRecord );

        // Still unloading
        if ( unloadResult == UnloadResult::InProgress )
        {
            return;
        }

        // Check if we need to switch type
        if ( ProcessSwitchRequestType( requestContext ) )
        {
            return;
        }

        m_pResourceLoader->OnUnloadComplete( GetResourceID(), m_pResourceRecord );
        SetStage( ResourceLoadStage::Complete );
    }
}