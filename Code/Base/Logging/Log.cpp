#include "Log.h"
#include "Base/Esoterica.h"
#include "Base/FileSystem/FileStreams.h"

//-------------------------------------------------------------------------

namespace EE
{
    class [[nodiscard]] LogConditionalScopeLockRead
    {
    public:

        LogConditionalScopeLockRead( Log::Mode mode, Threading::ReadWriteMutex& mutex ) 
            : m_mutex( mutex )
            , m_mode( mode )
        {
            if( m_mode == Log::Mode::ThreadSafe )
            {
                m_mutex.LockRead();
            }
        }

        ~LogConditionalScopeLockRead() 
        {
            if ( m_mode == Log::Mode::ThreadSafe )
            {
                m_mutex.UnlockRead();
            }
        }

    private:

        Threading::ReadWriteMutex&  m_mutex;
        Log::Mode                   m_mode;
    };

    class [[nodiscard]] LogConditionalScopeLockWrite
    {
    public:

        LogConditionalScopeLockWrite( Log::Mode mode, Threading::ReadWriteMutex& mutex ) 
            : m_mutex( mutex )
            , m_mode( mode )
        { 
            if ( m_mode == Log::Mode::ThreadSafe )
            {
                m_mutex.LockWrite();
            }
        }

        ~LogConditionalScopeLockWrite()
        {
            if ( m_mode == Log::Mode::ThreadSafe )
            {
                m_mutex.UnlockWrite();
            }
        }

    private:

        Threading::ReadWriteMutex&  m_mutex;
        Log::Mode                   m_mode;
    };

    //-------------------------------------------------------------------------

    Log::Log( LogCategory category, char const* pSourceInfo, Mode mode )
        : m_mode( mode )
        , m_category( category )
        , m_sourceInfo( pSourceInfo )
    {
        EE_ASSERT( pSourceInfo != nullptr );
    }

    Log::Log( LogCategory category, String const& sourceInfo, Mode mode )
        : m_mode( mode )
        , m_category( category )
        , m_sourceInfo( sourceInfo )
    {
        EE_ASSERT( !sourceInfo.empty() );
    }

    Log::Log( Log const& log )
        : m_mode( log.m_mode )
        , m_category( log.m_category )
        , m_sourceInfo( log.m_sourceInfo )
    {
        Append( log );
    }

    Log::~Log()
    {
        if ( m_category != LogCategory::Invalid && !m_sourceInfo.empty() )
        {
            ReflectToSystemLog( m_category, m_sourceInfo.c_str() );
        }
    }

    Log& Log::operator=( Log const& rhs )
    {
        EE_ASSERT( this != &rhs );

        m_category = rhs.m_category;
        m_sourceInfo = rhs.m_sourceInfo;

        Clear();
        Append( rhs );

        m_mode = rhs.m_mode;

        return *this;
    }

    void Log::Clear()
    {
        LogConditionalScopeLockWrite sl( m_mode, m_mutex );
        m_entries.clear();
        m_numWarnings = m_numErrors = 0;
    }

    void Log::AddLogEntryVarArgs( Severity severity, uint64_t userData, char const* pFormat, va_list args )
    {
        Entry entry;
        entry.m_severity = severity;
        entry.m_message.sprintf_va_list( pFormat, args );
        entry.m_userData = userData;

        // Timestamp
        entry.m_timestamp.resize( 8 );
        time_t const t = std::time( nullptr );
        strftime( entry.m_timestamp.data(), 9, "%H:%M:%S", std::localtime( &t ) );

        //-------------------------------------------------------------------------

        LogConditionalScopeLockWrite sl( m_mode, m_mutex );
        m_entries.emplace_back( eastl::move( entry ) );

        switch ( severity )
        {
            case EE::Severity::Warning:
            m_numWarnings++;
            break;

            case EE::Severity::FatalError:
            case EE::Severity::Error:
            m_numErrors++;
            break;

            case EE::Severity::Info:
            default:
            break;
        }
    }

    int32_t Log::GetNumLogEntries() const
    {
        LogConditionalScopeLockRead sl( m_mode, m_mutex );
        return (int32_t) m_entries.size();
    }

    TVector<Log::Entry> Log::GetLogEntriesThreadsafe() const
    {
        LogConditionalScopeLockRead sl( m_mode, m_mutex );
        return m_entries;
    }

    void Log::IterateLogEntries( TFunction<void( Entry const& entry )> const& perEntryFunction ) const
    {
        LogConditionalScopeLockRead sl( m_mode, m_mutex );
        for ( auto const& entry : m_entries )
        {
            perEntryFunction( entry );
        }
    }

    void Log::IterateInfos( TFunction<void( Entry const& entry )> const& perEntryFunction ) const
    {
        LogConditionalScopeLockRead sl( m_mode, m_mutex );
        for ( auto const& entry : m_entries )
        {
            if ( entry.m_severity == Severity::Info )
            {
                perEntryFunction( entry );
            }
        }
    }

    void Log::IterateWarnings( TFunction<void( Entry const& entry )> const& perEntryFunction ) const
    {
        LogConditionalScopeLockRead sl( m_mode, m_mutex );
        for ( auto const& entry : m_entries )
        {
            if ( entry.m_severity == Severity::Warning )
            {
                perEntryFunction( entry );
            }
        }
    }

    void Log::IterateErrors( TFunction<void( Entry const& entry )> const& perEntryFunction ) const
    {
        LogConditionalScopeLockRead sl( m_mode, m_mutex );
        for ( auto const& entry : m_entries )
        {
            if ( entry.m_severity == Severity::Error || entry.m_severity == Severity::FatalError )
            {
                perEntryFunction( entry );
            }
        }
    }

    void Log::Append( Log const& otherLog )
    {
        EE_ASSERT( this != &otherLog );

        LogConditionalScopeLockWrite sw( m_mode, m_mutex );
        LogConditionalScopeLockRead sr( otherLog.m_mode, otherLog.m_mutex );

        m_numErrors += otherLog.m_numErrors;
        m_numWarnings += otherLog.m_numWarnings;
        m_entries.insert( m_entries.end(), otherLog.m_entries.begin(), otherLog.m_entries.end() );
    }

    void Log::ReflectToSystemLog( LogCategory category, char const* pSourceInfo )
    {
        LogConditionalScopeLockRead sl( m_mode, m_mutex );
        for ( auto const& entry : m_entries )
        {
            SystemLog::AddEntry( entry.m_severity, category, pSourceInfo, "", -1, entry.m_message.c_str() );
        }
    }

    void Log::ReflectToSystemLogAndClear( LogCategory category, char const* pSourceInfo )
    {
        LogConditionalScopeLockWrite sl( m_mode, m_mutex );
        for ( auto const& entry : m_entries )
        {
            SystemLog::AddEntry( entry.m_severity, category, pSourceInfo, "", -1, entry.m_message.c_str() );
        }

        m_entries.clear();
        m_numWarnings = m_numErrors = 0;
    }

    void Log::SaveLogToString( String& logString ) const
    {
        LogConditionalScopeLockRead sl( m_mode, m_mutex );

        InlineString logLine;
        for ( auto const& entry : m_entries )
        {
            logLine.sprintf( "[%s] %s: %s\r\n", entry.m_timestamp.c_str(), GetSeverityAsString( entry.m_severity ), entry.m_message.c_str() );
            logString.append( logLine.c_str() );
        }
    }

    void Log::SaveLogToFile( FileSystem::Path const& logFilePath ) const
    {
        if ( !logFilePath.IsValid() || !logFilePath.IsFilePath() )
        {
            return;
        }

        logFilePath.EnsureDirectoryExists();

        //-------------------------------------------------------------------------

        String logData;
        SaveLogToString( logData );

        FileSystem::OutputFileStream logFile( logFilePath );
        logFile.Write( (void*) logData.data(), logData.size() );
    }
}