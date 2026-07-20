#include "Settings_Resource.h"
#include "Base/Settings/IniFile.h"
#include "Base/FileSystem/FileSystemUtils.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceSettings::ResourceSettings()
        : m_workingDirectoryPath( FileSystem::GetCurrentProcessPath() )
    {}

    void ResourceSettings::PostLoadOrModify()
    {
        // Compiled Resource Path
        //-------------------------------------------------------------------------

        m_compiledResourceDirectoryPath = m_workingDirectoryPath + m_compiledResourceDirectoryName.c_str();

        if ( !m_compiledResourceDirectoryPath.IsValid() )
        {
            EE_LOG_ERROR( LogCategory::Resource, "Resource Settings", "Invalid compiled data path: %s", m_compiledResourceDirectoryPath.c_str() );
        }

        m_compiledResourceDirectoryPath.MakeIntoDirectoryPath();

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        {
            // Raw Resource Path
            //-------------------------------------------------------------------------

            m_sourceDataDirectoryPath = m_workingDirectoryPath + m_sourceDataDirectoryPathStr.c_str();

            if ( !m_sourceDataDirectoryPath.IsValid() )
            {
                EE_LOG_ERROR( LogCategory::Resource, "Resource Settings", "Invalid source data path: %s", m_sourceDataDirectoryPath.c_str() );
            }

            m_sourceDataDirectoryPath.MakeIntoDirectoryPath();

            // Packaged Build Path
            //-------------------------------------------------------------------------

            m_packagedBuildCompiledResourceDirectoryPath = m_compiledResourceDirectoryPath.GetParentDirectory().GetParentDirectory();
            m_packagedBuildCompiledResourceDirectoryPath += m_packagedBuildName.c_str();
            m_packagedBuildCompiledResourceDirectoryPath += m_compiledResourceDirectoryName.c_str();

            if ( !m_packagedBuildCompiledResourceDirectoryPath.IsValid() )
            {
                EE_LOG_ERROR( LogCategory::Resource, "Resource Settings", "Invalid source data path: %s", m_sourceDataDirectoryPath.c_str() );
            }

            m_packagedBuildCompiledResourceDirectoryPath.MakeIntoDirectoryPath();

            // Compiled Resource DB
            //-------------------------------------------------------------------------

            m_compiledResourceDatabasePath = m_compiledResourceDirectoryPath + m_compiledDatabaseName.c_str();

            if ( !m_compiledResourceDatabasePath.IsValid() )
            {
                EE_LOG_ERROR( LogCategory::Resource, "Resource Settings", "Invalid compiled resource database path: %s", m_compiledResourceDatabasePath.c_str() );
            }

            // Resource Compiler Executable
            //-------------------------------------------------------------------------

            m_resourceCompilerExecutablePath = m_workingDirectoryPath + m_resourceCompilerExeName.c_str();

            if ( !m_resourceCompilerExecutablePath.IsValid() )
            {
                EE_LOG_ERROR( LogCategory::Resource, "Resource Settings", "Invalid resource compiler path: %s", m_resourceCompilerExecutablePath.c_str() );
            }

            // Resource Server Executable
            //-------------------------------------------------------------------------

            m_resourceServerExecutablePath = m_workingDirectoryPath + m_resourceServerExeName.c_str();

            if ( !m_resourceServerExecutablePath.IsValid() )
            {
                EE_LOG_ERROR( LogCategory::Resource, "Resource Settings", "Invalid resource server path: %s", m_resourceServerExecutablePath.c_str() );
            }
        }
        #endif
    }
}