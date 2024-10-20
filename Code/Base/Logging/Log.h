#pragma once

#include "LogEntry.h"
#include "Base/Types/Function.h"
#include "Base/Types/Arrays.h"
#include "Base/Threading/Threading.h"

//-------------------------------------------------------------------------

namespace EE::Log
{
    class EE_BASE_API Storage
    {

    public:

        // Write
        //-------------------------------------------------------------------------

        void AddDetailedEntryVarArgs( Severity severity, char const* pCategory, char const* pSourceInfo, char const* pFilename, int pLineNumber, char const* pMessageFormat, va_list args );

        inline void AddDetailedEntry( Severity severity, char const* pCategory, char const* pSourceInfo, char const* pFilename, int pLineNumber, char const* pMessageFormat, ... )
        {
            va_list args;
            va_start( args, pMessageFormat );
            AddDetailedEntryVarArgs( severity, pCategory, pSourceInfo, pFilename, pLineNumber, pMessageFormat, args );
            va_end( args );
        }

        inline void AddEntry( Severity severity, char const* pCategory, char const* pSourceInfo, char const* pMessageFormat, ... )
        {
            va_list args;
            va_start( args, pMessageFormat );
            AddDetailedEntryVarArgs( severity, pCategory, pSourceInfo, "", -1, pMessageFormat, args );
            va_end( args );
        }

        inline void AddEntry( Severity severity, char const* pCategory, char const* pSourceInfo, char const* pMessageFormat, va_list args )
        {
            AddDetailedEntryVarArgs( severity, pCategory, pSourceInfo, "", -1, pMessageFormat, args );
        }

        // Read
        //-------------------------------------------------------------------------

        void IterateLogEntries( TFunction<bool( Entry const& entry )> const& perEntryFunction )
        {
            Threading::ScopeLockRead sl( m_mutex );
            for ( auto const& entry : m_entries )
            {
                perEntryFunction( entry );
            }
        }

        // Returns a copy of the entire log
        TVector<Entry> GetEntries() const
        {
            Threading::ScopeLockRead sl( m_mutex );
            return m_entries;
        }

        int32_t GetNumWarnings() const { return m_numWarnings; }

        int32_t GetNumErrors() const { return m_numErrors; }

        // File
        //-------------------------------------------------------------------------

        void SaveToFile( FileSystem::Path const& logFilePath );

    protected:

        mutable Threading::ReadWriteMutex       m_mutex;
        TVector<Entry>                          m_entries;
        std::atomic<int32_t>                    m_numWarnings = 0;
        std::atomic<int32_t>                    m_numErrors = 0;
    };
}

