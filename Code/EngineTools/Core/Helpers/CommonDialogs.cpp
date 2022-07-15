#include "CommonDialogs.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"

//-------------------------------------------------------------------------

namespace EE
{
    FileSystem::Path SaveDialog( String const& extension, FileSystem::Path const& startingPath, String const& friendlyFilterName )
    {
        EE_ASSERT( !extension.empty() && extension.front() != '.' );

        FileSystem::Path outPath;

        // Select file
        //-------------------------------------------------------------------------

        TInlineString<10> filterString;
        filterString.sprintf( "*.%s", extension.c_str() );

        auto const selectedFilePath = pfd::save_file( "Save File", startingPath.c_str(), { friendlyFilterName.c_str(), filterString.c_str() } ).result();
        if ( selectedFilePath.empty() )
        {
            return outPath;
        }

        outPath = selectedFilePath.c_str();

        // Validate selected filename
        //-------------------------------------------------------------------------

        char const* pSelectedFileExtension = outPath.GetExtension();
        if ( pSelectedFileExtension == nullptr || _stricmp( extension.c_str(), pSelectedFileExtension ) != 0 )
        {
            InlineString errorString;
            errorString.sprintf( "Invalid extension provided! You need to have the .%s extension!", extension.c_str() );
            pfd::message( "Error", errorString.c_str(), pfd::choice::ok, pfd::icon::error ).result();
            outPath.Clear();
        }

        return outPath;
    }

    FileSystem::Path SaveDialog( ResourceTypeID resourceTypeID, FileSystem::Path const& startingPath, String const& friendlyFilterName )
    {
        FileSystem::Path outPath;

        TInlineString<5> const resourceTypeIDString = resourceTypeID.ToString();

        // Select file
        //-------------------------------------------------------------------------

        TInlineString<10> filterString;
        filterString.sprintf( "*.%s", resourceTypeIDString.c_str() );

        auto const selectedFilePath = pfd::save_file( "Save File", startingPath.c_str(), { friendlyFilterName.c_str(), filterString.c_str() } ).result();
        if ( selectedFilePath.empty() )
        {
            return outPath;
        }

        outPath = selectedFilePath.c_str();

        // Validate selected filename
        //-------------------------------------------------------------------------

        auto extStr = outPath.GetLowercaseExtensionAsString();
        if ( extStr.empty() )
        {
            outPath.ReplaceExtension( resourceTypeIDString.c_str() );
        }
        else
        {
            ResourceTypeID const newTypeID( extStr.c_str() );
            if ( resourceTypeID != newTypeID )
            {
                InlineString errorString;
                errorString.sprintf( "Invalid extension provided! You need to have the .%s extension!", resourceTypeIDString.c_str() );
                pfd::message( "Error", errorString.c_str(), pfd::choice::ok, pfd::icon::error ).result();
                outPath.Clear();
            }
        }

        return outPath;
    }
}