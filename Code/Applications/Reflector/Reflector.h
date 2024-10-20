#pragma once

#include "Applications/Reflector/Database/ReflectionDatabase.h"
#include "Base/Time/Time.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    class Reflector
    {
        enum class HeaderProcessResult
        {
            ErrorOccured,
            ParseHeader,
            IgnoreHeader,
        };

    public:

        bool ParseSolution( FileSystem::Path const& slnPath );
        bool Clean();
        bool Build();

    private:

        bool LogError( char const* pFormat, ... ) const;
        void LogWarning( char const* pFormat, ... ) const;

        // Solution Parsing
        //-------------------------------------------------------------------------

        bool ParseProject( FileSystem::Path const& prjPath );
        HeaderProcessResult ProcessHeaderFile( FileSystem::Path const& filePath, String& exportMacroName, TVector<String>& headerFileContents );

        // Code Generation
        //-------------------------------------------------------------------------

        bool WriteGeneratedFiles( class CodeGenerator const& generator );

    private:

        FileSystem::Path                    m_solutionPath;
        TVector<ProjectInfo>                m_projects;
        TVector<FileSystem::Path>           m_excludedProjectPaths;
    };
}