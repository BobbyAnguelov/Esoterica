#pragma once
#include "EngineTools/Resource/ResourceCompilerRegistry.h"
#include "EngineTools/Resource/ResourceCompilationDatabase.h"
#include "Base/Network/Servers/NetworkServer_WebSockets.h"
#include "Base/Resource/Settings/Settings_Resource.h"
#include "Base/Settings/SettingsRegistry.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Threading/TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct ResourceServerContext
    {
        ResourceServerContext( TypeSystem::TypeRegistry const& typeRegistry );
        ~ResourceServerContext();

        bool Initialize( FileSystem::Path const& iniFilePath );
        void Shutdown();

        FileSystem::Path const& GetSourceDataDirectory() const { return m_pSettings->m_sourceDataDirectoryPath; }
        FileSystem::Path const& GetCompiledDataDirectory() const { return m_pSettings->m_compiledResourceDirectoryPath; }
        FileSystem::Path const& GetPackagedDataDirectory() const { return m_pSettings->m_packagedBuildCompiledResourceDirectoryPath; }

    public:

        Network::Server_WS                                          m_networkServer;
        TypeSystem::TypeRegistry const&                             m_typeRegistry;
        ResourceSettings const*                                     m_pSettings = nullptr;
        SettingsRegistry                                            m_settingsRegistry;
        CompiledResourceDatabase                                    m_compileResourceDB;
        TaskSystem                                                  m_taskSystem = TaskSystem( 2 );
        CompilerRegistry*                                           m_pCompilerRegistry = nullptr;

        TVector<uint64_t>                                           m_nonWorkerClientIDs;

        // Set when we shutdown the server to skip processing of any scheduled tasks
        std::atomic<bool>                                           m_isExiting = false;
    };
}