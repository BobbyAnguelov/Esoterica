#pragma once
#include "EngineTools/Resource/ResourceCompiler.h"
#include "EngineTools/Resource/ResourceCompilationDatabase.h"
#include "EngineTools/Resource/ResourceCompilerNetworkMessages.h"
#include "Base/Network/Clients/NetworkClient_WebSockets.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Settings/SettingsRegistry.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/Threading/Threading.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceSettings;
    class CompilerRegistry;

    //-------------------------------------------------------------------------

    class ResourceCompilerApplication
    {
        constexpr static float const s_identificationResendCooldownSeconds = 1.0f;
        constexpr static float const s_heartbeatSendIntervalSeconds = 1.0f;
        constexpr static float const s_maxUpToDateLoopProcessingTimeMilliseconds = 60.0f;
        constexpr static float const s_maxCompletionLoopProcessingTimeMilliseconds = 60.0f;

        struct Request : public CompileContext
        {
            using CompileContext::CompileContext;

            UUID                                        m_sourceTaskID;
            CountdownTimer<PlatformClock>               m_heartbeatTimer;
        };

        //-------------------------------------------------------------------------

        struct AsyncCompileTask : public IPinnedTask
        {
            AsyncCompileTask( ResourceCompilerApplication* pApplication ) : IPinnedTask( 1 ), m_pApp( pApplication ) { EE_ASSERT( m_pApp != nullptr ); }
            virtual void Execute() override final;

        private:

            ResourceCompilerApplication* m_pApp = nullptr;
        };

    public:

        ResourceCompilerApplication();
        ~ResourceCompilerApplication();

        bool Initialize( struct CommandLine& argParser, FileSystem::Path const& iniFilePath );
        void Shutdown();

        int32_t Run();

    private:

        CompilationResult RunStandaloneCompile();

        bool RunWorker();
        void HandleNetworkMessage( Network::Message const& message );

    private:

        TypeSystem::TypeRegistry                        m_typeRegistry;
        SettingsRegistry                                m_settingsRegistry;
        CompiledResourceDatabase                        m_compiledResourceDB;
        CompilerRegistry*                               m_pCompilerRegistry = nullptr;
        Resource::ResourceSettings const*               m_pSettings = nullptr;

        // Standalone compilation
        bool                                            m_isStandaloneCompile = false;
        ResourceID                                      m_resourceToCompile;
        bool                                            m_isForPackagedBuild = false;
        bool                                            m_forceCompilation = false;

        // Worker mode
        int64_t                                         m_uniqueID = 0;
        Network::Client_WS                              m_networkClient;
        TaskSystem                                      m_taskSystem = TaskSystem( 1 );
        AsyncCompileTask                                m_asyncCompileTask = AsyncCompileTask( this );
        TVector<Request*>                               m_requests;
        TVector<Request*>                               m_upToDateCheckQueue;
        Threading::TLockFreeQueue<Request*>             m_compileQueue;
        Threading::TLockFreeQueue<Request*>             m_completedQueue;
        NetworkResourceCompilerRequest                  m_requestHeartbeatsMessageData;
        NetworkResourceCompilerResponse                 m_responseMessageData;
        CountdownTimer<PlatformClock>                   m_IDTimer;
    };
}