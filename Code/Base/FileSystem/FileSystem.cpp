#include "FileSystem.h"
#include <fstream>
#include <filesystem>

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    bool GetCurrentWorkingDirectoryPath( Path& outPath )
    {
        std::error_code ec;
        std::filesystem::path cwd = std::filesystem::current_path();

        if ( ec.value() != 0 )
        {
            EE_LOG_ERROR( LogCategory::FileSystem, "Current Working Dir", "Cant get current working dir - %s", ec.message().c_str() );
        }

        //-------------------------------------------------------------------------

        outPath = FileSystem::Path( cwd.string().c_str() );
        return ec.value() == 0;
    }

    //-------------------------------------------------------------------------

    bool EnsureDirectoryExists( char const* path )
    {
        if ( !IsExistingDirectory( path ) )
        {
            return CreateDir( path );
        }

        return true;
    }

    bool CreateDir( char const* path )
    {
        std::error_code ec;
        std::filesystem::create_directories( path, ec );

        if ( ec.value() != 0 )
        {
            EE_LOG_ERROR( LogCategory::FileSystem, "Create Directory", "Error creating directory '%s' - %s", path, ec.message().c_str() );
        }

        return ec.value() == 0;
    }

    bool EraseDir( char const* path )
    {
        std::error_code ec;
        std::filesystem::remove_all( path, ec );

        if ( ec.value() != 0 )
        {
            EE_LOG_ERROR( LogCategory::FileSystem, "Erase Directory", "Error deleting directory '%s' - %s", path, ec.message().c_str() );
        }

        return ec.value() == 0;
    }

    bool EraseFile( char const* path )
    {
        if ( IsExistingFile( path ) )
        {
            std::error_code ec;
            std::filesystem::remove( path, ec );

            if ( ec.value() != 0 )
            {
                EE_LOG_ERROR( LogCategory::FileSystem, "Erase File", "Error erasing file '%s' - %s", path, ec.message().c_str() );
            }

            return ec.value() == 0;
        }

        return true;
    }

    bool CopyExistingFile( char const* fromFilePath, char const* toFilePath )
    {
        std::error_code ec;
        std::filesystem::copy( fromFilePath, toFilePath, std::filesystem::copy_options::overwrite_existing, ec );

        if ( ec.value() != 0 )
        {
            EE_LOG_ERROR( LogCategory::FileSystem, "Copy File", "Error copying from %s to %s - %s", fromFilePath, toFilePath, ec.message().c_str() );
        }

        return ec.value() == 0;
    }

    bool MoveExistingFile( char const* fromFilePath, char const* toFilePath )
    {
        std::error_code ec;
        std::filesystem::rename( fromFilePath, toFilePath, ec );

        if ( ec.value() != 0 )
        {
            EE_LOG_ERROR( LogCategory::FileSystem, "Move File", "Error moving from '%s' to '%s' - %s", fromFilePath, toFilePath, ec.message().c_str() );
        }

        return ec.value() == 0;
    }

    //-------------------------------------------------------------------------

    extern bool WriteFileToDisk( char const* pPath, void const* pData, size_t size, bool overwrite, bool flushToDisk );

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

    bool WriteTextFile( char const* pPath, char const* pData, size_t size, bool overwrite, bool flushToDisk )
    {
        return WriteFileToDisk( pPath, pData, size, overwrite, flushToDisk );
    }

    bool UpdateTextFile( char const* pFilePath, char const* pData, size_t size, bool* pWasFileUpdated )
    {
        EE_ASSERT( pFilePath != nullptr );
        EE_ASSERT( pData != nullptr && size > 0 );

        bool shouldUpdateFile = false;
        if ( Exists( pFilePath ) )
        {
            String currentFileContents;
            if ( !FileSystem::ReadTextFile( pFilePath, currentFileContents ) )
            {
                EE_LOG_ERROR( LogCategory::FileSystem, "Update File", "Failed to read file (%s) during file update!", pFilePath );
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

        if ( pWasFileUpdated != nullptr )
        {
            *pWasFileUpdated = shouldUpdateFile;
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

    bool WriteBinaryFile( char const* pPath, void const* pData, size_t size, bool overwrite, bool flushToDisk )
    {
        return WriteFileToDisk( pPath, pData, size, overwrite, flushToDisk );
    }

    bool UpdateBinaryFile( char const* pFilePath, void const* pData, size_t size, bool* pWasFileUpdated )
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
                EE_LOG_ERROR( LogCategory::FileSystem, "Update File", "Failed to read file (%s) during file update!", pFilePath );
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

        if ( pWasFileUpdated != nullptr )
        {
            *pWasFileUpdated = shouldUpdateFile;
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