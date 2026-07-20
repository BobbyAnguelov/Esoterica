#pragma once

#include "ResourceServerContext.h"
#include "ResourceServerWorker.h"
#include "ResourceServerRequest.h"
#include "EngineTools/FileSystem/FileSystemWatcher.h"
#include "EngineTools/Core/DataFileUtils.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------
// The network resource server
//-------------------------------------------------------------------------
// Receives resource requests, triggers the compilation of said requests and returns the results
// Runs in a separate thread from the main UI
//-------------------------------------------------------------------------

namespace EE::Resource
{
    class PackagingTask;
    class CompilerRegistry;

    //-------------------------------------------------------------------------

    class ResourceServer
    {
    public:

        struct BusyState
        {
            int32_t                                 m_completedRequests = 0;
            int32_t                                 m_totalRequests = 0;
            bool                                    m_isBusy = false;
        };

        struct RecompilationBlocker
        {
            constexpr static float const            s_defaultBlockTime = 30.0f; // 30 seconds

        public:

            UUID                                    m_ID;
            DataPath                                m_path;
            CountdownTimer<PlatformClock>           m_timer;
            String                                  m_sourceID;
        };

        enum class PackagingStage
        {
            None, // Not Packaging
            Preparing,
            Packaging,
            Complete
        };

    public:

        ResourceServer( TypeSystem::TypeRegistry const& typeRegistry );

        bool Initialize( FileSystem::Path const& iniFilePath );
        void Shutdown();
        void Update();

        bool IsBusy() const;

        // Info
        //-------------------------------------------------------------------------

        inline String const& GetErrorMessage() const { return m_errorMessage; }
        inline String const& GetNetworkAddress() const { return m_context.m_pSettings->m_resourceServerNetworkAddress; }
        inline uint16_t GetNetworkPort() const { return m_context.m_pSettings->m_resourceServerPort; }
        inline FileSystem::Path const& GetSourceDataDirectory() const { return m_context.m_pSettings->m_sourceDataDirectoryPath; }
        inline FileSystem::Path const& GetCompiledResourceDirectory() const { return m_context.m_pSettings->m_compiledResourceDirectoryPath; }
        inline int32_t GetNumConnectedClients() const { return (int32_t) m_context.m_nonWorkerClientIDs.size(); }
        TInlineVector<Network::Server::ClientInfo const*, 32> GetConnectedClients() const;
        TInlineVector<ResourceServerWorker const*, 32> GetWorkers() const;

        // Compilers and Compilation
        //-------------------------------------------------------------------------

        inline CompilerRegistry const* GetCompilerRegistry() const { return m_context.m_pCompilerRegistry; }
        inline void CompileResource( ResourceID const& resourceID, bool forceRecompile = true ) { TryAddPendingRequest( forceRecompile ? RequestOrigin::ManualCompileForced : RequestOrigin::ManualCompile, resourceID ); }

        // Requests
        //-------------------------------------------------------------------------

        // Event fired after the requests lists has been updated
        inline TEventHandle<> OnRequestsUpdated() { return m_requestsUpdatedEvent; }
        TVector<Request const*> const& GetRequests() const { return ( TVector<Request const*>& ) m_requests; }
        inline void CleanHistory() { m_cleanupRequested = true; }

        // Packaging
        //-------------------------------------------------------------------------

        // Refresh the list of available maps to package
        void RefreshAvailableMapList();

        // Get list of maps in the raw source folder
        TVector<ResourceID> const& GetAllFoundMaps() { return m_allMaps; }

        // Get the current list of maps queued to be packaged
        TVector<ResourceID> const& GetMapsQueuedForPackaging() const { return m_mapsToBePackaged; }

        // Are we currently packaging a map
        inline bool IsPackaging() const { return m_packagingStage != PackagingStage::None && m_packagingStage != PackagingStage::Complete; }

        // Get the current stage of packaging
        PackagingStage GetPackagingStage() const { return m_packagingStage; }

        // How far are we along with the packaging process
        float GetPackagingProgress() const;

        // Add map to the to-be-packaged list
        void AddMapToPackagingList( ResourceID mapResourceID );

        // Remove map from the to-be-packaged list
        void RemoveMapFromPackagingList( ResourceID mapResourceID );

        // Do we have any maps on the to-be-packaged list
        bool CanStartPackaging() const;

        // Start the packaging process
        void StartPackaging();

        // Recompilation Blocking
        //-------------------------------------------------------------------------

        // Get the list of active recompilation blockers
        TVector<RecompilationBlocker> const& GetRecompilationBlockers() const { return m_recompilationBlockers; }

        TVector<ResourceID> GetBlockedRecompilationRequests() const { return m_blockedRecompilationRequests; }

        // Create a recompilation blocker that will defer any recompilation requests for this path (and anything under it) till the blocker expires or is removed
        void AddRecompilationBlocker( UUID const& blockerID, DataPath const& path, Seconds timeToBlock = RecompilationBlocker::s_defaultBlockTime, String const& optionalSourceID = String() );

        // Reset a recompilation blocker's timer
        void RefreshRecompilationBlocker( UUID const& blockerID, Seconds timeToBlock = RecompilationBlocker::s_defaultBlockTime );

        // Remove a recompilation blocker, this will trigger a recompile for any modified resources under the blocked path
        void RemoveRecompilationBlocker( UUID const& blockerID );

        // Tools
        //-------------------------------------------------------------------------

        bool DeleteCompiledResourceDatabase();

        // Data File Resave
        //-------------------------------------------------------------------------

        void RequestResaveOfDataFiles();
        void StartResaveOfDataFiles();
        void EndResaveOfDataFiles();
        bool IsResavingDataFiles() const;
        float GetDataFileResaveProgress() const;

    private:

        void UpdateNetwork();
        void UpdateFileSystemWatcher();

        // Requests
        //-------------------------------------------------------------------------

        Request* TryAddPendingRequest( uint64_t clientID, RequestOrigin origin, ResourceID const& resourceID, String const& extraInfo = String() );
        inline Request* TryAddPendingRequest( RequestOrigin origin, ResourceID const& resourceID, String const& extraInfo = String() ) {  return TryAddPendingRequest( 0, origin, resourceID, extraInfo ); }
        void ProcessPendingRequests();
        void CleanupCompletedRequests();

        // Workers/Tasks
        //-------------------------------------------------------------------------

        void UpdateWorkers();
        void ScheduleWorkerTask( Request* pRequest );

        // Recompilation Blocking
        //-------------------------------------------------------------------------

        bool IsResourceRecompilationBlocked( ResourceID const& ID ) const;
        void UpdateRecompilationBlockers();

        // Packaging
        //-------------------------------------------------------------------------

        void UpdatePackaging();
        void RunPackagingTask();
        void EnqueueResourceForPackaging( ResourceID const& resourceID );

        // Tools
        //-------------------------------------------------------------------------

        void UpdateResaveOfDataFiles();

    private:

        ResourceServerContext                                       m_context;
        String                                                      m_errorMessage;
        bool                                                        m_cleanupRequested = false;

        // Compilation Requests
        TVector<Request*>                                           m_pendingRequests;
        TVector<Request*>                                           m_requests;
        TEvent<>                                                    m_requestsUpdatedEvent;

        // Workers
        TVector<ResourceServerWorker>                               m_workers;
        int32_t                                                     m_workerEnqueueIndex = 0;

        // Packaging
        TVector<ResourceID>                                         m_allMaps;
        TVector<ResourceID>                                         m_mapsToBePackaged;
        TVector<Request const*>                                     m_packagingRequests;
        TVector<ResourceID>                                         m_packagingRuntimeDependencies;
        PinnedLambdaTask                                            m_packagingTask = PinnedLambdaTask( 1, [this] () { RunPackagingTask(); } );
        PackagingStage                                              m_packagingStage = PackagingStage::None;

        // File System Watcher
        FileSystem::Watcher                                         m_fileSystemWatcher;
        TVector<RecompilationBlocker>                               m_recompilationBlockers;
        TVector<ResourceID>                                         m_blockedRecompilationRequests;

        // Tools
        DataFileResaver*                                            m_pDataFileResaver = nullptr;
    };
}
