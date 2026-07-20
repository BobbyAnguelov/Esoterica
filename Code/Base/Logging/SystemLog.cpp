#include "SystemLog.h"
#include "Base/Memory/Memory.h"
#include "Base/Threading/Threading.h"
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
            TVector<Entry*>                 m_logEntries;
            Threading::ReadWriteMutex       m_mutex;
            std::atomic<int32_t>            m_fatalErrorIndex = InvalidIndex;
            std::atomic<int32_t>            m_numEntries;
            std::atomic<int32_t>            m_numWarnings = 0;
            std::atomic<int32_t>            m_numErrors = 0;
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

        for ( auto pEntry : g_pLog->m_logEntries )
        {
            EE::Delete( pEntry );
        }

        EE::Delete( g_pLog );
    }

    bool WasInitialized()
    {
        return g_pLog != nullptr;
    }

    //-------------------------------------------------------------------------

    void SaveToFile( FileSystem::Path const& logFilePath )
    {
        EE_ASSERT( WasInitialized() );

        if ( !logFilePath.IsValid() || !logFilePath.IsFilePath() )
        {
            return;
        }

        if ( !logFilePath.EnsureDirectoryExists() )
        {
            return;
        }

        String logData;

        {
            Threading::ScopeLockRead lock( g_pLog->m_mutex );
            for ( auto const& pEntry : g_pLog->m_logEntries )
            {
                if ( pEntry->m_sourceInfo.empty() )
                {
                    logData.append_sprintf( "[%s] %s >>> %s: %s, File: %s, %d\r\n", pEntry->m_timestamp.c_str(), pEntry->m_category.c_str(), GetSeverityAsString( pEntry->m_severity ), pEntry->m_message.c_str(), pEntry->m_filename.c_str(), pEntry->m_lineNumber );
                }
                else
                {
                    InlineString sourceInfo;

                    logData.append_sprintf( "[%s] %s >>> %s: %s, Source: %s, File: %s, %d\r\n", pEntry->m_timestamp.c_str(), pEntry->m_category.c_str(), GetSeverityAsString( pEntry->m_severity ), pEntry->m_message.c_str(), pEntry->m_sourceInfoStr.c_str(), pEntry->m_filename.c_str(), pEntry->m_lineNumber );
                }
            }
        }

        FileSystem::OutputFileStream logFile( logFilePath );
        logFile.Write( (void*) logData.data(), logData.size() );
    }

    //-------------------------------------------------------------------------

    bool HasFatalErrorOccurred()
    {
        EE_ASSERT( WasInitialized() );
        return g_pLog->m_fatalErrorIndex != InvalidIndex;
    }

    Entry const& GetFatalError()
    {
        EE_ASSERT( WasInitialized() && g_pLog->m_fatalErrorIndex != InvalidIndex );
        Threading::ScopeLockRead lock( g_pLog->m_mutex );
        return *g_pLog->m_logEntries[g_pLog->m_fatalErrorIndex];
    }

    //-------------------------------------------------------------------------

    int32_t GetNumEntries()
    {
        EE_ASSERT( WasInitialized() );
        return (int32_t) g_pLog->m_logEntries.size();
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

    int32_t GetNumMessages()
    {
        EE_ASSERT( WasInitialized() );
        Threading::ScopeLockRead lock( g_pLog->m_mutex );
        return Math::Max( ( (int32_t) g_pLog->m_logEntries.size() ) - g_pLog->m_numErrors - g_pLog->m_numWarnings, 0 );
    }

    void GetLogEntries( int32_t startIdx, int32_t numEntries, TVector<Entry const*>& outEntries )
    {
        EE_ASSERT( WasInitialized() );
        EE_ASSERT( startIdx >= 0 );
        EE_ASSERT( startIdx < g_pLog->m_numEntries );

        outEntries.clear();
        outEntries.reserve( numEntries );

        {
            Threading::ScopeLockRead lock( g_pLog->m_mutex );
            int32_t const endIdx = Math::Min( startIdx + numEntries, (int32_t) g_pLog->m_numEntries );
            for ( int32_t i = startIdx; i < endIdx; i++ )
            {
                outEntries.emplace_back( g_pLog->m_logEntries[i] );
            }
        }
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
            // Create entry
            //-------------------------------------------------------------------------

            Entry* pEntry = EE::New<Entry>();
            pEntry->m_category = StringID( pCategory );
            pEntry->m_filename = pFilename;
            pEntry->m_lineNumber = pLineNumber;
            pEntry->m_severity = severity;

            if ( pSourceInfo != nullptr )
            {
                pEntry->m_sourceInfoStr = pSourceInfo;

                TInlineVector<InlineString, 5> splitSource;
                StringUtils::Split( pEntry->m_sourceInfoStr, splitSource, "/", true );
                for ( auto const& str : splitSource )
                {
                    pEntry->m_sourceInfo.emplace_back( StringID( str.c_str() ) );
                }
            }

            // Message
            pEntry->m_message.sprintf_va_list( pMessageFormat, args );

            // Timestamp
            pEntry->m_timestamp.resize( 9 );
            time_t const t = std::time( nullptr );
            strftime( pEntry->m_timestamp.data(), 9, "%H:%M:%S", std::localtime( &t ) );

            // Immediate display of log
            //-------------------------------------------------------------------------
            // This uses a less verbose format, if you want more info look at the saved log

            InlineString traceMessage;
            if ( pEntry->m_sourceInfo.empty() )
            {
                traceMessage.sprintf( "[%s][%s][%s] %s", pEntry->m_timestamp.c_str(), GetSeverityAsString( pEntry->m_severity ), pEntry->m_category.c_str(), pEntry->m_message.c_str() );
            }
            else
            {
                traceMessage.sprintf( "[%s][%s][%s][%s] %s", pEntry->m_timestamp.c_str(), GetSeverityAsString( pEntry->m_severity ), pEntry->m_category.c_str(), pEntry->m_sourceInfoStr.c_str(), pEntry->m_message.c_str() );
            }

            // Print to debug trace
            EE_TRACE_MSG( traceMessage.c_str() );

            // Print to std out
            printf( "%s\n", traceMessage.c_str() );

            // Add to log
            //-------------------------------------------------------------------------

            {
                Threading::ScopeLockWrite lock( g_pLog->m_mutex );
                g_pLog->m_logEntries.emplace_back( pEntry );
            }

            if ( severity == Severity::FatalError )
            {
                g_pLog->m_fatalErrorIndex = (int32_t) g_pLog->m_numEntries;
            }

            g_pLog->m_numEntries++;

            if ( pEntry->m_severity > Severity::Info )
            {
                g_pLog->m_numWarnings += ( pEntry->m_severity == Severity::Warning ) ? 1 : 0;
                g_pLog->m_numErrors += ( pEntry->m_severity == Severity::Error ) ? 1 : 0;
            }
        }
    }

    void AddEntry( Severity severity, LogCategory category, char const* pSourceInfo, char const* pFilename, int pLineNumber, char const* pMessageFormat, ... )
    {
        va_list args;
        va_start( args, pMessageFormat );
        AddEntryVarArgs( severity, GetLogCategoryString( category ), pSourceInfo, pFilename, pLineNumber, pMessageFormat, args );
        va_end( args );
    }

    void AddEntryVarArgs( Severity severity, LogCategory category, char const* pSourceInfo, char const* pFilename, int pLineNumber, char const* pMessageFormat, va_list args )
    {
        AddEntryVarArgs( severity, GetLogCategoryString( category ), pSourceInfo, pFilename, pLineNumber, pMessageFormat, args );
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