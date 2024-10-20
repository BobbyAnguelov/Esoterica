#include "Reflector.h"
#include "ReflectorSettingsAndUtils.h"
#include "Clang/ClangParser.h"
#include "CodeGenerator.h"

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
    static bool SortProjectsByDependencies( TVector<ProjectInfo>& projects )
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

    //-------------------------------------------------------------------------

    bool Reflector::LogError( char const* pFormat, ... ) const
    {
        char buffer[256];

        va_list args;
        va_start( args, pFormat );
        VPrintf( buffer, 256, pFormat, args );
        va_end( args );

        std::cout << std::endl << " ! Error: " << buffer << std::endl;
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

        std::cout << std::endl << " * Warning: " << buffer << std::endl;
        EE_TRACE_MSG( buffer );
    }

    //-------------------------------------------------------------------------

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

            m_solutionPath = slnPath.GetParentDirectory();

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
                    FileSystem::Path const projectPath = m_solutionPath + projectPathString;

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
                        m_excludedProjectPaths.emplace_back( projectPath );
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
            if ( !SortProjectsByDependencies( m_projects ) )
            {
                return LogError( "Illegal dependency in projects detected!" );
            }
        }

        std::cout << std::endl << " >>> Solution Parsing - Complete! ( " << (float) parsingTime.ToSeconds() << " ms )" << std::endl << std::endl;
        return true;
    }

    bool Reflector::ParseProject( FileSystem::Path const& prjPath )
    {
        EE_ASSERT( prjPath.IsUnderDirectory( m_solutionPath ) );

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

        std::cout << "Done! ( " << prj.m_headerFiles.size() << " header(s) found! )" << std::endl;

        m_projects.push_back( prj );
        return true;
    }

    Reflector::HeaderProcessResult Reflector::ProcessHeaderFile( FileSystem::Path const& filePath, String& exportMacroName, TVector<String>& headerFileContents )
    {
        // Open header file
        bool const isModuleAPIHeader = filePath.IsFilenameEqual( Settings::g_moduleAPIFilename );
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

        for ( auto const& line : headerFileContents )
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

    //-------------------------------------------------------------------------

    bool Reflector::Clean()
    {
        TVector<FileSystem::Path> generatedFiles;

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

        //-------------------------------------------------------------------------

        std::cout << " * Cleaning Solution: " << m_solutionPath.c_str() << std::endl;
        std::cout << " ----------------------------------------------" << std::endl << std::endl;

        // Delete all auto-generated directories for valid projects
        for ( auto& prj : m_projects )
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

            // Clean all generated files
            prj.GetAllGeneratedFiles( generatedFiles );
            for ( auto const& generatedFilePath : generatedFiles )
            {
                if ( !CreateEmptyFile( generatedFilePath ) )
                {
                    return false;
                }
            }
        }

        std::cout << std::endl;

        // Delete all auto-generated directories for excluded projects (just to be safe)
        for ( auto& prjPath : m_excludedProjectPaths )
        {
            FileSystem::Path const autoGeneratedDirectory = Utils::GetAutogeneratedDirectoryNameForProject( prjPath.GetParentDirectory() );
            std::cout << " * Cleaning Excluded Project - " << autoGeneratedDirectory.c_str() << std::endl;

            if ( !FileSystem::EraseDir( autoGeneratedDirectory ) )
            {
                return LogError( " * Error erasing directory: %s", autoGeneratedDirectory.c_str() );
            }
        }

        std::cout << std::endl;

        FileSystem::Path engineTypeRegistrationFilePath = m_solutionPath + Reflection::Settings::g_runtimeTypeRegistrationHeaderPath;
        if ( FileSystem::EraseFile( engineTypeRegistrationFilePath ) )
        {
            std::cout << " * Cleaning Engine Type Registration - " << engineTypeRegistrationFilePath.c_str() << std::endl;
        }
        else
        {
            return LogError( " * Error erasing file: %s", engineTypeRegistrationFilePath.c_str() );
        }

        FileSystem::Path editorTypeRegistrationFilePath = m_solutionPath + Reflection::Settings::g_toolsTypeRegistrationHeaderPath;
        if ( FileSystem::EraseFile( editorTypeRegistrationFilePath ) )
        {
            std::cout << " * Cleaning Editor Type Registration - " << editorTypeRegistrationFilePath.c_str() << std::endl;
        }
        else
        {
            return LogError( " * Error erasing file: %s", editorTypeRegistrationFilePath.c_str() );
        }

        //-------------------------------------------------------------------------

        std::cout << std::endl;
        std::cout << " >>> Cleaning - Complete!" << std::endl << std::endl;
        return true;
    }

    bool Reflector::Build()
    {
        ReflectionDatabase database( m_projects );

        std::cout << " * Reflecting Solution: " << m_solutionPath.c_str() << std::endl;
        std::cout << " ----------------------------------------------" << std::endl << std::endl;

        Milliseconds time = 0;
        {
            ScopedTimer<PlatformClock> timer( time );

            //-------------------------------------------------------------------------

            // Create list of all headers to parse
            TVector<HeaderInfo*> headersToParse;
            for ( auto& prj : m_projects )
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
                }
            }

            //-------------------------------------------------------------------------

            if ( !headersToParse.empty() )
            {
                std::cout << " * Reflecting C++ Code - First Pass (With Dev Tools) - ";

                // Parse headers
                ClangParser clangParser( m_solutionPath, &database );
                if ( !clangParser.Parse( headersToParse, ClangParser::DevToolsPass ) )
                {
                    std::cout << "\n ! Error: " << clangParser.GetErrorMessage().c_str() << std::endl;
                    return false;
                }
                Milliseconds clangParsingTime = clangParser.GetParsingTime();
                Milliseconds clangVisitingTime = clangParser.GetVisitingTime();
                std::cout << "Complete! ( P:" << (float) clangParsingTime << "ms, V:" << (float) clangVisitingTime << "ms )" << std::endl;

                std::cout << " * Reflecting C++ Code - Second Pass (No Dev Tools) - ";

                // Second parse to detect dev-only types
                if ( !clangParser.Parse( headersToParse, ClangParser::NoDevToolsPass ) )
                {
                    std::cout << "\n ! Error: " << clangParser.GetErrorMessage().c_str() << std::endl;
                    return false;
                }
                clangParsingTime = clangParser.GetParsingTime();
                clangVisitingTime = clangParser.GetVisitingTime();
                std::cout << "Complete! ( P:" << (float) clangParsingTime << "ms, V:" << (float) clangVisitingTime << "ms )" << std::endl;

                // Finalize database data
                if ( !database.ValidateDataFileRegistrations() )
                {
                    std::cout << "\n ! Database Validation Error: " << database.GetErrorMessage() << std::endl;
                    return false;
                }

                if ( !database.ProcessAndValidateReflectedProperties() )
                {
                    std::cout << "\n ! Database Validation Error: " << database.GetErrorMessage() << std::endl;
                    return false;
                }

                if ( database.HasWarnings() )
                {
                    std::cout << "\n ! Database Validation Warnings: \n" << database.GetWarningMessage() << std::endl;
                }

                database.CleanupResourceHierarchy();
            }

            //-------------------------------------------------------------------------

            std::cout << " * Generating Code - ";
            Milliseconds codeGenTime = 0;

            CodeGenerator generator( m_solutionPath, database );
            {
                ScopedTimer<PlatformClock> codeGenTimer( codeGenTime );
                if ( !generator.GenerateCodeForSolution() )
                {
                    return LogError( generator.GetErrorMessage() );
                }
            }

            if ( !WriteGeneratedFiles( generator ) )
            {
                std::cout << "\n ! Error: Failed to write generated files! Attempting to clean all autogenerated folders!" << std::endl;
                Clean();
                return false;
            }

            std::cout << "Complete! ( " << (float) codeGenTime << "ms )" << std::endl;

            //-------------------------------------------------------------------------

            if ( generator.HasWarnings() )
            {
                LogWarning( generator.GetWarningMessage() );
            }
        }

        std::cout << std::endl;
        std::cout << " >>> Reflection - Complete! ( " << (float) time.ToSeconds() << "s )" << std::endl;

        return true;
    }

    bool Reflector::WriteGeneratedFiles( class CodeGenerator const& generator )
    {
        EE_ASSERT( !generator.HasErrors() );
        TVector<CodeGenerator::GeneratedFile> const& generatedFiles = generator.GetGeneratedFiles();

        // Delete all auto-generated directories for excluded projects (just to be safe)
        //-------------------------------------------------------------------------

        for ( auto& prjPath : m_excludedProjectPaths )
        {
            FileSystem::Path const autoGeneratedDirectory = Utils::GetAutogeneratedDirectoryNameForProject( prjPath.GetParentDirectory() );
            if ( autoGeneratedDirectory.Exists() && !FileSystem::EraseDir( autoGeneratedDirectory ) )
            {
                return LogError( " * Error erasing directory: %s", autoGeneratedDirectory.c_str() );
            }
        }

        // Get list of files in all known auto-generated directories
        //-------------------------------------------------------------------------

        TVector<FileSystem::Path> existingFilesOnDisk;
        TVector<FileSystem::Path> directoryContents;

        for ( auto& prj : m_projects )
        {
            FileSystem::Path const autoGeneratedDirectory = prj.GetAutogeneratedDirectoryPath();
            FileSystem::GetDirectoryContents( autoGeneratedDirectory, directoryContents );

            for ( auto const& path : directoryContents )
            {
                // There should be no directories inside the auto-generated folders
                if( path.IsDirectoryPath() )
                { 
                    if ( !FileSystem::EraseDir( path ) )
                    {
                        return LogError( " * Error erasing directory: %s", path.c_str() );
                    }
                }
                else // Push discovered path
                {
                    existingFilesOnDisk.emplace_back( path );
                }
            }
        }

        // Add runtime and tools registration headers
        VectorEmplaceBackUnique( existingFilesOnDisk, m_solutionPath + Reflection::Settings::g_runtimeTypeRegistrationHeaderPath );
        VectorEmplaceBackUnique( existingFilesOnDisk, m_solutionPath + Reflection::Settings::g_toolsTypeRegistrationHeaderPath );

        // Compare list of existing files with expected files and delete any files that shouldn't exist
        //-------------------------------------------------------------------------

        for ( FileSystem::Path const& existingFile : existingFilesOnDisk )
        {
            if ( !VectorContains( generatedFiles, existingFile, [] ( CodeGenerator::GeneratedFile const& lhs, FileSystem::Path const& rhs ) { return lhs.m_path == rhs; } ) )
            {
                if ( !FileSystem::EraseFile( existingFile ) )
                {
                    return LogError( " * Error erasing file: %s", existingFile.c_str() );
                }
            }
        }

        // Write out any files that have actually changed
        //-------------------------------------------------------------------------

        for ( CodeGenerator::GeneratedFile const& generatedFile : generatedFiles )
        {
            if ( !FileSystem::UpdateTextFile( generatedFile.m_path, generatedFile.m_contents ) )
            {
                return LogError( " * Error creating generated file: %s", generatedFile.m_path.c_str() );
            }
        }

        return true;
    }
}