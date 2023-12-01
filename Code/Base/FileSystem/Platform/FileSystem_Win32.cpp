#ifdef _WIN32
#include "../FileSystem.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include "Base/Encoding/Hash.h"
#include "Base/Math/Math.h"
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <fstream>
#include <filesystem>

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    char const Settings::s_pathDelimiter = '\\';

    constexpr static DWORD const g_maxPathBufferLength = 1024;

    //-------------------------------------------------------------------------

    bool GetFullPathString( char const* pPath, String& outPath )
    {
        if ( pPath != nullptr && pPath[0] != 0 )
        {
            // Warning: this function is slow, so use sparingly
            outPath.reserve( g_maxPathBufferLength );
            DWORD const length = GetFullPathNameA( pPath, g_maxPathBufferLength, outPath.data(), nullptr);
            EE_ASSERT( length != 0 && length != g_maxPathBufferLength );
            outPath.force_size( length );

            // Ensure directory paths have the final slash appended
            DWORD const result = GetFileAttributesA( outPath.c_str() );
            if ( result != INVALID_FILE_ATTRIBUTES && ( result & FILE_ATTRIBUTE_DIRECTORY ) && outPath[length - 1] != Settings::s_pathDelimiter )
            {
                outPath += Settings::s_pathDelimiter;
            }

            return true;
        }

        return false;
    }

    bool GetCorrectCaseForPath( char const* pPath, String& outPath )
    {
        bool failed = false;

        HANDLE hFile = CreateFile( pPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
        if ( hFile != INVALID_HANDLE_VALUE )
        {
            char buffer[g_maxPathBufferLength];
            DWORD dwRet = GetFinalPathNameByHandle( hFile, buffer, g_maxPathBufferLength, FILE_NAME_NORMALIZED );
            if ( dwRet < g_maxPathBufferLength )
            {
                outPath = buffer + 4;
            }
            else
            {
                failed = true;
            }

            CloseHandle( hFile );
        }
        else // Invalid handle i.e. file doesnt exist
        {
            failed = true;
        }

        //-------------------------------------------------------------------------

        if ( failed )
        {
            outPath = pPath;
        }

        //-------------------------------------------------------------------------

        return !failed;
    }

    //-------------------------------------------------------------------------

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

    bool CreateDir( char const* path )
    {
        // TODO: replace with appropriate windows call
        std::error_code ec;
        std::filesystem::create_directories( path, ec );
        return ec.value() == 0;
    }

    bool EraseDir( char const* path )
    {
        // TODO: replace with appropriate windows call
        std::error_code ec;
        std::filesystem::remove_all( path, ec );
        return ec.value() == 0;
    }

    bool EraseFile( char const* path )
    {
        return DeleteFile( path );
    }

    //-------------------------------------------------------------------------

    bool LoadFile( char const* pPath, Blob& fileData )
    {
        EE_ASSERT( pPath != nullptr );

        // Open file handle
        HANDLE hFile = CreateFile( pPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_POSIX_SEMANTICS, nullptr );
        if ( hFile == INVALID_HANDLE_VALUE )
        {
            return false;
        }

        // Get file size
        LARGE_INTEGER fileSizeLI;
        if ( !GetFileSizeEx( hFile, &fileSizeLI ) )
        {
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
            ReadFile( hFile, pBuffer, numBytesToRead, &bytesRead, nullptr );
            pBuffer += bytesRead;
            remainingBytesToRead -= bytesRead;
        }

        CloseHandle( hFile );
        return true;
    }
}

#endif