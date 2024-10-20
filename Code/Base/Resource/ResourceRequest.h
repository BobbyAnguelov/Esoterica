#pragma once

#include "ResourceRecord.h"
#include "ResourceLoader.h"
#include "Base/Types/Function.h"
#include "Base/Time/Timers.h"
#include "ResourceHeader.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class EE_BASE_API ResourceRequest
    {
    public:

        enum class Stage
        {
            None = -1,

            // Load Stages
            RequestRawResource,
            WaitForRawResourceRequest,
            LoadResource,
            WaitForLoadDependencies,
            InstallResource,
            WaitForInstallResource,

            // Unload Stages
            UninstallResource,
            UnloadResource,
            UnloadFailedResource,

            // Special Cases
            CancelWaitForLoadDependencies, // This stage is needed so we can resume correctly when going from load -> unload -> load
            CancelRawResourceRequest,

            Complete,
        };

        enum class Type
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
            SwitchToReload,
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
        ResourceRequest( ResourceRequesterID const& requesterID, Type type, ResourceRecord* pRecord, ResourceLoader* pResourceLoader );

        inline bool IsValid() const { return m_pResourceRecord != nullptr; }
        inline bool IsActive() const { return m_stage != Stage::Complete; }
        inline bool IsComplete() const { return m_stage == Stage::Complete; }
        inline bool IsLoadRequest() const { return m_type == Type::Load; }
        inline bool IsUnloadRequest() const { return m_type == Type::Unload; }

        inline Stage GetStage() const { return m_stage; }

        inline ResourceRecord const* GetResourceRecord() const { return m_pResourceRecord; }
        inline ResourceID const& GetResourceID() const { return m_pResourceRecord->GetResourceID(); }
        inline ResourceTypeID GetResourceTypeID() const { return m_pResourceRecord->GetResourceTypeID(); }
        inline LoadingStatus GetLoadingStatus() const { return m_pResourceRecord->GetLoadingStatus(); }

        inline bool operator==( ResourceRequest const& other ) const { return GetResourceID() == other.GetResourceID(); }
        inline bool operator!=( ResourceRequest const& other ) const { return GetResourceID() != other.GetResourceID(); }

        //-------------------------------------------------------------------------

        // Called by the resource system to update the request progress
        bool Update( RequestContext& requestContext );

        // Called by the resource provider once the request operation completes and provides the raw resource data
        void OnRawResourceRequestComplete( String const& filePath, String const& log );

        // This will interrupt a load task and convert it into an unload task
        void SwitchToLoadTask();

        // This will interrupt an unload task and convert it into a load task
        void SwitchToUnloadTask();

        //-------------------------------------------------------------------------

        void RequestRawResource( RequestContext& requestContext );
        void LoadResource( RequestContext& requestContext );
        void WaitForLoadDependencies( RequestContext& requestContext );
        void InstallResource( RequestContext& requestContext );
        void WaitForInstallResource( RequestContext& requestContext );
        void UninstallResource( RequestContext& requestContext );
        void UnloadResource( RequestContext& requestContext );
        void UnloadFailedResource( RequestContext& requestContext );
        void CancelRawRequestRequest( RequestContext& requestContext );

    private:

        ResourceRequesterID                     m_requesterID;
        ResourceRecord*                         m_pResourceRecord = nullptr;
        ResourceLoader*                         m_pResourceLoader = nullptr;
        FileSystem::Path                        m_rawResourcePath;
        InstallDependencyList                   m_pendingInstallDependencies;
        InstallDependencyList                   m_installDependencies;
        Type                                    m_type = Type::Invalid;
        Stage                                   m_stage = Stage::None;
        bool                                    m_isReloadRequest = false;

        #if EE_DEVELOPMENT_TOOLS
        Timer<PlatformClock>                    m_stageTimer;
        #endif
    };
}