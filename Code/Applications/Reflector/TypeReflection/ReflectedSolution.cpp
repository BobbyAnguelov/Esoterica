#include "ReflectedSolution.h"
#include "Base/Utils/TopologicalSort.h"
#include "Base/Time/Timers.h"
#include "Base/FileSystem/FileSystem.h"
#include "EASTL/sort.h"

#include <iostream>
#include <string>
#include <fstream>

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    ReflectedSolution::ReflectedSolution( FileSystem::Path const& path )
        : m_path( path )
        , m_parentDirectoryPath( path.GetParentDirectory() )
    {}

    bool ReflectedSolution::Parse()
    {
        if ( !m_path.IsValid() || m_path.IsDirectoryPath() || !m_path.MatchesExtension( "sln" ) )
        {
            return Utils::PrintError( "Invalid solution file name: %s", m_path.c_str() );
        }

        if ( !FileSystem::Exists( m_path ) )
        {
            return Utils::PrintError( "Solution doesn't exist: %s", m_path.c_str() );
        }

        //-------------------------------------------------------------------------

        std::cout << " * Parsing Solution: " << m_path << std::endl;
        std::cout << " ----------------------------------------------" << std::endl << std::endl;

        Milliseconds parsingTime = 0.0f;
        {
            ScopedTimer<PlatformClock> timer( parsingTime );

            std::ifstream slnFile( m_path );
            if ( !slnFile.is_open() )
            {
                return Utils::PrintError( "Could not open solution: %s", m_path.c_str() );
            }

            TVector<FileSystem::Path> projectFilePaths;

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
                    FileSystem::Path const projectPath = m_parentDirectoryPath + projectPathString;

                    // Filter projects
                    //-------------------------------------------------------------------------

                    bool excludeProject = true;

                    // Ensure projects are within the allowed layers
                    for ( auto i = 0; i < Settings::g_numAllowedProjects; i++ )
                    {
                        if ( line.find( Settings::g_allowedProjectNames[i] ) != String::npos )
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
                        projectFilePaths.push_back( projectPath );
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

            eastl::sort( projectFilePaths.begin(), projectFilePaths.end(), sortPredicate );

            for ( FileSystem::Path const& projectFilePath : projectFilePaths )
            {
                EE_ASSERT( projectFilePath.IsUnderDirectory( m_parentDirectoryPath ) );

                ReflectedProject project( projectFilePath );

                std::cout << " * Project: " << projectFilePath.c_str() << " - ";

                ReflectedProject::ParseResult const result = project.Parse();
                switch ( result )
                {
                    case ReflectedProject::ParseResult::ErrorOccured:
                    {
                        return false;
                    }
                    break;

                    case ReflectedProject::ParseResult::IgnoreProjectModuleNoHeaders:
                    {
                        std::cout << "Ignored! ( No headers found! )" << std::endl;
                    }
                    break;

                    case ReflectedProject::ParseResult::IgnoreProjectModuleNotRegistered:
                    {
                        std::cout << "Ignored! ( Module not registered! )" << std::endl;
                    }
                    break;

                    case ReflectedProject::ParseResult::IgnoreProjectNoModule:
                    {
                        std::cout << "Ignored! ( Module-less project )" << std::endl;
                    }
                    break;

                    case ReflectedProject::ParseResult::ValidProject:
                    {
                        std::cout << "Done! ( " << project.m_headerFiles.size() << " header(s) found! )" << std::endl;
                        m_projects.emplace_back( project );
                    }
                    break;
                }
            }

            // Sort projects by dependencies
            //-------------------------------------------------------------------------

            int32_t const numProjects = (int32_t) m_projects.size();
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
                int32_t const numDependencies = (int32_t) m_projects[p].m_dependencies.size();
                for ( auto d = 0; d < numDependencies; d++ )
                {
                    for ( auto pp = 0; pp < numProjects; pp++ )
                    {
                        if ( p != pp && m_projects[pp].m_ID == m_projects[p].m_dependencies[d] )
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
            TVector<ReflectedProject> sortedProjects;
            sortedProjects.reserve( numProjects );

            for ( auto& node : list )
            {
                sortedProjects.push_back( m_projects[node.m_ID] );
                sortedProjects.back().m_dependencyCount = depValue++;
            }
            m_projects.swap( sortedProjects );
        }

        std::cout << std::endl << " >>> Solution Parsing - Complete! ( " << (float) parsingTime.ToSeconds() << " ms )" << std::endl << std::endl;
        return true;
    }

    FileSystem::Path ReflectedSolution::GetRuntimeTypeRegistrationPath( FileSystem::Path const& solutionDirectoryPath )
    {
        EE_ASSERT( solutionDirectoryPath.IsDirectoryPath() );

        FileSystem::Path path = solutionDirectoryPath;
        path.Append( "Code\\Engine", true );
        path.Append( Settings::g_moduleDirectoryName, true );
        path.Append( Settings::g_autogeneratedDirectoryName, true );
        path.Append( Settings::g_autogeneratedTypeInfoDirectoryName, true );
        path.Append( Settings::g_typeRegistrationFileName, false );

        return path;
    }

    FileSystem::Path ReflectedSolution::GetToolsTypeRegistrationPath( FileSystem::Path const& solutionDirectoryPath )
    {
        EE_ASSERT( solutionDirectoryPath.IsDirectoryPath() );

        FileSystem::Path path = solutionDirectoryPath;
        path.Append( "Code\\EngineTools", true );
        path.Append( Settings::g_moduleDirectoryName, true );
        path.Append( Settings::g_autogeneratedDirectoryName, true );
        path.Append( Settings::g_autogeneratedTypeInfoDirectoryName, true );
        path.Append( Settings::g_typeRegistrationFileName, false );

        return path;
    }

    bool ReflectedSolution::DirtyAllProjectFiles()
    {
        for ( ReflectedProject const& project : m_projects )
        {
            String projectFileContents;
            if ( FileSystem::ReadTextFile( project.m_path, projectFileContents ) )
            {
                if ( !FileSystem::WriteTextFile( project.m_path, projectFileContents ) )
                {
                    return Utils::PrintError( " * Failed to re-save project file: %s", project.m_path.c_str() );
                }
            }
            else
            {
                return Utils::PrintError( " * Failed to re-save project file: %s", project.m_path.c_str() );
            }
        }

        return true;
    }
}