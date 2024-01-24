#include "Reflector.h"
#include "ReflectorSettingsAndUtils.h"
#include "Clang/ClangParser.h"
#include "CodeGenerator.h"
#include "Base/Application/ApplicationGlobalState.h"
#include "Base/ThirdParty/cmdParser/cmdParser.h"

#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/Time/Timers.h"
#include "Base/Utils/TopologicalSort.h"

#include <eastl/sort.h>
#include <fstream>
#include <iostream>
#include <filesystem>

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    bool Reflector::LogError( char const* pFormat, ... ) const
    {
        char buffer[256];

        va_list args;
        va_start( args, pFormat );
        VPrintf( buffer, 256, pFormat, args );
        va_end( args );

        std::cout << std::endl << "Error: " << buffer << std::endl;
        EE_TRACE_MSG( buffer );
        return false;
    }

    void Reflector::LogWarning( char const* pFormat, ... ) const
    {
        char buffer[256];

        va_list args;
        va_start( args, pFormat );
        VPrintf( buffer, 256, pFormat, args );
        va_end( args );

        std::cout << std::endl << "Warning: " << buffer << std::endl;
        EE_TRACE_MSG( buffer );
    }

    namespace
    {
        bool SortProjectsByDependencies( TVector<ProjectInfo>& projects )
        {
            int32_t const numProjects = (int32_t) projects.size();
            if ( numProjects <= 1 )
            {
                return true;
            }

            // Create list to sort
            TVector<TopologicalSorter::Node > list;
            for ( auto p = 0; p < numProjects; p++ )
            {
                list.push_back( TopologicalSorter::Node( p ) );
            }

            for ( auto p = 0; p < numProjects; p++ )
            {
                int32_t const numDependencies = (int32_t) projects[p].m_dependencies.size();
                for ( auto d = 0; d < numDependencies; d++ )
                {
                    for ( auto pp = 0; pp < numProjects; pp++ )
                    {
                        if ( p != pp && projects[pp].m_ID == projects[p].m_dependencies[d] )
                        {
                            list[p].m_children.push_back( &list[pp] );
                        }
                    }
                }
            }

            // Try to sort
            if ( !TopologicalSorter::Sort( list ) )
            {
                return false;
            }

            // Update type list
            uint32_t depValue = 0;
            TVector<ProjectInfo> sortedProjects;
            sortedProjects.reserve( numProjects );

            for ( auto& node : list )
            {
                sortedProjects.push_back( projects[node.m_ID] );
                sortedProjects.back().m_dependencyCount = depValue++;
            }
            projects.swap( sortedProjects );
            return true;
        }
    }

    bool Reflector::ParseSolution( FileSystem::Path const& slnPath )
    {
        if ( !slnPath.IsValid() || slnPath.IsDirectoryPath() || !slnPath.MatchesExtension( "sln" ) )
        {
            return LogError( "Invalid solution file name: %s", slnPath.c_str() );
        }

        if ( !FileSystem::Exists( slnPath ) )
        {
            return LogError( "Solution doesnt exist: %s", slnPath.c_str() );
        }

        //-------------------------------------------------------------------------

        std::cout << " * Parsing Solution: " << slnPath << std::endl;
        std::cout << " ----------------------------------------------" << std::endl << std::endl;

        Milliseconds parsingTime = 0.0f;
        {
            ScopedTimer<PlatformClock> timer( parsingTime );

            std::ifstream slnFile( slnPath );
            if ( !slnFile.is_open() )
            {
                return LogError( "Could not open solution: %s", slnPath.c_str() );
            }

            m_solution.m_path = slnPath.GetParentDirectory();
            m_reflectionDataPath = FileSystem::Path( FileSystem::GetCurrentProcessPath() + Settings::g_reflectionDatabaseDirectoryPath );

            TVector<FileSystem::Path> projectFiles;

            std::string stdLine;
            while ( std::getline( slnFile, stdLine ) )
            {
                String line( stdLine.c_str() );

                if ( line.find( "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}" ) != String::npos ) // VS Project UID
                {
                    auto projectNameStartIdx = line.find( " = \"" );
                    EE_ASSERT( projectNameStartIdx != std::string::npos );
                    projectNameStartIdx += 4;
                    auto projectNameEndIdx = line.find( "\", \"", projectNameStartIdx );
                    EE_ASSERT( projectNameEndIdx != std::string::npos );

                    auto projectPathStartIdx = projectNameEndIdx + 4;
                    EE_ASSERT( projectPathStartIdx < line.length() );
                    auto projectPathEndIdx = line.find( "\"", projectPathStartIdx );
                    EE_ASSERT( projectNameEndIdx != std::string::npos );

                    String const projectPathString = line.substr( projectPathStartIdx, projectPathEndIdx - projectPathStartIdx );
                    FileSystem::Path const projectPath = m_solution.m_path + projectPathString;

                    // Filter projects
                    //-------------------------------------------------------------------------

                    bool excludeProject = true;

                    // Ensure projects are within the allowed layers
                    for ( auto i = 0; i < Settings::g_numAllowedProjects; i++ )
                    {
                        if ( line.find( Settings::g_allowedProjectNames[i].m_pFullName ) != String::npos )
                        {
                            excludeProject = false;
                            break;
                        }
                    }

                    if ( excludeProject )
                    {
                        m_solution.m_excludedProjects.emplace_back( projectPath );
                    }
                    else
                    {
                        projectFiles.push_back( projectPath );
                    }
                }
            }

            slnFile.close();

            // Sort and parse project files
            //-------------------------------------------------------------------------

            auto sortPredicate = [] ( FileSystem::Path const& pathA, FileSystem::Path const& pathB )
            {
                return pathA.GetFullPath() < pathB.GetFullPath();
            };

            eastl::sort( projectFiles.begin(), projectFiles.end(), sortPredicate );

            for ( auto const& projectFile : projectFiles )
            {
                if ( !ParseProject( projectFile ) )
                {
                    return false;
                }
            }

            //-------------------------------------------------------------------------

            // Sort projects by dependencies
            if ( !SortProjectsByDependencies( m_solution.m_projects ) )
            {
                return LogError( "Illegal dependency in projects detected!" );
            }
        }

        std::cout << std::endl << " >>> Solution Parsing - Complete! ( " << (float) parsingTime.ToSeconds() << " ms )" << std::endl << std::endl;
        return true;
    }

    uint64_t Reflector::CalculateHeaderChecksum( FileSystem::Path const& engineIncludePath, FileSystem::Path const& filePath )
    {
        static char const* headerString = "Note: including file: ";
        uint64_t checksum = FileSystem::GetFileModifiedTime( filePath );
        return checksum;
    }

    bool Reflector::ParseProject( FileSystem::Path const& prjPath )
    {
        EE_ASSERT( prjPath.IsUnderDirectory( m_solution.m_path ) );

        std::ifstream prjFile( prjPath );
        if ( !prjFile.is_open() )
        {
            return LogError( "Could not open project file: %s", prjPath.c_str() );
        }

        bool moduleHeaderFound = false;
        bool moduleHeaderUnregistered = false;

        ProjectInfo prj;
        prj.m_ID = ProjectInfo::GetProjectID( prjPath );
        prj.m_path = prjPath.GetParentDirectory();
        prj.m_isToolsModule = Utils::IsFileUnderToolsProject( prjPath );

        std::string stdLine;
        while ( std::getline( prjFile, stdLine ) )
        {
            String line( stdLine.c_str() );

            // Name
            auto firstIdx = line.find( "<ProjectName>" );
            if ( firstIdx != String::npos )
            {
                firstIdx += 13;
                prj.m_name = line.substr( firstIdx, line.find( "</Project" ) - firstIdx );
                continue;
            }

            // Short Name
            for ( auto i = 0; i < Settings::g_numAllowedProjects; i++ )
            {
                if ( prj.m_name.find( Settings::g_allowedProjectNames[i].m_pFullName ) != String::npos )
                {
                    prj.m_shortName = Settings::g_allowedProjectNames[i].m_pShortName;
                }
            }

            // Dependencies
            firstIdx = line.find( "<ProjectReference Include=\"" );
            if ( firstIdx != String::npos )
            {
                firstIdx += 27;
                String dependencyPath = line.substr( firstIdx, line.find( "\">" ) - firstIdx );
                dependencyPath = prjPath.GetParentDirectory() + dependencyPath;
                auto dependencyID = ProjectInfo::GetProjectID( dependencyPath );
                prj.m_dependencies.push_back( dependencyID );
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

                String const headerFilePathStr = line.substr( firstIdx, secondIdx - firstIdx );

                // Ignore auto-generated files
                if ( headerFilePathStr.find( Reflection::Settings::g_autogeneratedDirectoryName ) != String::npos )
                {
                    continue;
                }

                // Ignore any headers on the ignored list
                for ( int32_t k = 0; k < Settings::g_numFilenamesToIgnore; k++ )
                {
                    if ( headerFilePathStr.find( Settings::g_filenameToIgnore[k] ) != String::npos )
                    {
                        continue;
                    }
                }

                // Is this the module header file?
                bool isModuleHeader = false;
                FileSystem::Path const headerFileFullPath( prj.m_path + headerFilePathStr );
                if ( headerFileFullPath.GetParentDirectory().GetDirectoryName() == Settings::g_moduleDirectoryName )
                {
                    if ( StringUtils::EndsWith( headerFileFullPath.GetFilename(), Settings::g_moduleFileSuffix ) )
                    {
                        isModuleHeader = true;
                        moduleHeaderFound = true;
                    }
                }

                TVector<String> headerFileContents;
                auto const result = ProcessHeaderFile( headerFileFullPath, prj.m_exportMacro, headerFileContents );
                switch ( result )
                {
                    case HeaderProcessResult::ParseHeader:
                    {
                        HeaderInfo& headerInfo = prj.m_headerFiles.emplace_back();
                        headerInfo.m_ID = HeaderInfo::GetHeaderID( headerFileFullPath );
                        headerInfo.m_projectID = prj.m_ID;
                        headerInfo.m_filePath = headerFileFullPath;
                        headerInfo.m_timestamp = FileSystem::GetFileModifiedTime( headerFileFullPath );
                        headerInfo.m_fileContents.swap( headerFileContents );

                        // Add to registered timestamp cache, use in up to date checks
                        m_registeredHeaderTimestamps.push_back( HeaderTimestamp( headerInfo.m_ID, headerInfo.m_timestamp ) );

                        if ( isModuleHeader )
                        {
                            prj.m_moduleHeaderID = headerInfo.m_ID;
                        }
                    }
                    break;

                    case HeaderProcessResult::IgnoreHeader:
                    {
                        // If the module file isn't registered ignore the entire project
                        if ( isModuleHeader )
                        {
                            moduleHeaderUnregistered = true;
                            break;
                        }
                    }
                    break;

                    case HeaderProcessResult::ErrorOccured:
                    {
                        std::cout << "Error processing: " << headerFileFullPath << std::endl;
                        prjFile.close();
                        return false;
                    }
                }
            }
        }

        prjFile.close();

        // Validation
        //-------------------------------------------------------------------------

        if ( prj.m_name.empty() )
        {
            return LogError( "Invalid project file detected: %s", prjPath.c_str() );
        }

        if ( prj.m_shortName.empty() )
        {
            return LogError( "Could not generate project short name from full name: %s", prj.m_name.c_str() );
        }

        // Print parse results
        //-------------------------------------------------------------------------

        std::cout << " * Project: " << prj.m_name.c_str() << " - ";

        if ( moduleHeaderFound && moduleHeaderUnregistered )
        {
            std::cout << "Ignored! ( Module not registered! )" << std::endl;
            return true;
        }

        if ( !moduleHeaderFound )
        {
            std::cout << "Ignored! ( Module-less project )" << std::endl;
            prj.m_headerFiles.clear();
            return true;
        }

        // Only add projects to the list that have headers to parse
        if ( prj.m_headerFiles.empty() )
        {
            std::cout << "Ignored! ( No headers found! )" << std::endl;
            return true;
        }

        std::cout << "Done! ( " << prj.m_headerFiles.size() << " header(s) found! )";

        m_solution.m_projects.push_back( prj );
        return true;
    }

    Reflector::HeaderProcessResult Reflector::ProcessHeaderFile( FileSystem::Path const& filePath, String& exportMacroName, TVector<String>& headerFileContents )
    {
        // Open header file
        bool const isModuleAPIHeader = filePath.IsFilenameEqual( Settings::g_moduleAPIFileName );
        std::ifstream hdrFile( filePath, std::ios::in | std::ios::binary | std::ios::ate );
        if ( !hdrFile.is_open() )
        {
            LogError( "Could not open header file: %s", filePath.c_str() );
            return HeaderProcessResult::ErrorOccured;
        }

        // Check file size
        uint32_t const size = (uint32_t) hdrFile.tellg();
        if ( size == 0 )
        {
            hdrFile.close();
            return HeaderProcessResult::IgnoreHeader;
        }
        hdrFile.seekg( 0, std::ios::beg );

        // Read file contents
        headerFileContents.clear();
        std::string stdLine;
        while ( std::getline( hdrFile, stdLine ) )
        {
            headerFileContents.emplace_back( stdLine.c_str() );
        }
        hdrFile.close();

        //-------------------------------------------------------------------------

        // Check for the EE registration macros
        bool exportMacroFound = false;
        uint32_t openCommentBlock = 0;

        for( auto const& line : headerFileContents )
        {
            // Check for comment blocks
            if ( line.find( "/*" ) != String::npos ) openCommentBlock++;
            if ( line.find( "*/" ) != String::npos ) openCommentBlock--;

            if ( openCommentBlock == 0 )
            {
                // Check for line comment
                auto const foundCommentIdx = line.find( "//" );

                // Check for registration macros
                for ( auto i = 0u; i < (uint32_t) ReflectionMacroType::NumMacros; i++ )
                {
                    ReflectionMacroType const macro = (ReflectionMacroType) i;
                    auto const foundMacroIdx = line.find( GetReflectionMacroText( macro ) );
                    bool const macroExists = foundMacroIdx != String::npos;
                    bool const uncommentedMacro = foundCommentIdx == String::npos || foundCommentIdx > foundMacroIdx;

                    if ( macroExists && uncommentedMacro )
                    {
                        // We should never have registration macros and the export definition in the same file
                        if ( exportMacroFound )
                        {
                            LogError( "EE registration macro found in the module export API header(%)!", filePath.c_str() );
                            return HeaderProcessResult::ErrorOccured;
                        }
                        else
                        {
                            return HeaderProcessResult::ParseHeader;
                        }
                    }
                }

                // Check header for the module export definition
                if ( isModuleAPIHeader )
                {
                    auto const foundExportIdx0 = line.find( "__declspec" );
                    auto const foundExportIdx1 = line.find( "dllexport" );
                    if ( foundExportIdx0 != String::npos && foundExportIdx1 != String::npos )
                    {
                        if ( !exportMacroName.empty() )
                        {
                            LogError( "Duplicate export macro definitions found!", filePath.c_str() );
                            return HeaderProcessResult::ErrorOccured;
                        }
                        else
                        {
                            auto defineIdx = line.find( "#define" );
                            if ( defineIdx != String::npos )
                            {
                                defineIdx += 8;
                                exportMacroName = line.substr( defineIdx, foundExportIdx0 - 1 - defineIdx );
                            }

                            return HeaderProcessResult::IgnoreHeader;
                        }
                    }
                }
            }
        }

        return HeaderProcessResult::IgnoreHeader;
    }

    bool Reflector::Clean()
    {
        std::cout << " * Cleaning Solution: " << m_solution.m_path.c_str() << std::endl;
        std::cout << " ----------------------------------------------" << std::endl << std::endl;

        // Delete all auto-generated directories for valid projects
        for ( auto& prj : m_solution.m_projects )
        {
            std::cout << " * Cleaning Project - " << prj.m_name.c_str() << std::endl;

            FileSystem::Path const autoGeneratedDirectory = prj.GetAutogeneratedDirectoryPath();
            if ( !FileSystem::EraseDir( autoGeneratedDirectory ) )
            {
                return LogError( " * Error erasing directory: %s", autoGeneratedDirectory.c_str() );
            }

            // Create autogen dir
            if ( !FileSystem::CreateDir( autoGeneratedDirectory ) )
            {
                return LogError( " * Error erasing directory: %s", autoGeneratedDirectory.c_str() );
            }

            // Create the module type info and code gen files
            auto CreateEmptyFile = [this] ( FileSystem::Path const& filePath )
            {
                std::fstream outputFile( filePath.c_str(), std::ios::out | std::ios::trunc );
                if ( !outputFile.is_open() )
                {
                    return LogError( " * Error creating module file: %s", filePath.c_str() );
                }
                outputFile.close();

                return true;
            };

            FileSystem::Path const& typeInfoFilePath = prj.GetTypeInfoFilePath();
            if ( !CreateEmptyFile( typeInfoFilePath ) )
            {
                return false;
            }

            FileSystem::Path const& codeGenFilePath = prj.GetCodeGenFilePath();
            if ( !CreateEmptyFile( codeGenFilePath ) )
            {
                return false;
            }
        }

        std::cout << std::endl;

        // Delete all auto-generated directories for excluded projects (just to be safe)
        for ( auto& prjPath : m_solution.m_excludedProjects )
        {
            FileSystem::Path const autoGeneratedDirectory = Utils::GetAutogeneratedDirectoryNameForProject( prjPath.GetParentDirectory() );
            std::cout << " * Cleaning Excluded Project - " << autoGeneratedDirectory.c_str() << std::endl;

            if ( !FileSystem::EraseDir( autoGeneratedDirectory ) )
            {
                return LogError( " * Error erasing directory: %s", autoGeneratedDirectory.c_str() );
            }
        }

        std::cout << std::endl;

        FileSystem::Path const globalAutoGeneratedDirectory = m_solution.m_path + Reflection::Settings::g_globalAutoGeneratedDirectory;
        if ( FileSystem::EraseDir( globalAutoGeneratedDirectory ) )
        {
            std::cout << " * Cleaning Global Autogenerated Directory - " << globalAutoGeneratedDirectory.c_str() << std::endl;
        }
        else
        {
            return LogError( " * Error erasing directory: %s", ( m_solution.m_path + Reflection::Settings::g_globalAutoGeneratedDirectory ).c_str() );
        }

        //-------------------------------------------------------------------------

        std::cout << std::endl;

        FileSystem::Path const databasePath( m_reflectionDataPath + "TypeDatabase.db" );
        bool const result = !FileSystem::Exists( databasePath ) || FileSystem::EraseFile( databasePath );
        if ( result )
        {
            std::cout << " * Deleted: " << databasePath.c_str() << std::endl;
        }
        else
        {
            std::cout << " * Error deleting: " << databasePath.c_str() << std::endl;
        }

        std::cout << std::endl;
        std::cout << " >>> Cleaning - Complete!" << std::endl << std::endl;
        return result;
    }

    bool Reflector::Build()
    {
        std::cout << " * Reflecting Solution: " << m_solution.m_path.c_str() << std::endl;
        std::cout << " ----------------------------------------------" << std::endl << std::endl;

        Milliseconds time = 0;
        {
            ScopedTimer<PlatformClock> timer( time );

            // Open connection to type database and read all previous data
            FileSystem::Path databasePath( m_reflectionDataPath + "TypeDatabase.db" );
            m_database.ReadDatabase( databasePath );

            // For now just reflect all detected headers as we have disabled the up-to-date check
            for ( ProjectInfo& projectInfo : m_solution.m_projects )
            {
                for ( HeaderInfo const& headerInfo : projectInfo.m_headerFiles )
                {
                    m_database.UpdateHeaderRecord( headerInfo );
                }
            }

            //-------------------------------------------------------------------------

            if ( !ReflectRegisteredHeaders() )
            {
                return false;
            }
        }

        std::cout << std::endl;
        std::cout << " >>> Reflection - Complete! ( " << (float) time.ToSeconds() << "s )" << std::endl;

        return true;
    }

    bool Reflector::ReflectRegisteredHeaders()
    {
        // Create list of all headers to parse
        TVector<HeaderInfo*> headersToParse;
        for ( auto& prj : m_solution.m_projects )
        {
            // Ignore projects with no module header
            if ( !prj.m_moduleHeaderID.IsValid() )
            {
                continue;
            }

            // Add all dirty headers to the list of file to be parsed
            bool moduleHeaderAdded = false;
            for ( HeaderInfo& headerInfo : prj.m_headerFiles )
            {
                headersToParse.push_back( &headerInfo );

                // Erase all types associated with this header from the database
                m_database.DeleteTypesForHeader( headerInfo.m_ID );
            }
        }

        //-------------------------------------------------------------------------

        if ( !headersToParse.empty() )
        {
            std::cout << " * Reflecting C++ Code - First Pass (With Dev Tools) - ";

            // Parse headers
            ClangParser clangParser( &m_solution, &m_database, m_reflectionDataPath );
            if ( !clangParser.Parse( headersToParse, ClangParser::DevToolsPass ) )
            {
                std::cout << "Error occurred!\n\n  Error: " << clangParser.GetErrorMessage().c_str() << std::endl;
                return false;
            }
            Milliseconds clangParsingTime = clangParser.GetParsingTime();
            Milliseconds clangVisitingTime = clangParser.GetVisitingTime();
            std::cout << "Complete! ( P:" << (float) clangParsingTime << "ms, V:" << (float) clangVisitingTime << "ms )" << std::endl;

            std::cout << " * Reflecting C++ Code - Second Pass (No Dev Tools) - ";

            // Second parse to detect dev-only types
            if ( !clangParser.Parse( headersToParse, ClangParser::NoDevToolsPass ) )
            {
                std::cout << "Error occurred!\n\n  Error: " << clangParser.GetErrorMessage().c_str() << std::endl;
                return false;
            }
            clangParsingTime = clangParser.GetParsingTime();
            clangVisitingTime = clangParser.GetVisitingTime();
            std::cout << "Complete! ( P:" << (float) clangParsingTime << "ms, V:" << (float) clangVisitingTime << "ms )" << std::endl;

            // Finalize database data
            m_database.UpdateProjectList( m_solution.m_projects );
            m_database.CleanupResourceHierarchy();
        }

        //-------------------------------------------------------------------------

        std::cout << " * Generating Code - ";
        Milliseconds time = 0;

        CodeGenerator generator( m_database );
        {
            ScopedTimer<PlatformClock> timer( time );
            if ( !generator.GenerateCodeForSolution( m_solution ) )
            {
                return LogError( generator.GetErrorMessage() );
            }
        }

        std::cout << "Complete! ( " << (float) time << "ms )" << std::endl;

        //-------------------------------------------------------------------------

        if ( !WriteTypeData() )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        if ( generator.HasWarnings() )
        {
            LogWarning( generator.GetWarningMessage() );
        }

        return true;
    }

    bool Reflector::WriteTypeData()
    {
        std::cout << " * Writing Type Database - ";

        Milliseconds time = 0;
        {
            ScopedTimer<PlatformClock> timer( time );
            if ( !m_database.WriteDatabase( m_reflectionDataPath + "TypeDatabase.db" ) )
            {
                return LogError( m_database.GetError().c_str() );
            }
        }

        std::cout << "Complete! ( " << (float) time << "ms )" << std::endl;
        return true;
    }
}

//-------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
    EE::ApplicationGlobalState State;

    // Set precision of cout
    //-------------------------------------------------------------------------

    std::cout.setf( std::ios::fixed, std::ios::floatfield );
    std::cout.precision( 2 );

    // Read CMD line arguments
    //-------------------------------------------------------------------------

    cli::Parser cmdParser( argc, argv );
    cmdParser.set_required<std::string>( "s", "SlnPath", "Solution Path." );
    cmdParser.set_optional<bool>( "clean", "Clean", false, "Clean Solution." );

    if ( !cmdParser.run() )
    {
        EE_LOG_ERROR( "Type System", "Reflector", "Invalid commandline arguments" );
        return 1;
    }

    EE::FileSystem::Path slnPath = cmdParser.get<std::string>( "s" ).c_str();
    bool const shouldBuild = !cmdParser.get<bool>( "clean" );

    // Execute reflector
    //-------------------------------------------------------------------------

    std::cout << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << " * Esoterica Reflector" << std::endl;
    std::cout << "===============================================" << std::endl << std::endl;

    // Parse solution
    EE::TypeSystem::Reflection::Reflector reflector;
    if ( reflector.ParseSolution( slnPath ) )
    {
        if ( !reflector.Clean() )
        {
            std::cout << std::endl << "Failed to clean: " << slnPath.c_str() << std::endl;
            EE_TRACE_MSG( "Failed to clean: %s", slnPath.c_str() );
            return 1;
        }

        if ( shouldBuild )
        {
            return reflector.Build() ? 0 : 1;
        }
    }

    return 0;
}