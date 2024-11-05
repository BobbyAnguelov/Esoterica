#include "ClangParser.h"
#include "ClangVisitors_TranslationUnit.h"
#include "Applications/Reflector/TypeReflection/ReflectionDatabase.h"
#include "Base/Time/Timers.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include <fstream>

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    static char const* const g_includePaths[] =
    {
        "Code\\",
        "Code\\Base\\ThirdParty\\EA\\EABase\\include\\common\\",
        "Code\\Base\\ThirdParty\\EA\\EASTL\\include\\",
        "Code\\Base\\ThirdParty\\",
        "Code\\Base\\ThirdParty\\imgui\\",
        "External\\PhysX\\physx\\include\\",
        "External\\Optick\\include\\",
        #if EE_ENABLE_NAVPOWER
        "External\\NavPower\\include\\"
        #endif
    };

    constexpr static int const g_numIncludePaths = sizeof( g_includePaths ) / sizeof( g_includePaths[0] );

    constexpr static char const* const g_tempDataPath = "\\..\\_Temp\\";

    //-------------------------------------------------------------------------

    ClangParser::ClangParser( FileSystem::Path const& solutionPath, ReflectionDatabase* pDatabase )
        : m_context( solutionPath, pDatabase )
        , m_totalParsingTime( 0 )
        , m_totalVisitingTime( 0 )
        , m_reflectionDataPath( FileSystem::GetCurrentProcessPath() + g_tempDataPath )
    {}

    bool ClangParser::Parse( TVector<ReflectedHeader*> const& headers, Pass pass )
    {
        m_context.m_detectDevOnlyTypesAndProperties = ( pass == NoDevToolsPass );

        // Create single amalgamated header file for all headers to parse
        //-------------------------------------------------------------------------

        std::ofstream reflectorFileStream;
        FileSystem::Path const reflectorHeader = m_reflectionDataPath + "Reflector.h";
        reflectorHeader.EnsureDirectoryExists();
        reflectorFileStream.open( reflectorHeader.c_str(), std::ios::out | std::ios::trunc );
        EE_ASSERT( !reflectorFileStream.fail() );

        String includeStr;
        m_context.m_headersToVisit.clear();
        for ( ReflectedHeader const* pHeader : headers )
        {
            // Exclude dev tools
            if ( pass == NoDevToolsPass && pHeader->m_isToolsHeader )
            {
                continue;
            }

            m_context.m_headersToVisit.emplace_back( pHeader->m_ID, pHeader );
            includeStr += "#include \"" + pHeader->m_path.GetString() + "\"\n";
        }

        reflectorFileStream.write( includeStr.c_str(), includeStr.size() );
        reflectorFileStream.close();

        // Clang args
        TInlineVector<String, 10> fullIncludePaths;
        TInlineVector<char const*, 10> clangArgs;
        for ( auto i = 0; i < g_numIncludePaths; i++ )
        {
            String const fullPath = m_context.m_solutionDirectoryPath.GetString() + g_includePaths[i];
            String const shortPath = Platform::Win32::GetShortPath( fullPath );
            fullIncludePaths.push_back( "-I" + shortPath );
            clangArgs.push_back( fullIncludePaths.back().c_str() );

            if ( !FileSystem::Exists( fullPath ) )
            {
                m_context.LogError( "Invalid include path: %s", fullPath.c_str() );
                return false;
            }
        }

        clangArgs.push_back( "-x" );
        clangArgs.push_back( "c++" );
        clangArgs.push_back( "-std=c++17" );
        clangArgs.push_back( "-O0" );
        clangArgs.push_back( "-D NDEBUG" );
        clangArgs.push_back( "-Werror" );
        clangArgs.push_back( "-Wno-multichar" );
        clangArgs.push_back( "-Wno-deprecated-builtins" );
        clangArgs.push_back( "-fparse-all-comments" );
        clangArgs.push_back( "-fms-extensions" );
        clangArgs.push_back( "-fms-compatibility" );
        clangArgs.push_back( "-Wno-unknown-warning-option" );
        clangArgs.push_back( "-Wno-return-type-c-linkage" );
        clangArgs.push_back( "-Wno-gnu-folding-constant" );
        clangArgs.push_back( "-Wno-vla-extension-static-assert" );

        // Exclude dev tools
        if ( pass == NoDevToolsPass )
        {
            clangArgs.push_back( "-D EE_SHIPPING" );
        }

        //-------------------------------------------------------------------------

        // Set up clang
        auto idx = clang_createIndex( 0, 1 );
        uint32_t const clangOptions = CXTranslationUnit_DetailedPreprocessingRecord | CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_IncludeBriefCommentsInCodeCompletion;

        // Parse Headers
        CXTranslationUnit tu;
        CXErrorCode result = CXError_Failure;
        {
            ScopedTimer<PlatformClock> timer( m_totalParsingTime );
            result = clang_parseTranslationUnit2( idx, reflectorHeader.c_str(), clangArgs.data(), clangArgs.size(), 0, 0, clangOptions, &tu );
        }

        // Handle result of parse
        if ( result == CXError_Success )
        {
            ScopedTimer<PlatformClock> timer( m_totalVisitingTime );
            m_context.Reset( &tu );
            auto cursor = clang_getTranslationUnitCursor( tu );
            clang_visitChildren( cursor, VisitTranslationUnit, &m_context );
        }
        else
        {
            switch ( result )
            {
                case CXError_Failure:
                m_context.LogError( "Clang Unknown failure" );
                break;

                case CXError_Crashed:
                m_context.LogError( "Clang crashed" );
                break;

                case CXError_InvalidArguments:
                m_context.LogError( "Clang Invalid arguments" );
                break;

                case CXError_ASTReadError:
                m_context.LogError( "Clang AST read error" );
                break;
            }
        }
        clang_disposeIndex( idx );

        //-------------------------------------------------------------------------

        // Check that we've processed all detected macros
        if ( !m_context.HasErrorOccured() )
        {
            m_context.CheckForUnhandledReflectionMacros();
        }

        // If we have an error from the parser, prepend the header to it
        if ( m_context.HasErrorOccured() )
        {
            m_context.LogError( "\n%s", m_context.GetErrorMessage() );
        }

        return !m_context.HasErrorOccured();
    }
}