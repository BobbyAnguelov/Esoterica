#include "ResourceSettings.h"
#include "System/IniFile.h"

#include "System/FileSystem/FileSystemUtils.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    bool ResourceSettings::ReadSettings( IniFile const& ini )
    {
        String tmp;
        m_workingDirectoryPath = FileSystem::GetCurrentProcessPath();

        // Runtime settings
        //-------------------------------------------------------------------------

        String compiledResourceDirectoryName;

        if ( ini.TryGetString( "Paths:CompiledResourceDirectoryName", compiledResourceDirectoryName ) )
        {
            m_compiledResourcePath = m_workingDirectoryPath + compiledResourceDirectoryName;
            if ( !m_compiledResourcePath.IsValid() )
            {
                EE_LOG_ERROR( "Resource", "Resource Settings", "Invalid compiled data path: %s", m_compiledResourcePath.c_str());
                return false;
            }

            m_compiledResourcePath.MakeIntoDirectoryPath();
        }
        else
        {
            EE_LOG_ERROR( "Resource", "Resource Settings", "Failed to read compiled data path from ini file" );
            return false;
        }

        // Development only settings
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        {
            // Paths
            //-------------------------------------------------------------------------

            if ( ini.TryGetString( "Paths:RawResourcePath", tmp ) )
            {
                m_rawResourcePath = m_workingDirectoryPath + tmp;
                if ( !m_rawResourcePath.IsValid() )
                {
                    EE_LOG_ERROR( "Resource", "Resource Settings", "Invalid source data path: %s", m_compiledResourcePath.c_str() );
                    return false;
                }
            }
            else
            {
                EE_LOG_ERROR( "Resource", "Resource Settings", "Failed to read source data path from ini file" );
                return false;
            }

            if ( ini.TryGetString( "Paths:PackagedBuildRelativePath", tmp ) )
            {
                if ( tmp.empty() )
                {
                    EE_LOG_ERROR( "Resource", "Resource Settings", "Invalid packaged build relative path: %s", tmp.c_str() );
                    return false;
                }
                else
                {
                    m_packagedBuildCompiledResourcePath = m_compiledResourcePath.GetParentDirectory();
                    m_packagedBuildCompiledResourcePath += tmp;
                    m_packagedBuildCompiledResourcePath += compiledResourceDirectoryName;
                    m_packagedBuildCompiledResourcePath.MakeIntoDirectoryPath();
                    EE_ASSERT( m_packagedBuildCompiledResourcePath.IsValid() );
                }
            }
            else
            {
                EE_LOG_ERROR( "Resource", "Resource Settings", "Failed to read packaged build path from ini file" );
                return false;
            }

            // Compiled Resource DB
            //-------------------------------------------------------------------------

            if ( ini.TryGetString( "Resource:CompiledResourceDatabaseName", tmp ) )
            {
                m_compiledResourceDatabasePath = m_workingDirectoryPath + tmp;
                if ( !m_compiledResourceDatabasePath.IsValid() )
                {
                    EE_LOG_ERROR( "Resource", "Resource Settings", "Invalid compiled resource database path: %s", m_compiledResourceDatabasePath.c_str() );
                    return false;
                }
            }
            else
            {
                EE_LOG_ERROR( "Resource", "Resource Settings", "Failed to read compiled resource database path from ini file" );
                return false;
            }

            // Resource Compiler
            //-------------------------------------------------------------------------

            if ( ini.TryGetString( "Resource:ResourceCompilerExecutablePath", tmp ) )
            {
                m_resourceCompilerExecutablePath = m_workingDirectoryPath + tmp;
                if ( !m_resourceCompilerExecutablePath.IsValid() )
                {
                    EE_LOG_ERROR( "Resource", "Resource Settings", "Invalid resource compiler path: %s", m_resourceCompilerExecutablePath.c_str() );
                    return false;
                }
            }
            else
            {
                EE_LOG_ERROR( "Resource", "Resource Settings", "Failed to read resource compiler exe path from ini file" );
                return false;
            }

            // Resource Server
            //-------------------------------------------------------------------------

            if ( ini.TryGetString( "Resource:ResourceServerExecutablePath", tmp ) )
            {
                m_resourceServerExecutablePath = m_workingDirectoryPath + tmp;
                if ( !m_resourceServerExecutablePath.IsValid() )
                {
                    EE_LOG_ERROR( "Resource", "Resource Settings", "Invalid resource server path: %s", m_resourceServerExecutablePath.c_str() );
                    return false;
                }
            }
            else
            {
                EE_LOG_ERROR( "Resource", "Resource Settings", "Failed to read resource server exe path from ini file" );
                return false;
            }

            if ( !ini.TryGetString( "Resource:ResourceServerAddress", m_resourceServerNetworkAddress ) )
            {
                EE_LOG_ERROR( "Resource", "Resource Settings", "Failed to read ResourceServerAddress from ini file" );
                return false;
            }

            uint32_t tempValue;
            if ( ini.TryGetUInt( "Resource:ResourceServerPort", tempValue ) )
            {
                m_resourceServerPort = (uint16_t) tempValue;
            }
            else
            {
                EE_LOG_ERROR( "Resource", "Resource Settings", "Failed to read ResourceServerPort from ini file" );
                return false;
            }
        }
        #endif

        return true;
    }
}