#ifdef _WIN32
#include "../FileSystem.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include "Base/Encoding/Hash.h"
#include "Base/Math/Math.h"
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    Path GetCurrentProcessPath()
    {
        return Path( EE::Platform::Win32::GetCurrentModulePath() ).GetParentDirectory();
    }

    bool Exists( char const* pPath )
    {
        DWORD dwAttrib = GetFileAttributes( pPath );
        return ( dwAttrib != INVALID_FILE_ATTRIBUTES );
    }

    bool IsReadOnly( char const* pPath )
    {
        DWORD dwAttrib = GetFileAttributes( pPath );
        return ( dwAttrib != INVALID_FILE_ATTRIBUTES && ( dwAttrib & FILE_ATTRIBUTE_READONLY ) );
    }

    bool IsExistingFile( char const* pPath )
    {
        DWORD dwAttrib = GetFileAttributes( pPath );
        return ( dwAttrib != INVALID_FILE_ATTRIBUTES && !( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) );
    }

    bool IsExistingDirectory( char const* pPath )
    {
        DWORD dwAttrib = GetFileAttributes( pPath );
        return ( dwAttrib != INVALID_FILE_ATTRIBUTES && ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) );
    }

    bool IsFileReadOnly( char const* pPath )
    {
        DWORD dwAttrib = GetFileAttributes( pPath );
        return ( dwAttrib != INVALID_FILE_ATTRIBUTES && !( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) && ( dwAttrib & FILE_ATTRIBUTE_READONLY ) );
    }

    uint64_t GetFileModifiedTime( char const* path )
    {
        ULARGE_INTEGER fileWriteTime;
        fileWriteTime.QuadPart = 0;

        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA( path, &findData );
        if ( hFind != INVALID_HANDLE_VALUE )
        {
            fileWriteTime.LowPart = findData.ftLastWriteTime.dwLowDateTime;
            fileWriteTime.HighPart = findData.ftLastWriteTime.dwHighDateTime;
            FindClose( hFind );
        }

        return fileWriteTime.QuadPart;
    }

    //-------------------------------------------------------------------------

    bool WriteFileToDisk( char const* pPath, void const* pData, size_t size, bool overwrite = true, bool flushToDisk = false )
    {
        // FILE_FLAG_WRITE_THROUGH forces writes straight to disk, bypassing the
        // intermediate OS cache — combine with flushToDisk for full durability control,
        // or omit if you just want normal cached writes + optional explicit flush.
        DWORD flagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
        HANDLE hFile = CreateFile( pPath, GENERIC_WRITE, 0, nullptr, overwrite ? CREATE_ALWAYS : CREATE_NEW, flagsAndAttributes, nullptr );
        if ( hFile == INVALID_HANDLE_VALUE )
        {
            String const errorString = Platform::Win32::GetLastErrorMessage();
            EE_LOG_ERROR( LogCategory::FileSystem, "WriteFile", "Failed to open file handle for write: %s, Error: %s", pPath, errorString.c_str() );
            return false;
        }

        bool success = true;
        BYTE const* pWritePtr = static_cast<const BYTE*>( pData );
        size_t remainingBytes = size;

        // WriteFile can write fewer bytes than requested in a single call, so loop until everything is written or an error occurs.
        while ( remainingBytes > 0 )
        {
            DWORD bytesToWrite = static_cast<DWORD>( remainingBytes > MAXDWORD ? MAXDWORD : remainingBytes );
            DWORD bytesWritten = 0;

            if ( !WriteFile( hFile, pWritePtr, bytesToWrite, &bytesWritten, nullptr ) )
            {
                String const errorString = Platform::Win32::GetLastErrorMessage();
                EE_LOG_ERROR( LogCategory::FileSystem, "WriteFile", "Failed to write file: %s, Error: %s", pPath, errorString.c_str() );
                success = false;
                break;
            }

            if ( bytesWritten == 0 )
            {
                String const errorString = Platform::Win32::GetLastErrorMessage();
                EE_LOG_ERROR( LogCategory::FileSystem, "WriteFile", "Failed to write file: %s, Error: %s", pPath, errorString.c_str() );
                success = false;
                break;
            }

            pWritePtr += bytesWritten;
            remainingBytes -= bytesWritten;
        }

        if ( success && flushToDisk )
        {
            success = ( FlushFileBuffers( hFile ) != 0 );
        }

        CloseHandle( hFile );
        return success;
    }

    bool ReadBinaryFile( char const* pPath, Blob& fileData )
    {
        EE_ASSERT( pPath != nullptr );

        // Open file handle
        HANDLE hFile = CreateFileA( pPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr );
        if ( hFile == INVALID_HANDLE_VALUE )
        {
            String errorString = Platform::Win32::GetLastErrorMessage();
            EE_LOG_ERROR( LogCategory::FileSystem, "ReadBinaryFile", "Failed to open file handle for read: %s, Error: %s", pPath, errorString.c_str() );
            return false;
        }

        // Get file size
        LARGE_INTEGER fileSizeLI;
        if ( !GetFileSizeEx( hFile, &fileSizeLI ) )
        {
            String const errorString = Platform::Win32::GetLastErrorMessage();
            EE_LOG_ERROR( LogCategory::FileSystem, "ReadBinaryFile", "Failed to get file size for: %s, Error: %s", pPath, errorString.c_str() );
            CloseHandle( hFile );
            return false;
        }

        // Allocate destination memory
        size_t const fileSize = (size_t) ( fileSizeLI.QuadPart );
        fileData.resize( fileSize );

        // Read file
        static constexpr DWORD const defaultReadBufferSize = 65536;
        DWORD bytesRead = 0;
        DWORD remainingBytesToRead = (DWORD) fileSize;

        uint8_t* pBuffer = fileData.data();
        while ( remainingBytesToRead != 0 )
        {
            DWORD const numBytesToRead = Math::Min( defaultReadBufferSize, remainingBytesToRead );
            if ( !ReadFile( hFile, pBuffer, numBytesToRead, &bytesRead, nullptr ) )
            {
                fileData.clear();
                String const errorString = Platform::Win32::GetLastErrorMessage();
                EE_LOG_ERROR( LogCategory::FileSystem, "ReadBinaryFile", "Failed to get read from binary file: %s, Error: %s", pPath, errorString.c_str() );
                CloseHandle( hFile );
                return false;
            }

            pBuffer += bytesRead;
            remainingBytesToRead -= bytesRead;
        }

        CloseHandle( hFile );
        return true;
    }
}
#endif