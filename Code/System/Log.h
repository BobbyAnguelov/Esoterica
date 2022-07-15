#pragma once

#include "System/_Module/API.h"
#include "System/Types/String.h"
#include "System/Types/Arrays.h"
#include "System/FileSystem/FileSystemPath.h"

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

    struct LogEntry
    {
        String      m_timestamp;
        String      m_message;
        String      m_channel;
        String      m_filename;
        uint32_t      m_lineNumber;
        Severity    m_severity;
    };

    // Lifetime
    //-------------------------------------------------------------------------

    EE_SYSTEM_API void Initialize();
    EE_SYSTEM_API void Shutdown();
    EE_SYSTEM_API bool IsInitialized();

    // Logging
    //-------------------------------------------------------------------------

    EE_SYSTEM_API void AddEntry( Severity severity, char const* pChannel, char const* pFilename, int pLineNumber, char const* pMessageFormat, ... );
    EE_SYSTEM_API void AddEntryVarArgs( Severity severity, char const* pChannel, char const* pFilename, int pLineNumber, char const* pMessageFormat, va_list args );
    EE_SYSTEM_API TVector<LogEntry> const& GetLogEntries();
    EE_SYSTEM_API int32_t GetNumWarnings();
    EE_SYSTEM_API int32_t GetNumErrors();

    // Output
    //-------------------------------------------------------------------------

    EE_SYSTEM_API void SaveToFile( FileSystem::Path const& logFilePath );

    // Warnings and errors
    //-------------------------------------------------------------------------

    EE_SYSTEM_API bool HasFatalErrorOccurred();
    EE_SYSTEM_API LogEntry const& GetFatalError();

    // Transfers a list of unhandled warnings and errors - useful for displaying all errors for a given frame.
    // Calling this function will clear the list of warnings and errors.
    EE_SYSTEM_API TVector<LogEntry> GetUnhandledWarningsAndErrors();
}

//-------------------------------------------------------------------------

#define EE_LOG_MESSAGE( CHANNEL, ... ) EE::Log::AddEntry( EE::Log::Severity::Message, CHANNEL, __FILE__, __LINE__, __VA_ARGS__ )
#define EE_LOG_WARNING( CHANNEL, ... ) EE::Log::AddEntry( EE::Log::Severity::Warning, CHANNEL, __FILE__, __LINE__, __VA_ARGS__ )
#define EE_LOG_ERROR( CHANNEL, ... ) EE::Log::AddEntry( EE::Log::Severity::Error, CHANNEL, __FILE__, __LINE__, __VA_ARGS__ )
#define EE_LOG_FATAL_ERROR( CHANNEL, ... ) EE::Log::AddEntry( EE::Log::Severity::FatalError, CHANNEL, __FILE__, __LINE__, __VA_ARGS__ ); EE_HALT()