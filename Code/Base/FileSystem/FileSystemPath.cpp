#include "FileSystemPath.h"
#include "FileSystem.h"
#include "Base/Math/Math.h"
#include "Base/Encoding/Hash.h"

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    static bool IsValidPath( char const* pPathString )
    {
        // TODO: path validation
        return pPathString != nullptr && strlen( pPathString ) > 0;
    }

    //-------------------------------------------------------------------------

    size_t Path::FindExtensionStartIdx( String const& path, char const pathDelimiter, bool supportMultiExtensions )
    {
        // Try to find the last period in the path
        //-------------------------------------------------------------------------

        size_t extensionIdx = path.rfind( '.' );
        if ( extensionIdx == String::npos )
        {
            return extensionIdx;
        }

        // If we have the path delimiter and it is after our last period, then there's no extension
        //-------------------------------------------------------------------------

        size_t const pathDelimiterIdx = path.find_last_of( pathDelimiter );
        if ( pathDelimiterIdx > extensionIdx )
        {
            extensionIdx = String::npos;
            return extensionIdx;
        }

        // Try to handle multi-extensions like ".tar.gzip"
        //-------------------------------------------------------------------------

        size_t prevIdx = extensionIdx;

        if ( supportMultiExtensions )
        {
            while ( extensionIdx != String::npos && extensionIdx > pathDelimiterIdx )
            {
                prevIdx = extensionIdx;
                extensionIdx = path.rfind( '.', extensionIdx - 1 );
            }

            EE_ASSERT( prevIdx != String::npos );
        }

        //-------------------------------------------------------------------------

        prevIdx++;
        extensionIdx = prevIdx;
        return extensionIdx;
    }

    bool Path::GetParentDirectory( String const& path, String& outParentPath )
    {
        size_t lastDelimiterIdx = path.rfind( Path::s_pathDelimiter );

        // Handle directory paths
        if ( lastDelimiterIdx == path.length() - 1 )
        {
            lastDelimiterIdx = path.rfind( Path::s_pathDelimiter, lastDelimiterIdx - 1 );
        }

        //-------------------------------------------------------------------------

        outParentPath.clear();

        // If we found a parent, create the substring for it
        if ( lastDelimiterIdx != String::npos )
        {
            outParentPath = Path( path.substr( 0, lastDelimiterIdx + 1 ) );
        }

        return !outParentPath.empty();
    }

    //-------------------------------------------------------------------------

    Path::Path( char const* pPath )
    {
        if ( IsValidPath( pPath ) )
        {
            if ( !GetFullPathString( pPath, m_fullpath ) )
            {
                m_fullpath.clear();
            }
        }

        UpdatePathInternals();
    }

    Path& Path::operator=( Path& rhs )
    {
        m_fullpath = rhs.m_fullpath;
        m_hashCode = rhs.m_hashCode;
        m_isDirectoryPath = rhs.m_isDirectoryPath;
        return *this;
    }

    Path& Path::operator=( Path const& rhs )
    {
        m_fullpath = rhs.m_fullpath;
        m_hashCode = rhs.m_hashCode;
        m_isDirectoryPath = rhs.m_isDirectoryPath;
        return *this;
    }

    Path& Path::operator=( Path&& rhs )
    {
        m_fullpath.swap( rhs.m_fullpath );
        m_hashCode = rhs.m_hashCode;
        m_isDirectoryPath = rhs.m_isDirectoryPath;
        return *this;
    }

    //-------------------------------------------------------------------------

    bool Path::Exists() const
    {
        EE_ASSERT( IsValid() );
        return m_isDirectoryPath ? IsExistingDirectory( m_fullpath.c_str() ) : IsExistingFile( m_fullpath.c_str() );
    }

    bool Path::IsReadOnly() const
    {
        EE_ASSERT( IsValid() );
        return FileSystem::IsReadOnly( m_fullpath.c_str() );
    }

    bool Path::IsUnderDirectory( Path const& parentDirectory ) const
    {
        if ( m_fullpath.length() < parentDirectory.m_fullpath.length() )
        {
            return false;
        }

        int32_t const result = String::comparei( m_fullpath.begin(), m_fullpath.begin() + parentDirectory.m_fullpath.length(), parentDirectory.m_fullpath.begin(), parentDirectory.m_fullpath.end() );
        return result == 0;
    }

    bool Path::EnsureDirectoryExists() const
    {
        if ( !Exists() )
        {
            if ( IsDirectoryPath() )
            {
                return CreateDir( m_fullpath );
            }
            else
            {
                Path const parentDirectory = GetParentDirectory();
                return CreateDir( parentDirectory );
            }
        }

        return true;
    }

    void Path::ReplaceParentDirectory( Path const& newParentDirectory )
    {
        EE_ASSERT( IsValid() );

        size_t lastDelimiterIdx = m_fullpath.rfind( s_pathDelimiter );

        // Handle directory paths
        if ( lastDelimiterIdx == m_fullpath.length() - 1 )
        {
            lastDelimiterIdx = m_fullpath.rfind( s_pathDelimiter, lastDelimiterIdx - 1 );
        }

        m_fullpath.replace( 0, lastDelimiterIdx + 1, newParentDirectory.ToString() );
        UpdatePathInternals();
    }

    //-------------------------------------------------------------------------

    Path& Path::Append( char const* pPathString, bool asDirectory )
    {
        EE_ASSERT( IsValid() );

        m_fullpath += pPathString;
        bool const result = GetFullPathString( m_fullpath.c_str(), m_fullpath );
        EE_ASSERT( result );
        m_isDirectoryPath = m_fullpath[m_fullpath.length() - 1] == s_pathDelimiter;

        if ( asDirectory && !IsDirectoryPath() )
        {
            m_fullpath += s_pathDelimiter;
        }

        if ( !IsValidPath( m_fullpath.c_str() ) )
        {
            m_fullpath.clear();
        }

        UpdatePathInternals();
        return *this;
    }

    TInlineVector<String, 10> Path::Split() const
    {
        TInlineVector<String, 10> split;

        size_t previousDelimiterIdx = 0;
        size_t currentDelimiterIdx = m_fullpath.find( s_pathDelimiter );
        while ( currentDelimiterIdx != String::npos )
        {
            EE_ASSERT( currentDelimiterIdx > previousDelimiterIdx );
            split.emplace_back( m_fullpath.substr( previousDelimiterIdx, currentDelimiterIdx - previousDelimiterIdx ) );

            previousDelimiterIdx = currentDelimiterIdx + 1;
            currentDelimiterIdx = m_fullpath.find( s_pathDelimiter, previousDelimiterIdx );
        }

        return split;
    }

    TInlineVector<Path, 10> Path::GetDirectoryHierarchy() const
    {
        TInlineVector<Path, 10> directoryHierarchy;

        // Get all previous parent paths
        directoryHierarchy.emplace_back( GetParentDirectory() );
        while ( directoryHierarchy.back().IsValid() )
        {
            directoryHierarchy.emplace_back( directoryHierarchy.back().GetParentDirectory() );
        }

        // Remove lats invalid path
        if ( !directoryHierarchy.empty() )
        {
            directoryHierarchy.pop_back();
        }

        return directoryHierarchy;
    }

    int32_t Path::GetDirectoryDepth() const
    {
        int32_t dirDepth = -1;

        if ( IsValid() )
        {
            size_t delimiterIdx = m_fullpath.find( s_pathDelimiter );
            while ( delimiterIdx != String::npos )
            {
                dirDepth++;
                delimiterIdx = m_fullpath.find( s_pathDelimiter, delimiterIdx + 1 );
            }
        }

        return dirDepth;
    }

    int32_t Path::GetPathDepth() const
    {
        int32_t pathDepth = GetDirectoryDepth();

        if ( IsFilePath() )
        {
            pathDepth++;
        }

        return pathDepth;
    }

    //-------------------------------------------------------------------------

    bool Path::IsFilenameEqual( char const* pString ) const
    {
        EE_ASSERT( pString != nullptr );
        return strcmp( GetFilenameSubstr(), pString ) == 0;
    }

    //-------------------------------------------------------------------------

    Path Path::GetParentDirectory() const
    {
        EE_ASSERT( IsValid() );
        String path;
        bool result = GetParentDirectory( m_fullpath, path );
        EE_ASSERT( result );
        return Path( eastl::move( path ) );
    }

    void Path::MakeIntoDirectoryPath()
    {
        if ( !IsDirectoryPath() )
        {
            m_fullpath.push_back( s_pathDelimiter );
            UpdatePathInternals();
        }
    }

    String Path::GetDirectoryName() const
    {
        EE_ASSERT( IsValid() && IsDirectoryPath() );

        size_t idx = m_fullpath.rfind( s_pathDelimiter );
        EE_ASSERT( idx != String::npos );

        idx = m_fullpath.rfind( s_pathDelimiter, Math::Max( size_t( 0 ), idx - 1 ) );
        if ( idx == String::npos )
        {
            return String();
        }

        idx++;
        return m_fullpath.substr( idx, m_fullpath.length() - idx - 1 );
    }

    //-------------------------------------------------------------------------

    void Path::AppendExtension( char const* pAdditionalExtension )
    {
        EE_ASSERT( IsValid() && IsFilePath() && pAdditionalExtension != nullptr );
        m_fullpath.append( "." );
        m_fullpath.append( pAdditionalExtension );
    }

    bool Path::HasExtension() const
    {
        return FindExtensionStartIdx( m_fullpath ) != String::npos;
    }

    bool Path::MatchesExtension( char const* inExtension ) const
    {
        size_t const extensionIdx = FindExtensionStartIdx( m_fullpath );
        if ( extensionIdx == String::npos )
        {
            return false;
        }

        return _stricmp( inExtension, &m_fullpath.at( extensionIdx ) ) == 0;
    }

    char const* Path::GetExtension() const
    {
        EE_ASSERT( IsValid() && IsFilePath() );
        size_t const extIdx = FindExtensionStartIdx( m_fullpath );
        if ( extIdx != String::npos )
        {
            return &m_fullpath[extIdx];
        }
        else
        {
            return nullptr;
        }
    }

    void Path::ReplaceExtension( const char* pExtension )
    {
        EE_ASSERT( IsValid() && IsFilePath() && pExtension != nullptr );
        EE_ASSERT( pExtension[0] != 0 && pExtension[0] != '.' );

        size_t const extIdx = FindExtensionStartIdx( m_fullpath );
        if ( extIdx != String::npos )
        {
            m_fullpath = m_fullpath.substr( 0, extIdx ) + pExtension;
        }
        else // No extension so just append
        {
            m_fullpath.append( "." );
            m_fullpath.append( pExtension );
        }

        UpdatePathInternals();
    }

    void Path::UpdatePathInternals()
    {
        if ( m_fullpath.empty() )
        {
            m_hashCode = 0;
            m_isDirectoryPath = false;
        }
        else
        {
            m_hashCode = Hash::GetHash32( m_fullpath );
            m_isDirectoryPath = m_fullpath[m_fullpath.length() - 1] == s_pathDelimiter;
        }
    }

    char const* Path::GetFilenameSubstr() const
    {
        EE_ASSERT( IsValid() && IsFilePath() );
        auto idx = m_fullpath.find_last_of( s_pathDelimiter );
        EE_ASSERT( idx != String::npos );

        idx++;
        return &m_fullpath[idx];
    }

    String Path::GetFilenameWithoutExtension() const
    {
        EE_ASSERT( IsValid() && IsFilePath() );
        auto filenameStartIdx = m_fullpath.find_last_of( s_pathDelimiter );
        EE_ASSERT( filenameStartIdx != String::npos );
        filenameStartIdx++;

        //-------------------------------------------------------------------------

        size_t extStartIdx = FindExtensionStartIdx( m_fullpath );
        if ( extStartIdx != String::npos )
        {
            return m_fullpath.substr( filenameStartIdx, extStartIdx - filenameStartIdx - 1 );
        }
        else
        {
            return String( &m_fullpath[filenameStartIdx] );
        }
    }
}