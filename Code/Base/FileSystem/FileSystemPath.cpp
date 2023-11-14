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
                return CreateDir( GetParentDirectory() );
            }
        }

        return true;
    }

    void Path::ReplaceParentDirectory( Path const& newParentDirectory )
    {
        EE_ASSERT( IsValid() );

        size_t lastDelimiterIdx = m_fullpath.rfind( Settings::s_pathDelimiter );

        // Handle directory paths
        if ( lastDelimiterIdx == m_fullpath.length() - 1 )
        {
            lastDelimiterIdx = m_fullpath.rfind( Settings::s_pathDelimiter, lastDelimiterIdx - 1 );
        }

        m_fullpath.replace( 0, lastDelimiterIdx + 1, newParentDirectory.ToString() );
        UpdatePathInternals();
    }

    //-------------------------------------------------------------------------

    Path& Path::Append( char const* pPathString, bool asDirectory )
    {
        EE_ASSERT( IsValid() );

        m_fullpath += pPathString;
        m_fullpath = GetFullPathString( m_fullpath.c_str() );

        if ( asDirectory && !IsDirectoryPath() )
        {
            m_fullpath += Settings::s_pathDelimiter;
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
        size_t currentDelimiterIdx = m_fullpath.find( Settings::s_pathDelimiter );
        while ( currentDelimiterIdx != String::npos )
        {
            EE_ASSERT( currentDelimiterIdx > previousDelimiterIdx );
            split.emplace_back( m_fullpath.substr( previousDelimiterIdx, currentDelimiterIdx - previousDelimiterIdx ) );

            previousDelimiterIdx = currentDelimiterIdx + 1;
            currentDelimiterIdx = m_fullpath.find( Settings::s_pathDelimiter, previousDelimiterIdx );
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
            size_t delimiterIdx = m_fullpath.find( Settings::s_pathDelimiter );
            while ( delimiterIdx != String::npos )
            {
                dirDepth++;
                delimiterIdx = m_fullpath.find( Settings::s_pathDelimiter, delimiterIdx + 1 );
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

        size_t lastDelimiterIdx = m_fullpath.rfind( Settings::s_pathDelimiter );

        // Handle directory paths
        if ( lastDelimiterIdx == m_fullpath.length() - 1 )
        {
            lastDelimiterIdx = m_fullpath.rfind( Settings::s_pathDelimiter, lastDelimiterIdx - 1 );
        }

        //-------------------------------------------------------------------------

        Path parentPath;

        // If we found a parent, create the substring for it
        if ( lastDelimiterIdx != String::npos )
        {
            parentPath = Path( m_fullpath.substr( 0, lastDelimiterIdx + 1 ) );
        }

        return parentPath;
    }

    void Path::MakeIntoDirectoryPath()
    {
        if ( !IsDirectoryPath() )
        {
            m_fullpath.push_back( Settings::s_pathDelimiter );
            UpdatePathInternals();
        }
    }

    String Path::GetDirectoryName() const
    {
        EE_ASSERT( IsValid() && IsDirectoryPath() );

        size_t idx = m_fullpath.rfind( Settings::s_pathDelimiter );
        EE_ASSERT( idx != String::npos );

        idx = m_fullpath.rfind( Settings::s_pathDelimiter, Math::Max( size_t( 0 ), idx - 1 ) );
        if ( idx == String::npos )
        {
            return String();
        }

        idx++;
        return m_fullpath.substr( idx, m_fullpath.length() - idx - 1 );
    }

    //-------------------------------------------------------------------------

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

    char const* Path::GetFilenameSubstr() const
    {
        EE_ASSERT( IsValid() && IsFilePath() );
        auto idx = m_fullpath.find_last_of( Settings::s_pathDelimiter );
        EE_ASSERT( idx != String::npos );

        idx++;
        return &m_fullpath[idx];
    }

    String Path::GetFileNameWithoutExtension() const
    {
        EE_ASSERT( IsValid() && IsFilePath() );
        auto filenameStartIdx = m_fullpath.find_last_of( Settings::s_pathDelimiter );
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