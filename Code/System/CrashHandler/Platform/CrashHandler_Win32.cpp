#if _WIN32
#include "../CrashHandler.h"
#include "System/Esoterica.h"
#include "System/Log.h"
#include <windows.h>
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>
#include <strsafe.h>
#include <Psapi.h>

#pragma comment( lib, "Dbghelp.lib" )

//-------------------------------------------------------------------------

namespace EE::CrashHandling
{
    void GenerateCrashDump( EXCEPTION_POINTERS* pExceptionPointers )
    {
        DWORD dwBufferSize = MAX_PATH;
        HANDLE hDumpFile;
        SYSTEMTIME stLocalTime;
        MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInfo;

        // Get process path
        //-------------------------------------------------------------------------

        CHAR processPath[MAX_PATH];
        HANDLE pProcess = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, false, GetCurrentProcessId() );
        if ( pProcess != nullptr )
        {
            GetModuleFileNameEx( pProcess, 0, processPath, MAX_PATH );
            CloseHandle( pProcess );
        }

        // Get directory
        for ( int32_t i = (int32_t) strlen( processPath ) - 1; i > 0; --i )
        {
            if ( processPath[i] == '\\' )
            {
                processPath[i] = '\0';
                break;
            }
        }

        // Create file name for dump
        //-------------------------------------------------------------------------

        GetLocalTime( &stLocalTime );

        CHAR crashDumpDirectoryPath[MAX_PATH];
        StringCchPrintf( crashDumpDirectoryPath, MAX_PATH, "%s\\%s", processPath, "CrashDumps" );
        CreateDirectory( crashDumpDirectoryPath, NULL );

        CHAR minidumpFilePath[MAX_PATH];
        StringCchPrintf(    minidumpFilePath, MAX_PATH, "%s\\%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
                            crashDumpDirectoryPath,
                            stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
                            stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
                            GetCurrentProcessId(), GetCurrentThreadId() );

        CHAR stackTraceFilePath[MAX_PATH];
        StringCchPrintf(    stackTraceFilePath, MAX_PATH, "%s\\%04d%02d%02d-%02d%02d%02d-%ld-%ld.txt",
                            crashDumpDirectoryPath,
                            stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
                            stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
                            GetCurrentProcessId(), GetCurrentThreadId() );

        // Create dump
        //-------------------------------------------------------------------------

        hDumpFile = CreateFile( minidumpFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0 );

        minidumpExceptionInfo.ThreadId = GetCurrentThreadId();
        minidumpExceptionInfo.ExceptionPointers = pExceptionPointers;
        minidumpExceptionInfo.ClientPointers = TRUE;

        MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithDataSegs, ( pExceptionPointers == nullptr ) ? NULL : &minidumpExceptionInfo, NULL, NULL );
    }

    LONG WINAPI VectoredExceptionHandler( _EXCEPTION_POINTERS* pExceptionPtrs )
    {
        switch ( pExceptionPtrs->ExceptionRecord->ExceptionCode )
        {
            case EXCEPTION_ACCESS_VIOLATION:
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            case EXCEPTION_DATATYPE_MISALIGNMENT:
            case EXCEPTION_FLT_STACK_CHECK:
            case EXCEPTION_ILLEGAL_INSTRUCTION:
            case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            case EXCEPTION_PRIV_INSTRUCTION:
            case EXCEPTION_STACK_OVERFLOW:
            case EXCEPTION_BREAKPOINT:
            case EXCEPTION_SINGLE_STEP:
            {
                GenerateCrashDump( pExceptionPtrs );
                Log::SaveToFile();
            }
            break;
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }

    static PVOID g_pExceptionHandler = nullptr;

    //-------------------------------------------------------------------------

    void Initialize()
    {
        g_pExceptionHandler = AddVectoredExceptionHandler( 1, VectoredExceptionHandler );
    }

    void Shutdown()
    {
        EE_ASSERT( g_pExceptionHandler != nullptr );
        RemoveVectoredExceptionHandler( g_pExceptionHandler );
    }

    void RecordAssert( char const* pFile, int32_t line, char const* pAssertInfo )
    {
        EE_TRACE_MSG( pAssertInfo );
        Log::AddEntry( Log::Severity::Error, "Assert", "Assert", pFile, line, pAssertInfo );
    }

    void RecordAssertVarArgs( char const* pFile, int line, char const* pAssertInfoFormat, ... )
    {
        va_list args;
        va_start( args, pAssertInfoFormat );
        char buffer[512];
        vsprintf_s( buffer, 512, pAssertInfoFormat, args );
        va_end( args );

        RecordAssert( pFile, line, &buffer[0] );
    }
}
#endif