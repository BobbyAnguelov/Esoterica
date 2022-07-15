#pragma once

#include "FileSystem.h"
#include "System/Algorithm/Hash.h"

//-------------------------------------------------------------------------
// File System Path
//-------------------------------------------------------------------------
// A helper to abstract and perform some file system path operations
// Has a fair amount of limitations that have been made for performance reasons
//
// For directory paths, the only robust methods to distinguish between a file and a directory path string is to end with a path delimiter
// So any path that do not end in the path delimiter are assumed to be files
//
// On case-insensitive file systems, paths are forced to be lowercase

namespace EE::FileSystem
{
    class EE_SYSTEM_API Path
    {

    public:

        Path() = default;
        Path( Path&& path ) { *this = std::move( path ); }
        Path( Path const& path ) : m_fullpath( path.m_fullpath ), m_hashCode( path.m_hashCode ), m_isDirectoryPath( path.m_isDirectoryPath ) {}
        Path( String&& path ) : Path( path.c_str() ) {}
        Path( String const& path ) : Path( path.c_str() ) {}
        Path( char const* pPath );

        Path& operator=( Path& rhs );
        Path& operator=( Path const& rhs );
        Path& operator=( Path&& rhs );

        inline bool IsValid() const { return !m_fullpath.empty(); }
        inline size_t Length() const { return m_fullpath.length(); }
        inline String const& GetFullPath() const { return m_fullpath; }
        inline void Clear() { m_fullpath.clear(); m_hashCode = 0; }

        // Queries
        //-------------------------------------------------------------------------

        // Checks the file system to check if the file/directory exists
        inline bool Exists() const
        {
            EE_ASSERT( IsValid() );
            return m_isDirectoryPath ? IsExistingDirectory( m_fullpath.c_str() ) : IsExistingFile( m_fullpath.c_str() ); 
        }

        // Checks the file system to check if the file/directory is read-only
        inline bool IsReadOnly() const
        {
            EE_ASSERT( IsValid() );
            return FileSystem::IsReadOnly( m_fullpath.c_str() );
        }

        // Check if this path is under a specific directory
        bool IsUnderDirectory( Path const& parentDirectory ) const;

        // Get the directory depth for this path e.g. D:\Foo\Bar\Moo.txt = 2
        int32_t GetDirectoryDepth() const;

        // Get the full path depth for this path e.g. D:\Foo\Bar\Moo.txt = 3
        int32_t GetPathDepth() const;

        // Basic Operations
        //-------------------------------------------------------------------------

        // This function will check the actual filesystem and ensure that the path string has the correct format whether its a file or a directory
        // Note: this is not the cheapest function so avoid calling at runtime
        void EnsureCorrectPathStringFormat();

        // Ensures that the directory exists (uses the parent directory for file paths)
        bool EnsureDirectoryExists() const;

        // Append a string to the end of the path. Optional specify if we should add a path delimiter at the end of the appended string
        Path& Append( char const* pPathString, bool asDirectory = false );

        // Append a string to the end of the path. Optional specify if we should add a path delimiter at the end of the appended string
        inline Path& Append( String const& pathString, bool asDirectory = false ) { return Append( pathString.c_str(), asDirectory ); }

        inline Path& operator+=( char const* pPathString ) { return Append( pPathString ); }
        inline Path operator+( char const* pPathString ) const { return Path( m_fullpath ).Append( pPathString ); }
        inline Path& operator+=( String const& pathString ) { return Append( pathString.c_str() ); }
        inline Path operator+( String const& pathString ) const { return Path( m_fullpath ).Append( pathString.c_str() ); }

        // Splits the path into multiple strings e.g. D:\Foo\Bar\Moo.txt -> { D:, Foo, Bar, Moo.txt }
        TInlineVector<String, 10> Split() const;

        // Splits the path into multiple paths starting from the parent and going to the root e.g. D:\Foo\Bar\Moo.txt -> { D:\Foo\Bar, D:\Foo, D:\ }
        TInlineVector<Path, 10> GetDirectoryHierarchy() const;

        // Files
        //-------------------------------------------------------------------------
        // NOTE! These functions just operate on the string path and infer information from it, they dont check the actual file system

        // This will return true if the path string doesnt end with a path delimiter
        // NOTE! Does not actually verify if the path is a file
        inline bool IsFilePath() const { EE_ASSERT( IsValid() ); return !m_isDirectoryPath; }

        // Returns the filename as provided
        // Note: The casing might be incorrect on windows. If you want the correct case for the filename, please use the GetFilenameWithCorrectCase function.
        inline String GetFilename() const { return String( GetFilenameSubstr() ); }

        // Check if the filename including extension is equal to the supplied string
        bool IsFilenameEqual( char const* pString ) const;

        // Check if the filename including extension is equal to the supplied string
        inline bool IsFilenameEqual( String const& pString ) const { EE_ASSERT( !pString.empty() ); return IsFilenameEqual( pString.c_str() ); }

        // Get the filename for this path without the extension ( only valid to call on file paths )
        String GetFileNameWithoutExtension() const;

        // Extensions
        //-------------------------------------------------------------------------
        // Extensions dont include the "."

        // Does this path have an extension?
        bool HasExtension() const;

        // Case insensitive extension comparison
        bool MatchesExtension( char const* inExtension ) const;

        // Case insensitive extension comparison
        inline bool MatchesExtension( String const& inExtension ) const { return MatchesExtension( inExtension.c_str() ); }

        // Returns a ptr to the extension substr (excluding the '.'). Returns nullptr if there is no extension!
        char const* GetExtension() const;

        // Returns the extension for this path (excluding the '.'). Returns an empty string if there is no extension!
        inline TInlineString<6> GetExtensionAsString() const
        {
            char const* const pExtensionSubstr = GetExtension();
            return TInlineString<6>( pExtensionSubstr == nullptr ? "" : pExtensionSubstr );
        }

        // Returns a lowercase version of the extension (excluding the '.') if one exists else returns an empty string
        inline TInlineString<6> GetLowercaseExtensionAsString() const
        {
            char const* const pExtensionSubstr = GetExtension();
            TInlineString<6> ext( pExtensionSubstr == nullptr ? "" : pExtensionSubstr );
            ext.make_lower();
            return ext;
        }

        // Replaces the extension (excluding the '.') for this path (will create an extensions if no extension exists)
        void ReplaceExtension( const char* pExtension );

        // Replaces the extension (excluding the '.') for this path (will create an extensions if no extension exists)
        inline void ReplaceExtension( String const& extension ) { ReplaceExtension( extension.c_str() ); }

        // Replaces the extension (excluding the '.') for this path (will create an extensions if no extension exists)
        template<size_t S> void ReplaceExtension( TInlineString<S> const& extension ) { ReplaceExtension( extension.c_str() ); }

        // Directories
        //-------------------------------------------------------------------------
        // NOTE! These functions just operate on the string path and infer information from it, they dont check the actual file system

        // This will return true if the path string ends with a path delimiter
        // NOTE! Does not actually verify if the path is a directory
        inline bool IsDirectoryPath() const { EE_ASSERT( IsValid() ); return m_isDirectoryPath; }

        // This will ensure that this path ends in a path delimiter
        void MakeIntoDirectoryPath();

        // Gets the parent directory for this path
        Path GetParentDirectory() const;

        // Gets the directory name, only valid to call on directory paths
        String GetDirectoryName() const;

        // Replace the parent directory of this path to another one ( i.e. rebase the path )
        void ReplaceParentDirectory( Path const& newParentDirectory );

        // Conversion
        //-------------------------------------------------------------------------

        inline operator char const*( ) const { return m_fullpath.c_str(); }
        inline String const& ToString() const { return m_fullpath; }
        inline String const& GetString() const { return m_fullpath; }
        inline char const* c_str() const { return m_fullpath.c_str(); }

        // Comparison
        //-------------------------------------------------------------------------

        inline uint32_t GetHashCode() const { return m_hashCode; }
        inline bool operator==( Path const& RHS ) const { return m_hashCode == RHS.m_hashCode; }
        inline bool operator!=( Path const& RHS ) const { return m_hashCode != RHS.m_hashCode; }

    private:

        inline void UpdatePathInternals()
        {
            if ( m_fullpath.empty() )
            {
                m_hashCode = 0;
                m_isDirectoryPath = false;
            }
            else
            {
                m_hashCode = Hash::GetHash32( m_fullpath );
                m_isDirectoryPath = m_fullpath[m_fullpath.length() - 1] == Settings::s_pathDelimiter;
            }
        }

        char const* GetFilenameSubstr() const;

    private:

        String      m_fullpath;
        uint32_t    m_hashCode = 0;
        bool        m_isDirectoryPath = false;
    };
}

//-------------------------------------------------------------------------
// Path Helpers
//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    EE_FORCE_INLINE uint64_t GetFileModifiedTime( Path const& filePath )
    {
        return GetFileModifiedTime( filePath.c_str() );
    }

    EE_FORCE_INLINE bool EraseFile( Path const& filePath )
    {
        EE_ASSERT( filePath.IsFilePath() );
        return EraseFile( filePath.c_str() );
    }

    EE_FORCE_INLINE bool LoadFile( Path const& filePath, Blob& fileData )
    {
        EE_ASSERT( filePath.IsFilePath() );
        return LoadFile( filePath.c_str(), fileData );
    }

    EE_FORCE_INLINE bool CreateDir( Path const& path ) { EE_ASSERT( path.IsDirectoryPath() ); return CreateDir( path.c_str() ); }
    EE_FORCE_INLINE bool EraseDir( Path const& path ){ EE_ASSERT( path.IsDirectoryPath() ); return EraseDir( path.c_str() ); }
}

//-------------------------------------------------------------------------

namespace eastl
{
    template <>
    struct hash<EE::FileSystem::Path>
    {
        eastl_size_t operator()( EE::FileSystem::Path const& path ) const { return path.GetHashCode(); }
    };
}