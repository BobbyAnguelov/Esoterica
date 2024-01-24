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

        struct HeaderTimestamp
        {
            HeaderTimestamp( HeaderID ID, uint64_t timestamp ) : m_ID( ID ), m_timestamp( timestamp ) {}

            HeaderID                        m_ID;
            uint64_t                        m_timestamp;
        };

    public:

        Reflector() = default;

        bool ParseSolution( FileSystem::Path const& slnPath );
        bool Clean();
        bool Build();

    private:

        bool LogError( char const* pFormat, ... ) const;
        void LogWarning( char const* pFormat, ... ) const;

        bool ParseProject( FileSystem::Path const& prjPath );

        HeaderProcessResult ProcessHeaderFile( FileSystem::Path const& filePath, String& exportMacro, TVector<String>& headerFileContents );
        uint64_t CalculateHeaderChecksum( FileSystem::Path const& engineIncludePath, FileSystem::Path const& filePath );

        bool ReflectRegisteredHeaders();
        bool WriteTypeData();

    private:

        FileSystem::Path                    m_reflectionDataPath;
        SolutionInfo                        m_solution;
        ReflectionDatabase                  m_database;

        // Up to data checks
        TVector<HeaderTimestamp>            m_registeredHeaderTimestamps;
    };
}