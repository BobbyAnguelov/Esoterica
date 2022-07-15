#include "Log.h"
#include "System/Threading/Threading.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileStreams.h"
#include <ctime>

//-------------------------------------------------------------------------

namespace EE::Log
{
    namespace
    {
        static char const* const g_severityLabels[] = { "Message", "Warning", "Error", "Fatal Error" };

        struct LogData
        {
            TVector<LogEntry>               m_logEntries;
            TVector<LogEntry>               m_unhandledWarningsAndErrors;
            Threading::Mutex                m_mutex;
            int32_t                           m_fatalErrorIndex = InvalidIndex;
            int32_t                           m_numWarnings = 0;
            int32_t                           m_numErrors = 0;
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

    bool IsInitialized()
    {
        return g_pLog != nullptr;
    }

    //-------------------------------------------------------------------------

    TVector<EE::Log::LogEntry> const& GetLogEntries()
    {
        EE_ASSERT( IsInitialized() );
        return g_pLog->m_logEntries;
    }

    void AddEntry( Severity severity, char const* pChannel, char const* pFilename, int pLineNumber, char const* pMessageFormat, ... )
    {
        EE_ASSERT( IsInitialized() );
        va_list args;
        va_start( args, pMessageFormat );
        AddEntryVarArgs( severity, pChannel, pFilename, pLineNumber, pMessageFormat, args );
        va_end( args );
    }

    void AddEntryVarArgs( Severity severity, char const* pChannel, char const* pFilename, int pLineNumber, char const* pMessageFormat, va_list args )
    {
        EE_ASSERT( IsInitialized() && pFilename != nullptr );

        char msgbuffer[1024];
        VPrintf( msgbuffer, 1024, pMessageFormat, args );
        va_end( args );

        {
            std::lock_guard<std::mutex> lock( g_pLog->m_mutex );

            if ( severity == Severity::FatalError )
            {
                g_pLog->m_fatalErrorIndex = (int32_t) g_pLog->m_logEntries.size();
            }

            auto& entry = g_pLog->m_logEntries.emplace_back( LogEntry() );

            //-------------------------------------------------------------------------

            entry.m_message = msgbuffer;
            entry.m_channel = pChannel;
            entry.m_filename = pFilename;
            entry.m_lineNumber = pLineNumber;
            entry.m_severity = severity;
            entry.m_timestamp.resize( 9 );

            auto t = std::time( nullptr );
            strftime( entry.m_timestamp.data(), 9, "%H:%M:%S", std::localtime( &t ) );

            // Immediate display of log
            //-------------------------------------------------------------------------
            // This uses a less verbose format, if you want more info look at the saved log

            Printf( msgbuffer, 1024, "[%s][%s][%s] %s", entry.m_timestamp.c_str(), g_severityLabels[(int32_t) entry.m_severity], entry.m_channel.c_str(), entry.m_message.c_str() );

            // Print to debug trace
            EE_TRACE_MSG( msgbuffer );

            // Print to std out
            printf( "%s\n", msgbuffer );

            // Track unhandled warnings and errors
            //-------------------------------------------------------------------------

            if ( entry.m_severity > Severity::Message )
            {
                g_pLog->m_numWarnings += ( entry.m_severity == Severity::Warning ) ? 1 : 0;
                g_pLog->m_numErrors += ( entry.m_severity == Severity::Error ) ? 1 : 0;
                g_pLog->m_unhandledWarningsAndErrors.emplace_back( entry );
            }
        }
    }

    //-------------------------------------------------------------------------

    void SaveToFile( FileSystem::Path const& logFilePath )
    {
        EE_ASSERT( IsInitialized() && logFilePath.IsValid() && logFilePath.IsFilePath() );

        logFilePath.EnsureDirectoryExists();

        String logData;

        char buffer[1024];
        std::lock_guard<std::mutex> lock( g_pLog->m_mutex );
        for ( auto const& entry : g_pLog->m_logEntries )
        {
            Printf( buffer, 1024, "[%s] %s >>> %s: %s, Source: %s, %i\r\n", entry.m_timestamp.c_str(), entry.m_channel.c_str(), g_severityLabels[(int32_t) entry.m_severity], entry.m_message.c_str(), entry.m_filename.c_str(), entry.m_lineNumber );
            logData.append( buffer );
        }

        FileSystem::OutputFileStream logFile( logFilePath );
        logFile.Write( (void*) logData.data(), logData.size() );
    }

    //-------------------------------------------------------------------------

    bool HasFatalErrorOccurred()
    {
        EE_ASSERT( IsInitialized() );
        return g_pLog->m_fatalErrorIndex != InvalidIndex;
    }

    LogEntry const& GetFatalError()
    {
        EE_ASSERT( IsInitialized() && g_pLog->m_fatalErrorIndex != InvalidIndex );
        return g_pLog->m_logEntries[g_pLog->m_fatalErrorIndex];
    }

    //-------------------------------------------------------------------------

    TVector<Log::LogEntry> GetUnhandledWarningsAndErrors()
    {
        EE_ASSERT( IsInitialized() );
        std::lock_guard<std::mutex> lock( g_pLog->m_mutex );

        TVector<Log::LogEntry> outEntries = g_pLog->m_unhandledWarningsAndErrors;
        g_pLog->m_unhandledWarningsAndErrors.clear();
        return outEntries;
    }

    int32_t GetNumWarnings()
    {
        EE_ASSERT( IsInitialized() );
        return g_pLog->m_numWarnings;
    }

    int32_t GetNumErrors()
    {
        EE_ASSERT( IsInitialized() );
        return g_pLog->m_numErrors;
    }
}