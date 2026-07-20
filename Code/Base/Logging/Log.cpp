#include "Log.h"
#include "Base/Esoterica.h"
#include "Base/FileSystem/FileStreams.h"

//-------------------------------------------------------------------------

namespace EE
{
    static void FillEntry( Log::Entry& entry, Severity severity, char const* pFormat, va_list arguments )
    {
        // Timestamp
        entry.m_timestamp.resize( 9 );
        time_t const t = std::time( nullptr );
        strftime( entry.m_timestamp.data(), 9, "%H:%M:%S", std::localtime( &t ) );

        entry.m_severity = severity;
        entry.m_message.sprintf_va_list( pFormat, arguments );
    }

    //-------------------------------------------------------------------------

    Log::Log( LogCategory category, char const* pSourceInfo )
        : m_category( category )
        , m_sourceInfo( pSourceInfo )
    {
        EE_ASSERT( pSourceInfo != nullptr );
    }

    Log::Log( LogCategory category, String const& sourceInfo )
        : m_category( category )
        , m_sourceInfo( sourceInfo )
    {
        EE_ASSERT( !sourceInfo.empty() );
    }

    Log::~Log()
    {
        if ( m_category != LogCategory::Invalid && !m_sourceInfo.empty() )
        {
            ReflectToSystemLog( m_category, m_sourceInfo.c_str() );
        }
    }

    bool Log::LogError( char const* pFormat, va_list args )
    {
        FillEntry( m_entries.emplace_back(), Severity::Error, pFormat, args );
        m_numErrors++;
        return false;
    }

    void Log::LogWarning( char const* pFormat, va_list args )
    {
        FillEntry( m_entries.emplace_back(), Severity::Warning, pFormat, args );
        m_numWarnings++;
    }

    void Log::LogInfo( char const* pFormat, va_list args )
    {
        FillEntry( m_entries.emplace_back(), Severity::Info, pFormat, args );
    }

    void Log::Append( Log const& otherLog )
    {
        m_numErrors += otherLog.m_numErrors;
        m_numWarnings += otherLog.m_numWarnings;
        m_entries.insert( m_entries.end(), otherLog.m_entries.begin(), otherLog.m_entries.end() );
    }

    void Log::ReflectToSystemLog( LogCategory category, char const* pSourceInfo )
    {
        for ( auto const& entry : m_entries )
        {
            SystemLog::AddEntry( entry.m_severity, category, pSourceInfo, "", -1, entry.m_message.c_str() );
        }
    }

    void Log::ReflectToSystemLogAndClear( LogCategory category, char const* pSourceInfo )
    {
        ReflectToSystemLog( category, pSourceInfo );
        ClearLog();
    }

    void Log::SaveLogToString( String& logString )
    {
        InlineString logLine;

        for ( auto const& entry : m_entries )
        {
            logLine.sprintf( "[%s] %s: %s\r\n", entry.m_timestamp.c_str(), GetSeverityAsString( entry.m_severity ), entry.m_message.c_str() );
            logString.append( logLine.c_str() );
        }
    }

    void Log::SaveLogToFile( FileSystem::Path const& logFilePath )
    {
        if ( !logFilePath.IsValid() || !logFilePath.IsFilePath() )
        {
            return;
        }

        logFilePath.EnsureDirectoryExists();

        String logData;
        SaveLogToString( logData );

        FileSystem::OutputFileStream logFile( logFilePath );
        logFile.Write( (void*) logData.data(), logData.size() );
    }

    //-------------------------------------------------------------------------

    LogThreadSafe::LogThreadSafe( LogCategory category, char const* pSourceInfo )
        : m_category( category )
        , m_sourceInfo( pSourceInfo )
    {
        EE_ASSERT( pSourceInfo != nullptr );
    }

    LogThreadSafe::LogThreadSafe( LogCategory category, String const& sourceInfo )
        : m_category( category )
        , m_sourceInfo( sourceInfo )
    {
        EE_ASSERT( !sourceInfo.empty() );
    }

    LogThreadSafe::~LogThreadSafe()
    {
        if ( m_category != LogCategory::Invalid && !m_sourceInfo.empty() )
        {
            ReflectToSystemLog( m_category, m_sourceInfo.c_str() );
        }
    }

    bool LogThreadSafe::LogError( char const* pFormat, va_list args )
    {
        Threading::ScopeLockWrite sl( m_mutex );
        FillEntry( m_entries.emplace_back(), Severity::Error, pFormat, args );
        m_numErrors++;
        return false;
    }

    void LogThreadSafe::LogWarning( char const* pFormat, va_list args )
    {
        Threading::ScopeLockWrite sl( m_mutex );
        FillEntry( m_entries.emplace_back(), Severity::Warning, pFormat, args );
        m_numWarnings++;
    }

    void LogThreadSafe::LogInfo( char const* pFormat, va_list args )
    {
        Threading::ScopeLockWrite sl( m_mutex );
        FillEntry( m_entries.emplace_back(), Severity::Info, pFormat, args );
    }

    void LogThreadSafe::Append( Log const& otherLog )
    {
        Threading::ScopeLockWrite sl( m_mutex );
        m_numErrors += otherLog.GetNumErrors();
        m_numWarnings += otherLog.GetNumWarnings();
        m_entries.insert( m_entries.end(), otherLog.GetLogEntries().begin(), otherLog.GetLogEntries().end() );
    }

    void LogThreadSafe::Append( LogThreadSafe const& otherLog )
    {
        Threading::ScopeLockWrite sw( m_mutex );
        Threading::ScopeLockRead sr( otherLog.m_mutex );

        m_numErrors += otherLog.m_numErrors;
        m_numWarnings += otherLog.m_numWarnings;
        m_entries.insert( m_entries.end(), otherLog.m_entries.begin(), otherLog.m_entries.end() );
    }

    void LogThreadSafe::ReflectToSystemLog( LogCategory category, char const* pSourceInfo )
    {
        Threading::ScopeLockRead sl( m_mutex );
        for ( auto const& entry : m_entries )
        {
            SystemLog::AddEntry( entry.m_severity, category, pSourceInfo, "", -1, entry.m_message.c_str() );
        }
    }

    void LogThreadSafe::ReflectToSystemLogAndClear( LogCategory category, char const* pSourceInfo )
    {
        ReflectToSystemLog( category, pSourceInfo );

        {
            Threading::ScopeLockWrite sl( m_mutex );
            ClearLog();
        }
    }

    void LogThreadSafe::SaveLogToString( String& logString )
    {
        Threading::ScopeLockRead sl( m_mutex );

        InlineString logLine;

        for ( auto const& entry : m_entries )
        {
            logLine.sprintf( "[%s] %s: %s\r\n", entry.m_timestamp.c_str(), GetSeverityAsString( entry.m_severity ), entry.m_message.c_str() );
            logString.append( logLine.c_str() );
        }
    }

    void LogThreadSafe::SaveLogToFile( FileSystem::Path const& logFilePath )
    {
        if ( !logFilePath.IsValid() || !logFilePath.IsFilePath() )
        {
            return;
        }

        logFilePath.EnsureDirectoryExists();

        String logData;
        InlineString logLine;

        Threading::ScopeLockRead sl( m_mutex );
        for ( auto const& entry : m_entries )
        {
            logLine.sprintf( "[%s] %s: %s\r\n", entry.m_timestamp.c_str(), GetSeverityAsString( entry.m_severity ), entry.m_message.c_str() );
            logData.append( logLine.c_str() );
        }

        FileSystem::OutputFileStream logFile( logFilePath );
        logFile.Write( (void*) logData.data(), logData.size() );
    }
}