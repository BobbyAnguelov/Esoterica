#ifdef _WIN32
#include "PlatformUtils_Win32.h"
#include "Base/Memory/Memory.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <shellapi.h>

//-------------------------------------------------------------------------

namespace EE::Platform::Win32
{
    uint32_t GetProcessID( char const* processName )
    {
        uint32_t processID = 0;
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof( PROCESSENTRY32 );

        HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, NULL );

        if ( Process32First( snapshot, &entry ) )
        {
            while ( Process32Next( snapshot, &entry ) )
            {
                if ( strcmp( entry.szExeFile, processName ) == 0 )
                {
                    processID = entry.th32ProcessID;
                }
            }
        }

        CloseHandle( snapshot );
        return processID;
    }

    uint32_t StartProcess( char const* exePath, char const* cmdLine )
    {
        PROCESS_INFORMATION processInfo;
        memset( &processInfo, 0, sizeof( processInfo ) );

        STARTUPINFO startupInfo;
        memset( &startupInfo, 0, sizeof( startupInfo ) );
        startupInfo.hStdInput = GetStdHandle( STD_INPUT_HANDLE );
        startupInfo.hStdOutput = GetStdHandle( STD_OUTPUT_HANDLE );
        startupInfo.hStdError = GetStdHandle( STD_ERROR_HANDLE );
        startupInfo.dwFlags = STARTF_USESTDHANDLES;
        startupInfo.cb = sizeof( startupInfo );

        char fullCmdLine[MAX_PATH];
        if ( cmdLine != nullptr )
        {
            Printf( fullCmdLine, MAX_PATH, "%s %s", exePath, cmdLine );
        }
        else
        {
            Printf( fullCmdLine, MAX_PATH, "%s", exePath );
        }

        if ( CreateProcess( nullptr, fullCmdLine, nullptr, nullptr, false, 0, nullptr, nullptr, &startupInfo, &processInfo ) )
        {
            return processInfo.dwProcessId;
        }

        return 0;
    }

    bool KillProcess( uint32_t processID )
    {
        EE_ASSERT( processID != 0 );
        HANDLE pProcess = OpenProcess( PROCESS_TERMINATE, false, processID );
        if ( pProcess != nullptr )
        {
            return TerminateProcess( pProcess, 0 ) > 0;
        }

        return false;
    }

    String GetProcessPath( uint32_t processID )
    {
        EE_ASSERT( processID != 0 );

        String returnPath;
        HANDLE pProcess = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, false, processID );
        if ( pProcess != nullptr )
        {
            TCHAR currentProcessPath[MAX_PATH + 1];
            if ( GetModuleFileNameEx( pProcess, 0, currentProcessPath, MAX_PATH + 1 ) )
            {
                returnPath = currentProcessPath;
            }
            CloseHandle( pProcess );
        }

        return returnPath;
    }

    String GetCurrentModulePath()
    {
        String path;
        TCHAR exepath[MAX_PATH + 1];
        if ( GetModuleFileName( 0, exepath, MAX_PATH + 1 ) )
        {
            path = exepath;
        }

        return path;
    }

    String GetLastErrorMessage()
    {
        auto const errorID = GetLastError();

        LPSTR pMessageBuffer = nullptr;
        auto size = FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorID, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPSTR) &pMessageBuffer, 0, NULL );
        String message( pMessageBuffer, size );
        LocalFree( pMessageBuffer );
        return message;
    }

    String GetShortPath( String const& origPath )
    {
        TCHAR path[MAX_PATH + 1];
        DWORD const returnValue = GetShortPathName( origPath.c_str(), path, MAX_PATH + 1 );
        if ( returnValue > 0 && returnValue <= MAX_PATH )
        {
            return String( path );
        }

        return String();
    }

    String GetLongPath( String const& origPath )
    {
         TCHAR path[MAX_PATH + 1];
         DWORD const returnValue = GetLongPathNameA( origPath.c_str(), path, MAX_PATH + 1 );
         if ( returnValue > 0 && returnValue <= MAX_PATH )
         {
             return String( path );
         }

         return String();
    }

    void OpenInExplorer( char const* path )
    {
        EE_ASSERT( path != nullptr && path[0] != 0 );
        InlineString cmdLine( InlineString::CtorSprintf(), "/select, %s", path );
        ShellExecute( 0, NULL, "explorer.exe", cmdLine.c_str(), NULL, SW_SHOWNORMAL );
    }
}
#endif