#include "ReflectedHeader.h"
#include "Base/FileSystem/FileSystem.h"
#include "ReflectionSettings.h"
#include <fstream>
#include <string>

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    static char const* g_macroNames[] =
    {
        "EE_REFLECT_MODULE",
        "EE_REFLECT_ENUM",
        "EE_REFLECT_TYPE",
        "EE_REFLECT",
        "EE_RESOURCE",
        "EE_DATA_FILE",
        "EE_ENTITY_COMPONENT",
        "EE_SINGLETON_ENTITY_COMPONENT",
        "EE_ENTITY_SYSTEM",
        "EE_ENTITY_WORLD_SYSTEM"
    };

    //-------------------------------------------------------------------------

    char const* ReflectedHeader::GetReflectionMacroText( ReflectionMacro macro )
    {
        uint32_t const macroIdx = (uint32_t) macro;
        EE_ASSERT( macroIdx < (uint32_t) ReflectionMacro::NumMacros );
        return g_macroNames[macroIdx];
    }

    //-------------------------------------------------------------------------

    ReflectedHeader::ReflectedHeader( FileSystem::Path const& headerPath, FileSystem::Path const& projectTypeInfoPath, StringID projectID, bool isToolsProject )
        : m_ID( GetHeaderID( headerPath ) )
        , m_path( headerPath )
        , m_projectID( projectID )
        , m_timestamp( FileSystem::GetFileModifiedTime( headerPath ) )
        , m_isToolsHeader( isToolsProject )
    {
        // Check if this is a special header
        if ( m_path.GetParentDirectory().GetDirectoryName() == Settings::g_moduleDirectoryName )
        {
            m_isModuleAPIHeader = m_path.IsFilenameEqual( Settings::g_moduleAPIFilename );

            if ( StringUtils::EndsWith( m_path.GetFilename(), Settings::g_moduleFileSuffix ) )
            {
                m_isModuleHeader = true;
            }
        }

        // Set up type-info output path
        EE_ASSERT( projectTypeInfoPath.IsDirectoryPath() );
        InlineString const filenameStr( InlineString::CtorSprintf(), "%s.%s.%s", headerPath.GetFilenameWithoutExtension().c_str(), Settings::g_typeinfoFileSuffix, "cpp" );
        m_typeInfoPath = projectTypeInfoPath.GetAppended( filenameStr.c_str() );
    }

    ReflectedHeader::ParseResult ReflectedHeader::Parse()
    {
        // Open header file
        std::ifstream hdrFile( m_path, std::ios::in | std::ios::binary | std::ios::ate );
        if ( !hdrFile.is_open() )
        {
            Utils::PrintError( "Could not open header file: %s", m_path.c_str() );
            return ParseResult::ErrorOccured;
        }

        // Check file size
        uint32_t const size = (uint32_t) hdrFile.tellg();
        if ( size == 0 )
        {
            hdrFile.close();
            return ParseResult::IgnoreHeader;
        }
        hdrFile.seekg( 0, std::ios::beg );

        // Read file contents
        std::string stdLine;
        while ( std::getline( hdrFile, stdLine ) )
        {
            m_fileContents.emplace_back( stdLine.c_str() );
        }
        hdrFile.close();

        //-------------------------------------------------------------------------

        // Check for the EE registration macros
        bool exportMacroFound = false;
        uint32_t openCommentBlock = 0;

        for ( auto const& line : m_fileContents )
        {
            // Check for comment blocks
            if ( line.find( "/*" ) != String::npos ) openCommentBlock++;
            if ( line.find( "*/" ) != String::npos ) openCommentBlock--;

            if ( openCommentBlock == 0 )
            {
                // Check for line comment
                auto const foundCommentIdx = line.find( "//" );

                // Check for registration macros
                for ( auto i = 0u; i < (uint32_t) ReflectionMacro::NumMacros; i++ )
                {
                    ReflectionMacro const macro = (ReflectionMacro) i;
                    auto const foundMacroIdx = line.find( GetReflectionMacroText( macro ) );
                    bool const macroExists = foundMacroIdx != String::npos;
                    bool const uncommentedMacro = foundCommentIdx == String::npos || foundCommentIdx > foundMacroIdx;

                    if ( macroExists && uncommentedMacro )
                    {
                        // We should never have registration macros and the export definition in the same file
                        if ( exportMacroFound )
                        {
                            Utils::PrintError( "EE registration macro found in the module export API header(%)!", m_path.c_str() );
                            return ParseResult::ErrorOccured;
                        }
                        else
                        {
                            return ParseResult::ProcessHeader;
                        }
                    }
                }

                // Check header for the module export definition
                if ( m_isModuleAPIHeader )
                {
                    auto const foundExportIdx0 = line.find( "__declspec" );
                    auto const foundExportIdx1 = line.find( "dllexport" );
                    if ( foundExportIdx0 != String::npos && foundExportIdx1 != String::npos )
                    {
                        if ( !m_exportMacro.empty() )
                        {
                            Utils::PrintError( "Duplicate export macro definitions found!", m_path.c_str() );
                            return ParseResult::ErrorOccured;
                        }
                        else
                        {
                            auto defineIdx = line.find( "#define" );
                            if ( defineIdx != String::npos )
                            {
                                defineIdx += 8;
                                m_exportMacro = line.substr( defineIdx, foundExportIdx0 - 1 - defineIdx );
                            }

                            return ParseResult::IgnoreHeader;
                        }
                    }
                }
            }
        }

        return ParseResult::IgnoreHeader;
    }
}