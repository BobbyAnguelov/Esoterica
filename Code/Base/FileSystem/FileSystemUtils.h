#pragma once

#include "FileSystemPath.h"

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    //-------------------------------------------------------------------------
    // Directory Reader
    //-------------------------------------------------------------------------

    // What should we return
    enum class DirectoryReaderOutput
    {
        All,
        OnlyFiles,
        OnlyDirectories
    };

    // Should we expand sub-directories when reading contents?
    enum class DirectoryReaderMode
    {
        Expand,
        DontExpand,
    };

    // Get the contents of a specified directory
    // The extension filter is a list of extensions excluding the period e.g. extensionfilter = { "txt", "exe" }
    EE_BASE_API bool GetDirectoryContents( Path const& directoryPath, TVector<Path>& contents, DirectoryReaderOutput output = DirectoryReaderOutput::All, DirectoryReaderMode mode = DirectoryReaderMode::Expand, TVector<String> const& extensionFilters = TVector<String>() );

    // Get the contents of a specified directory
    // All paths will be matched against the supplied regex expression
    // WARNING!!! THIS IS VERY SLOW
    EE_BASE_API bool GetDirectoryContents( Path const& directoryPath, char const* const pRegexExpression, TVector<Path>& contents, DirectoryReaderOutput output = DirectoryReaderOutput::All, DirectoryReaderMode mode = DirectoryReaderMode::Expand );

    //-------------------------------------------------------------------------
    // Miscellaneous functions
    //-------------------------------------------------------------------------

    EE_BASE_API Path GetCurrentProcessPath();
}