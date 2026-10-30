#pragma once

#include "Base/Types/Function.h"
#include "Base/Types/Arrays.h"
#include "Base/Threading/Threading.h"
#include "Base/Types/Severity.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_BASE_API Log
    {
    public:

        struct Entry
        {
            Severity    m_severity = Severity::Info;
            String      m_timestamp;
            String      m_message;
            uint64_t    m_userData = 0;
        };

        enum class Mode
        {
            NotThreadSafe,
            ThreadSafe
        };

    public:

        explicit Log( Mode mode = Mode::NotThreadSafe ) : m_mode( mode ) {}

        // Create a log that will automatically dump its contents to the system log on destruction
        Log( LogCategory category, char const* pSourceInfo, Mode mode = Mode::NotThreadSafe );

        // Create a log that will automatically dump its contents to the system log on destruction
        Log( LogCategory category, String const& sourceInfo, Mode mode = Mode::NotThreadSafe );

        // Copy Ctor
        Log( Log const& log );

        ~Log();

        Log& operator=( Log const& log );

        void Clear();

        inline bool HasWarnings() const { return m_numWarnings > 0; }
        inline bool HasErrors() const { return m_numErrors > 0; }
        int32_t GetNumWarnings() const { return m_numWarnings; }
        int32_t GetNumErrors() const { return m_numErrors; }

        void AddLogEntryVarArgs( Severity severity, uint64_t userData, char const* pFormat, va_list args );
        inline void AddLogEntry( Severity severity, uint64_t userData, char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); AddLogEntryVarArgs( severity, userData, pFormat, args ); va_end( args ); }
        template<typename T> inline void AddLogEntry( Severity severity, T const* pUserData, char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); AddLogEntryVarArgs( severity, uintptr_t( pUserData ), pFormat, args ); va_end( args ); }

        inline bool LogErrorVarArgs( char const* pFormat, va_list args ) { AddLogEntryVarArgs( Severity::Error, 0, pFormat, args ); return false; }
        inline bool LogError( char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); AddLogEntryVarArgs( Severity::Error, 0, pFormat, args ); va_end( args ); return false; }

        inline void LogWarningVarArgs( char const* pFormat, va_list args ) { AddLogEntryVarArgs( Severity::Warning, 0, pFormat, args ); }
        inline void LogWarning( char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); AddLogEntryVarArgs( Severity::Warning, 0, pFormat, args ); va_end( args ); }

        inline void LogInfoVarArgs( char const* pFormat, va_list args ) { AddLogEntryVarArgs( Severity::Info, 0, pFormat, args ); }
        inline void LogInfo( char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); AddLogEntryVarArgs( Severity::Info, 0, pFormat, args ); va_end( args ); }

        inline bool LogErrorWithUserDataVarArgs( uint64_t userData, char const* pFormat, va_list args ) { AddLogEntryVarArgs( Severity::Error, userData, pFormat, args ); return false; }
        inline bool LogErrorWithUserData( uint64_t userData, char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); AddLogEntryVarArgs( Severity::Error, userData, pFormat, args ); va_end( args ); return false; }
        template<typename T> inline bool LogErrorWithUserData( T const* pUserData, char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); AddLogEntryVarArgs( Severity::Error, uintptr_t( pUserData ), pFormat, args ); va_end( args ); return false; }

        inline void LogWarningWithUserDataVarArgs( uint64_t userData, char const* pFormat, va_list args ) { AddLogEntryVarArgs( Severity::Warning, userData, pFormat, args ); }
        inline void LogWarningWithUserData( uint64_t userData, char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); AddLogEntryVarArgs( Severity::Warning, userData, pFormat, args ); va_end( args ); }
        template<typename T> inline void LogWarningWithUserData( T const* pUserData, char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); AddLogEntryVarArgs( Severity::Warning, uintptr_t( pUserData ), pFormat, args ); va_end( args ); }

        inline void LogInfoWithUserDataVarArgs( uint64_t userData, char const* pFormat, va_list args ) { AddLogEntryVarArgs( Severity::Info, userData, pFormat, args ); }
        inline void LogInfoWithUserData( uint64_t userData, char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); AddLogEntryVarArgs( Severity::Info, userData, pFormat, args ); va_end( args ); }
        template<typename T> inline void LogInfoWithUserData( T const* pUserData, char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); AddLogEntryVarArgs( Severity::Info, uintptr_t( pUserData ), pFormat, args ); va_end( args ); }

        // Get the number of log entries
        int32_t GetNumLogEntries() const;

        // Get the set of log entries directly (Not threadsafe)
        TVector<Entry> const& GetLogEntries() const { EE_ASSERT( m_mode == Mode::NotThreadSafe ); return m_entries; }

        // Returns the log entries by value and takes a lock (Threadsafe)
        TVector<Entry> GetLogEntriesThreadsafe() const;

        // Iterate over all log entries
        void IterateLogEntries( TFunction<void( Entry const& entry )> const& perEntryFunction ) const;

        // Iterate over all log info messages
        void IterateInfos( TFunction<void( Entry const& entry )> const& perEntryFunction ) const;

        // Iterate over all log warnings
        void IterateWarnings( TFunction<void( Entry const& entry )> const& perEntryFunction ) const;

        // Iterate over all log errors
        void IterateErrors( TFunction<void( Entry const& entry )> const& perEntryFunction ) const;

        // Append another logs contents to this log
        void Append( Log const& otherLog );

        // Prints log contents out to the system log for the given category and source
        void ReflectToSystemLog( LogCategory category, char const* pSourceInfo );

        // Prints log contents out to the system log for the given category and source, clears the log after
        void ReflectToSystemLogAndClear( LogCategory category, char const* pSourceInfo );

        // Saves the log out to a single string
        void SaveLogToString( String& logString ) const;

        // Saves the log out to a file
        void SaveLogToFile( FileSystem::Path const& logFilePath ) const;

    private:

        Mode                                    m_mode = Mode::NotThreadSafe;
        mutable Threading::ReadWriteMutex       m_mutex;
        TVector<Entry>                          m_entries;
        std::atomic<int32_t>                    m_numWarnings = 0;
        std::atomic<int32_t>                    m_numErrors = 0;
        LogCategory                             m_category = LogCategory::Invalid;
        String                                  m_sourceInfo;
    };
}