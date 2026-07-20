#include "FileSystemUtils.h"
#include <filesystem>
#include <regex>

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    bool GetDirectoryContents( Path const& directoryPath, TVector<Path>& contents, DirectoryReaderOutput output, DirectoryReaderMode mode, TVector<Extension> const& extensionFilter )
    {
        EE_ASSERT( directoryPath.IsDirectoryPath() );

        contents.clear();

        if ( !directoryPath.Exists() )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        uint32_t const numExtensionFilters = (uint32_t) extensionFilter.size();
        TVector<Extension> lowercaseExtensionFilters = extensionFilter;
        for ( auto& extFilter : lowercaseExtensionFilters )
        {
            extFilter.make_lower();
        }

        // Path processing
        //-------------------------------------------------------------------------

        Extension fileLowercaseExtension;

        auto ProcessPath = [&] ( std::filesystem::path const& path )
        {
            if ( !std::filesystem::exists( path ) )
            {
                return;
            }

            if ( std::filesystem::is_directory( path ) )
            {
                if ( output != DirectoryReaderOutput::OnlyFiles )
                {
                    contents.emplace_back( Path( path.string().c_str() ) );
                }
            }
            else if ( std::filesystem::is_regular_file( path ) )
            {
                if ( output == DirectoryReaderOutput::OnlyDirectories )
                {
                    return;
                }

                bool shouldAddFile = ( numExtensionFilters == 0 );

                InlineString pathStr( path.string().c_str() );
                size_t const extStartIdx = FileSystem::Path::FindExtensionStartIdx( pathStr, FileSystem::Path::s_pathDelimiter );
                if ( extStartIdx != InlineString::npos )
                {
                    InlineString extStr = &pathStr[extStartIdx];
                    extStr.make_lower();

                    for ( auto i = 0u; i < numExtensionFilters; i++ )
                    {
                        if ( extStr == lowercaseExtensionFilters[i].c_str() )
                        {
                            shouldAddFile = true;
                            break;
                        }
                    }
                }

                if ( shouldAddFile )
                {
                    contents.emplace_back( Path( path.string().c_str() ) );
                }
            }
            else
            {
                // Do nothing
            }
        };

        // Directory iteration
        //-------------------------------------------------------------------------

        switch ( mode )
        {
            case DirectoryReaderMode::Recursive:
            {
                for ( auto& directoryEntry : std::filesystem::recursive_directory_iterator( directoryPath.c_str() ) )
                {
                    ProcessPath( directoryEntry.path() );
                }
            }
            break;

            case DirectoryReaderMode::NoRecursion:
            {
                for ( auto& directoryEntry : std::filesystem::directory_iterator( directoryPath.c_str() ) )
                {
                    ProcessPath( directoryEntry.path() );
                }
            }
            break;
        }

        //-------------------------------------------------------------------------

        return true;
    }

    bool GetDirectoryContents( Path const& directoryPath, char const* const pRegexExpression, TVector<Path>& contents, DirectoryReaderOutput output, DirectoryReaderMode mode )
    {
        EE_ASSERT( directoryPath.IsDirectoryPath() );
        EE_ASSERT( pRegexExpression != nullptr );

        contents.clear();

        if ( !directoryPath.Exists() )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        std::regex const patternToMatch( pRegexExpression, std::regex::icase );

        // Path processing
        //-------------------------------------------------------------------------

        auto ProcessPath = [&] ( std::filesystem::path const& path )
        {
            if ( std::filesystem::is_directory( path ) )
            {
                if ( output == DirectoryReaderOutput::OnlyFiles )
                {
                    return;
                }

                auto const pathStr = path.string();
                if ( std::regex_match( pathStr, patternToMatch ) )
                {
                    contents.emplace_back( Path( pathStr.c_str() ) );
                }
            }
            else if ( std::filesystem::is_regular_file( path ) )
            {
                if ( output == DirectoryReaderOutput::OnlyDirectories )
                {
                    return;
                }

                auto const pathStr = path.string();
                if ( std::regex_match( pathStr, patternToMatch ) )
                {
                    contents.emplace_back( Path( pathStr.c_str() ) );
                }
            }
            else
            {
                // Do nothing
            }
        };

        // Directory iteration
        //-------------------------------------------------------------------------

        switch ( mode )
        {
            case DirectoryReaderMode::Recursive:
            {
                for ( auto& directoryEntry : std::filesystem::recursive_directory_iterator( directoryPath.c_str() ) )
                {
                    ProcessPath( directoryEntry.path() );
                }
            }
            break;

            case DirectoryReaderMode::NoRecursion:
            {
                for ( auto& directoryEntry : std::filesystem::directory_iterator( directoryPath.c_str() ) )
                {
                    ProcessPath( directoryEntry.path() );
                }
            }
            break;
        }

        return true;
    }
}