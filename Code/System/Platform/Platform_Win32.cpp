#ifdef _WIN32
#include "PlatformUtils_Win32.h"
#include "System/Esoterica.h"
#include "System/Logging/LoggingSystem.h"
#include "System/Types/Arrays.h"

#include <windows.h>
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>
#include <strsafe.h>
#include <Psapi.h>
#include <stdio.h>

#pragma comment( lib, "Dbghelp.lib" )

//-------------------------------------------------------------------------

namespace EE::Platform
{
    //-------------------------------------------------------------------------
    // Stack Walking
    //-------------------------------------------------------------------------

    struct Symbol
    {
        uint64_t    m_address;
        uint64_t    m_baseOfImage;
        String      m_imageName;
        String      m_file;
        String      m_symbol;
        uint32_t    m_lineNumber;
    };

    TVector<Symbol> WalkStack( PCONTEXT pExceptionContext )
    {
        HANDLE process = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, false, GetCurrentProcessId() );

        // Walk the stack
        //-------------------------------------------------------------------------

        HANDLE thread = OpenThread( READ_CONTROL, false, GetCurrentThreadId() );

        // Copy the context because StackWalk64 modifies it
        CONTEXT context = *pExceptionContext;

        // Grab the exception callstack
        STACKFRAME64 stackFrame = {};
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = context.Rip;
        stackFrame.AddrFrame.Offset = context.Rsp;
        stackFrame.AddrStack.Offset = context.Rsp;

        // Process each frame of the stack
        TInlineVector<uint64_t, 256> callstackAddresses;
        while ( StackWalk64( IMAGE_FILE_MACHINE_AMD64, process, thread, &stackFrame, &context, nullptr, &SymFunctionTableAccess64, &SymGetModuleBase64, nullptr ) )
        {
            uint64_t const address = stackFrame.AddrPC.Offset;
            callstackAddresses.emplace_back( address );
        }

        CloseHandle( thread );

        // Symbolificate the stack addresses
        //-------------------------------------------------------------------------

        TVector<Symbol> callstack;

        // For local processes, we let DbgHelp invade the process and figure out which modules and symbol files to load automatically
        SymSetOptions( SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME );
        if ( SymInitializeW( process, nullptr, true ) )
        {
            callstack.reserve( callstackAddresses.size() );
            for ( uint64_t const address : callstackAddresses )
            {
                // retrieve module information
                IMAGEHLP_MODULE64 module = {};
                module.SizeOfStruct = sizeof( IMAGEHLP_MODULE64 );
                BOOL const successfulModule = SymGetModuleInfo64( process, address, &module );

                // retrieve filename and line number
                DWORD displacement = 0u;
                IMAGEHLP_LINE64 line = {};
                line.SizeOfStruct = sizeof( IMAGEHLP_LINE64 );
                BOOL const successfulLine = SymGetLineFromAddr64( process, address, &displacement, &line );

                // retrieve function
                DWORD64 displacement64 = 0u;
                ULONG64 buffer[( sizeof( SYMBOL_INFO ) + MAX_SYM_NAME * sizeof( char ) + sizeof( ULONG64 ) - 1u ) / sizeof( ULONG64 )] = {};
                SYMBOL_INFO* symbolInfo = reinterpret_cast<SYMBOL_INFO*>( buffer );
                symbolInfo->SizeOfStruct = sizeof( SYMBOL_INFO );
                symbolInfo->MaxNameLen = MAX_SYM_NAME;
                BOOL const successfulSymbol = SymFromAddr( process, address, &displacement64, symbolInfo );

                callstack.emplace_back( Symbol
                {
                    address,
                    successfulModule ? module.BaseOfImage : 0ull,
                    successfulModule ? module.ImageName : String(),
                    successfulLine ? line.FileName : String(),
                    successfulSymbol ? symbolInfo->Name : String(),
                    successfulLine ? line.LineNumber : 0u
                } );
            }

            if ( !SymCleanup( process ) )
            {
                EE_TRACE_MSG( Win32::GetLastErrorMessage().c_str() );
            }
        }
        else
        {
            EE_TRACE_MSG( Win32::GetLastErrorMessage().c_str() );
        }

        CloseHandle( process );

        //-------------------------------------------------------------------------

        return callstack;
    }

    //-------------------------------------------------------------------------
    // Crash Handling
    //-------------------------------------------------------------------------

    void GenerateCrashDump( EXCEPTION_POINTERS* pExceptionPointers )
    {
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
        for ( int i = (int) strlen( processPath ) - 1; i > 0; --i )
        {
            if ( processPath[i] == '\\' )
            {
                processPath[i] = '\0';
                break;
            }
        }

        // Create file name for dump
        //-------------------------------------------------------------------------

        SYSTEMTIME stLocalTime;
        GetLocalTime( &stLocalTime );

        CHAR crashDumpDirectoryPath[MAX_PATH];
        StringCchPrintf( crashDumpDirectoryPath, MAX_PATH, "%s\\%s", processPath, "CrashDumps" );
        CreateDirectory( crashDumpDirectoryPath, NULL );

        CHAR minidumpFilePath[MAX_PATH];
        StringCchPrintf( minidumpFilePath, MAX_PATH, "%s\\%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
                            crashDumpDirectoryPath,
                            stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
                            stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
                            GetCurrentProcessId(), GetCurrentThreadId() );

        CHAR stackTraceFilePath[MAX_PATH];
        StringCchPrintf( stackTraceFilePath, MAX_PATH, "%s\\%04d%02d%02d-%02d%02d%02d-%ld-%ld.txt",
                            crashDumpDirectoryPath,
                            stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
                            stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
                            GetCurrentProcessId(), GetCurrentThreadId() );

        // Create dump
        //-------------------------------------------------------------------------

        HANDLE hDumpFile = CreateFile( minidumpFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0 );

        MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInfo;
        minidumpExceptionInfo.ThreadId = GetCurrentThreadId();
        minidumpExceptionInfo.ExceptionPointers = pExceptionPointers;
        minidumpExceptionInfo.ClientPointers = TRUE;

        MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithDataSegs, ( pExceptionPointers == nullptr ) ? NULL : &minidumpExceptionInfo, NULL, NULL );

        CloseHandle( hDumpFile );

        // Generate stack
        //-------------------------------------------------------------------------

        auto stack = WalkStack( pExceptionPointers->ContextRecord );
        if ( !stack.empty() )
        {
            InlineString stackFileData;

            for ( auto const& stackEntry : stack )
            {
                stackFileData.append_sprintf( "%s (%s:%d)", stackEntry.m_symbol.c_str(), stackEntry.m_file.c_str(), stackEntry.m_lineNumber );
            }

            HANDLE hStackTraceFile = CreateFile( stackTraceFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0 );

            DWORD dwBytesToWrite = (DWORD) strlen( stackFileData.c_str() );
            DWORD dwBytesWritten = 0;
            WriteFile( hStackTraceFile, stackFileData.c_str(), dwBytesToWrite, &dwBytesWritten, NULL );

            CloseHandle( hStackTraceFile );
        }
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
                Log::System::SaveToFile();
            }
            break;
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }

    static PVOID g_pExceptionHandler = nullptr;

    //-------------------------------------------------------------------------
    // Platform
    //-------------------------------------------------------------------------

    void Initialize()
    {
        g_pExceptionHandler = AddVectoredExceptionHandler( 1, VectoredExceptionHandler );
    }

    void Shutdown()
    {
        RemoveVectoredExceptionHandler( g_pExceptionHandler );
    }
}
#endif