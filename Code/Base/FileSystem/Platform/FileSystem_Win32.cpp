#ifdef _WIN32
#include "../FileSystem.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include "Base/Encoding/Hash.h"
#include "Base/Math/Math.h"
#include "Base/Types/UUID.h"
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <fstream>
#include <filesystem>

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
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

        if ( ec.value() != 0 )
        {
            EE_LOG_ERROR( "FileSystem", "Create Directory", "Error creating directory %s - %s", path, ec.message().c_str() );
        }

        return ec.value() == 0;
    }

    bool EraseDir( char const* path )
    {
        // TODO: replace with appropriate windows call
        std::error_code ec;
        std::filesystem::remove_all( path, ec );

        if ( ec.value() != 0 )
        {
            EE_LOG_ERROR( "FileSystem", "Erase Directory", "Error deleting directory %s - %s", path, ec.message().c_str() );
        }

        return ec.value() == 0;
    }

    bool EraseFile( char const* path )
    {
        if ( IsExistingFile( path ) )
        {
            return DeleteFile( path );
        }

        return true;
    }

    bool CopyExistingFile( char const* fromFilePath, char const* toFilePath )
    {
        std::error_code ec;
        std::filesystem::copy( fromFilePath, toFilePath, std::filesystem::copy_options::overwrite_existing, ec );

        if ( ec.value() != 0 )
        {
            EE_LOG_ERROR( "FileSystem", "Copy File", "Error copying from %s to %s - %s", fromFilePath, toFilePath, ec.message().c_str() );
        }

        return ec.value() == 0;
    }

    bool MoveExistingFile( char const* fromFilePath, char const* toFilePath )
    {
        std::error_code ec;
        std::filesystem::rename( fromFilePath, toFilePath, ec );

        if ( ec.value() != 0 )
        {
            EE_LOG_ERROR( "FileSystem", "Move File", "Error moving from %s to %s - %s", fromFilePath, toFilePath, ec.message().c_str() );
        }

        return ec.value() == 0;
    }

    //-------------------------------------------------------------------------

    static bool WriteFile( char const* pPath, void const* pData, size_t size, bool isBinary )
    {
        EE_ASSERT( pPath != nullptr );
        EE_ASSERT( pData != nullptr && size > 0 );

        // Write to a temp file first
        //-------------------------------------------------------------------------

        Path tmpPath( pPath );
        tmpPath = tmpPath.GetParentDirectory();
        tmpPath.Append( UUID::GenerateID().ToString().c_str() );

        FILE* pFile = fopen( tmpPath.c_str(), isBinary ? "wb" : "w" );
        if ( pFile == nullptr )
        {
            EE_LOG_ERROR( "FileSystem", "Write File", "Error writing temp file %s.", tmpPath.c_str() );
            return false;
        }

        fwrite( pData, size, 1, pFile );
        fclose( pFile );

        // Rename to final name
        //-------------------------------------------------------------------------

        std::error_code ec;
        std::filesystem::rename( tmpPath.c_str(), pPath, ec );

        if ( ec.value() != 0 )
        {
            EE_LOG_ERROR( "FileSystem", "Write File", "Failed to rename tmp path (%s) to final path (%s) during file write. Error: %s", tmpPath.c_str(), pPath, ec.message().c_str() );
            return false;
        }

        return ec.value() == 0;
    }

    //-------------------------------------------------------------------------

    bool ReadTextFile( char const* pFilePath, String& fileData )
    {
        // Open the stream to 'lock' the file.
        std::ifstream f( pFilePath, std::ios::in );
        if ( f.fail() )
        {
            return false;
        }

        // Obtain the size of the file.
        uintmax_t const fileSize = std::filesystem::file_size( pFilePath );

        // Create a buffer.
        fileData.resize( fileSize );
        Memory::MemsetZero( fileData.data(), fileSize );

        // Read the whole file into the buffer.
        f.read( fileData.data(), fileSize );

        // Handle any EOL conversions that result in a smaller string than expected
        fileData.resize( f.gcount() );

        return true;
    }

    bool WriteTextFile( char const* pPath, char const* pData, size_t size )
    {
        return WriteFile( pPath, pData, size, false );
    }

    bool UpdateTextFile( char const* pFilePath, char const* pData, size_t size )
    {
        EE_ASSERT( pFilePath != nullptr );
        EE_ASSERT( pData != nullptr && size > 0 );

        bool shouldUpdateFile = false;
        if ( Exists( pFilePath ) )
        {
            String currentFileContents;
            if ( !FileSystem::ReadTextFile( pFilePath, currentFileContents ) )
            {
                EE_LOG_ERROR( "FileSystem", "Update File", "Failed to read file (%s) during file update!", pFilePath );
                return false;
            }

            if ( currentFileContents != pData )
            {
                shouldUpdateFile = true;
            }
        }
        else
        {
            shouldUpdateFile = true;
        }

        //-------------------------------------------------------------------------

        if ( shouldUpdateFile )
        {
            return FileSystem::WriteTextFile( pFilePath, pData );
        }
        else
        {
            return true;
        }
    }

    //-------------------------------------------------------------------------

    bool ReadBinaryFile( char const* pPath, Blob& fileData )
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

    bool WriteBinaryFile( char const* pPath, void const* pData, size_t size )
    {
        return WriteFile( pPath, pData, size, true );
    }

    bool UpdateBinaryFile( char const* pFilePath, void const* pData, size_t size )
    {
        EE_ASSERT( pFilePath != nullptr );
        EE_ASSERT( pData != nullptr && size > 0 );

        bool shouldUpdateFile = false;
        if ( Exists( pFilePath ) )
        {
            // Note: this is a very naive and sub-optimal way to do this
            // TODO: if this is proving to be too slow, then replace this naive code below with a proper file diff

            Blob currentFileContents;
            if ( !FileSystem::ReadBinaryFile( pFilePath, currentFileContents ) )
            {
                EE_LOG_ERROR( "FileSystem", "Update File", "Failed to read file (%s) during file update!", pFilePath );
                return false;
            }

            if ( currentFileContents.size() != size )
            {
                shouldUpdateFile = true;
            }
            else
            {
                uint8_t const* pByteData = (uint8_t const*) pData;
                for ( int32_t i = 0; i < size; i++ )
                {
                    if ( pByteData[i] != currentFileContents[i] )
                    {
                        shouldUpdateFile = true;
                        break;
                    }
                }
            }
        }
        else
        {
            shouldUpdateFile = true;
        }

        //-------------------------------------------------------------------------

        if ( shouldUpdateFile )
        {
            return FileSystem::WriteBinaryFile( pFilePath, pData, size );
        }
        else
        {
            return true;
        }
    }
}
#endif