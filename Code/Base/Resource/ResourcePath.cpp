#include "ResourcePath.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct SubResourcePathUtils
    {
        inline bool IsValid() const { return !m_parentExtension.empty() && m_parentExtension.length() <= 4; }

        bool DecomposePathString( String const& pathStr )
        {
            // Check for path delimiter
            m_lastPathDelimiterIndex = pathStr.rfind( ResourcePath::s_pathDelimiter );
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
            size_t const extIdx = FileSystem::FindExtensionStartIdx( m_parentPath.c_str(), ResourcePath::s_pathDelimiter );

            // If no extension found, parent is likely a directory
            if ( extIdx == String::npos )
            {
                return false;
            }

            // Ensure extension is less than 4 characters since resource types use a 4CC type ID
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
            outPath[parentPathPortionLength] = ResourcePath::s_subResourceFilePathDelimiter;

            // Copy child resource name with null terminator
            memcpy( outPath.data() + parentPathPortionLength + 1, m_childResourceFilename.data(), m_childResourceFilename.length() * sizeof( m_parentPath[0] ) );
        }

    public:

        InlineString    m_parentPath;
        InlineString    m_parentExtension;
        InlineString    m_childResourceFilename;
        size_t          m_lastPathDelimiterIndex = String::npos;
    };

    //-------------------------------------------------------------------------

    // Extremely naive data path validation function, this is definitely not robust!
    bool ResourcePath::IsValidPath( char const* pPath )
    {
        EE_ASSERT( pPath != nullptr );

        if ( strlen( pPath ) == 0 )
        {
            return false;
        }

        if ( StringUtils::CompareInsensitive( ResourcePath::s_pathPrefix, pPath, s_pathPrefixLength ) != 0 )
        {
            return false;
        }

        if ( strchr( pPath, '\\' ) != nullptr )
        {
            return false;
        }

        return true;
    }

    ResourcePath ResourcePath::FromFileSystemPath( FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& filePath )
    {
        EE_ASSERT( rawResourceDirectoryPath.IsValid() && rawResourceDirectoryPath.IsDirectoryPath() && filePath.IsValid() );

        ResourcePath path;

        if( filePath.IsUnderDirectory( rawResourceDirectoryPath ) )
        {
            String tempPath = ResourcePath::s_pathPrefix;
            tempPath.append( filePath.GetString().substr( rawResourceDirectoryPath.Length() ) );
            eastl::replace( tempPath.begin(), tempPath.end(), FileSystem::Settings::s_pathDelimiter, ResourcePath::s_pathDelimiter );
            path = ResourcePath( tempPath );
        }

        return path;
    }

    FileSystem::Path ResourcePath::ToFileSystemPath( FileSystem::Path const& rawResourceDirectoryPath, ResourcePath const& resourcePath )
    {
        EE_ASSERT( rawResourceDirectoryPath.IsValid() && rawResourceDirectoryPath.IsDirectoryPath() && resourcePath.IsValid() );

        //-------------------------------------------------------------------------

        // Replace slashes and remove prefix
        String tempPath( rawResourceDirectoryPath );
        tempPath += resourcePath.m_path.substr( 7 );

        // Check if it's a sub-resource path
        if ( resourcePath.IsFile() )
        {
            SubResourcePathUtils spu;
            if ( spu.DecomposePathString( tempPath ) )
            {
                spu.GenerateFilePath( tempPath );
            }
        }

        // Finalize path
        eastl::replace( tempPath.begin(), tempPath.end(), ResourcePath::s_pathDelimiter, FileSystem::Settings::s_pathDelimiter );
        FileSystem::GetCorrectCaseForPath( tempPath.c_str(), tempPath );

        return FileSystem::Path( tempPath );
    }

    //-------------------------------------------------------------------------

    ResourcePath::ResourcePath( String const& path )
        : m_path( path )
    {
        EE_ASSERT( m_path.empty() || IsValidPath( m_path ) );
        OnPathMemberChanged();
    }

    ResourcePath::ResourcePath( char const* pPath )
        : m_path( pPath )
    {
        EE_ASSERT( m_path.empty() || IsValidPath( m_path ) );
        OnPathMemberChanged();
    }

    ResourcePath::ResourcePath( String&& path )
    {
        m_path.swap( path );
        EE_ASSERT( m_path.empty() || IsValidPath( m_path ) );
        OnPathMemberChanged();
    }

    ResourcePath::ResourcePath( ResourcePath const& path )
        : m_path( path.m_path )
        , m_ID( path.m_ID )
    {}

    ResourcePath::ResourcePath( ResourcePath&& path )
        : m_ID( path.m_ID )
    {
        m_path.swap( path.m_path );
    }

    ResourcePath& ResourcePath::operator=( ResourcePath const& path )
    {
        m_path = path.m_path;
        m_ID = path.m_ID;
        return *this;
    }

    ResourcePath& ResourcePath::operator=( ResourcePath&& path )
    {
        m_path.swap( path.m_path );
        m_ID = path.m_ID;
        return *this;
    }

    //-------------------------------------------------------------------------

    void ResourcePath::OnPathMemberChanged()
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

    String ResourcePath::GetFileName() const
    {
        EE_ASSERT( IsValid() && IsFile() );

        auto filenameStartIdx = m_path.find_last_of( s_pathDelimiter );
        EE_ASSERT( filenameStartIdx != String::npos );
        filenameStartIdx++;

        return m_path.substr( filenameStartIdx, m_path.length() - filenameStartIdx );
    }

    String ResourcePath::GetFileNameWithoutExtension() const
    {
        EE_ASSERT( IsValid() && IsFile() );

        auto filenameStartIdx = m_path.find_last_of( s_pathDelimiter );
        EE_ASSERT( filenameStartIdx != String::npos );
        filenameStartIdx++;

        //-------------------------------------------------------------------------

        size_t extStartIdx = FileSystem::FindExtensionStartIdx( m_path, ResourcePath::s_pathDelimiter );
        if ( extStartIdx != String::npos )
        {
            return m_path.substr( filenameStartIdx, extStartIdx - filenameStartIdx - 1 );
        }
        else
        {
            return String( &m_path[filenameStartIdx] );
        }
    }

    String ResourcePath::GetParentDirectory() const
    {
        EE_ASSERT( IsValid() );

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

        return parentPath;
    }

    int32_t ResourcePath::GetDirectoryDepth() const
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

    int32_t ResourcePath::GetPathDepth() const
    {
        int32_t pathDepth = GetDirectoryDepth();

        if ( IsFile() )
        {
            pathDepth++;
        }

        return pathDepth;
    }

    bool ResourcePath::IsSubResourcePath() const
    {
        EE_ASSERT( IsValid() );
        SubResourcePathUtils spu;
        return spu.DecomposePathString( m_path );
    }

    ResourcePath ResourcePath::GetParentResourcePath() const
    {
        EE_ASSERT( IsValid() );

        ResourcePath parentResourcePath;
        SubResourcePathUtils spu;
        if ( spu.DecomposePathString( m_path ) )
        {
            parentResourcePath = ResourcePath( spu.m_parentPath.c_str() );
        }

        return parentResourcePath;
    }

    char const* ResourcePath::GetExtension() const
    {
        EE_ASSERT( IsValid() && IsFile() );

        size_t const extIdx = FileSystem::FindExtensionStartIdx( m_path.c_str(), ResourcePath::s_pathDelimiter );
        if ( extIdx != String::npos )
        {
            return &m_path.c_str()[extIdx];
        }
        else
        {
            return nullptr;
        }
    }

    void ResourcePath::ReplaceExtension( const char* pExtension )
    {
        EE_ASSERT( IsValid() && IsFile() && pExtension != nullptr );
        EE_ASSERT( pExtension[0] != 0 && pExtension[0] != '.' );

        size_t const extIdx = FileSystem::FindExtensionStartIdx( m_path.c_str(), ResourcePath::s_pathDelimiter );
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