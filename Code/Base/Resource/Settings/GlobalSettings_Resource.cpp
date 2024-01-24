#include "GlobalSettings_Resource.h"
#include "Base/Settings/IniFile.h"
#include "Base/FileSystem/FileSystemUtils.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceGlobalSettings::ResourceGlobalSettings()
        : m_workingDirectoryPath( FileSystem::GetCurrentProcessPath() )
    {}

    bool ResourceGlobalSettings::LoadSettings( Settings::IniFile const& ini )
    {
        // Settings
        //-------------------------------------------------------------------------

        m_compiledResourceDirectoryName = ini.GetStringOrDefault( "Resource:CompiledResourceDirectoryName", s_defaultCompiledResourceDirectoryName ); 

        #if EE_DEVELOPMENT_TOOLS
        {
            m_rawResourcePathStr = ini.GetStringOrDefault( "Resource:RawResourcePath", s_defaultRawResourcePath );
            m_packagedBuildName = ini.GetStringOrDefault( "Resource:PackagedBuildName", s_defaultPackagedBuildName );
            m_compiledDBName = ini.GetStringOrDefault( "Resource:CompiledResourceDatabaseName", s_defaultCompiledResourceDatabaseName );
            m_resourceCompilerExeName = ini.GetStringOrDefault( "Resource:ResourceCompilerExecutable", s_defaultResourceCompilerExecutableName );
            m_resourceServerExeName = ini.GetStringOrDefault( "Resource:ResourceServerExecutable", s_defaultResourceServerExecutableName );
            m_resourceServerNetworkAddress = ini.GetStringOrDefault( "Resource:ResourceServerAddress", s_defaultResourceServerAddress );
            m_resourceServerPort = (uint16_t) ini.GetUIntOrDefault( "Resource:ResourceServerPort", s_defaultResourceServerPort );
        }
        #endif

        //-------------------------------------------------------------------------

        return TryGeneratePaths();
    }

    bool ResourceGlobalSettings::TryGeneratePaths()
    {
        // Compiled Resource Path
        //-------------------------------------------------------------------------

        m_compiledResourcePath = m_workingDirectoryPath + m_compiledResourceDirectoryName.c_str();

        if ( !m_compiledResourcePath.IsValid() )
        {
            EE_LOG_ERROR( "Resource", "Resource Settings", "Invalid compiled data path: %s", m_compiledResourcePath.c_str() );
            return false;
        }

        m_compiledResourcePath.MakeIntoDirectoryPath();

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        {
            // Raw Resource Path
            //-------------------------------------------------------------------------

            m_rawResourcePath = m_workingDirectoryPath + m_rawResourcePathStr.c_str();

            if ( !m_rawResourcePath.IsValid() )
            {
                EE_LOG_ERROR( "Resource", "Resource Settings", "Invalid source data path: %s", m_rawResourcePath.c_str() );
                return false;
            }

            m_rawResourcePath.MakeIntoDirectoryPath();

            // Packaged Build Path
            //-------------------------------------------------------------------------

            m_packagedBuildCompiledResourcePath = m_compiledResourcePath.GetParentDirectory().GetParentDirectory();
            m_packagedBuildCompiledResourcePath += m_packagedBuildName.c_str();
            m_packagedBuildCompiledResourcePath += m_compiledResourceDirectoryName.c_str();

            if ( !m_packagedBuildCompiledResourcePath.IsValid() )
            {
                EE_LOG_ERROR( "Resource", "Resource Settings", "Invalid source data path: %s", m_rawResourcePath.c_str() );
                return false;
            }

            m_packagedBuildCompiledResourcePath.MakeIntoDirectoryPath();

            // Compiled Resource DB
            //-------------------------------------------------------------------------

            m_compiledResourceDatabasePath = m_compiledResourcePath + m_compiledDBName.c_str();

            if ( !m_compiledResourceDatabasePath.IsValid() )
            {
                EE_LOG_ERROR( "Resource", "Resource Settings", "Invalid compiled resource database path: %s", m_compiledResourceDatabasePath.c_str() );
                return false;
            }

            // Resource Compiler Executable
            //-------------------------------------------------------------------------

            m_resourceCompilerExecutablePath = m_workingDirectoryPath + m_resourceCompilerExeName.c_str();

            if ( !m_resourceCompilerExecutablePath.IsValid() )
            {
                EE_LOG_ERROR( "Resource", "Resource Settings", "Invalid resource compiler path: %s", m_resourceCompilerExecutablePath.c_str() );
                return false;
            }

            // Resource Server Executable
            //-------------------------------------------------------------------------

            m_resourceServerExecutablePath = m_workingDirectoryPath + m_resourceServerExeName.c_str();

            if ( !m_resourceServerExecutablePath.IsValid() )
            {
                EE_LOG_ERROR( "Resource", "Resource Settings", "Invalid resource server path: %s", m_resourceServerExecutablePath.c_str() );
                return false;
            }
        }
        #endif

        return true;
    }

    bool ResourceGlobalSettings::SaveSettings( Settings::IniFile& ini ) const
    {
        ini.CreateSection( "Resource" );
        ini.SetString( "Resource:CompiledResourceDirectoryName", m_compiledResourceDirectoryName );

        #if EE_DEVELOPMENT_TOOLS
        ini.SetString( "Resource:RawResourcePath", m_rawResourcePathStr );
        ini.SetString( "Resource:PackagedBuildName", m_packagedBuildName );
        ini.SetString( "Resource:CompiledResourceDatabaseName", m_compiledDBName );
        ini.SetString( "Resource:ResourceCompilerExecutable", m_resourceCompilerExeName );
        ini.SetString( "Resource:ResourceServerExecutable", m_resourceServerExeName );
        ini.SetString( "Resource:ResourceServerAddress", m_resourceServerNetworkAddress );
        ini.SetUInt( "Resource:ResourceServerPort", m_resourceServerPort );
        #endif

        return true;
    }
}