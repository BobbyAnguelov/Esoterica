#pragma once
#include "ReflectedProject.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    class ReflectedSolution
    {
    public:

        static FileSystem::Path GetRuntimeTypeRegistrationPath( FileSystem::Path const& solutionDirectoryPath );
        static FileSystem::Path GetToolsTypeRegistrationPath( FileSystem::Path const& solutionDirectoryPath );

    public:

        ReflectedSolution( FileSystem::Path const& path );

        bool Parse();

        bool DirtyAllProjectFiles();

        FileSystem::Path GetRuntimeTypeRegistrationPath() const { return GetRuntimeTypeRegistrationPath( m_parentDirectoryPath ); }
        FileSystem::Path GetToolsTypeRegistrationPath() const { return GetToolsTypeRegistrationPath( m_parentDirectoryPath ); }

    public:

        FileSystem::Path                                m_path;
        FileSystem::Path                                m_parentDirectoryPath;
        TVector<ReflectedProject>                       m_projects;
        TVector<FileSystem::Path>                       m_excludedProjectPaths;
    };
}