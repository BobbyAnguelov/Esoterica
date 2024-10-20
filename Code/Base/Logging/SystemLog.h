#pragma once
#include "LogEntry.h"
#include "Base/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

namespace EE::FileSystem { class Path; }

//-------------------------------------------------------------------------

namespace EE::SystemLog
{
    // Lifetime
    //-------------------------------------------------------------------------

    EE_BASE_API void Initialize();
    EE_BASE_API void Shutdown();
    EE_BASE_API bool WasInitialized();

    // Accessors
    //-------------------------------------------------------------------------

    EE_BASE_API TVector<Log::Entry> const& GetLogEntries();
    EE_BASE_API int32_t GetNumWarnings();
    EE_BASE_API int32_t GetNumErrors();

    EE_BASE_API bool HasFatalErrorOccurred();
    EE_BASE_API Log::Entry const& GetFatalError();

    // Transfers a list of unhandled warnings and errors - useful for displaying all errors for a given frame.
    // Calling this function will clear the list of warnings and errors.
    EE_BASE_API TVector<Log::Entry> GetUnhandledWarningsAndErrors();

    // Output
    //-------------------------------------------------------------------------

    EE_BASE_API void SetLogFilePath( FileSystem::Path const& logFilePath );
    EE_BASE_API void SaveToFile();
}