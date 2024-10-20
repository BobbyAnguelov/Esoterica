#pragma once

#include "Base/_Module/API.h"
#include "Base/Math/Math.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Settings/Settings.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class EE_BASE_API ResourceGlobalSettings : public Settings::GlobalSettings
    {
        EE_REFLECT_TYPE( ResourceGlobalSettings );

        // Paths
        //-------------------------------------------------------------------------
        // Default paths are relative to the working directory of the process
        constexpr static char const * const s_defaultSourceDataPath = "./../../Data/";
        constexpr static char const * const s_defaultPackagedBuildName = "x64_Shipping";

        // Resource Compiler
        //-------------------------------------------------------------------------

        constexpr static char const * const s_defaultResourceCompilerExecutableName = "EsotericaResourceCompiler.exe";
        constexpr static char const * const s_defaultCompiledResourceDirectoryName = "CompiledData";
        constexpr static char const * const s_defaultCompiledResourceDatabaseName = "CompiledData.db";

        // Resource Server
        //-------------------------------------------------------------------------

        constexpr static char const * const s_defaultResourceServerExecutableName = "EsotericaResourceServer.exe";
        constexpr static char const * const s_defaultResourceServerAddress = "127.0.0.1";
        constexpr static uint16_t const s_defaultResourceServerPort = 5556;

    public:

        ResourceGlobalSettings();

        virtual bool LoadSettings( Settings::IniFile const& ini ) override;
        virtual bool SaveSettings( Settings::IniFile& ini ) const override;

    private:

        bool TryGeneratePaths();

    public:

        FileSystem::Path const  m_workingDirectoryPath;

        // Settings
        //-------------------------------------------------------------------------

        String                  m_compiledResourceDirectoryName = s_defaultCompiledResourceDirectoryName;

        #if EE_DEVELOPMENT_TOOLS
        String                  m_sourceDataDirectoryPathStr = s_defaultSourceDataPath;
        String                  m_packagedBuildName = s_defaultPackagedBuildName;
        String                  m_compiledDBName = s_defaultCompiledResourceDatabaseName;
        String                  m_resourceCompilerExeName = s_defaultResourceCompilerExecutableName;
        String                  m_resourceServerExeName = s_defaultResourceServerExecutableName;
        String                  m_resourceServerNetworkAddress = s_defaultResourceServerAddress;
        uint16_t                m_resourceServerPort = 5556;
        #endif

        // Derived Paths
        //-------------------------------------------------------------------------

        FileSystem::Path        m_compiledResourceDirectoryPath;

        #if EE_DEVELOPMENT_TOOLS
        FileSystem::Path        m_sourceDataDirectoryPath;
        FileSystem::Path        m_packagedBuildCompiledResourceDirectoryPath;
        FileSystem::Path        m_compiledResourceDatabasePath;
        FileSystem::Path        m_resourceCompilerExecutablePath;
        FileSystem::Path        m_resourceServerExecutablePath;
        #endif
    };
}