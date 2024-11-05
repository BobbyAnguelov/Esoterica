#include "ReflectedProject.h"
#include <fstream>
#include <string>

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    static bool IsUnderToolsPath( FileSystem::Path const& path )
    {
        String const& filePathStr = path.GetString();
        for ( int32_t i = 0; i < Settings::g_numToolsProjects; i++ )
        {
            InlineString const searchStr( InlineString::CtorSprintf(), "\\%s\\", Settings::g_toolsProjects[i] );
            if ( filePathStr.find( searchStr.c_str() ) != String::npos )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    ReflectedProject::ReflectedProject( FileSystem::Path const& path )
        : m_ID( GetProjectID( path ) )
        , m_path( path )
        , m_parentDirectoryPath( path.GetParentDirectory() )
        , m_typeInfoDirectoryPath( GetProjectTypeInfoPath( m_parentDirectoryPath ) )
        , m_isToolsProject( IsUnderToolsPath( m_path ) )
    {}

    ReflectedProject::ParseResult ReflectedProject::Parse()
    {
        std::ifstream prjFile( m_path );
        if ( !prjFile.is_open() )
        {
            Utils::PrintError( "Could not open project file: %s", m_path.c_str() );
            return ParseResult::ErrorOccured;
        }

        bool moduleHeaderFound = false;
        bool moduleHeaderUnregistered = false;

        std::string stdLine;
        while ( std::getline( prjFile, stdLine ) )
        {
            String line( stdLine.c_str() );

            // Name
            auto firstIdx = line.find( "<ProjectName>" );
            if ( firstIdx != String::npos )
            {
                firstIdx += 13;
                m_name = line.substr( firstIdx, line.find( "</Project" ) - firstIdx );
                continue;
            }

            // Dependencies
            firstIdx = line.find( "<ProjectReference Include=\"" );
            if ( firstIdx != String::npos )
            {
                firstIdx += 27;
                String dependencyPath = line.substr( firstIdx, line.find( "\">" ) - firstIdx );
                dependencyPath = m_parentDirectoryPath + dependencyPath;
                StringID dependencyID = GetProjectID( dependencyPath );
                m_dependencies.push_back( dependencyID );
                continue;
            }

            // Header Files
            firstIdx = line.find( "<ClInclude" );
            if ( firstIdx != String::npos )
            {
                firstIdx = line.find( "Include=\"" );
                EE_ASSERT( firstIdx != String::npos );
                firstIdx += 9;
                auto secondIdx = line.find( "\"", firstIdx );
                EE_ASSERT( secondIdx != String::npos );

                String const headerPathStr = line.substr( firstIdx, secondIdx - firstIdx );

                // Ignore auto-generated files
                if ( headerPathStr.find( Reflection::Settings::g_autogeneratedDirectoryName ) != String::npos )
                {
                    continue;
                }

                // Create reflected header
                FileSystem::Path const headerFileFullPath( m_parentDirectoryPath + headerPathStr );
                ReflectedHeader hdr( headerFileFullPath, m_typeInfoDirectoryPath, m_ID, m_isToolsProject );

                if ( hdr.m_isModuleHeader )
                {
                    EE_ASSERT( !moduleHeaderFound );
                    moduleHeaderFound = true;
                }

                ReflectedHeader::ParseResult const result = hdr.Parse();
                switch ( result )
                {
                    case ReflectedHeader::ParseResult::ProcessHeader:
                    {
                        m_headerFiles.emplace_back( hdr );

                        if ( hdr.m_isModuleHeader )
                        {
                            EE_ASSERT( !m_moduleHeaderID.IsValid() );
                            m_moduleHeaderID = hdr.m_ID;
                        }
                    }
                    break;

                    case ReflectedHeader::ParseResult::IgnoreHeader:
                    {
                        // Extract the export macro from the API header
                        if ( hdr.m_isModuleAPIHeader )
                        {
                            EE_ASSERT( m_exportMacro.empty() );
                            m_exportMacro = hdr.m_exportMacro;
                        }

                        // If the module file isn't registered ignore the entire project
                        if ( hdr.m_isModuleHeader )
                        {
                            moduleHeaderUnregistered = true;
                            break;
                        }
                    }
                    break;

                    case ReflectedHeader::ParseResult::ErrorOccured:
                    {
                        Utils::PrintError( "Error processing: %s", headerFileFullPath.c_str() );
                        prjFile.close();
                        return ParseResult::ErrorOccured;
                    }
                }
            }
        }

        prjFile.close();

        // Name Validation
        //-------------------------------------------------------------------------

        if ( m_name.empty() )
        {
            Utils::PrintError( "Invalid project file detected: %s", m_path.c_str() );
            return ParseResult::ErrorOccured;
        }

        m_isBaseProject = ( m_name == Settings::g_allowedProjectNames[0] );

        // Return result
        //-------------------------------------------------------------------------

        if ( !moduleHeaderFound )
        {
            m_headerFiles.clear();
            return ParseResult::IgnoreProjectNoModule;
        }

        if ( moduleHeaderFound && moduleHeaderUnregistered )
        {
            m_headerFiles.clear();
            return ParseResult::IgnoreProjectModuleNotRegistered;
        }

        // Only add projects to the list that have headers to parse
        if ( m_headerFiles.empty() )
        {
            return ParseResult::IgnoreProjectModuleNoHeaders;
        }

        return ParseResult::ValidProject;
    }
}