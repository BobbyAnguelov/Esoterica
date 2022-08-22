#include "FileSystemUtils.h"
#include <filesystem>
#include <regex>

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    bool GetDirectoryContents( Path const& directoryPath, TVector<Path>& contents, DirectoryReaderOutput output, DirectoryReaderMode mode, TVector<String> const& extensionFilter )
    {
        EE_ASSERT( directoryPath.IsDirectoryPath() );

        contents.clear();

        if ( !Exists( directoryPath ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        uint32_t const numExtensionFilters = (uint32_t) extensionFilter.size();
        TVector<String> lowercaseExtensionFilters = extensionFilter;
        for ( auto& extFilter : lowercaseExtensionFilters )
        {
            extFilter.make_lower();
        }

        // Path processing
        //-------------------------------------------------------------------------

        TInlineString<15> fileLowercaseExtension;

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

                // Optional: filter by extensions
                if ( numExtensionFilters > 0 && path.has_extension() )
                {
                    // Todo: there's likely a more efficient way to do a case insensitive compare
                    fileLowercaseExtension = TInlineString<15>( path.extension().u8string().c_str() + 1 );
                    fileLowercaseExtension.make_lower();

                    for ( auto i = 0u; i < numExtensionFilters; i++ )
                    {
                        if ( fileLowercaseExtension == lowercaseExtensionFilters[i].c_str() )
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
            case DirectoryReaderMode::Expand:
            {
                for ( auto& directoryEntry : std::filesystem::recursive_directory_iterator( directoryPath.c_str() ) )
                {
                    ProcessPath( directoryEntry.path() );
                }
            }
            break;

            case DirectoryReaderMode::DontExpand:
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

        if ( !Exists( directoryPath ) )
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
            case DirectoryReaderMode::Expand:
            {
                for ( auto& directoryEntry : std::filesystem::recursive_directory_iterator( directoryPath.c_str() ) )
                {
                    ProcessPath( directoryEntry.path() );
                }
            }
            break;

            case DirectoryReaderMode::DontExpand:
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