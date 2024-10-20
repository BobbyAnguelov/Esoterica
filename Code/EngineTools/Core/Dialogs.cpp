#include "Dialogs.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/TypeSystem/ResourceInfo.h"

#include <windows.h>
#include <shobjidl.h> 
#include "ToolsContext.h"

//-------------------------------------------------------------------------

namespace EE
{
    static inline HWND GetDialogParentWindow()
    {
        HWND hwndOwner = GetActiveWindow();
        return hwndOwner ? hwndOwner : GetForegroundWindow();
    }

    static void ConvertExtensionFilters( TInlineVector<FileDialog::ExtensionFilter, 2> const& filters, TInlineVector<COMDLG_FILTERSPEC, 5>& outFilterSpec )
    {
        for ( FileDialog::ExtensionFilter const& extFilter : filters )
        {
            if ( !extFilter.IsValid() )
            {
                continue;
            }

            COMDLG_FILTERSPEC const spec = { extFilter.m_displayText.data(), extFilter.m_filter.data() };
            outFilterSpec.emplace_back( spec );
        }
    }

    static void SetStartingPath( IFileDialog* pDialog, FileSystem::Path const& startingPath )
    {
        if ( startingPath.IsValid() )
        {
            String const directoryPath = startingPath.IsFilePath() ? startingPath.GetParentDirectory().GetString() : startingPath.GetString();
            String const filename = startingPath.IsFilePath() ? startingPath.GetFilename() : String();

            if ( !directoryPath.empty() )
            {
                WString const wdir( WString::CtorConvert(), directoryPath );

                IShellItem *pStartPath = nullptr;
                if ( SUCCEEDED( SHCreateItemFromParsingName( wdir.c_str(), NULL, IID_PPV_ARGS( &pStartPath ) ) ) )
                {
                    pDialog->SetFolder( pStartPath );
                    pStartPath->Release();
                }
            }

            if ( !filename.empty() )
            {
                WString const wfilename( WString::CtorConvert(), filename );
                pDialog->SetFileName( wfilename.c_str() );
            }
        }
    }

    static void ValidateResult( TInlineVector<FileDialog::ExtensionFilter, 2> const& filters, FileDialog::Result& result )
    {
        // Ensure all results are files and have extensions
        //-------------------------------------------------------------------------

        // Ensure results have an extension
        for ( int32_t i = int32_t( result.m_filePaths.size() ) - 1; i >= 0; i-- )
        {
            // Remove any non-file paths
            if ( !result.m_filePaths[i].IsFilePath() )
            {
                MessageDialog::Error( "Error", "Invalid file selected: %s", result.m_filePaths[i].c_str() );
                result.m_filePaths.erase_unsorted( result.m_filePaths.begin() + i );
                continue;
            }

            // Try to fix-up missing extensions
            FileSystem::Extension const ext = result.m_filePaths[i].GetExtensionAsString();
            if ( ext.empty() )
            {
                // We dont know what the extension should be so remove the invalid result
                if ( filters.empty() )
                {
                    MessageDialog::Error( "Error", "File with no extension selected: %s", result.m_filePaths[i].c_str() );
                    result.m_filePaths.erase_unsorted( result.m_filePaths.begin() + i );
                    continue;
                }
                else // Only a single filter so we can assume what the extension should be
                {
                    result.m_filePaths[i].AppendExtension( filters[0].m_extension );
                }
            }
        }

        // Ensure all extensions are valid
        //-------------------------------------------------------------------------

        if ( !filters.empty() )
        {
            for ( int32_t i = int32_t( result.m_filePaths.size() ) - 1; i >= 0; i-- )
            {
                EE_ASSERT( result.m_filePaths[i].IsFilePath() );
                FileSystem::Extension const ext = result.m_filePaths[i].GetExtensionAsString();
                EE_ASSERT( !ext.empty() );

                bool validExtension = false;
                for ( auto const& filter : filters )
                {
                    if ( ext.comparei( filter.m_extension ) == 0 )
                    {
                        validExtension = true;
                        continue;
                    }
                }

                if ( !validExtension )
                {
                    MessageDialog::Error( "Error", "Invalid extension detected: %s", result.m_filePaths[i].c_str() );
                    result.m_filePaths.erase_unsorted( result.m_filePaths.begin() + i );
                    continue;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    FileDialog::ExtensionFilter::ExtensionFilter( FileSystem::Extension ext, String displayText )
        : m_extension( ext )
    {
        if ( !m_extension.empty() )
        {
            m_filter = WString( WString::CtorConvert(), String( String::CtorSprintf(), "*.%s", m_extension.c_str() ) );

            if ( displayText.empty() )
            {
                m_displayText = m_filter;
            }
            else
            {
                m_displayText = WString( WString::CtorConvert(), displayText );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Folder Dialog
    //-------------------------------------------------------------------------

    FileDialog::Result FileDialog::SelectFolder( FileSystem::Path const& startingPath )
    {
        Result result;

        if ( SUCCEEDED( CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE ) ) )
        {
            // Create the FileOpenDialog object.
            IFileOpenDialog *pFileOpenDialog = nullptr;
            if ( SUCCEEDED( CoCreateInstance( CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>( &pFileOpenDialog ) ) ) )
            {
                // Set options
                //-------------------------------------------------------------------------

                DWORD options;
                pFileOpenDialog->GetOptions( &options );

                options |= FOS_PICKFOLDERS;
                options |= FOS_FORCEFILESYSTEM;
                options |= FOS_PATHMUSTEXIST;

                pFileOpenDialog->SetOptions( options );

                // Try set starting folder
                //-------------------------------------------------------------------------

                if ( startingPath.IsValid() )
                {
                    EE_ASSERT( startingPath.IsDirectoryPath() );
                    WString const wstr( WString::CtorConvert(), startingPath.GetString() );

                    IShellItem *pStartFolder = nullptr;
                    if ( SUCCEEDED( SHCreateItemFromParsingName( wstr.c_str(), NULL, IID_PPV_ARGS( &pStartFolder ) ) ) )
                    {
                        pFileOpenDialog->SetFolder( pStartFolder );
                        pStartFolder->Release();
                    }
                }

                // Show the dialog
                //-------------------------------------------------------------------------

                if ( SUCCEEDED( pFileOpenDialog->Show( GetDialogParentWindow() ) ) )
                {
                    // Get the file name from the dialog box.
                    IShellItem *pItem;
                    if ( SUCCEEDED( pFileOpenDialog->GetResult( &pItem ) ) )
                    {
                        PWSTR pReturnedFilePath = nullptr;
                        if ( SUCCEEDED( pItem->GetDisplayName( SIGDN_FILESYSPATH, &pReturnedFilePath ) ) )
                        {
                            String const pathStr( String::CtorConvert(), (wchar_t*) pReturnedFilePath );
                            CoTaskMemFree( pReturnedFilePath );
                            result.m_filePaths.emplace_back( pathStr );
                        }
                        pItem->Release();
                    }
                }

                pFileOpenDialog->Release();
            }
            CoUninitialize();
        }

        return result;
    }

    //-------------------------------------------------------------------------
    // Load Dialog
    //-------------------------------------------------------------------------

    FileDialog::Result FileDialog::Load( TInlineVector<ExtensionFilter, 2> const& filters, String const& title, bool allowMultiselect, FileSystem::Path const& startingPath )
    {
        Result result;

        if ( SUCCEEDED( CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE ) ) )
        {
            // Create the FileOpenDialog object.
            IFileOpenDialog *pFileOpenDialog = nullptr;
            if ( SUCCEEDED( CoCreateInstance( CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>( &pFileOpenDialog ) ) ) )
            {
                // Create file type restrictions
                //-------------------------------------------------------------------------

                TInlineVector<COMDLG_FILTERSPEC, 5> fileSpec;
                ConvertExtensionFilters( filters, fileSpec );

                // Set options
                //-------------------------------------------------------------------------

                DWORD options;
                pFileOpenDialog->GetOptions( &options );

                options |= FOS_FORCEFILESYSTEM;
                options |= FOS_PATHMUSTEXIST;

                if ( allowMultiselect )
                {
                    options |= FOS_ALLOWMULTISELECT;
                }

                if ( !fileSpec.empty() )
                {
                    options |= FOS_STRICTFILETYPES;
                    pFileOpenDialog->SetFileTypes( (uint32_t) fileSpec.size(), fileSpec.data() );
                }

                pFileOpenDialog->SetOptions( options );

                // Set title
                //-------------------------------------------------------------------------

                if ( !title.empty() )
                {
                    WString wtitle( WString::CtorConvert(), title );
                    pFileOpenDialog->SetTitle( wtitle.data() );
                }

                // Try set starting path
                //-------------------------------------------------------------------------

                SetStartingPath( pFileOpenDialog, startingPath );

                // Show the dialog
                //-------------------------------------------------------------------------

                if ( SUCCEEDED( pFileOpenDialog->Show( GetDialogParentWindow() ) ) )
                {
                    // Get the file name(s) from the dialog box.
                    IShellItemArray* pResults = nullptr;
                    if ( SUCCEEDED( pFileOpenDialog->GetResults( &pResults ) ) )
                    {
                        DWORD count = 0; pResults->GetCount( &count );
                        for ( DWORD i = 0; i < count; ++i )
                        {
                            IShellItem* pItem = nullptr;
                            if ( SUCCEEDED( pResults->GetItemAt( i, &pItem ) ) )
                            {
                                PWSTR pReturnedFilePath = nullptr;
                                if ( SUCCEEDED( pItem->GetDisplayName( SIGDN_FILESYSPATH, &pReturnedFilePath ) ) )
                                {
                                    String const pathStr( String::CtorConvert(), (wchar_t*) pReturnedFilePath );
                                    CoTaskMemFree( pReturnedFilePath );
                                    result.m_filePaths.emplace_back( pathStr );
                                }
                                pItem->Release();
                            }
                        }

                        pResults->Release();
                    }
                }

                pFileOpenDialog->Release();
            }
            CoUninitialize();
        }

        ValidateResult( filters, result );

        return result;
    }

    FileDialog::Result FileDialog::Load( ToolsContext const* pToolsContext, ResourceTypeID resourceTypeID, FileSystem::Path const& startingPath )
    {
        TypeSystem::ResourceInfo const* pResourceInfo = pToolsContext->m_pTypeRegistry->GetResourceInfo( resourceTypeID );
        EE_ASSERT( pResourceInfo != nullptr );

        TInlineVector<ExtensionFilter, 2> filters;
        filters.emplace_back( resourceTypeID.ToExtension(), pResourceInfo->m_friendlyName );

        String const title( String::CtorSprintf(), "Save %s", pResourceInfo->m_friendlyName.c_str() );

        Result result = Load( filters, title, false, startingPath );

        // Ensure that all selected files are within the source data directory
        for ( int32_t i = int32_t( result.m_filePaths.size() ) - 1; i >= 0; i-- )
        {
            if ( !result.m_filePaths[i].IsUnderDirectory( pToolsContext->GetSourceDataDirectory() ) )
            {
                MessageDialog::Error( "Error", "Selected file is not with the raw resource folder!" );
                result.m_filePaths.erase_unsorted( result.m_filePaths.begin() + i );
            }
        }

        return result;
    }

    //-------------------------------------------------------------------------
    // Save Dialog
    //-------------------------------------------------------------------------

    FileDialog::Result FileDialog::Save( TInlineVector<ExtensionFilter, 2> const& filters, String const& title, FileSystem::Path const& startingPath )
    {
        Result result;

        if ( SUCCEEDED( CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE ) ) )
        {
            // Create the FileOpenDialog object.
            IFileSaveDialog *pFileSaveDialog = nullptr;
            if ( SUCCEEDED( CoCreateInstance( CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>( &pFileSaveDialog ) ) ) )
            {
                // Create file type restrictions
                //-------------------------------------------------------------------------

                TInlineVector<COMDLG_FILTERSPEC, 5> fileSpec;
                ConvertExtensionFilters( filters, fileSpec );

                // Set options
                //-------------------------------------------------------------------------

                DWORD options;
                pFileSaveDialog->GetOptions( &options );

                options |= FOS_FORCEFILESYSTEM;
                options |= FOS_PATHMUSTEXIST;
                options &= ~FOS_NOREADONLYRETURN;

                if ( !fileSpec.empty() )
                {
                    options |= FOS_STRICTFILETYPES;
                    pFileSaveDialog->SetFileTypes( (uint32_t) fileSpec.size(), fileSpec.data() );
                }

                pFileSaveDialog->SetOptions( options );

                // Set title
                //-------------------------------------------------------------------------

                if ( !title.empty() )
                {
                    WString wtitle( WString::CtorConvert(), title );
                    pFileSaveDialog->SetTitle( wtitle.data() );
                }

                // Try set starting path
                //-------------------------------------------------------------------------

                SetStartingPath( pFileSaveDialog, startingPath );

                // Show the dialog
                //-------------------------------------------------------------------------

                if ( SUCCEEDED( pFileSaveDialog->Show( GetDialogParentWindow() ) ) )
                {
                    // Get the file name from the dialog box.
                    IShellItem* pItem = nullptr;
                    if ( SUCCEEDED( pFileSaveDialog->GetResult( &pItem ) ) )
                    {
                        PWSTR pReturnedFilePath = nullptr;
                        if ( SUCCEEDED( pItem->GetDisplayName( SIGDN_FILESYSPATH, &pReturnedFilePath ) ) )
                        {
                            String const pathStr( String::CtorConvert(), (wchar_t*) pReturnedFilePath );
                            CoTaskMemFree( pReturnedFilePath );
                            result.m_filePaths.emplace_back( pathStr );
                        }
                        pItem->Release();
                    }
                }

                pFileSaveDialog->Release();
            }
            CoUninitialize();
        }

        ValidateResult( filters, result );

        return result;
    }

    FileDialog::Result FileDialog::Save( ToolsContext const* pToolsContext, ResourceTypeID resourceTypeID, FileSystem::Path const& startingPath )
    {
        TypeSystem::ResourceInfo const* pResourceInfo = pToolsContext->m_pTypeRegistry->GetResourceInfo( resourceTypeID );
        EE_ASSERT( pResourceInfo != nullptr );

        TInlineVector<ExtensionFilter, 2> filters;
        filters.emplace_back( resourceTypeID.ToExtension(), pResourceInfo->m_friendlyName );

        String const title( String::CtorSprintf(), "Save %s", pResourceInfo->m_friendlyName.c_str() );

        Result result = Save( filters, title, startingPath );

        //-------------------------------------------------------------------------

        // Ensure that all selected files are within the source data directory
        for ( int32_t i = int32_t( result.m_filePaths.size() ) - 1; i >= 0; i-- )
        {
            if ( !result.m_filePaths[i].IsUnderDirectory( pToolsContext->GetSourceDataDirectory() ) )
            {
                MessageDialog::Error( "Error", "Selected file is not with the raw resource folder!" );
                result.m_filePaths.erase_unsorted( result.m_filePaths.begin() + i );
            }
        }

        return result;
    }

    //-------------------------------------------------------------------------
    // Message Box
    //-------------------------------------------------------------------------

    MessageDialog::Result MessageDialog::ShowInternal( Severity severity, MessageDialog::Type type, String const& title, String const& message )
    {
        UINT style = MB_TOPMOST | MB_SYSTEMMODAL;

        switch ( severity )
        {
            case Severity::Info:
            {
                if ( type == Type::Ok )
                {
                    style |= MB_ICONINFORMATION;
                }
                else
                {
                    style |= MB_ICONQUESTION;
                }
            }
            break;

            case Severity::Warning:
            {
                style |= MB_ICONWARNING;
            }
            break;

            case Severity::Error:
            case Severity::FatalError:
            {
                style |= MB_ICONERROR;
            }
            break;
        }

        //-------------------------------------------------------------------------

        switch ( type )
        {
            case Type::Ok: style |= MB_OK; break;
            case Type::OkCancel: style |= MB_OKCANCEL; break;
            case Type::YesNo: style |= MB_YESNO; break;
            case Type::YesNoCancel: style |= MB_YESNOCANCEL; break;
            case Type::RetryCancel: style |= MB_RETRYCANCEL; break;
            case Type::AbortRetryIgnore: style |= MB_ABORTRETRYIGNORE; break;
            case Type::CancelTryContinue: style |= MB_CANCELTRYCONTINUE; break;
        }

        //-------------------------------------------------------------------------

        Result result = Result::Yes;

        int32_t resultCode = MessageBox( GetDialogParentWindow(), message.c_str(), title.c_str(), style );
        switch( resultCode )
        {
            case IDOK:
            case IDYES:
            {
                result = Result::Yes;
            }
            break;

            case IDNO:
            {
                result = Result::No;
            }
            break;

            case IDCANCEL:
            case IDABORT:
            {
                result = Result::Cancel;
            }
            break;

            case IDRETRY:
            case IDTRYAGAIN:
            {
                result = Result::Retry;
            }
            break;

            case IDIGNORE:
            case IDCONTINUE:
            {
                result = Result::Continue;
            }
            break;

            case IDCLOSE:
            {
                result = Result::Cancel;
            }
            break;

            default:
            {
                result = Result::Yes;
            }
            break;
        }

        return result;
    }
}