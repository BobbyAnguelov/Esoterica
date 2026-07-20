#pragma once

#include "Base/_Module/API.h"
#include "Base/Math/Math.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Settings/Settings.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class EE_BASE_API ResourceSettings : public Settings
    {
        EE_REFLECT_TYPE( ResourceSettings );

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

        ResourceSettings();

        // Info
        //-------------------------------------------------------------------------

        virtual char const* GetSectionName() const override { return "Resource"; }

        // Resource Provider
        //-------------------------------------------------------------------------

        // Should we use the packaged resource data or the network resource provider
        inline bool UsePackagedResourceProvider() const
        {
            #if EE_DEVELOPMENT_TOOLS
            return m_usePackagedResourceProvider;
            #else
            return true;
            #endif
        }

        #if EE_DEVELOPMENT_TOOLS
        inline void SetUsePackagedResourceProvider( bool usePackagedResourceProvider )
        {
            m_usePackagedResourceProvider = usePackagedResourceProvider;
        }
        #endif

    private:

        virtual void PostLoadOrModify() override;

    public:

        FileSystem::Path const  m_workingDirectoryPath;

        // Settings
        //-------------------------------------------------------------------------

        EE_REFLECT();
        String                  m_compiledResourceDirectoryName = s_defaultCompiledResourceDirectoryName;

        #if EE_DEVELOPMENT_TOOLS
        EE_REFLECT();
        String                  m_sourceDataDirectoryPathStr = s_defaultSourceDataPath;

        EE_REFLECT();
        String                  m_packagedBuildName = s_defaultPackagedBuildName;

        EE_REFLECT();
        String                  m_compiledDatabaseName = s_defaultCompiledResourceDatabaseName;

        EE_REFLECT();
        String                  m_resourceCompilerExeName = s_defaultResourceCompilerExecutableName;

        EE_REFLECT();
        String                  m_resourceServerExeName = s_defaultResourceServerExecutableName;

        EE_REFLECT();
        String                  m_resourceServerNetworkAddress = s_defaultResourceServerAddress;

        EE_REFLECT();
        uint16_t                m_resourceServerPort = 5556;
        #endif

        // Transient Settings
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        bool                    m_usePackagedResourceProvider = false;
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