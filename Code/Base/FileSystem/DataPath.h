#pragma once

#include "IDataFile.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------
// Data Path
//-------------------------------------------------------------------------
// Always relative to the data directory (data://)
// NB!!!    Data paths are case-insensitive.
//          It is up to users on case sensitive file systems to ensure that there wont be two files that a data path can resolve to
//-------------------------------------------------------------------------
//
// Data paths support the concept of intra file paths. i.e. data://parentFile.pr/internalID.ch
// This allows us to directly specify internal objects without having to create explicit file
// 
// When converting to file system paths, the intra-file paths will can converted in two ways:
// 
// A flattened version: data://parentfile.pr/childItem.ch -> C:\data\parentfile_childItem.ch
// A path to the parent file: data://parentfile.pr/childItem.ch -> C:\data\parentfile.pr
//
// NB!!!    Data path do not support folders with the extension delimiter '.' in them, these will be considered intra file paths!
// 
//-------------------------------------------------------------------------

namespace EE
{
    class EE_BASE_API DataPath
    {
        EE_CUSTOM_SERIALIZE_WRITE_FUNCTION( archive )
        {
            archive << m_path;
            return archive;
        }

        EE_CUSTOM_SERIALIZE_READ_FUNCTION( archive )
        {
            archive << m_path;
            OnPathMemberChanged();
            return archive;
        }

    public:

        constexpr static char const* s_pathPrefix = "data://";
        constexpr static int32_t const s_pathPrefixLength = 7;

        // The delimiter used to separate data path elements
        constexpr static char const s_pathDelimiter = '/';

        // The delimiter used between the parent resource name and the child name
        constexpr static char const s_intraPathFlattenedPathDelimiter = '_';

        static bool IsValidPath( char const* pPath );
        static bool IsValidPath( String const& path ) { return IsValidPath( path.c_str() ); }

        static DataPath FromFileSystemPath( FileSystem::Path const& dataDirectory, FileSystem::Path const& filePath );

    public:

        DataPath() = default;
        virtual ~DataPath() = default;

        DataPath( DataPath&& path );
        DataPath( DataPath const& path );
        explicit DataPath( String const& path );
        explicit DataPath( char const* pPath );
        explicit DataPath( String&& path );

        //-------------------------------------------------------------------------

        virtual bool IsValid() const { return !m_path.empty() && IsValidPath( m_path ); }
        inline void Clear() { m_path.clear(); m_ID = 0; }
        inline uint32_t GetID() const { return m_ID; }

        // Path info
        //-------------------------------------------------------------------------

        // Returns the filename
        String GetFilename() const;

        // Returns the filename with all the 'extensions' removed (i.e. file.final.png -> file )
        String GetFilenameWithoutExtension() const;

        // Get the containing directory path for this path
        DataPath GetParentDirectory() const;

        // Get the directory depth for this path e.g. D:\Foo\Bar\Moo.txt = 2
        int32_t GetDirectoryDepth() const;

        // Get the full path depth for this path e.g. D:\Foo\Bar\Moo.txt = 3
        int32_t GetPathDepth() const;

        // Is this a file path
        inline bool IsFilePath() const { EE_ASSERT( DataPath::IsValid() ); return m_path.back() != s_pathDelimiter; }

        // Is this a directory path
        inline bool IsDirectoryPath() const { EE_ASSERT( DataPath::IsValid() ); return ( m_path.back() == s_pathDelimiter ); }

        // Intra-file Paths
        //-------------------------------------------------------------------------

        // Is this an intra-file path? i.e. data://file.f/item.i
        // Optionally returns the parent path for valid intra-file paths
        bool IsIntraFilePath( DataPath* pParentPath = nullptr ) const;

        // Get the parent file for this intra-file path e.g. for "data://file.f/item.i" this will return "data//file.f"
        // If this is not a intra-file path, this will return itself
        DataPath GetIntraFilePathParent() const;

        // Extension
        //-------------------------------------------------------------------------

        // Returns the extension for this path (excluding the '.'). Returns an empty string if there is no extension!
        char const* GetExtension() const;

        // Returns the extension for this path (excluding the '.'). Returns an empty string if there is no extension!
        inline FileSystem::Extension GetExtensionAsString() const
        {
            char const* const pExtensionSubstr = GetExtension();
            return FileSystem::Extension( pExtensionSubstr == nullptr ? "" : pExtensionSubstr );
        }

        // Returns a lowercase version of the extension (excluding the '.') if one exists else returns an empty string
        inline FileSystem::Extension GetLowercaseExtensionAsString() const
        {
            char const* const pExtensionSubstr = GetExtension();
            FileSystem::Extension ext( pExtensionSubstr == nullptr ? "" : pExtensionSubstr );
            ext.make_lower();
            return ext;
        }

        // Replaces the extension (excluding the '.') for this path (will create an extensions if no extension exists)
        void ReplaceExtension( const char* pExtension );

        // Replaces the extension (excluding the '.') for this path (will create an extensions if no extension exists)
        inline void ReplaceExtension( String const& extension ) { ReplaceExtension( extension.c_str() ); }

        // Replaces the extension (excluding the '.') for this path (will create an extensions if no extension exists)
        template<size_t S> void ReplaceExtension( TInlineString<S> const& extension ) { ReplaceExtension( extension.c_str() ); }

        // Conversion
        //-------------------------------------------------------------------------

        inline String const& GetString() const { return m_path; }

        inline char const* c_str() const { return m_path.c_str(); }

        // By default this will generate the flattened path for intra-file paths, if this is not a intra-file path, this will return the file-system path to itself
        FileSystem::Path GetFileSystemPath( FileSystem::Path const& dataDirectory ) const;

        // Get the file system path for the parent file for an intra-file path, if this is not a intra-file path, this will return the file-system path to itself
        FileSystem::Path GetParentFileSystemPath( FileSystem::Path const& dataDirectory ) const;

        // More lenient conversion function that will not assert on invalid data - useful for serialization/conversions
        virtual bool TrySetFromString( String const& path );

        // Operators
        //-------------------------------------------------------------------------

        inline bool operator==( DataPath const& rhs ) const { return m_ID == rhs.m_ID; }
        inline bool operator!=( DataPath const& rhs ) const { return m_ID != rhs.m_ID; }

        DataPath& operator=( DataPath&& path );
        DataPath& operator=( DataPath const& path );

    protected:

        void OnPathMemberChanged();

    protected:

        String                  m_path;

    private:

        uint32_t                m_ID = 0;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class TDataFilePath final : public DataPath
    {
        static_assert( std::is_base_of<EE::IDataFile, T>::value, "Invalid specialization for TDataFilePath, only classes derived from IDataFile are allowed." );

        EE_CUSTOM_SERIALIZE_WRITE_FUNCTION( archive )
        {
            archive << m_path;
            return archive;
        }

        EE_CUSTOM_SERIALIZE_READ_FUNCTION( archive )
        {
            archive << m_path;
            OnPathMemberChanged();
            return archive;
        }

    public:

        TDataFilePath() = default;

        TDataFilePath( TDataFilePath<T> const& rhs ) : DataPath( static_cast<DataPath const&>( rhs ) ) {}

        TDataFilePath( DataPath const& path )
        {
            if ( path.IsValid() )
            {
                EE_ASSERT( _stricmp( path.GetExtensionAsString().c_str(), T::GetStaticExtension().c_str() ) == 0 );
                m_path = path.GetString();
                OnPathMemberChanged();
            }
        }

        // Info
        //-------------------------------------------------------------------------

        virtual bool IsValid() const override { return DataPath::IsValid() && _stricmp( GetExtensionAsString().c_str(), T::GetStaticExtension().c_str() ); }

        inline FileSystem::Extension GetExtension() const { return T::GetStaticExtension; }

        // Read/Write operations
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        inline bool TryRead( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& dataDirectoryPath, T& outData ) const
        {
            return IDataFile::TryReadFromFile( typeRegistry, GetFileSystemPath( dataDirectoryPath ), outData );
        }

        inline bool TryWrite( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& dataDirectoryPath, T const* pDataFile ) const
        {
            return IDataFile::TryWriteToFile( typeRegistry, GetFileSystemPath( dataDirectoryPath ), pDataFile );
        }
        #endif

        // Conversion
        //-------------------------------------------------------------------------

        virtual bool TrySetFromString( String const& path ) override
        {
            if ( IsValidPath( path ) )
            {
                m_path = path;
                OnPathMemberChanged();

                // Check extension
                if ( IsValid() )
                {
                    return true;
                }
            }

            Clear();
            return false;
        }
    };
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
