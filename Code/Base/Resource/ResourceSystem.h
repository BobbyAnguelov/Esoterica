#pragma once

#include "Base/_Module/API.h"
#include "ResourcePtr.h"
#include "Base/Threading/Threading.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/Systems.h"
#include "Base/Types/Event.h"
#include "Base/Time/TimeStamp.h"
#include "Base/Types/HashMap.h"

//-------------------------------------------------------------------------

namespace EE { class TaskScheduler; }

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceProvider;
    class ResourceLoader;
    class ResourceRequest;
    class ResourceGlobalSettings;

    //-------------------------------------------------------------------------

    class EE_BASE_API ResourceSystem : public ISystem
    {
        friend class ResourceDebugView;

        struct PendingRequest
        {
            enum class Type { Load, Unload };

        public:

            PendingRequest() = default;

            PendingRequest( Type type, ResourceRecord* pRecord, ResourceRequesterID const& requesterID )
                : m_pRecord( pRecord )
                , m_requesterID( requesterID )
                , m_type( type )
            {
                EE_ASSERT( m_pRecord != nullptr );
            }

            ResourceRecord*         m_pRecord = nullptr;
            ResourceRequesterID     m_requesterID;
            Type                    m_type = Type::Load;
        };

        #if EE_DEVELOPMENT_TOOLS
        struct CompletedRequestLog
        {
            CompletedRequestLog( PendingRequest::Type type, ResourceID ID ) : m_type( type ), m_ID( ID ) {}

            PendingRequest::Type    m_type;
            ResourceID              m_ID;
            TimeStamp               m_time;
        };
        #endif

    public:

        EE_SYSTEM( ResourceSystem );

    public:

        ResourceSystem( TaskSystem& taskSystem );
        ~ResourceSystem();

        inline bool WasInitialized() const { return m_pResourceProvider != nullptr; }
        ResourceGlobalSettings const& GetSettings() const;
        void Initialize( ResourceProvider* pResourceProvider );
        void Shutdown();

        // Do we still have work we need to perform
        bool IsBusy() const;

        // Update all active requests (if you set the 'waitForAsyncTask' to true, this function will block and wait for the async task to complete)
        void Update( bool waitForAsyncTask = false );

        // Blocking wait for all requests to be completed
        void WaitForAllRequestsToComplete();

        // Resource Loaders
        //-------------------------------------------------------------------------

        void RegisterResourceLoader( ResourceLoader* pLoader );
        void UnregisterResourceLoader( ResourceLoader* pLoader );

        // Loading/Unloading
        //-------------------------------------------------------------------------

        // Request a load of a resource, can optionally provide a ResourceRequesterID for identification of the request source
        void LoadResource( ResourcePtr& resourcePtr, ResourceRequesterID const& requesterID = ResourceRequesterID() );

        // Request an unload of a resource, can optionally provide a ResourceRequesterID for identification of the request source
        void UnloadResource( ResourcePtr& resourcePtr, ResourceRequesterID const& requesterID = ResourceRequesterID() );

        template<typename T>
        inline void LoadResource( TResourcePtr<T>& resourcePtr, ResourceRequesterID const& requesterID = ResourceRequesterID() ) { LoadResource( (ResourcePtr&) resourcePtr, requesterID ); }

        template<typename T>
        inline void UnloadResource( TResourcePtr<T>& resourcePtr, ResourceRequesterID const& requesterID = ResourceRequesterID() ) { UnloadResource( (ResourcePtr&) resourcePtr, requesterID ); }

        // Hot Reload
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void RequestResourceHotReload( ResourceID const& resourceID );
        inline bool RequiresHotReloading() const { return !m_externallyUpdatedResources.empty(); }
        inline TInlineVector<ResourceRequesterID, 20> const& GetUsersToBeReloaded() const { return m_usersThatRequireReload; }
        inline TInlineVector<ResourceID, 20> const& GetResourcesToBeReloaded() const { return m_externallyUpdatedResources; }
        void ClearHotReloadRequests();
        #endif

    private:

        void UpdateResourceProvider();

        // Non-copyable
        ResourceSystem( const ResourceSystem& ) = delete;
        ResourceSystem( const ResourceSystem&& ) = delete;
        ResourceSystem& operator=( const ResourceSystem& ) = delete;
        ResourceSystem& operator=( const ResourceSystem&& ) = delete;

        ResourceRecord* FindOrCreateResourceRecord( ResourceID const& resourceID );
        ResourceRecord* FindExistingResourceRecord( ResourceID const& resourceID );

        void AddPendingRequest( PendingRequest&& request );
        ResourceRequest* TryFindActiveRequest( ResourceRecord const* pResourceRecord ) const;

        // Returns a list of all unique external references for the given resource
        void GetUsersForResource( ResourceRecord const* pResourceRecord, TInlineVector<ResourceRequesterID, 20>& requesterIDs ) const;

        // Returns a list of all unique external references for the given resource
        void GetDependentResourcesForResource( ResourceRecord const* pResourceRecord, TInlineVector<ResourceID, 20>& dependentResources ) const;

        // Process all queued resource requests
        void ProcessResourceRequests();

    private:

        TaskSystem&                                             m_taskSystem;
        ResourceProvider*                                       m_pResourceProvider = nullptr;
        THashMap<ResourceTypeID, ResourceLoader*>               m_resourceLoaders;
        THashMap<ResourceID, ResourceRecord*>                   m_resourceRecords;
        mutable Threading::RecursiveMutex                       m_accessLock;

        // Requests
        TVector<PendingRequest>                                 m_pendingRequests;
        TVector<ResourceRequest*>                               m_activeRequests;
        TVector<ResourceRequest*>                               m_completedRequests;

        // ASync
        AsyncTask                                               m_asyncProcessingTask;
        std::atomic<bool>                                       m_isAsyncTaskRunning = false;

        #if EE_DEVELOPMENT_TOOLS
        TInlineVector<ResourceRequesterID, 20>                  m_usersThatRequireReload;
        TInlineVector<ResourceID, 20>                           m_externallyUpdatedResources;
        TVector<CompletedRequestLog>                            m_history;
        #endif
    };
}