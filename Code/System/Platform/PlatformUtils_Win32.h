#pragma once
#ifdef _WIN32

#include "System/_Module/API.h"
#include "System/Types/String.h"

//-------------------------------------------------------------------------
// Platform Specific Helpers/Functions
//-------------------------------------------------------------------------

namespace EE::Platform::Win32
{
    // File system
    //-------------------------------------------------------------------------

    EE_SYSTEM_API String GetShortPath( String const& origPath );
    EE_SYSTEM_API String GetLongPath( String const& origPath );

    // Processes
    //-------------------------------------------------------------------------

    EE_SYSTEM_API uint32_t GetProcessID( char const* processName );
    EE_SYSTEM_API String GetProcessPath( uint32_t processID );
    EE_SYSTEM_API String GetCurrentModulePath();
    EE_SYSTEM_API String GetLastErrorMessage();

    // Try to start a window process and returns the process ID
    EE_SYSTEM_API uint32_t StartProcess( char const* exePath, char const* cmdLine = nullptr );

    // Kill a running process
    EE_SYSTEM_API bool KillProcess( uint32_t processID );

    // Check if a named process is currently running
    inline bool IsProcessRunning( char const* processName, uint32_t* pProcessID ) { return GetProcessID( processName ) != 0; }

    // Open a path in explorer
    EE_SYSTEM_API void OpenInExplorer( char const* path );
}
#endif