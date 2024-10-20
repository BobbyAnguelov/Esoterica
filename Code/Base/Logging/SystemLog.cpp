#include "SystemLog.h"
#include "Base/Threading/Threading.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/FileSystem/FileStreams.h"
#include "Base/FileSystem/FileSystemPath.h"
#include <ctime>

//-------------------------------------------------------------------------

namespace EE::SystemLog
{
    namespace
    {
        struct LogData
        {
            TVector<Log::Entry>             m_logEntries;
            TVector<Log::Entry>             m_unhandledWarningsAndErrors;
            FileSystem::Path                m_logPath;
            Threading::Mutex                m_mutex;
            int32_t                         m_fatalErrorIndex = InvalidIndex;
            int32_t                         m_numWarnings = 0;
            int32_t                         m_numErrors = 0;
        };

        static LogData*                     g_pLog = nullptr;
    }

    //-------------------------------------------------------------------------

    void Initialize()
    {
        EE_ASSERT( g_pLog == nullptr );
        g_pLog = EE::New<LogData>();
    }

    void Shutdown()
    {
        EE_ASSERT( g_pLog != nullptr );
        EE::Delete( g_pLog );
    }

    bool WasInitialized()
    {
        return g_pLog != nullptr;
    }

    //-------------------------------------------------------------------------

    TVector<Log::Entry> const& GetLogEntries()
    {
        EE_ASSERT( WasInitialized() );
        return g_pLog->m_logEntries;
    }

    //-------------------------------------------------------------------------

    void SetLogFilePath( FileSystem::Path const& logFilePath )
    {
        EE_ASSERT( WasInitialized() );
        g_pLog->m_logPath = logFilePath;
    }

    void SaveToFile()
    {
        EE_ASSERT( WasInitialized() );

        if ( !g_pLog->m_logPath.IsValid() || !g_pLog->m_logPath.IsFilePath() )
        {
            return;
        }

        g_pLog->m_logPath.EnsureDirectoryExists();

        String logData;
        InlineString logLine;

        std::lock_guard<std::mutex> lock( g_pLog->m_mutex );
        for ( auto const& entry : g_pLog->m_logEntries )
        {
            if ( entry.m_sourceInfo.empty() )
            {
                logLine.sprintf( "[%s] %s >>> %s: %s, File: %s, %d\r\n", entry.m_timestamp.c_str(), entry.m_category.c_str(), GetSeverityAsString( entry.m_severity ), entry.m_message.c_str(), entry.m_filename.c_str(), entry.m_lineNumber );
            }
            else
            {
                logLine.sprintf( "[%s] %s >>> %s: %s, Source: %s, File: %s, %d\r\n", entry.m_timestamp.c_str(), entry.m_category.c_str(), GetSeverityAsString( entry.m_severity ), entry.m_message.c_str(), entry.m_sourceInfo.c_str(), entry.m_filename.c_str(), entry.m_lineNumber );
            }

            logData.append( logLine.c_str() );
        }

        FileSystem::OutputFileStream logFile( g_pLog->m_logPath );
        logFile.Write( (void*) logData.data(), logData.size() );
    }

    //-------------------------------------------------------------------------

    bool HasFatalErrorOccurred()
    {
        EE_ASSERT( WasInitialized() );
        return g_pLog->m_fatalErrorIndex != InvalidIndex;
    }

    Log::Entry const& GetFatalError()
    {
        EE_ASSERT( WasInitialized() && g_pLog->m_fatalErrorIndex != InvalidIndex );
        return g_pLog->m_logEntries[g_pLog->m_fatalErrorIndex];
    }

    //-------------------------------------------------------------------------

    TVector<Log::Entry> GetUnhandledWarningsAndErrors()
    {
        EE_ASSERT( WasInitialized() );
        std::lock_guard<std::mutex> lock( g_pLog->m_mutex );

        TVector<Log::Entry> outEntries = g_pLog->m_unhandledWarningsAndErrors;
        g_pLog->m_unhandledWarningsAndErrors.clear();
        return outEntries;
    }

    int32_t GetNumWarnings()
    {
        EE_ASSERT( WasInitialized() );
        return g_pLog->m_numWarnings;
    }

    int32_t GetNumErrors()
    {
        EE_ASSERT( WasInitialized() );
        return g_pLog->m_numErrors;
    }
}

//-------------------------------------------------------------------------

namespace EE::SystemLog
{
    void AddEntry( Severity severity, char const* pCategory, char const* pSourceInfo, char const* pFilename, int pLineNumber, char const* pMessageFormat, ... )
    {
        EE_ASSERT( WasInitialized() );
        va_list args;
        va_start( args, pMessageFormat );
        AddEntryVarArgs( severity, pCategory, pSourceInfo, pFilename, pLineNumber, pMessageFormat, args );
        va_end( args );
    }

    void AddEntryVarArgs( Severity severity, char const* pCategory, char const* pSourceInfo, char const* pFilename, int pLineNumber, char const* pMessageFormat, va_list args )
    {
        EE_ASSERT( WasInitialized() );
        EE_ASSERT( pCategory != nullptr && pFilename != nullptr && pMessageFormat != nullptr );

        {
            std::lock_guard<std::mutex> lock( g_pLog->m_mutex );

            if ( severity == Severity::FatalError )
            {
                g_pLog->m_fatalErrorIndex = (int32_t) g_pLog->m_logEntries.size();
            }

            auto& entry = g_pLog->m_logEntries.emplace_back( Log::Entry() );

            //-------------------------------------------------------------------------

            entry.m_category = pCategory;
            entry.m_sourceInfo = ( pSourceInfo != nullptr ) ? pSourceInfo : String();
            entry.m_filename = pFilename;
            entry.m_lineNumber = pLineNumber;
            entry.m_severity = severity;
            entry.m_timestamp.resize( 9 );

            // Message
            entry.m_message.sprintf_va_list( pMessageFormat, args );
            va_end( args );

            // Timestamp
            entry.m_timestamp.resize( 9 );
            time_t const t = std::time( nullptr );
            strftime( entry.m_timestamp.data(), 9, "%H:%M:%S", std::localtime( &t ) );

            // Immediate display of log
            //-------------------------------------------------------------------------
            // This uses a less verbose format, if you want more info look at the saved log

            InlineString traceMessage;
            if ( entry.m_sourceInfo.empty() )
            {
                traceMessage.sprintf( "[%s][%s][%s] %s", entry.m_timestamp.c_str(), GetSeverityAsString( entry.m_severity ), entry.m_category.c_str(), entry.m_message.c_str() );
            }
            else
            {
                traceMessage.sprintf( "[%s][%s][%s][%s] %s", entry.m_timestamp.c_str(), GetSeverityAsString( entry.m_severity ), entry.m_category.c_str(), entry.m_sourceInfo.c_str(), entry.m_message.c_str() );
            }

            // Print to debug trace
            EE_TRACE_MSG( traceMessage.c_str() );

            // Print to std out
            printf( "%s\n", traceMessage.c_str() );

            // Track unhandled warnings and errors
            //-------------------------------------------------------------------------

            if ( entry.m_severity > Severity::Info )
            {
                g_pLog->m_numWarnings += ( entry.m_severity == Severity::Warning ) ? 1 : 0;
                g_pLog->m_numErrors += ( entry.m_severity == Severity::Error ) ? 1 : 0;
                g_pLog->m_unhandledWarningsAndErrors.emplace_back( entry );
            }
        }
    }

    //-------------------------------------------------------------------------

    void LogAssert( char const* pFile, int32_t line, char const* pAssertInfo )
    {
        EE_TRACE_MSG( pAssertInfo );

        if ( g_pLog != nullptr )
        {
            AddEntry( Severity::Error, "Assert", "Assert", pFile, line, pAssertInfo );
        }
    }

    void LogAssertVarArgs( char const* pFile, int line, char const* pAssertInfoFormat, ... )
    {
        va_list args;
        va_start( args, pAssertInfoFormat );
        char buffer[512];
        vsprintf_s( buffer, 512, pAssertInfoFormat, args );
        va_end( args );

        LogAssert( pFile, line, &buffer[0] );
    }
}