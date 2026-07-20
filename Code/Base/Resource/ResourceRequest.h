#pragma once

#include "ResourceLoader.h"
#include "Base/Types/Function.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    enum class ResourceLoadStage : int8_t
    {
        None = -1,

        // Load Stages
        RequestRawResource,
        WaitForRawResourceRequest,
        LoadResource,

        // Install Stages
        WaitForInstallDependencies,
        InstallResource,

        // Unload Stages
        UninstallResource,
        UnloadResource,

        Complete,
    };

    #if EE_DEVELOPMENT_TOOLS
    EE_BASE_API char const* GetResourceLoadStageName( ResourceLoadStage stage );
    #endif

    //-------------------------------------------------------------------------

    class EE_BASE_API ResourceRequest
    {
    public:

        enum class RequestType
        {
            Invalid = -1,
            Load,
            Unload,
        };

        enum class SwitchRequestType
        {
            None = 0,
            SwitchToLoad,
            SwitchToUnload,
        };

        struct RequestContext
        {
            TFunction<void( ResourceRequest* )> m_createRawRequestRequestFunction;
            TFunction<void( ResourceRequest* )> m_cancelRawRequestRequestFunction;
            TFunction<void( ResourceRequesterID const&, ResourcePtr& )> m_loadResourceFunction;
            TFunction<void( ResourceRequesterID const&, ResourcePtr& )> m_unloadResourceFunction;
        };

    public:

        ResourceRequest() = default;
        ResourceRequest( RequestType type, ResourceRecord* pRecord, ResourceLoader* pResourceLoader );

        inline bool IsValid() const { return m_pResourceRecord != nullptr; }
        inline bool IsActive() const { return m_stage != ResourceLoadStage::Complete; }
        inline bool IsComplete() const { return m_stage == ResourceLoadStage::Complete; }
        inline bool IsLoadRequest() const { return m_requestType == RequestType::Load || m_switchRequestType == SwitchRequestType::SwitchToLoad; }
        inline bool IsUnloadRequest() const { return m_requestType == RequestType::Unload || m_switchRequestType == SwitchRequestType::SwitchToUnload; }

        inline ResourceRecord const* GetResourceRecord() const { return m_pResourceRecord; }

        ResourceID const& GetResourceID() const;
        ResourceTypeID GetResourceTypeID() const;
        LoadingStatus GetLoadingStatus() const;

        inline bool operator==( ResourceRequest const& other ) const { return GetResourceID() == other.GetResourceID(); }
        inline bool operator!=( ResourceRequest const& other ) const { return GetResourceID() != other.GetResourceID(); }

        //-------------------------------------------------------------------------

        // Called by the resource system to update the request progress
        bool Update( RequestContext& requestContext );

        // Called by the resource provider once the request operation completes and provides the raw resource data
        void OnRawResourceRequestComplete( String const& filePath, String const& log );

        // This will interrupt a load task and convert it into an unload task
        void RequestSwitchToLoadTask();

        // This will interrupt an unload task and convert it into a load task
        void RequestSwitchToUnloadTask();

    private:

        // Set the current stage of the load/unload
        void SetStage( ResourceLoadStage newStage );

        void SetLoadFailed();

        void RequestRawResource( RequestContext& requestContext );
        void WaitForRawResourceRequest( RequestContext& requestContext );
        void LoadResource( RequestContext& requestContext );
        void RequestInstallDependencies( RequestContext& requestContext );
        void WaitForInstallDependencies( RequestContext& requestContext );
        void InstallResource( RequestContext& requestContext );

        void UninstallResource( RequestContext& requestContext );
        void UnloadInstallDependencies( RequestContext& requestContext );
        void UnloadResource( RequestContext& requestContext );

        // Log an error to both the output log and the record
        void LogError( char const* pFormat, ... );

        // This will handle any requested switches between load/unload
        // Be very careful where this is called, is only allowed to be called once a stage completes
        bool ProcessSwitchRequestType( RequestContext& requestContext );

    private:

        ResourceRecord*                         m_pResourceRecord = nullptr;
        ResourceLoader*                         m_pResourceLoader = nullptr;
        FileSystem::Path                        m_rawResourcePath;
        InstallDependencyList                   m_pendingInstallDependencies;
        InstallDependencyList                   m_installDependencies;
        RequestType                             m_requestType = RequestType::Invalid;
        SwitchRequestType                       m_switchRequestType = SwitchRequestType::None;
        ResourceLoadStage                       m_stage = ResourceLoadStage::None;
        bool                                    m_hasLoadFailed = false;

        #if EE_DEVELOPMENT_TOOLS
        Timer<PlatformClock>                    m_stageTimer;
        #endif
    };
}