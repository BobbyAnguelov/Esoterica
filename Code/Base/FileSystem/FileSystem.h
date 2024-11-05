#pragma once

#include "Base/_Module/API.h"
#include "FileSystemPath.h"

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    // General functions
    //-------------------------------------------------------------------------

    // Does the path point to an existing file/directory
    EE_BASE_API bool Exists( char const* pPath );
    EE_FORCE_INLINE bool Exists( String const& filePath ) { return Exists( filePath.c_str() ); }
    EE_FORCE_INLINE bool Exists( Path const& filePath ) { return Exists( filePath.c_str() ); }

    // Is this file/directory read only
    EE_BASE_API bool IsReadOnly( char const* pPath );
    EE_FORCE_INLINE bool IsReadOnly( String const& path ) { return IsReadOnly( path.c_str() ); }
    EE_FORCE_INLINE bool IsReadOnly( Path const& path ) { return IsReadOnly( path.c_str() ); }

    // File functions
    //-------------------------------------------------------------------------
    // These function also perform additional validation based on the path type

    // Does the path refer to an existing file
    EE_BASE_API bool IsExistingFile( char const* pPath );
    EE_FORCE_INLINE bool IsExistingFile( String const& filePath ) { return IsExistingFile( filePath.c_str() ); }
    EE_FORCE_INLINE bool IsExistingFile( Path const& filePath ) { return IsExistingFile( filePath.c_str() ); }

    EE_BASE_API bool IsFileReadOnly( char const* pFilePath );
    EE_FORCE_INLINE bool IsFileReadOnly( String const& filePath ) { return IsFileReadOnly( filePath.c_str() ); }
    EE_FORCE_INLINE bool IsFileReadOnly( Path const& filePath ) { return IsFileReadOnly( filePath.c_str() ); }

    EE_BASE_API uint64_t GetFileModifiedTime( char const* pFilePath );
    EE_FORCE_INLINE uint64_t GetFileModifiedTime( String const& filePath ) { return GetFileModifiedTime( filePath.c_str() ); }
    EE_FORCE_INLINE uint64_t GetFileModifiedTime( Path const& filePath ) { return GetFileModifiedTime( filePath.c_str() ); }
    
    EE_BASE_API bool EraseFile( char const* pFilePath );
    EE_FORCE_INLINE bool EraseFile( String const& filePath ) { return EraseFile( filePath.c_str() ); }
    EE_FORCE_INLINE bool EraseFile( Path const& filePath ) { return EraseFile( filePath.c_str() ); }

    EE_BASE_API bool CopyExistingFile( char const* fromFilePath, char const* toFilePath );
    EE_FORCE_INLINE bool CopyExistingFile( String const& fromFilePath, String const& toFilePath ) { return CopyExistingFile( fromFilePath.c_str(), toFilePath.c_str() ); }
    EE_FORCE_INLINE bool CopyExistingFile( Path const& fromFilePath, Path const& toFilePath ) { return CopyExistingFile( fromFilePath.c_str(), toFilePath.c_str() ); }

    EE_BASE_API bool MoveExistingFile( char const* fromFilePath, char const* toFilePath );
    EE_FORCE_INLINE bool MoveExistingFile( String const& fromFilePath, String const& toFilePath ) { return MoveExistingFile( fromFilePath.c_str(), toFilePath.c_str() ); }
    EE_FORCE_INLINE bool MoveExistingFile( Path const& fromFilePath, Path const& toFilePath ) { return MoveExistingFile( fromFilePath.c_str(), toFilePath.c_str() ); }

    EE_FORCE_INLINE bool RenameExistingFile( char const* fromFilePath, char const* toFilePath ) { return MoveExistingFile( fromFilePath, toFilePath ); }
    EE_FORCE_INLINE bool RenameExistingFile( String const& fromFilePath, String const& toFilePath ) { return MoveExistingFile( fromFilePath.c_str(), toFilePath.c_str() ); }
    EE_FORCE_INLINE bool RenameExistingFile( Path const& fromFilePath, Path const& toFilePath ) { return MoveExistingFile( fromFilePath.c_str(), toFilePath.c_str() ); }

    // TextFiles
    //-------------------------------------------------------------------------

    EE_BASE_API bool ReadTextFile( char const* pFilePath, String& fileData );
    EE_FORCE_INLINE bool ReadTextFile( String const& filePath, String& fileData ) { return ReadTextFile( filePath.c_str(), fileData ); }
    EE_FORCE_INLINE bool ReadTextFile( Path const& filePath, String& fileData ) { return ReadTextFile( filePath.c_str(), fileData ); }

    EE_BASE_API bool WriteTextFile( char const* pFilePath, char const* pData, size_t size );
    EE_FORCE_INLINE bool WriteTextFile( char const* pFilePath, String const& fileData ) { return WriteTextFile( pFilePath, fileData.data(), fileData.size() ); }
    EE_FORCE_INLINE bool WriteTextFile( String const& filePath, String const& fileData ) { return WriteTextFile( filePath.c_str(), fileData ); }

    // This acts as a write operation but will read the file contents first and only write the data if the file needs to be updated!
    EE_BASE_API bool UpdateTextFile( char const* pFilePath, char const* pData, size_t size );
    EE_FORCE_INLINE bool UpdateTextFile( String const& filePath, char const* pData, size_t size ) { return UpdateTextFile( filePath.c_str(), pData, size ); }
    EE_FORCE_INLINE bool UpdateTextFile( Path const& filePath, char const* pData, size_t size ) { return UpdateTextFile( filePath.c_str(), pData, size ); }

    EE_FORCE_INLINE bool UpdateTextFile( char const* pFilePath, String const& fileData ) { return UpdateTextFile( pFilePath, fileData.data(), fileData.size() ); }
    EE_FORCE_INLINE bool UpdateTextFile( String const& filePath, String const& fileData ) { return UpdateTextFile( filePath.c_str(), fileData.data(), fileData.size() ); }
    EE_FORCE_INLINE bool UpdateTextFile( Path const& filePath, String const& fileData ) { return UpdateTextFile( filePath.c_str(), fileData.data(), fileData.size() ); }

    // Binary Files
    //-------------------------------------------------------------------------

    EE_BASE_API bool ReadBinaryFile( char const* pFilePath, Blob& fileData );
    EE_FORCE_INLINE bool ReadBinaryFile( String const& filePath, Blob& fileData ) { return ReadBinaryFile( filePath.c_str(), fileData ); }
    EE_FORCE_INLINE bool ReadBinaryFile( Path const& filePath, Blob& fileData ) { return ReadBinaryFile( filePath.c_str(), fileData ); }

    EE_BASE_API bool WriteBinaryFile( char const* pFilePath, void const* pData, size_t size );
    EE_FORCE_INLINE bool WriteBinaryFile( char const* pFilePath, Blob const& fileData ) { WriteBinaryFile( pFilePath, fileData.data(), fileData.size() ); }
    EE_FORCE_INLINE bool WriteBinaryFile( String const& filePath, Blob const& fileData ) { return WriteBinaryFile( filePath.c_str(), fileData ); }

    // This acts as a write operation but will check the file contents first and only write the data if the file needs to be updated!
    EE_BASE_API bool UpdateBinaryFile( char const* pFilePath, void const* pData, size_t size );
    EE_FORCE_INLINE bool UpdateBinaryFile( String const& filePath, void const* pData, size_t size ) { return UpdateBinaryFile( filePath.c_str(), pData, size ); }
    EE_FORCE_INLINE bool UpdateBinaryFile( Path const& filePath, void const* pData, size_t size ) { return UpdateBinaryFile( filePath.c_str(), pData, size ); }

    EE_FORCE_INLINE bool UpdateBinaryFile( char const* pFilePath, Blob const& fileData ) { UpdateBinaryFile( pFilePath, fileData.data(), fileData.size() ); }
    EE_FORCE_INLINE bool UpdateBinaryFile( String const& filePath, Blob const& fileData ) { return UpdateBinaryFile( filePath.c_str(), fileData ); }
    EE_FORCE_INLINE bool UpdateBinaryFile( Path const& filePath, Blob const& fileData ) { return UpdateBinaryFile( filePath.c_str(), fileData ); }

    // Directory Functions
    //-------------------------------------------------------------------------

    // Does the path refer to an existing directory
    EE_BASE_API bool IsExistingDirectory( char const* pPath );
    EE_FORCE_INLINE bool IsExistingDirectory( String const& filePath ) { return IsExistingDirectory( filePath.c_str() ); }
    EE_FORCE_INLINE bool IsExistingDirectory( Path const& filePath ) { EE_ASSERT( filePath.IsDirectoryPath() ); return IsExistingDirectory( filePath.c_str() ); }

    EE_BASE_API bool CreateDir( char const* path );
    EE_FORCE_INLINE bool CreateDir( String const& path ) { return CreateDir( path.c_str() ); }
    EE_FORCE_INLINE bool CreateDir( Path const& path ) { EE_ASSERT( path.IsDirectoryPath() ); return CreateDir( path.c_str() ); }

    EE_BASE_API bool EraseDir( char const* path );
    EE_FORCE_INLINE bool EraseDir( String const& path ) { return EraseDir( path.c_str() ); }
    EE_FORCE_INLINE bool EraseDir( Path const& path ) { EE_ASSERT( path.IsDirectoryPath() ); return EraseDir( path.c_str() ); }

    // Check if the directory exists, if it doesn't then create it
    EE_BASE_API bool EnsureDirectoryExists( char const* path );
    EE_FORCE_INLINE bool EnsureDirectoryExists( String const& path ) { return EnsureDirectoryExists( path.c_str() ); }
    EE_FORCE_INLINE bool EnsureDirectoryExists( Path const& path ) { EE_ASSERT( path.IsDirectoryPath() ); return EnsureDirectoryExists( path.c_str() ); }
}