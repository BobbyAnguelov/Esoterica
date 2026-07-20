#pragma once

#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Types/String.h"
#include "Base/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------
// Data Path
//-------------------------------------------------------------------------
// Always relative to the data directory (data://)
// NB!!!    Data paths are case-insensitive.
//          It is up to users on case sensitive file systems to ensure that there wont be two files that a data path can resolve to
//-------------------------------------------------------------------------
//
// Data paths support the concept of sub-filenames. i.e. data://actualFilename.file:subFile.tmp
// This allows us to directly specify internal objects without having to create explicit files for them
//
//-------------------------------------------------------------------------

namespace EE
{
    struct IDataFile;

    class EE_BASE_API DataPath
    {
        EE_CUSTOM_SERIALIZE_WRITE_FUNCTION( archive )
        {
            archive << GetString();
            return archive;
        }

        EE_CUSTOM_SERIALIZE_READ_FUNCTION( archive )
        {
            InlineString tmpPath;
            archive << tmpPath;
            FromString( tmpPath.c_str(), CreationOptions::AllowInvalidPath );
            return archive;
        }

    public:

        constexpr static char const* s_pathPrefix = "data://";
        constexpr static int32_t const s_pathPrefixLength = 7;

        // The delimiter used to separate data path elements
        constexpr static char const s_pathDelimiter = '/';

        // The delimiter used between the parent resource name and the child name
        constexpr static char const s_subFileDelimiter = ':';

        // The delimiter used between the parent resource name and the child name
        constexpr static char const s_subFileFlattenedPathDelimiter = '_';

        //-------------------------------------------------------------------------

        enum CreationOptions
        {
            Default = 0, // Will assert on invalid path str
            AllowInvalidPath // Silently fails and creates and invalid data path
        };

        //-------------------------------------------------------------------------

        static bool IsValidPath( char const* pPath );

        static inline bool IsValidPath( String const& path ) { return IsValidPath( path.c_str() ); }

    public:

        DataPath() = default;
        virtual ~DataPath() = default;

        explicit DataPath( char const* pPath, CreationOptions opt = CreationOptions::Default ) { FromString( pPath, opt ); }
        explicit DataPath( String const& path, CreationOptions opt = CreationOptions::Default ) : DataPath( path.c_str(), opt ) {}
        explicit DataPath( String&& path, CreationOptions opt = CreationOptions::Default ) : DataPath( path.c_str(), opt ) {}

        explicit DataPath( FileSystem::Path const& filePath, FileSystem::Path const& dataDirectoryPath );

        DataPath( DataPath&& path );
        DataPath( DataPath const& path );

        //-------------------------------------------------------------------------

        virtual bool IsValid() const { return !m_path.empty(); }
        inline bool IsRootPath() const { return IsValid() && m_path.comparei( s_pathPrefix ) == 0; }
        inline void Clear() { m_path.clear(); m_ID = 0; }
        inline uint64_t GetID() const { return m_ID; }

        // Path info
        //-------------------------------------------------------------------------

        // Returns the filename
        String GetFilename() const;

        // Returns the filename with all the 'extensions' removed (i.e. file.final.png -> file )
        String GetFilenameWithoutExtension() const;

        // Replace the filename for this path ( only valid to call on file paths )
        void ReplaceFilename( char const* pFilename, bool alsoClearSubFilename = true );

        // Replace the filename for this path ( only valid to call on file paths )
        void ReplaceFilename( String const& filename, bool alsoClearSubFilename = true ) { ReplaceFilename( filename.c_str(), alsoClearSubFilename ); }

        // Replace the filename for this path ( only valid to call on file paths )
        void ReplaceFilename( InlineString const& filename, bool alsoClearSubFilename = true ) { ReplaceFilename( filename.c_str(), alsoClearSubFilename ); }

        // Append a filename to a directory path
        void AppendFilename( char const* pFilename );

        // Append a filename to a directory path
        void AppendFilename( String const& filename ) { AppendFilename( filename.c_str() ); }

        // Append a filename to a directory path
        void AppendFilename( InlineString const& filename ) { AppendFilename( filename.c_str() ); }

        // Get the directory path - if this is a directory return itself, if this is a file then return the parent directory
        inline DataPath GetDirectoryPath() const { return ( IsDirectoryPath() ) ? *this : GetParentDirectory(); }

        // Get the containing directory path for this path
        DataPath GetParentDirectory() const;

        // Returns the directory name
        String GetDirectoryName() const;

        // Do we have a parent directory
        bool HasParentDirectory() const;

        // Get the directory depth for this path e.g. D:\Foo\Bar\Moo.txt = 2
        int32_t GetDirectoryDepth() const;

        // Get the full path depth for this path e.g. D:\Foo\Bar\Moo.txt = 3
        int32_t GetPathDepth() const;

        // Check if this path is under a specific path
        bool IsUnder( DataPath const& parentpath ) const;

        // Is this a directory path
        inline bool IsDirectoryPath() const { EE_ASSERT( DataPath::IsValid() ); return m_path.back() == s_pathDelimiter; }

        // Is this a file path
        inline bool IsFilePath() const { EE_ASSERT( DataPath::IsValid() ); return !IsDirectoryPath(); }

        // Get a filesystem path for this data path
        FileSystem::Path GetFileSystemPath( FileSystem::Path const& dataDirectoryPath ) const;

        // Extension
        //-------------------------------------------------------------------------
        // Operates on the actual filename and not the internal object if one is specified

        // Returns the extension for this path (excluding the '.'). Returns an empty string if there is no extension!
        FileSystem::Extension GetExtension() const;

        // Replaces the extension (excluding the '.') for this path (will create an extensions if no extension exists)
        void ReplaceExtension( const char* pExtension );

        // Replaces the extension (excluding the '.') for this path (will create an extensions if no extension exists)
        inline void ReplaceExtension( String const& extension ) { ReplaceExtension( extension.c_str() ); }

        // Replaces the extension (excluding the '.') for this path (will create an extensions if no extension exists)
        template<size_t S> void ReplaceExtension( TInlineString<S> const& extension ) { ReplaceExtension( extension.c_str() ); }

        // Sub-File Paths
        //-------------------------------------------------------------------------

        // Do we have an optional sub-filename set?
        inline bool HasSubFilename() const { return !m_subFilenamePath.empty(); }

        // Get the optional sub-filename set
        char const* GetSubFilename() const;

        // Set the optional sub-filename set - only single extension filenames are allowed!
        void SetSubFilename( char const* pObjectName );

        // Set the optional sub-filename set - only single extension filenames are allowed!
        void SetSubFilename( String const& objectName ) { SetSubFilename( objectName.c_str() ); }

        // Clear the sub-filename
        void ClearSubFilename() { m_subFilenamePath.clear(); CalculateID(); }

        // Get without the sub-filename
        inline DataPath GetPathWithoutSubFilename() const { DataPath d( *this ); d.ClearSubFilename(); return d; }

        // Returns the sub-filename extension
        FileSystem::Extension GetSubFilenameExtension() const;

        // Get the flattened out filesystem path - 'data://parent.file:child.tmp' -> 'D:\parent_child.tmp'
        FileSystem::Path GetFlattenedFileSystemPath( FileSystem::Path const& dataDirectoryPath ) const;

        // Returns the sub-filename without the extension
        String GetSubFilenameWithoutExtension() const;

        // Conversion
        //-------------------------------------------------------------------------

        inline String const& GetString() const { return HasSubFilename() ? m_subFilenamePath : m_path; }
        inline char const* c_str() const { return GetString().c_str(); }

        // Operators
        //-------------------------------------------------------------------------

        inline bool operator==( DataPath const& rhs ) const { return m_ID == rhs.m_ID; }
        inline bool operator!=( DataPath const& rhs ) const { return m_ID != rhs.m_ID; }

        DataPath& operator=( DataPath&& path );
        DataPath& operator=( DataPath const& path );

        inline bool operator<( DataPath const& RHS ) const { return GetString() < RHS.GetString(); }
        inline bool operator>( DataPath const& RHS ) const { return GetString() > RHS.GetString(); }

    protected:

        void FromString( char const* pPath, CreationOptions opt );
        void CalculateID();

    protected:

        String                  m_path; // The path to the file relative to the data folder
        String                  m_subFilenamePath; // Optional: the full path including the optional subFilename

    private:

        uint64_t                m_ID = 0;
        int16_t                 m_lastPathDelimiterIdx = InvalidIndex;
        int16_t                 m_extensionDelimiterIdx = InvalidIndex;
        int16_t                 m_subFilenameExtensionDelimiterIdx = InvalidIndex;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class TDataFilePath final : public DataPath
    {
        static_assert( std::is_base_of<EE::IDataFile, T>::value, "Invalid specialization for TDataFilePath, only classes derived from IDataFile are allowed." );

    public:

        TDataFilePath() = default;

        TDataFilePath( TDataFilePath<T> const& rhs ) : DataPath( static_cast<DataPath const&>( rhs ) ) {}

        TDataFilePath( DataPath const& rhs )
        {
            if ( rhs.IsValid() )
            {
                ClearSubFilename(); // Sub-filenames are not allowed!!!

                EE_ASSERT( _stricmp( rhs.GetExtension().c_str(), T::GetStaticExtension().c_str() ) == 0 );
                DataPath::operator=( rhs );
            }
        }

        // Info
        //-------------------------------------------------------------------------

        virtual bool IsValid() const override { return !m_path.empty() && ( GetExtension() == T::GetStaticExtension() ); }
    };

    //-------------------------------------------------------------------------

    inline bool SortComparison_DataPath( DataPath const& a, DataPath const& b )
    {
        return a.GetString().comparei( b.GetString() ) < 0;
    }
}

// Support for THashMap
//-------------------------------------------------------------------------

namespace eastl
{
    template <>
    struct hash<EE::DataPath>
    {
        eastl_size_t operator()( EE::DataPath const& ID ) const
        {
            return ID.GetID();
        }
    };
}