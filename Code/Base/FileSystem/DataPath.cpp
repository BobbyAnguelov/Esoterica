#include "DataPath.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct IntraPathUtils
    {
        inline bool IsValid() const { return !m_parentExtension.empty() && m_parentExtension.length() <= 4; }

        bool DecomposePathString( String const& pathStr )
        {
            // Check for path delimiter
            m_lastPathDelimiterIndex = pathStr.rfind( DataPath::s_pathDelimiter );
            if ( m_lastPathDelimiterIndex == String::npos )
            {
                return false;
            }

            // Check for directory path
            if ( m_lastPathDelimiterIndex == pathStr.length() - 1 )
            {
                return false;
            }

            // If we found a parent, create the substring for it and get the extension
            m_parentPath = InlineString( pathStr.c_str(), m_lastPathDelimiterIndex );
            m_childResourceFilename = &pathStr[m_lastPathDelimiterIndex + 1];
            size_t const extIdx = FileSystem::Path::FindExtensionStartIdx( m_parentPath.c_str(), DataPath::s_pathDelimiter );

            // If no extension found, parent is likely a directory
            if ( extIdx == String::npos )
            {
                return false;
            }

            m_parentExtension = InlineString( &m_parentPath[extIdx] );
            return IsValid();
        }

        void GenerateFilePath( String& outPath )
        {
            EE_ASSERT( IsValid() );

            size_t const parentPathPortionLength = m_parentPath.length() - m_parentExtension.length() - 1; // parent length without extension and delimiter
            outPath.resize( parentPathPortionLength + m_childResourceFilename.length() + 1 ); // Add back delimiter and child resource name

            // Copy parent path without extension and null terminator
            memcpy( outPath.data(), m_parentPath.data(), ( parentPathPortionLength ) * sizeof( m_parentPath[0] ) );

            // Set delimiter
            outPath[parentPathPortionLength] = DataPath::s_intraPathFlattenedPathDelimiter;

            // Copy child resource name with null terminator
            memcpy( outPath.data() + parentPathPortionLength + 1, m_childResourceFilename.data(), m_childResourceFilename.length() * sizeof( m_parentPath[0] ) );
        }

    public:

        InlineString    m_parentPath;
        InlineString    m_parentExtension;
        InlineString    m_childResourceFilename;
        size_t          m_lastPathDelimiterIndex = String::npos;
    };

    // Extremely naive data path validation function, this is definitely not robust!
    bool DataPath::IsValidPath( char const* pPath )
    {
        EE_ASSERT( pPath != nullptr );

        if ( strlen( pPath ) == 0 )
        {
            return false;
        }

        if ( StringUtils::CompareInsensitive( DataPath::s_pathPrefix, pPath, s_pathPrefixLength ) != 0 )
        {
            return false;
        }

        if ( strchr( pPath, '\\' ) != nullptr )
        {
            return false;
        }

        return true;
    }

    DataPath DataPath::FromFileSystemPath( FileSystem::Path const& dataDirectoryPath, FileSystem::Path const& filePath )
    {
        EE_ASSERT( dataDirectoryPath.IsValid() && dataDirectoryPath.IsDirectoryPath() && filePath.IsValid() );

        DataPath path;

        if ( filePath.IsUnderDirectory( dataDirectoryPath ) )
        {
            String tempPath = DataPath::s_pathPrefix;
            tempPath.append( filePath.GetString().substr( dataDirectoryPath.Length() ) );
            eastl::replace( tempPath.begin(), tempPath.end(), FileSystem::Path::s_pathDelimiter, DataPath::s_pathDelimiter );
            path = DataPath( tempPath );
        }

        return path;
    }

    FileSystem::Path DataPath::GetFileSystemPath( FileSystem::Path const& dataDirectoryPath ) const
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( dataDirectoryPath.IsValid() && dataDirectoryPath.IsDirectoryPath() );

        //-------------------------------------------------------------------------

        String decomposedResourcePath = m_path;

        // Check if it's a sub-resource path
        if ( IsFilePath() )
        {
            IntraPathUtils ipu;
            if ( ipu.DecomposePathString( m_path ) )
            {
                ipu.GenerateFilePath( decomposedResourcePath );
            }
        }

        // Replace slashes and remove prefix
        String tempPath( dataDirectoryPath );
        tempPath += decomposedResourcePath.substr( 7 );

        // Finalize path
        eastl::replace( tempPath.begin(), tempPath.end(), DataPath::s_pathDelimiter, FileSystem::Path::s_pathDelimiter );
        FileSystem::Path::GetCorrectCaseForPath( tempPath.c_str(), tempPath );

        return FileSystem::Path( tempPath );
    }

    FileSystem::Path DataPath::GetParentFileSystemPath( FileSystem::Path const& dataDirectoryPath ) const
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( dataDirectoryPath.IsValid() && dataDirectoryPath.IsDirectoryPath() );

        String decomposedResourcePath = m_path;

        // Check if it's a sub-resource path
        if ( IsFilePath() )
        {
            // Try to decompose the path, if we fail just return the current path
            IntraPathUtils ipu;
            if ( ipu.DecomposePathString( m_path ) )
            {
                decomposedResourcePath = ipu.m_parentPath;
            }
        }

        // Replace slashes and remove prefix
        String tempPath( dataDirectoryPath );
        tempPath += decomposedResourcePath.substr( 7 );

        // Finalize path
        eastl::replace( tempPath.begin(), tempPath.end(), DataPath::s_pathDelimiter, FileSystem::Path::s_pathDelimiter );
        FileSystem::Path::GetCorrectCaseForPath( tempPath.c_str(), tempPath );

        return FileSystem::Path( tempPath );
    }

    //-------------------------------------------------------------------------

    DataPath::DataPath( String const& path )
        : m_path( path )
    {
        EE_ASSERT( m_path.empty() || IsValidPath( m_path ) );
        OnPathMemberChanged();
    }

    DataPath::DataPath( char const* pPath )
        : m_path( pPath )
    {
        EE_ASSERT( m_path.empty() || IsValidPath( m_path ) );
        OnPathMemberChanged();
    }

    DataPath::DataPath( String&& path )
    {
        m_path.swap( path );
        EE_ASSERT( m_path.empty() || IsValidPath( m_path ) );
        OnPathMemberChanged();
    }

    DataPath::DataPath( DataPath const& path )
        : m_path( path.m_path )
        , m_ID( path.m_ID )
    {
    }

    DataPath::DataPath( DataPath&& path )
        : m_ID( path.m_ID )
    {
        m_path.swap( path.m_path );
    }

    DataPath& DataPath::operator=( DataPath const& path )
    {
        m_path = path.m_path;
        m_ID = path.m_ID;
        return *this;
    }

    DataPath& DataPath::operator=( DataPath&& path )
    {
        m_path.swap( path.m_path );
        m_ID = path.m_ID;
        return *this;
    }

    //-------------------------------------------------------------------------

    void DataPath::OnPathMemberChanged()
    {
        if ( IsValidPath( m_path ) )
        {
            m_path.make_lower();
            m_ID = Hash::GetHash32( m_path.c_str() );
        }
        else
        {
            m_path.clear();
            m_ID = 0;
        }
    }

    bool DataPath::TrySetFromString( String const& path )
    {
        if ( IsValidPath( path ) )
        {
            m_path = path;
            OnPathMemberChanged();
            return true;
        }

        Clear();
        return false;
    }

    String DataPath::GetFilename() const
    {
        EE_ASSERT( DataPath::IsValid() && IsFilePath() );

        auto filenameStartIdx = m_path.find_last_of( s_pathDelimiter );
        EE_ASSERT( filenameStartIdx != String::npos );
        filenameStartIdx++;

        return m_path.substr( filenameStartIdx, m_path.length() - filenameStartIdx );
    }

    String DataPath::GetFilenameWithoutExtension() const
    {
        EE_ASSERT( DataPath::IsValid() && IsFilePath() );

        auto filenameStartIdx = m_path.find_last_of( s_pathDelimiter );
        EE_ASSERT( filenameStartIdx != String::npos );
        filenameStartIdx++;

        //-------------------------------------------------------------------------

        size_t extStartIdx = FileSystem::Path::FindExtensionStartIdx( m_path, DataPath::s_pathDelimiter );
        if ( extStartIdx != String::npos )
        {
            return m_path.substr( filenameStartIdx, extStartIdx - filenameStartIdx - 1 );
        }
        else
        {
            return String( &m_path[filenameStartIdx] );
        }
    }

    DataPath DataPath::GetParentDirectory() const
    {
        EE_ASSERT( DataPath::IsValid() );

        size_t lastDelimiterIdx = m_path.rfind( s_pathDelimiter );

        // Handle directory paths
        if ( lastDelimiterIdx == m_path.length() - 1 )
        {
            lastDelimiterIdx = m_path.rfind( s_pathDelimiter, lastDelimiterIdx - 1 );
        }

        //-------------------------------------------------------------------------

        String parentPath;

        // If we found a parent, create the substring for it
        if ( lastDelimiterIdx != String::npos )
        {
            parentPath = m_path.substr( 0, lastDelimiterIdx + 1 );
        }

        return DataPath( parentPath );
    }

    int32_t DataPath::GetDirectoryDepth() const
    {
        int32_t dirDepth = -1;

        if ( IsValid() )
        {
            size_t delimiterIdx = m_path.find( s_pathDelimiter, s_pathPrefixLength );
            while ( delimiterIdx != String::npos )
            {
                dirDepth++;
                delimiterIdx = m_path.find( s_pathDelimiter, delimiterIdx + 1 );
            }
        }

        return dirDepth;
    }

    int32_t DataPath::GetPathDepth() const
    {
        int32_t pathDepth = GetDirectoryDepth();

        if ( IsFilePath() )
        {
            pathDepth++;
        }

        return pathDepth;
    }

    bool DataPath::IsIntraFilePath( DataPath* pParentPath ) const
    {
        EE_ASSERT( DataPath::IsValid() );
        IntraPathUtils ipu;

        if ( ipu.DecomposePathString( m_path ) )
        {
            if ( pParentPath != nullptr )
            {
                *pParentPath = DataPath( ipu.m_parentPath.c_str() );
            }
            return true;
        }
        else
        {
            if ( pParentPath != nullptr )
            {
                pParentPath->Clear();
            }
        }

        return false;
    }

    DataPath DataPath::GetIntraFilePathParent() const
    {
        EE_ASSERT( DataPath::IsValid() );

        DataPath parentDataPath;

        IntraPathUtils ipu;
        if ( ipu.DecomposePathString( m_path ) )
        {
            parentDataPath = DataPath( ipu.m_parentPath.c_str() );
        }
        else
        {
            parentDataPath = *this;
        }

        return parentDataPath;
    }

    char const* DataPath::GetExtension() const
    {
        EE_ASSERT( DataPath::IsValid() && IsFilePath() );

        size_t const extIdx = FileSystem::Path::FindExtensionStartIdx( m_path.c_str(), DataPath::s_pathDelimiter );
        if ( extIdx != String::npos )
        {
            return &m_path.c_str()[extIdx];
        }
        else
        {
            return nullptr;
        }
    }

    void DataPath::ReplaceExtension( const char* pExtension )
    {
        EE_ASSERT( DataPath::IsValid() && IsFilePath() && pExtension != nullptr );
        EE_ASSERT( pExtension[0] != 0 && pExtension[0] != '.' );

        size_t const extIdx = FileSystem::Path::FindExtensionStartIdx( m_path.c_str(), DataPath::s_pathDelimiter );
        if ( extIdx != String::npos )
        {
            m_path = m_path.substr( 0, extIdx ) + pExtension;
        }
        else // No extension, so just append
        {
            m_path.append( "." );
            m_path.append( pExtension );
        }

        OnPathMemberChanged();
    }
}