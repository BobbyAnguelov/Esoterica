#include "Log.h"
#include "Base/Esoterica.h"
#include "Base/FileSystem/FileStreams.h"

//-------------------------------------------------------------------------

namespace EE::Log
{
    void Storage::AddDetailedEntryVarArgs( Severity severity, char const* pCategory, char const* pSourceInfo, char const* pFilename, int pLineNumber, char const* pMessageFormat, va_list args )
    {
        EE_ASSERT( severity != Severity::FatalError ); // Fatal errors are only allowed for the system log
        EE_ASSERT( pCategory != nullptr && pFilename != nullptr && pMessageFormat != nullptr );
        Threading::ScopeLockWrite sl( m_mutex );

        auto& entry = m_entries.emplace_back( Entry() );

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

        // Track warnings and errors
        //-------------------------------------------------------------------------

        if ( entry.m_severity > Severity::Info )
        {
            m_numWarnings += ( entry.m_severity == Severity::Warning ) ? 1 : 0;
            m_numErrors += ( entry.m_severity == Severity::Error ) ? 1 : 0;
        }
    }

    void Storage::SaveToFile( FileSystem::Path const& logFilePath )
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

        FileSystem::OutputFileStream logFile( logFilePath );
        logFile.Write( (void*) logData.data(), logData.size() );
    }
}