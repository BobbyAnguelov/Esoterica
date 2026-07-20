#include "DataPath.h"
#include "Base/Math/Math.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct DataPathValidator
    {
        struct PathInfo
        {
            int16_t                 m_lastPathDelimiterIdx = InvalidIndex;
            int16_t                 m_extensionDelimiterIdx = InvalidIndex;
            int16_t                 m_subFilenameDelimiterIdx = InvalidIndex;
            int16_t                 m_subFilenameExtensionDelimiterIdx = InvalidIndex;
            int16_t                 m_totalInputPathLength = 0;
            int16_t                 m_pathLength = 0;
            int16_t                 m_subFilenameLength = 0;
            bool                    m_isDirectoryPath = false;
        };

        static bool Validate( char const* pPath, PathInfo& outInfo )
        {
            if ( pPath == nullptr )
            {
                return false;
            }

            // Clear path internals
            //-------------------------------------------------------------------------

            outInfo.m_extensionDelimiterIdx = InvalidIndex;
            outInfo.m_lastPathDelimiterIdx = DataPath::s_pathPrefixLength - 1;
            outInfo.m_subFilenameDelimiterIdx = InvalidIndex;
            outInfo.m_subFilenameExtensionDelimiterIdx = InvalidIndex;

            //-------------------------------------------------------------------------

            int16_t charIdx = 0;
            int16_t numSubFilenameDelimiters = 0;
            int16_t numInvalidCharsFound = 0;

            TInlineVector<int16_t, 4> extensionDelimiterIndices;

            while ( pPath[charIdx] != 0 )
            {
                // Ensure that the prefix is found
                if ( charIdx < DataPath::s_pathPrefixLength )
                {
                    if ( tolower( pPath[charIdx] ) != DataPath::s_pathPrefix[charIdx] )
                    {
                        return false;
                    }
                }
                else
                {
                    // Check for delimiters and invalid chars
                    if ( pPath[charIdx] == DataPath::s_subFileDelimiter )
                    {
                        outInfo.m_subFilenameDelimiterIdx = charIdx;
                        numSubFilenameDelimiters++;
                    }
                    else if ( pPath[charIdx] == '\\' )
                    {
                        numInvalidCharsFound++;
                    }
                    else if ( pPath[charIdx] == DataPath::s_pathDelimiter )
                    {
                        if ( outInfo.m_subFilenameDelimiterIdx != InvalidIndex && charIdx > outInfo.m_subFilenameDelimiterIdx )
                        {
                            return false;
                        }

                        outInfo.m_lastPathDelimiterIdx = charIdx;
                    }
                    else if ( pPath[charIdx] == '.' )
                    {
                        extensionDelimiterIndices.emplace_back( charIdx );
                    }
                }

                charIdx++;
            }

            //-------------------------------------------------------------------------

            if ( charIdx == 0 )
            {
                return false;
            }

            if ( numInvalidCharsFound > 0 )
            {
                return false;
            }

            if ( numSubFilenameDelimiters > 1 )
            {
                return false;
            }

            outInfo.m_totalInputPathLength = charIdx;

            //-------------------------------------------------------------------------

            // If this has a sub-filename present
            if ( numSubFilenameDelimiters == 1 )
            {
                // We need at least two extensions
                if ( extensionDelimiterIndices.size() < 2 )
                {
                    return false;
                }

                // We need an extension on either side of the internal object delimiter
                if ( extensionDelimiterIndices.front() > outInfo.m_subFilenameDelimiterIdx )
                {
                    return false;
                }

                outInfo.m_pathLength = outInfo.m_subFilenameDelimiterIdx;
                outInfo.m_subFilenameLength = outInfo.m_totalInputPathLength - outInfo.m_subFilenameDelimiterIdx;
            }
            else
            {
                outInfo.m_pathLength = outInfo.m_totalInputPathLength;
                outInfo.m_subFilenameLength = 0;
            }

            // Get extensions
            //-------------------------------------------------------------------------

            outInfo.m_extensionDelimiterIdx = InvalidIndex;
            outInfo.m_subFilenameExtensionDelimiterIdx = InvalidIndex;

            int32_t const numExtensionDelimiters = (int32_t) extensionDelimiterIndices.size();
            if ( numExtensionDelimiters > 0 )
            {
                if ( outInfo.m_subFilenameDelimiterIdx != InvalidIndex )
                {
                    // Get the last extension between the last path delimiter and the sub-filename delimiter
                    for ( int32_t i = 0; i < numExtensionDelimiters; i++ )
                    {
                        if ( extensionDelimiterIndices[i] > outInfo.m_subFilenameDelimiterIdx )
                        {
                            break;
                        }

                        if ( extensionDelimiterIndices[i] > outInfo.m_lastPathDelimiterIdx )
                        {
                            outInfo.m_extensionDelimiterIdx = extensionDelimiterIndices[i];
                        }
                    }

                    // Get the sub-filename delimiter
                    if ( extensionDelimiterIndices.back() > outInfo.m_subFilenameDelimiterIdx )
                    {
                        outInfo.m_subFilenameExtensionDelimiterIdx = extensionDelimiterIndices.back();
                    }
                }
                else // No sub-filename
                {
                    if ( extensionDelimiterIndices.back() > outInfo.m_lastPathDelimiterIdx )
                    {
                        outInfo.m_extensionDelimiterIdx = extensionDelimiterIndices.back();
                    }
                }
            }

            //-------------------------------------------------------------------------

            outInfo.m_isDirectoryPath = ( outInfo.m_lastPathDelimiterIdx == ( charIdx - 1 ) );

            //-------------------------------------------------------------------------

            return true;
        }
    };

    //-------------------------------------------------------------------------

    bool DataPath::IsValidPath( char const* pPath )
    {
        DataPathValidator::PathInfo info;
        return DataPathValidator::Validate( pPath, info );
    }

    static bool IsValidFilename( char const* pFilenameString )
    {
        if ( pFilenameString == nullptr )
        {
            return false;
        }

        StringView const str( pFilenameString );
        if ( str.empty() )
        {
            return false;
        }

        bool extensionFound = false;
        for ( size_t i = 0; i < str.length(); i++ )
        {
            if ( str[i] == '.' )
            {
                extensionFound = true;
            }

            if ( str[i] == DataPath::s_pathDelimiter || str[i] == FileSystem::Path::s_pathDelimiter )
            {
                return false;
            }
        }

        return extensionFound;
    }

    //-------------------------------------------------------------------------

    DataPath::DataPath( FileSystem::Path const& filePath, FileSystem::Path const& dataDirectoryPath )
    {
        EE_ASSERT( dataDirectoryPath.IsValid() && dataDirectoryPath.IsDirectoryPath() && filePath.IsValid() );
        EE_ASSERT( filePath.IsUnderDirectory( dataDirectoryPath ) );

        InlineString tmpStr = DataPath::s_pathPrefix;
        tmpStr.append( &filePath.GetString()[dataDirectoryPath.Length()] );
        eastl::replace( tmpStr.begin(), tmpStr.end(), FileSystem::Path::s_pathDelimiter, DataPath::s_pathDelimiter );

        if ( filePath.IsDirectoryPath() && tmpStr[tmpStr.length() - 1] != DataPath::s_pathDelimiter )
        {
            tmpStr.push_back( DataPath::s_pathDelimiter );
        }

        FromString( tmpStr.c_str(), CreationOptions::Default );
    }

    DataPath::DataPath( DataPath const& rhs )
        : m_path( rhs.m_path )
        , m_subFilenamePath( rhs.m_subFilenamePath )
        , m_ID( rhs.m_ID )
        , m_lastPathDelimiterIdx( rhs.m_lastPathDelimiterIdx )
        , m_extensionDelimiterIdx( rhs.m_extensionDelimiterIdx )
        , m_subFilenameExtensionDelimiterIdx( rhs.m_subFilenameExtensionDelimiterIdx )
    {}

    DataPath::DataPath( DataPath&& rhs )
        : m_ID( rhs.m_ID )
        , m_lastPathDelimiterIdx( rhs.m_lastPathDelimiterIdx )
        , m_extensionDelimiterIdx( rhs.m_extensionDelimiterIdx )
        , m_subFilenameExtensionDelimiterIdx( rhs.m_subFilenameExtensionDelimiterIdx )
    {
        m_path.swap( rhs.m_path );
        m_subFilenamePath.swap( rhs.m_subFilenamePath );
    }

    DataPath& DataPath::operator=( DataPath const& rhs )
    {
        m_path = rhs.m_path;
        m_subFilenamePath = rhs.m_subFilenamePath;
        m_ID = rhs.m_ID;
        m_extensionDelimiterIdx = rhs.m_extensionDelimiterIdx;
        m_lastPathDelimiterIdx = rhs.m_lastPathDelimiterIdx;
        m_subFilenameExtensionDelimiterIdx = rhs.m_subFilenameExtensionDelimiterIdx;
        return *this;
    }

    DataPath& DataPath::operator=( DataPath&& rhs )
    {
        m_path.swap( rhs.m_path );
        m_subFilenamePath.swap( rhs.m_subFilenamePath );
        m_ID = rhs.m_ID;
        m_extensionDelimiterIdx = rhs.m_extensionDelimiterIdx;
        m_lastPathDelimiterIdx = rhs.m_lastPathDelimiterIdx;
        m_subFilenameExtensionDelimiterIdx = rhs.m_subFilenameExtensionDelimiterIdx;
        return *this;
    }

    //-------------------------------------------------------------------------

    void DataPath::FromString( char const* pPath, CreationOptions opt )
    {
        DataPathValidator::PathInfo info;
        bool const isValidPath = DataPathValidator::Validate( pPath, info );

        if ( isValidPath )
        {
            m_lastPathDelimiterIdx = info.m_lastPathDelimiterIdx;
            m_extensionDelimiterIdx = info.m_extensionDelimiterIdx;
            m_subFilenameExtensionDelimiterIdx = info.m_subFilenameExtensionDelimiterIdx;

            // Set path and sub-filename
            //-------------------------------------------------------------------------

            if ( info.m_subFilenameLength > 0 )
            {
                m_subFilenamePath.resize( info.m_totalInputPathLength );
                memcpy( m_subFilenamePath.data(), pPath, info.m_totalInputPathLength );
                m_subFilenamePath.make_lower();

                m_path = m_subFilenamePath.substr( 0, info.m_subFilenameDelimiterIdx );
            }
            else
            {
                bool const addPathDelimiter = info.m_isDirectoryPath && info.m_lastPathDelimiterIdx != ( info.m_pathLength - 1 );
                size_t const pathLength = info.m_pathLength + ( ( addPathDelimiter ) ? 1 : 0 );

                m_path.resize( pathLength );
                memcpy( m_path.data(), pPath, pathLength );

                if ( addPathDelimiter )
                {
                    m_path[pathLength - 2] = s_pathDelimiter;
                }

                m_path.make_lower();
            }
        }

        if ( opt != AllowInvalidPath )
        {
            EE_ASSERT( isValidPath );
        }

        CalculateID();
    }

    void DataPath::CalculateID()
    {
        m_ID = 0;

        if ( !m_subFilenamePath.empty() )
        {
            m_ID += Hash::GetHash64( m_subFilenamePath.c_str() );
        }
        else
        {
            m_ID += Hash::GetHash64( m_path.c_str() );
        }
    }

    String DataPath::GetFilename() const
    {
        EE_ASSERT( IsValid() && IsFilePath() );
        return m_path.substr( m_lastPathDelimiterIdx + 1 );
    }

    String DataPath::GetFilenameWithoutExtension() const
    {
        EE_ASSERT( IsValid() && IsFilePath() );
        EE_ASSERT( m_extensionDelimiterIdx != InvalidIndex );
        return m_path.substr( m_lastPathDelimiterIdx + 1, m_extensionDelimiterIdx - m_lastPathDelimiterIdx - 1 );
    }

    String DataPath::GetDirectoryName() const
    {
        EE_ASSERT( IsValid() && IsDirectoryPath() );

        size_t idx = m_path.rfind( s_pathDelimiter );
        EE_ASSERT( idx != String::npos );

        idx = m_path.rfind( s_pathDelimiter, Math::Max( size_t( 0 ), idx - 1 ) );
        if ( idx == String::npos )
        {
            return String();
        }

        idx++;
        return m_path.substr( idx, m_path.length() - idx - 1 );
    }

    bool DataPath::HasParentDirectory() const
    {
        EE_ASSERT( IsValid() );

        int32_t numDelimitersFound = 0;

        // Handle directory paths
        size_t lastDelimiterIdx = m_path.rfind( s_pathDelimiter );
        if ( lastDelimiterIdx == m_path.length() - 1 )
        {
            lastDelimiterIdx = m_path.rfind( s_pathDelimiter, lastDelimiterIdx - 1 );
        }

        while ( lastDelimiterIdx != String::npos )
        {
            lastDelimiterIdx = m_path.rfind( s_pathDelimiter, lastDelimiterIdx - 1 );
            numDelimitersFound++;
            if ( numDelimitersFound > 2 )
            {
                return true;
            }
        }

        EE_ASSERT( numDelimitersFound >= 0 );
        return false;
    }

    void DataPath::ReplaceFilename( char const* pFilename, bool alsoClearSubFilename )
    {
        EE_ASSERT( IsFilePath() );
        EE_ASSERT( IsValidFilename( pFilename ) );

        if ( alsoClearSubFilename )
        {
            m_subFilenamePath.clear();
        }

        size_t const pathOriginalLength = m_path.length();
        m_path.resize( m_lastPathDelimiterIdx + 1 ); // Remove old filename
        m_path.append( pFilename );

        if ( !m_subFilenamePath.empty() )
        {
            InlineString subFileStr( &m_subFilenamePath[pathOriginalLength] );
            m_subFilenamePath = m_path + subFileStr.c_str();
        }

        CalculateID();
    }

    void DataPath::AppendFilename( char const* pFilename )
    {
        EE_ASSERT( IsDirectoryPath() );
        EE_ASSERT( IsValidFilename( pFilename ) );

        m_path.append( pFilename );
        CalculateID();
    }

    DataPath DataPath::GetParentDirectory() const
    {
        EE_ASSERT( DataPath::IsValid() );

        if ( m_lastPathDelimiterIdx != InvalidIndex )
        {
            String parentPath;

            // If we found a parent, create the substring for it
            if ( IsDirectoryPath() )
            {
                size_t const parentDelimiterIdx = m_path.rfind( s_pathDelimiter, m_lastPathDelimiterIdx - 1 );
                if ( parentDelimiterIdx != String::npos )
                {
                    parentPath = m_path.substr( 0, parentDelimiterIdx + 1 );
                }
            }
            else if ( m_lastPathDelimiterIdx != InvalidIndex )
            {
                parentPath = m_path.substr( 0, m_lastPathDelimiterIdx + 1 );
            }

            return DataPath( parentPath );
        }
        else
        {
            return DataPath();
        }
    }

    int32_t DataPath::GetDirectoryDepth() const
    {
        int32_t dirDepth = 0;

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

    bool DataPath::IsUnder( DataPath const& parentpath ) const
    {
        if ( m_path.length() < parentpath.m_path.length() )
        {
            return false;
        }

        int32_t const result = String::comparei( m_path.begin(), m_path.begin() + parentpath.m_path.length(), parentpath.m_path.begin(), parentpath.m_path.end() );
        return result == 0;
    }

    FileSystem::Extension DataPath::GetExtension() const
    {
        EE_ASSERT( DataPath::IsValid() && IsFilePath() ); // This is called by the derived class's 'IsValid' so dont call the virtual 'IsValid'

        FileSystem::Extension ext;
        if ( m_extensionDelimiterIdx != InvalidIndex )
        {
            size_t const size = m_path.length() - m_extensionDelimiterIdx - 1;
            ext.resize( size );
            memcpy( ext.data(), &m_path[m_extensionDelimiterIdx + 1], size );
        }

        return ext;
    }

    void DataPath::ReplaceExtension( const char* pExtension )
    {
        EE_ASSERT( pExtension != nullptr );
        EE_ASSERT( pExtension[0] != 0 && pExtension[0] != '.' );
        EE_ASSERT( IsValid() && IsFilePath() );

        size_t const pathOriginalLength = m_path.length();

        FileSystem::Extension ext;
        m_path.erase( m_extensionDelimiterIdx + 1 );
        m_path.append( pExtension );
        m_path.make_lower();

        if ( !m_subFilenamePath.empty() )
        {
            InlineString subFileStr( &m_subFilenamePath[pathOriginalLength] );
            m_subFilenamePath = m_path + subFileStr.c_str();
        }

        CalculateID();
    }

    FileSystem::Path DataPath::GetFileSystemPath( FileSystem::Path const& dataDirectoryPath ) const
    {
        EE_ASSERT( dataDirectoryPath.IsValid() && dataDirectoryPath.IsDirectoryPath() );
        EE_ASSERT( IsValid() );

        //-------------------------------------------------------------------------

        String filePath( dataDirectoryPath );
        filePath.append( m_path.substr( s_pathPrefixLength ) );
        eastl::replace( filePath.begin(), filePath.end(), DataPath::s_pathDelimiter, FileSystem::Path::s_pathDelimiter );
        FileSystem::Path::GetCorrectCaseForPath( filePath.c_str(), filePath );

        return FileSystem::Path( filePath );
    }

    FileSystem::Path DataPath::GetFlattenedFileSystemPath( FileSystem::Path const& dataDirectoryPath ) const
    {
        EE_ASSERT( dataDirectoryPath.IsValid() && dataDirectoryPath.IsDirectoryPath() );
        EE_ASSERT( IsValid() && HasSubFilename() );

        //-------------------------------------------------------------------------

        String filePath( dataDirectoryPath );
        filePath.append( m_path.substr( s_pathPrefixLength, m_extensionDelimiterIdx - s_pathPrefixLength ) );
        filePath.push_back( s_subFileFlattenedPathDelimiter );
        filePath.append( m_subFilenamePath.substr( m_path.length() + 1 ) );

        eastl::replace( filePath.begin(), filePath.end(), DataPath::s_pathDelimiter, FileSystem::Path::s_pathDelimiter );
        FileSystem::Path::GetCorrectCaseForPath( filePath.c_str(), filePath );

        return FileSystem::Path( filePath );
    }

    //-------------------------------------------------------------------------

    void DataPath::SetSubFilename( char const* pFilename )
    {
        EE_ASSERT( pFilename != nullptr );
        EE_ASSERT( IsValid() && IsFilePath() );

        //-------------------------------------------------------------------------

        auto IsValidSubFilename = [] ( char const* pFilename )
        {
            if ( pFilename == nullptr )
            {
                return false;
            }

            int16_t charIdx = 0;
            int16_t numExtensionDelimitersFound = 0;

            while ( pFilename[charIdx] != 0 )
            {
                // Check for delimiters and invalid chars
                if ( pFilename[charIdx] == DataPath::s_subFileDelimiter )
                {
                    return false;
                }

                if ( pFilename[charIdx] == '\\' )
                {
                    return false;
                }

                if ( pFilename[charIdx] == DataPath::s_pathDelimiter )
                {
                    return false;
                }

                if ( pFilename[charIdx] == '.' )
                {
                    numExtensionDelimitersFound++;
                }

                charIdx++;
            }

            //-------------------------------------------------------------------------

            if ( charIdx == 0 )
            {
                return false;
            }

            if ( numExtensionDelimitersFound != 1 )
            {
                return false;
            }

            return true;
        };

        EE_ASSERT( IsValidSubFilename( pFilename ) );

        m_subFilenamePath = m_path;
        m_subFilenamePath.push_back( s_subFileDelimiter );
        m_subFilenamePath.append( pFilename );
        m_subFilenamePath.make_lower();

        size_t const extensionDelimiterIdx = m_subFilenamePath.rfind( '.' );
        EE_ASSERT( extensionDelimiterIdx != String::npos );
        m_subFilenameExtensionDelimiterIdx = (int16_t) extensionDelimiterIdx;

        CalculateID();
    }

    char const* DataPath::GetSubFilename() const
    {
        EE_ASSERT( IsValid() && HasSubFilename() );

        return &m_subFilenamePath[m_path.length() + 1];
    }

    FileSystem::Extension DataPath::GetSubFilenameExtension() const
    {
        EE_ASSERT( IsValid() && HasSubFilename() );
        EE_ASSERT( m_subFilenameExtensionDelimiterIdx != InvalidIndex );

        FileSystem::Extension ext;
        size_t const size = m_subFilenamePath.length() - m_subFilenameExtensionDelimiterIdx - 1;
        ext.resize( size );
        memcpy( ext.data(), &m_subFilenamePath[m_subFilenameExtensionDelimiterIdx + 1], size );

        return ext;
    }

    String DataPath::GetSubFilenameWithoutExtension() const
    {
        EE_ASSERT( IsValid() && HasSubFilename() );
        EE_ASSERT( m_subFilenameExtensionDelimiterIdx != InvalidIndex );

        return m_subFilenamePath.substr( m_path.length() + 1, m_subFilenameExtensionDelimiterIdx - m_path.length() - 1 );
    }
}