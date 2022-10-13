#pragma once

#include "IResource.h"
#include "ResourceRequesterID.h"
#include "System/Types/LoadingStatus.h"
#include "System/Types/UUID.h"
#include "System/Time/Time.h"
#include <atomic>

//-------------------------------------------------------------------------

namespace EE::Resource
{
    //-------------------------------------------------------------------------
    // A unique record for each requested resource
    //-------------------------------------------------------------------------
    // The resource record is not threadsafe so the resource system needs to ensure that all external access is threadsafe

    class EE_SYSTEM_API ResourceRecord
    {
        friend class ResourceSystem;
        friend class ResourceRequest;
        friend class ResourceLoader;
        friend class ResourceDebugView;

    public:

        ResourceRecord( ResourceID resourceID ) : m_resourceID( resourceID ) { EE_ASSERT( resourceID.IsValid() ); }
        ~ResourceRecord();

        inline bool IsValid() const { return m_resourceID.IsValid(); }
        inline ResourceID const& GetResourceID() const { return m_resourceID; }
        inline ResourceTypeID GetResourceTypeID() const { return m_resourceID.GetResourceTypeID(); }

        inline void SetLoadingStatus( LoadingStatus status ) { m_loadingStatus = status; }
        inline LoadingStatus GetLoadingStatus() const { return m_loadingStatus; }

        inline IResource* GetResourceData() { return m_pResource; }
        inline IResource const* GetResourceData() const { return m_pResource; }
        inline void SetResourceData( IResource* pResourceData ) { m_pResource = pResourceData; }

        template<typename T>
        inline T* GetResourceData() { return reinterpret_cast<T*>( m_pResource ); }

        //-------------------------------------------------------------------------

        inline bool HasReferences() const { return !m_references.empty(); }

        inline void AddReference( ResourceRequesterID const& requesterID )
        {
            m_references.emplace_back( requesterID );
        }

        inline void RemoveReference( ResourceRequesterID const& requesterID )
        {
            auto iter = eastl::find( m_references.begin(), m_references.end(), requesterID );
            EE_ASSERT( iter != m_references.end() );
            m_references.erase_unsorted( iter );
        }

        //-------------------------------------------------------------------------

        inline bool IsLoading() const { return m_loadingStatus == LoadingStatus::Loading; }
        inline bool IsLoaded() const { return m_loadingStatus == LoadingStatus::Loaded; }
        inline bool IsUnloading() const { return m_loadingStatus == LoadingStatus::Unloading; }
        inline bool IsUnloaded() const { return m_loadingStatus == LoadingStatus::Unloaded; }
        inline bool HasLoadingFailed() const { return m_loadingStatus == LoadingStatus::Failed; }

        inline TInlineVector<ResourceID, 4> const& GetInstallDependencies() const { return m_installDependencyResourceIDs; }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        inline Milliseconds GetFileReadTime() const { return m_fileReadTime; }
        inline Milliseconds GetLoadTime() const { return m_loadTime; }
        inline Milliseconds GetDependenciesWaitTime() const { return m_waitForDependenciesTime; }
        inline Milliseconds GetInstallTime() const { return m_installTime; }
        #endif

    protected:

        ResourceID                              m_resourceID;                                   // The ID of the resource this record refers to
        IResource*                              m_pResource = nullptr;                          // The actual loaded resource data
        std::atomic<LoadingStatus>              m_loadingStatus = LoadingStatus::Unloaded;      // The state of this resource (atomic since it will be modify by resource requests which run across multiple frames)
        TVector<ResourceRequesterID>            m_references;                                   // The list of references to this resources
        TInlineVector<ResourceID, 4>            m_installDependencyResourceIDs;                 // The list of resources that need to be loaded and installed before we can install this resource

        #if EE_DEVELOPMENT_TOOLS
        Milliseconds                            m_fileReadTime = 0;
        Milliseconds                            m_loadTime = 0;
        Milliseconds                            m_waitForDependenciesTime = 0;
        Milliseconds                            m_installTime = 0;
        #endif
    };
}