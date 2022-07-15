#pragma once

#include "ResourceServerWorker.h"
#include "ResourceCompilationRequest.h"
#include "CompiledResourceDatabase.h"
#include "EngineTools/Core/FileSystem/FileSystemWatcher.h"
#include "EngineTools/Resource/ResourceCompilerRegistry.h"
#include "System/Network/IPC/IPCMessageServer.h"
#include "System/Resource/ResourceSettings.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Threading/TaskSystem.h"

//-------------------------------------------------------------------------
// The network resource server
//-------------------------------------------------------------------------
// Receives resource requests, triggers the compilation of said requests and returns the results
// Runs in a separate thread from the main UI
//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceServer : public FileSystem::IFileSystemChangeListener
    {
    public:

        struct BusyState
        {
            int32_t     m_completedRequests = 0;
            int32_t     m_totalRequests = 0;
            bool        m_isBusy = false;
        };

    public:

        ResourceServer();
        ~ResourceServer();

        bool Initialize( IniFile const& iniFile );
        void Shutdown();
        void Update();

        BusyState GetBusyState() const;

        inline String const& GetErrorMessage() const { return m_errorMessage; }
        inline String const& GetNetworkAddress() const { return m_settings.m_resourceServerNetworkAddress; }
        inline uint16_t GetNetworkPort() const { return m_settings.m_resourceServerPort; }
        inline FileSystem::Path const& GetRawResourceDir() const { return m_settings.m_rawResourcePath; }
        inline FileSystem::Path const& GetCompiledResourceDir() const { return m_settings.m_compiledResourcePath; }

        // Compilers and Compilation
        //-------------------------------------------------------------------------

        inline CompilerRegistry const* GetCompilerRegistry() const { return m_pCompilerRegistry; }
        inline void CompileResource( ResourceID const& resourceID ) { CreateResourceRequest( resourceID, 0, CompilationRequest::Origin::ManualCompile ); }
        inline void PackageResource( ResourceID const& resourceID ) { CreateResourceRequest( resourceID, 0, CompilationRequest::Origin::Package ); }

        // Requests
        //-------------------------------------------------------------------------
        
        TVector<CompilationRequest const*> const& GetActiveRequests() const { return ( TVector<CompilationRequest const*>& ) m_activeRequests; }
        TVector<CompilationRequest const*> const& GetPendingRequests() const { return ( TVector<CompilationRequest const*>& ) m_pendingRequests; }
        TVector<CompilationRequest const*> const& GetCompletedRequests() const { return ( TVector<CompilationRequest const*>& ) m_completedRequests; }
        inline void RequestCleanupOfCompletedRequests() { m_cleanupRequested = true; }

        // Workers
        //-------------------------------------------------------------------------

        inline int32_t GetNumWorkers() const { return (int32_t) m_workers.size(); }
        inline ResourceServerWorker::Status GetWorkerStatus( int32_t workerIdx ) const { return m_workers[workerIdx]->GetStatus(); }
        inline ResourceID const& GetCompilationTaskResourceID( int32_t workerIdx ) const { return m_workers[workerIdx]->GetRequestResourceID(); }

        // Clients
        //-------------------------------------------------------------------------

        inline int32_t GetNumConnectedClients() const { return m_networkServer.GetNumConnectedClients(); }
        inline uint32_t GetClientID( int32_t clientIdx ) const { return m_networkServer.GetConnectedClientInfo( clientIdx ).m_ID; }
        inline Network::AddressString const& GetConnectedClientAddress( int32_t clientIdx ) const { return m_networkServer.GetConnectedClientInfo( clientIdx ).m_address; }

        // Packaging
        //-------------------------------------------------------------------------

        bool IsPackaging() const { return m_isPackaging; }

        // How far are we along with the packaging process
        inline float GetPackagingProgress() const { return ( float( m_completedPackagingRequests.size() ) / m_resourcesToBePackaged.size() ); }

        // Start the packaging process
        void StartPackaging();

        // Refresh the list of available maps to package
        void RefreshAvailableMapList();

        // Get list of maps in the raw source folder
        TVector<ResourceID> const& GetAllFoundMaps() { return m_allMaps; }

        // Get the current list of maps queued to be packaged
        TVector<ResourceID> const& GetMapsQueuedForPackaging() const { return m_mapsToBePackaged; }

        // Do we have any maps on the to-be-packaged list
        bool CanStartPackaging() const;

        // Add map to the to-be-packaged list
        void AddMapToPackagingList( ResourceID mapResourceID );

        // Remove map from the to-be-packaged list
        void RemoveMapFromPackagingList( ResourceID mapResourceID );

    private:

        // Requests
        //-------------------------------------------------------------------------

        void CleanupCompletedRequests();
        void CreateResourceRequest( ResourceID const& resourceID, uint32_t clientID = 0, CompilationRequest::Origin origin = CompilationRequest::Origin::External );
        void NotifyClientOnCompletedRequest( CompilationRequest* pRequest );

        // Up-to-date system
        //-------------------------------------------------------------------------

        void PerformResourceUpToDateCheck( CompilationRequest* pRequest, TVector<ResourcePath> const& compileDependencies ) const;
        bool TryReadCompileDependencies( FileSystem::Path const& resourceFilePath, TVector<ResourcePath>& outDependencies, String* pErrorLog = nullptr ) const;
        bool IsResourceUpToDate( ResourceID const& resourceID ) const;
        void WriteCompiledResourceRecord( CompilationRequest* pRequest );
        bool IsCompileableResourceType( ResourceTypeID ID ) const;

        // File system listener
        //-------------------------------------------------------------------------

        virtual void OnFileModified( FileSystem::Path const& filePath ) override final;

        // Packaging
        //-------------------------------------------------------------------------

        void EnqueueResourceForPackaging( ResourceID const& resourceID );

    private:

        TypeSystem::TypeRegistry                m_typeRegistry;
        CompilerRegistry*                       m_pCompilerRegistry = nullptr;
        String                                  m_errorMessage;
        bool                                    m_cleanupRequested = false;
        Network::IPC::Server                    m_networkServer;

        // Settings
        ResourceSettings                        m_settings;
        uint32_t                                m_maxSimultaneousCompilationTasks = 16;

        // Compilation Requests
        CompiledResourceDatabase                m_compiledResourceDatabase;
        TVector<CompilationRequest*>            m_completedRequests;
        TVector<CompilationRequest*>            m_pendingRequests;
        TVector<CompilationRequest*>            m_activeRequests;

        // Workers
        TaskSystem                              m_taskSystem;
        TVector<ResourceServerWorker*>          m_workers;

        // Packaging
        TVector<ResourceID>                     m_allMaps;
        TVector<ResourceID>                     m_mapsToBePackaged;
        TVector<ResourceID>                     m_resourcesToBePackaged;
        TVector<ResourceID>                     m_completedPackagingRequests;
        bool                                    m_isPackaging = false;

        // Busy state tracker
        int32_t                                 m_numRequestedResources = 0; // Counter that is request each time the server becomes idle

        // File System Watcher
        FileSystem::FileSystemWatcher           m_fileSystemWatcher;
    };
}