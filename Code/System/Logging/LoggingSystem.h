#pragma once
#include "Log.h"
#include "System/Types/String.h"
#include "System/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

namespace EE::FileSystem { class Path; }

//-------------------------------------------------------------------------

namespace EE::Log
{
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

    EE_SYSTEM_API char const* GetSeverityAsString( Severity severity );

    // Lifetime
    //-------------------------------------------------------------------------

    struct EE_SYSTEM_API System
    {
        // Lifetime
        //-------------------------------------------------------------------------

        static void Initialize();
        static void Shutdown();
        static bool IsInitialized();

        // Accessors
        //-------------------------------------------------------------------------

        static TVector<LogEntry> const& GetLogEntries();
        static int32_t GetNumWarnings();
        static int32_t GetNumErrors();

        static bool HasFatalErrorOccurred();
        static LogEntry const& GetFatalError();

        // Transfers a list of unhandled warnings and errors - useful for displaying all errors for a given frame.
        // Calling this function will clear the list of warnings and errors.
        static TVector<LogEntry> GetUnhandledWarningsAndErrors();

        // Output
        //-------------------------------------------------------------------------

        static void SetLogFilePath( FileSystem::Path const& logFilePath );
        static void SaveToFile();
    };
}