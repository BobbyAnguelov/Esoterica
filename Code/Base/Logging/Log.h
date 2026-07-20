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
            Severity    m_severity;
            String      m_timestamp;
            String      m_message;
        };

    public:

        Log() = default;

        // Create a log that will automatically dump its contents to the system log on destruction
        Log( LogCategory category, char const* pSourceInfo );

        // Create a log that will automatically dump its contents to the system log on destruction
        Log( LogCategory category, String const& sourceInfo );

        ~Log();

        void ClearLog() 
        {
            m_entries.clear();
            m_numWarnings = m_numErrors = 0;
        }

        inline bool HasWarnings() const { return m_numWarnings > 0; }
        inline bool HasErrors() const { return m_numErrors > 0; }
        int32_t GetNumWarnings() const { return m_numWarnings; }
        int32_t GetNumErrors() const { return m_numErrors; }

        bool LogError( char const* pFormat, va_list args );
        inline bool LogError( char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); LogError( pFormat, args ); va_end( args ); return false; }

        void LogWarning( char const* pFormat, va_list args );
        inline void LogWarning( char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); LogWarning( pFormat, args ); va_end( args ); }
      
        void LogInfo( char const* pFormat, va_list args );
        inline void LogInfo( char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); LogInfo( pFormat, args ); va_end( args ); }

        int32_t GetNumLogEntries() const { return (int32_t) m_entries.size(); }
        TVector<Entry> const& GetLogEntries() const { return m_entries; }

        void IterateLogEntries( TFunction<void( Entry const& entry )> const& perEntryFunction )
        {
            for ( auto const& entry : m_entries )
            {
                perEntryFunction( entry );
            }
        }

        void IterateInfos( TFunction<void( Entry const& entry )> const& perEntryFunction )
        {
            for ( auto const& entry : m_entries )
            {
                if ( entry.m_severity == Severity::Info )
                {
                    perEntryFunction( entry );
                }
            }
        }

        void IterateWarnings( TFunction<void( Entry const& entry )> const& perEntryFunction )
        {
            for ( auto const& entry : m_entries )
            {
                if ( entry.m_severity == Severity::Warning )
                {
                    perEntryFunction( entry );
                }
            }
        }

        void IterateErrors( TFunction<void( Entry const& entry )> const& perEntryFunction )
        {
            for ( auto const& entry : m_entries )
            {
                if ( entry.m_severity == Severity::Error || entry.m_severity == Severity::FatalError )
                {
                    perEntryFunction( entry );
                }
            }
        }

        // Append another logs contents to this log
        void Append( Log const& otherLog );

        // Prints log contents out to the system log for the given category and source
        void ReflectToSystemLog( LogCategory category, char const* pSourceInfo );

        // Prints log contents out to the system log for the given category and source, clears the log after
        void ReflectToSystemLogAndClear( LogCategory category, char const* pSourceInfo );

        // Saves the log out to a single string
        void SaveLogToString( String& logString );

        // Saves the log out to a file
        void SaveLogToFile( FileSystem::Path const& logFilePath );

    private:

        TVector<Entry>              m_entries;
        int32_t                     m_numWarnings = 0;
        int32_t                     m_numErrors = 0;
        LogCategory                 m_category = LogCategory::Invalid;
        String                      m_sourceInfo;
    };

    //-------------------------------------------------------------------------

    class EE_BASE_API LogThreadSafe
    {

    public:

        LogThreadSafe() = default;

        // Create a log that will automatically dump its contents to the system log on destruction
        LogThreadSafe( LogCategory category, char const* pSourceInfo );

        // Create a log that will automatically dump its contents to the system log on destruction
        LogThreadSafe( LogCategory category, String const& sourceInfo );

        ~LogThreadSafe();

        void ClearLog()
        {
            Threading::ScopeLockRead sl( m_mutex );
            m_entries.clear();
        }

        inline bool HasWarnings() const { return m_numWarnings > 0; }
        inline bool HasErrors() const { return m_numErrors > 0; }
        int32_t GetNumWarnings() const { return m_numWarnings; }
        int32_t GetNumErrors() const { return m_numErrors; }

        bool LogError( char const* pFormat, va_list args );
        inline bool LogError( char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); LogError( pFormat, args ); va_end( args ); return false; }
 
        void LogWarning( char const* pFormat, va_list args );
        inline void LogWarning( char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); LogWarning( pFormat, args ); va_end( args ); }
    
        void LogInfo( char const* pFormat, va_list args );
        inline void LogInfo( char const* pFormat, ... ) { va_list args; va_start( args, pFormat ); LogInfo( pFormat, args ); va_end( args ); }

        void IterateLogEntries( TFunction<void( Log::Entry const& entry )> const& perEntryFunction )
        {
            Threading::ScopeLockRead sl( m_mutex );
            for ( auto const& entry : m_entries )
            {
                perEntryFunction( entry );
            }
        }

        int32_t GetNumLogEntries() const
        {
            Threading::ScopeLockRead sl( m_mutex );
            return (int32_t) m_entries.size();
        }

        // Returns a copy of the entire log
        TVector<Log::Entry> GetLogEntries() const
        {
            Threading::ScopeLockRead sl( m_mutex );
            return m_entries;
        }

        // Append another logs contents to this log
        void Append( Log const& otherLog );

        // Append another logs contents to this log
        void Append( LogThreadSafe const& otherLog );

        // Prints log contents out to the system log for the given category and source
        void ReflectToSystemLog( LogCategory category, char const* pSourceInfo );

        // Prints log contents out to the system log for the given category and source, clears the log after
        void ReflectToSystemLogAndClear( LogCategory category, char const* pSourceInfo );

        // Saves the log out to a single string
        void SaveLogToString( String& logString );

        // Saves the log out to a file
        void SaveLogToFile( FileSystem::Path const& logFilePath );

    private:

        mutable Threading::ReadWriteMutex       m_mutex;
        TVector<Log::Entry>                     m_entries;
        std::atomic<int32_t>                    m_numWarnings = 0;
        std::atomic<int32_t>                    m_numErrors = 0;
        LogCategory                             m_category = LogCategory::Invalid;
        String                                  m_sourceInfo;
    };
}

// Mixin Macros
//-------------------------------------------------------------------------
// If you dont want to derive from log::storage, use these macros to mix in the functionality

#define EE_EMBED_LOGGING_METHODS( logStorageName );\
inline bool HasWarnings() const { return !logStorageName.HasWarnings(); }\
inline bool HasErrors() const { return !logStorageName.HasErrors(); }\
bool LogError( char const* pFormat, ... )\
{\
    va_list args;\
    va_start( args, pFormat );\
    logStorageName.LogWarning( pFormat, args );\
    va_end( args );\
    return false;\
}\
bool LogErrorWithCategory( char const* pCategory, char const* pFormat, ... )\
{\
    va_list args;\
    va_start( args, pFormat );\
    logStorageName.LogErrorWithCategory( pCategory, pFormat, args );\
    va_end( args );\
    return false;\
}\
void LogWarning( char const* pFormat, ... )\
{\
    va_list args;\
    va_start( args, pFormat );\
    logStorageName.LogWarning( pFormat, args );\
    va_end( args );\
}\
void LogWarningWithCategory( char const* pCategory, char const* pFormat, ... )\
{\
    va_list args;\
    va_start( args, pFormat );\
    logStorageName.LogWarningWithCategory( pCategory, pFormat, args );\
    va_end( args );\
}\
void LogInfo( char const* pFormat, ... )\
{\
    va_list args;\
    va_start( args, pFormat );\
    logStorageName.LogInfo( pFormat, args );\
    va_end( args );\
}\
void LogInfoWithCategory( char const* pCategory, char const* pFormat, ... )\
{\
    va_list args;\
    va_start( args, pFormat );\
    logStorageName.LogInfoWithCategory( pCategory, pFormat, args );\
    va_end( args );\
}\
TVector<LogCategory::Entry> const& GetLogEntries() const { return logStorageName.GetLogEntries(); }\
void IterateLogEntries( TFunction<bool( LogCategory::Entry const& entry )> const& perEntryFunction ) { logStorageName.IterateLogEntries( perEntryFunction ); }\

//-------------------------------------------------------------------------

#define EE_EMBED_LOGGING_METHODS_THREADSAFE( logStorageName );\
inline bool HasWarnings() const { return !logStorageName.HasWarnings(); }\
inline bool HasErrors() const { return !logStorageName.HasErrors(); }\
bool LogError( char const* pFormat, ... )\
{\
    va_list args;\
    va_start( args, pFormat );\
    logStorageName.LogWarning( pFormat, args );\
    va_end( args );\
    return false;\
}\
bool LogErrorWithCategory( char const* pCategory, char const* pFormat, ... )\
{\
    va_list args;\
    va_start( args, pFormat );\
    logStorageName.LogErrorWithCategory( pCategory, pFormat, args );\
    va_end( args );\
    return false;\
}\
void LogWarning( char const* pFormat, ... )\
{\
    va_list args;\
    va_start( args, pFormat );\
    logStorageName.LogWarning( pFormat, args );\
    va_end( args );\
}\
void LogWarningWithCategory( char const* pCategory, char const* pFormat, ... )\
{\
    va_list args;\
    va_start( args, pFormat );\
    logStorageName.LogWarningWithCategory( pCategory, pFormat, args );\
    va_end( args );\
}\
void LogInfo( char const* pFormat, ... )\
{\
    va_list args;\
    va_start( args, pFormat );\
    logStorageName.LogInfo( pFormat, args );\
    va_end( args );\
}\
void LogInfoWithCategory( char const* pCategory, char const* pFormat, ... )\
{\
    va_list args;\
    va_start( args, pFormat );\
    logStorageName.LogInfoWithCategory( pCategory, pFormat, args );\
    va_end( args );\
}\
TVector<LogCategory::Entry> GetLogEntries() const { return logStorageName.GetLogEntries(); }\
void IterateLogEntries( TFunction<bool( LogCategory::Entry const& entry )> const& perEntryFunction ) { logStorageName.IterateLogEntries( perEntryFunction ); }\
