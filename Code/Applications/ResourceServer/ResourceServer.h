#pragma once

#include "ResourceServerContext.h"
#include "ResourceCompilationRequest.h"
#include "EngineTools/Core/FileSystem/FileSystemWatcher.h"
#include "System/Network/IPC/IPCMessageServer.h"
#include "System/Resource/ResourceSettings.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Threading/TaskSystem.h"
#include "System/Threading/Threading.h"

//-------------------------------------------------------------------------
// The network resource server
//-------------------------------------------------------------------------
// Receives resource requests, triggers the compilation of said requests and returns the results
// Runs in a separate thread from the main UI
//-------------------------------------------------------------------------

namespace EE::Resource
{
    class CompilationTask;
    class PackagingTask;

    //-------------------------------------------------------------------------

    class ResourceServer : public FileSystem::IFileSystemChangeListener
    {
    public:

        struct BusyState
        {
            int32_t     m_completedRequests = 0;
            int32_t     m_totalRequests = 0;
            bool        m_isBusy = false;
        };

        enum class PackagingStage
        {
            None, // Not Packaging
            Preparing,
            Packaging,
            Complete
        };

    public:

        ResourceServer() = default;
        ~ResourceServer();

        bool Initialize( IniFile const& iniFile );
        void Shutdown();
        void Update();

        bool IsBusy() const;

        inline String const& GetErrorMessage() const { return m_errorMessage; }
        inline String const& GetNetworkAddress() const { return m_settings.m_resourceServerNetworkAddress; }
        inline uint16_t GetNetworkPort() const { return m_settings.m_resourceServerPort; }
        inline FileSystem::Path const& GetRawResourceDir() const { return m_settings.m_rawResourcePath; }
        inline FileSystem::Path const& GetCompiledResourceDir() const { return m_settings.m_compiledResourcePath; }

        // Compilers and Compilation
        //-------------------------------------------------------------------------

        inline CompilerRegistry const* GetCompilerRegistry() const { return m_pCompilerRegistry; }
        inline void CompileResource( ResourceID const& resourceID, bool forceRecompile = true ) { CreateResourceRequest( resourceID, 0, forceRecompile ? CompilationRequest::Origin::ManualCompileForced : CompilationRequest::Origin::ManualCompile ); }
        inline void PackageResource( ResourceID const& resourceID ) { CreateResourceRequest( resourceID, 0, CompilationRequest::Origin::Package ); }

        // Requests
        //-------------------------------------------------------------------------

        TVector<CompilationRequest const*> const& GetRequests() const { return ( TVector<CompilationRequest const*>& ) m_requests; }
        inline void CleanHistory() { m_cleanupRequested = true; }

        // Clients
        //-------------------------------------------------------------------------

        inline int32_t GetNumConnectedClients() const { return m_networkServer.GetNumConnectedClients(); }
        inline uint32_t GetClientID( int32_t clientIdx ) const { return m_networkServer.GetConnectedClientInfo( clientIdx ).m_ID; }
        inline Network::AddressString const& GetConnectedClientAddress( int32_t clientIdx ) const { return m_networkServer.GetConnectedClientInfo( clientIdx ).m_address; }

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

    private:

        // Requests
        //-------------------------------------------------------------------------

        CompilationRequest* CreateResourceRequest( ResourceID const& resourceID, uint32_t clientID = 0, CompilationRequest::Origin origin = CompilationRequest::Origin::External );
        void ProcessCompletedRequests();
        void NotifyClientOnCompletedRequest( CompilationRequest* pRequest );

        // File system listener
        //-------------------------------------------------------------------------

        virtual void OnFileModified( FileSystem::Path const& filePath ) override final;

    private:

        Network::IPC::Server                                        m_networkServer;
        TypeSystem::TypeRegistry                                    m_typeRegistry;
        TaskSystem                                                  m_taskSystem;
        CompilerRegistry*                                           m_pCompilerRegistry = nullptr;
        String                                                      m_errorMessage;
        bool                                                        m_cleanupRequested = false;

        // Settings
        ResourceSettings                                            m_settings;

        // Compilation Requests
        CompiledResourceDatabase                                    m_compiledResourceDatabase;
        TVector<CompilationRequest*>                                m_requests;
        Threading::LockFreeQueue<CompilationTask*>                  m_completedTasks;
        std::atomic<int64_t>                                        m_numScheduledTasks = 0;

        // Workers
        ResourceServerContext                                       m_context;

        // Packaging
        TVector<ResourceID>                                         m_allMaps;
        TVector<ResourceID>                                         m_mapsToBePackaged;
        TVector<CompilationRequest const*>                          m_packagingRequests;
        PackagingTask*                                              m_pPackagingTask = nullptr;
        PackagingStage                                              m_packagingStage = PackagingStage::None;

        // File System Watcher
        FileSystem::FileSystemWatcher                               m_fileSystemWatcher;
    };
}