#pragma once

#include "System/_Module/API.h"
#include "System/Types/String.h"
#include "System/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::FileSystem { class Path; }

//-------------------------------------------------------------------------

namespace EE::Log
{
    enum class Severity
    {
        Message = 0,
        Warning,
        Error,
        FatalError,
    };

    EE_SYSTEM_API char const* GetSeverityAsString( Severity severity );

    struct LogEntry
    {
        String      m_timestamp;
        String      m_category;
        String      m_sourceInfo; // Extra optional information about the source of the log
        String      m_message;
        String      m_filename;
        uint32_t    m_lineNumber;
        Severity    m_severity;
    };

    // Lifetime
    //-------------------------------------------------------------------------

    EE_SYSTEM_API void Initialize();
    EE_SYSTEM_API void Shutdown();
    EE_SYSTEM_API bool IsInitialized();

    // Logging
    //-------------------------------------------------------------------------

    EE_SYSTEM_API void AddEntry( Severity severity, char const* pCategory, char const* pSourceInfo, char const* pFilename, int pLineNumber, char const* pMessageFormat, ... );
    EE_SYSTEM_API void AddEntryVarArgs( Severity severity, char const* pCategory, char const* pSourceInfo, char const* pFilename, int pLineNumber, char const* pMessageFormat, va_list args );
    EE_SYSTEM_API TVector<LogEntry> const& GetLogEntries();
    EE_SYSTEM_API int32_t GetNumWarnings();
    EE_SYSTEM_API int32_t GetNumErrors();

    // Output
    //-------------------------------------------------------------------------

    EE_SYSTEM_API void SetLogFilePath( FileSystem::Path const& logFilePath );
    EE_SYSTEM_API void SaveToFile();

    // Warnings and errors
    //-------------------------------------------------------------------------

    EE_SYSTEM_API bool HasFatalErrorOccurred();
    EE_SYSTEM_API LogEntry const& GetFatalError();

    // Transfers a list of unhandled warnings and errors - useful for displaying all errors for a given frame.
    // Calling this function will clear the list of warnings and errors.
    EE_SYSTEM_API TVector<LogEntry> GetUnhandledWarningsAndErrors();
}

//-------------------------------------------------------------------------

#define EE_LOG_MESSAGE( category, source, ... ) EE::Log::AddEntry( EE::Log::Severity::Message, category, source, __FILE__, __LINE__, __VA_ARGS__ )
#define EE_LOG_WARNING( category, source, ... ) EE::Log::AddEntry( EE::Log::Severity::Warning, category, source, __FILE__, __LINE__, __VA_ARGS__ )
#define EE_LOG_ERROR( category, source, ... ) EE::Log::AddEntry( EE::Log::Severity::Error, category, source, __FILE__, __LINE__, __VA_ARGS__ )
#define EE_LOG_FATAL_ERROR( category, source, ... ) EE::Log::AddEntry( EE::Log::Severity::FatalError, category, source, __FILE__, __LINE__, __VA_ARGS__ ); EE_HALT()