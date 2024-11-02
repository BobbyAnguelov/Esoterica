#include "EditorTool_ResourceBrowser.h"
#include "EngineTools/Resource/ResourceDescriptorCreator.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/FileSystem/FileRegistry.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Core/Dialogs.h"
#include "EngineTools/Core/CommonToolTypes.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/TypeSystem/DataFileInfo.h"
#include "Base/TypeSystem/ResourceInfo.h"
#include "Base/Profiling.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include <eastl/sort.h>

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceBrowserTreeItem final : public TreeListViewItem
    {

    public:

        ResourceBrowserTreeItem( TreeListViewItem* pParent, ToolsContext const& toolsContext, FileRegistry::DirectoryInfo const* pDirectoryEntry )
            : TreeListViewItem( pParent )
            , m_toolsContext( toolsContext )
            , m_nameID( pDirectoryEntry->m_filePath.GetDirectoryName() )
            , m_path( pDirectoryEntry->m_filePath )
            , m_dataPath( pDirectoryEntry->m_resourcePath )
        {
            EE_ASSERT( m_path.IsValid() );
            EE_ASSERT( m_dataPath.IsValid() );

            //-------------------------------------------------------------------------

            for ( auto const& childDirectory : pDirectoryEntry->m_directories )
            {
                CreateChild<ResourceBrowserTreeItem>( m_toolsContext, &childDirectory );
            }
        }

        ResourceBrowserTreeItem( TreeListViewItem* pParent, ToolsContext const& toolsContext, FileRegistry::FileInfo const* pFileEntry )
            : TreeListViewItem( pParent )
            , m_toolsContext( toolsContext )
            , m_nameID( pFileEntry->m_filePath.GetFilename() )
            , m_path( pFileEntry->m_filePath )
            , m_dataPath( pFileEntry->m_dataPath )
        {
            EE_ASSERT( m_path.IsValid() );
            EE_ASSERT( m_dataPath.IsValid() );
        }

        virtual StringID GetNameID() const { return m_nameID; }
        virtual uint64_t GetUniqueID() const override { return m_dataPath.GetID(); }
        virtual bool HasContextMenu() const override { return true; }
        virtual bool IsActivatable() const override { return false; }

        virtual InlineString GetDisplayName() const override
        {
            InlineString displayName;
            displayName.sprintf( EE_ICON_FOLDER" %s", GetNameID().c_str() );
            return displayName;
        }

        // File System Info
        //-------------------------------------------------------------------------

        inline FileSystem::Path const& GetFilePath() const { return m_path; }
        inline DataPath const& GetDataPath() const { return m_dataPath; }

        // Signals
        //-------------------------------------------------------------------------

        bool OnUserActivate()
        {
            if ( IsExpanded() )
            {
                SetExpanded( false );
            }
            else
            {
                SetExpanded( true );
            }

            return true;
        }

        virtual bool OnDoubleClick() override { return OnUserActivate(); }
        virtual bool OnEnterPressed() override { return OnUserActivate(); }

    protected:

        ToolsContext const&                     m_toolsContext;
        StringID                                m_nameID;
        FileSystem::Path                        m_path;
        DataPath                                m_dataPath;
    };

    //-------------------------------------------------------------------------

    ResourceBrowserEditorTool::ResourceBrowserEditorTool( ToolsContext const* pToolsContext )
        : EditorTool( pToolsContext, "Resource Browser" )
        , m_dataDirectoryPathDepth( m_pToolsContext->GetSourceDataDirectory().GetPathDepth() )
    {
        m_folderTreeView.SetFlag( TreeListView::ExpandItemsOnlyViaArrow, true );
        m_folderTreeView.SetFlag( TreeListView::UseSmallFont, false );
        m_folderTreeView.SetFlag( TreeListView::ShowBranchesFirst, false );
        m_folderTreeView.SetFlag( TreeListView::SortTree, true );

        m_filter.SetFilterHelpText( "Search..." );

        // Rebuild the tree whenever the file registry updates
        m_resourceDatabaseUpdateEventBindingID = m_pToolsContext->m_pFileRegistry->OnFileSystemCacheUpdated().Bind( [this] () { m_updateFolderAndFileViews = true; } );

        // Create descriptor category tree
        //-------------------------------------------------------------------------

        TypeSystem::TypeRegistry const* pTypeRegistry = m_pToolsContext->m_pTypeRegistry;
        TVector<TypeSystem::TypeInfo const*> descriptorTypeInfos = pTypeRegistry->GetAllDerivedTypes( Resource::ResourceDescriptor::GetStaticTypeID(), false, false );

        for ( auto pTypeInfo : descriptorTypeInfos )
        {
            auto pRD = (Resource::ResourceDescriptor const*) pTypeInfo->GetDefaultInstance();
            if ( !pRD->IsUserCreateableDescriptor() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            String category = pTypeInfo->m_ID.c_str();
            StringUtils::ReplaceAllOccurrencesInPlace( category, "::", "/" );
            StringUtils::ReplaceAllOccurrencesInPlace( category, "EE/", "" );
            size_t const foundIdx = category.find_last_of( '/' );
            EE_ASSERT( foundIdx != String::npos );
            category = category.substr( 0, foundIdx );

            auto pResourceInfo = pTypeRegistry->GetResourceInfo( pRD->GetCompiledResourceTypeID() );
            m_categorizedDescriptorTypes.AddItem( category, pResourceInfo->m_friendlyName.c_str(), pTypeInfo );
        }
    }

    ResourceBrowserEditorTool::~ResourceBrowserEditorTool()
    {
        m_pToolsContext->m_pFileRegistry->OnFileSystemCacheUpdated().Unbind( m_resourceDatabaseUpdateEventBindingID );
        EE::Delete( m_pResourceDescriptorCreator );
    }

    void ResourceBrowserEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        CreateToolWindow( "Browser", [this] ( UpdateContext const& context, bool isFocused ) { DrawBrowserWindow( context, isFocused ); } );

        // Generate all possible type filters
        //-------------------------------------------------------------------------

        for ( auto const& resourceInfo : m_pToolsContext->m_pTypeRegistry->GetRegisteredResourceTypes() )
        {
            TypeFilter typeFilter;
            typeFilter.m_friendlyName = resourceInfo.second->m_friendlyName;
            typeFilter.m_resourceTypeID = resourceInfo.second->m_resourceTypeID;
            typeFilter.m_extension = resourceInfo.second->m_resourceTypeID.ToString();
            m_allPossibleTypeFilters.emplace_back( typeFilter );
        }

        for ( auto const& dataFileInfo : m_pToolsContext->m_pTypeRegistry->GetRegisteredDataFileTypes() )
        {
            TypeFilter typeFilter;
            typeFilter.m_friendlyName = dataFileInfo.second->m_friendlyName;
            typeFilter.m_resourceTypeID = ResourceTypeID();
            typeFilter.m_extension = FourCC::ToString( dataFileInfo.second->m_extensionFourCC );
            m_allPossibleTypeFilters.emplace_back( typeFilter );
        }

        auto SortPredicate = [] ( TypeFilter const& a, TypeFilter const& b )
        {
            return a.m_extension < b.m_extension;
        };

        eastl::sort( m_allPossibleTypeFilters.begin(), m_allPossibleTypeFilters.end(), SortPredicate );
    }

    void ResourceBrowserEditorTool::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        if ( m_pToolsContext->m_pFileRegistry->IsBuildingCaches() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        TreeListViewContext treeViewContext;
        treeViewContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildFolderTreeView( pRootItem ); };
        treeViewContext.m_drawItemContextMenuFunction = [this] ( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) { DrawFolderContextMenu( selectedItemsWithContextMenus ); };

        if ( m_updateFolderAndFileViews )
        {
            m_folderTreeView.RebuildTree( treeViewContext, true );
            GenerateFileList();
            m_updateFolderAndFileViews = false;
        }

        //-------------------------------------------------------------------------

        if ( m_navigationRequest.IsSet() )
        {
            HandleNavigationRequest();
            m_navigationRequest.Clear();
        }

        //-------------------------------------------------------------------------

        if ( m_pResourceDescriptorCreator != nullptr )
        {
            if ( !m_pResourceDescriptorCreator->Draw() )
            {
                EE::Delete( m_pResourceDescriptorCreator );
            }
        }
    }

    void ResourceBrowserEditorTool::DrawBrowserWindow( UpdateContext const& context, bool isFocused )
    {
        // Draw progress bar
        //-------------------------------------------------------------------------

        if ( m_pToolsContext->m_pFileRegistry->IsBuildingCaches() )
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text( m_pToolsContext->m_pFileRegistry->IsFileSystemCacheBuilt() ? "Building Descriptor Cache: " : "Building File System Cache: " );
            ImGui::SameLine();
            ImGui::ProgressBar( m_pToolsContext->m_pFileRegistry->GetProgress() );
        }

        // Draw UI
        //-------------------------------------------------------------------------

        if ( m_pToolsContext->m_pFileRegistry->IsFileSystemCacheBuilt() )
        {
            // Controls
            //-------------------------------------------------------------------------

            DrawControlRow( context );
            DrawResourceTypeFilterRow( context );

            // Update Filter
            //-------------------------------------------------------------------------

            if ( m_filterUpdated )
            {
                UpdateFolderTreeVisibility();
                GenerateFileList();
            }
            m_filterUpdated = false;

            // Folders
            //-------------------------------------------------------------------------

            bool const isTextFilterSet = m_filter.HasFilterSet();
            if ( !isTextFilterSet )
            {
                ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 1, 0 ) );
                ImGui::BeginChild( "left pane", ImVec2( 150, 0 ), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX | ImGuiChildFlags_AlwaysUseWindowPadding );
                ImGui::PopStyleVar();
                {
                    DrawFolderView( context );
                }
                ImGui::EndChild();
                ImGui::SameLine( 0, 0 );
            }

            // Files
            //-------------------------------------------------------------------------

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 1, 0 ) );
            ImGui::BeginChild( "item view", ImVec2( 0, 0 ), ImGuiChildFlags_AlwaysUseWindowPadding );
            ImGui::PopStyleVar();
            {
                DrawFileView( context );
            }
            ImGui::EndChild();
        }
    }

    void ResourceBrowserEditorTool::HandleNavigationRequest()
    {
        EE_ASSERT( m_navigationRequest.IsSet() );

        // Clear all filters
        //-------------------------------------------------------------------------

        m_selectedTypeFilterIndices.clear();
        UpdateFolderTreeVisibility();

        // Select folder
        //-------------------------------------------------------------------------

        DataPath const directoryPath = m_navigationRequest.m_path.IsDirectoryPath() ? m_navigationRequest.m_path : m_navigationRequest.m_path.GetParentDirectory();
        if ( !directoryPath.IsValid() )
        {
            return;
        }

        auto pFoundItem = m_folderTreeView.FindItem( directoryPath.GetID() );
        if ( pFoundItem == nullptr )
        {
            return;
        }

        m_folderTreeView.SetSelection( pFoundItem );
        m_folderTreeView.SetViewToSelection();
        SetSelectedFolder( directoryPath );

        // Select file
        //-------------------------------------------------------------------------

        if ( m_navigationRequest.m_path.IsFilePath() )
        {
            SetSelectedFile( m_navigationRequest.m_path, true );
        }
    }

    //-------------------------------------------------------------------------

    bool ResourceBrowserEditorTool::DrawDeleteFileConfirmationDialog( UpdateContext const& context, FileRegistry::FileInfo const& fileToDelete )
    {
        bool shouldClose = false;

        ImGui::Text( "Are you sure you want to delete this file?" );
        ImGui::Text( "This cannot be undone!" );

        if ( ImGui::Button( "Ok", ImVec2( 143, 0 ) ) )
        {            
            m_folderTreeView.ClearSelection();
            FileSystem::EraseFile( fileToDelete.m_filePath );
            shouldClose = true;
        }

        ImGui::SameLine( 0, 6 );

        if ( ImGui::Button( "Cancel", ImVec2( 143, 0 ) ) )
        {
            shouldClose = true;
        }
        ImGui::SetItemDefaultFocus();

        //-------------------------------------------------------------------------

        return !shouldClose;
    }

    void ResourceBrowserEditorTool::DrawCreationControls( UpdateContext const& context )
    {
        ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );

        if ( ImGuiX::ButtonColored( EE_ICON_PLUS" CREATE", Colors::Green, Colors::White, ImVec2( -1, 0 ) ) )
        {
            ImGui::OpenPopup( "CreateNewDescriptor" );
        }
        ImGuiX::ItemTooltip( "Create a new resource descriptor and fill out the settings by hand! This is how you create resources that dont have a source file." );

        if ( ImGui::BeginPopup( "CreateNewDescriptor" ) )
        {
            DrawDescriptorMenuCategory( m_pToolsContext->GetSourceDataDirectory(), m_categorizedDescriptorTypes.GetRootCategory() );
            ImGui::EndPopup();
        }
        ImGuiX::ItemTooltip( "Create a resource descriptor based on a source file. This will pop up a helper window to help you with the creation." );
    }

    void ResourceBrowserEditorTool::DrawDescriptorMenuCategory( FileSystem::Path const& path, Category<TypeSystem::TypeInfo const*> const& category )
    {
        auto DrawItems = [this, &path, &category] ()
        {
            for ( auto const& childCategory : category.m_childCategories )
            {
                DrawDescriptorMenuCategory( path, childCategory );
            }

            for ( auto const& item : category.m_items )
            {
                if ( ImGui::MenuItem( item.m_name.c_str() ) )
                {
                    m_pResourceDescriptorCreator = EE::New<ResourceDescriptorCreator>( m_pToolsContext, item.m_data->m_ID, path );
                }
            }
        };

        //-------------------------------------------------------------------------

        if ( category.m_depth == -1 )
        {
            DrawItems();
        }
        else
        {
            if ( ImGui::BeginMenu( category.m_name.c_str() ) )
            {
                DrawItems();
                ImGui::EndMenu();
            }
        }
    }

    void ResourceBrowserEditorTool::DrawControlRow( UpdateContext const& context )
    {
        ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );

        constexpr static float const buttonWidth = 30;

        //-------------------------------------------------------------------------
        // Create / Import Buttons
        //-------------------------------------------------------------------------

        if ( ImGuiX::IconButtonColored( EE_ICON_PLUS, "CREATE", Colors::Green, Colors::White ) )
        {
            ImGui::OpenPopup( "CreateNewDescriptor" );
        }
        ImGuiX::ItemTooltip( "Create a new resource descriptor and fill out the settings by hand! This is how you create resources that dont have a source file." );

        if ( ImGui::BeginPopup( "CreateNewDescriptor" ) )
        {
            DrawDescriptorMenuCategory( m_pToolsContext->GetSourceDataDirectory(), m_categorizedDescriptorTypes.GetRootCategory() );
            ImGui::EndPopup();
        }
        ImGuiX::ItemTooltip( "Create a resource descriptor based on a source file. This will pop up a helper window to help you with the creation." );

        //-------------------------------------------------------------------------

        ImGui::SameLine();
        if ( ImGuiX::IconButton( EE_ICON_IMPORT, "IMPORT", Colors::White ) )
        {
        }

        //-------------------------------------------------------------------------

        ImGui::SameLine();
        float const filterWidth = ImGui::GetContentRegionAvail().x - ( 2 * ( buttonWidth + ImGui::GetStyle().ItemSpacing.x ) );
        m_filterUpdated |= m_filter.UpdateAndDraw( filterWidth );

        //-------------------------------------------------------------------------

        ImGui::SameLine();
        if ( ImGuiX::ToggleButton( EE_ICON_RAW, EE_ICON_RAW_OFF, m_showRawFiles, ImVec2( buttonWidth, 0 ) ) )
        {
            m_filterUpdated = true;
        }

        //-------------------------------------------------------------------------

        auto DrawFiltersMenu = [this] ()
        {
            InlineString label;
            int32_t const numTypeFilters = (int32_t) m_allPossibleTypeFilters.size();
            for ( int32_t i = 0; i < numTypeFilters; i++ )
            {
                bool isChecked = VectorContains( m_selectedTypeFilterIndices, i );
                label.sprintf( "%s (.%s)", m_allPossibleTypeFilters[i].m_friendlyName.c_str(), m_allPossibleTypeFilters[i].m_extension.c_str() );

                if ( ImGui::Checkbox( label.c_str(), &isChecked ) )
                {
                    if ( isChecked )
                    {
                        m_selectedTypeFilterIndices.emplace_back( i );
                    }
                    else
                    {
                        m_selectedTypeFilterIndices.erase_first_unsorted( i );
                    }

                    m_filterUpdated = true;
                }
            }
        };

        ImGui::SameLine();
        ImGuiX::DropDownIconButton( EE_ICON_FILTER, "##Resource Filters", DrawFiltersMenu, ImGuiX::Style::s_colorText, ImVec2( buttonWidth, 0 ) );
    }

    void ResourceBrowserEditorTool::DrawResourceTypeFilterRow( UpdateContext const& context )
    {
        if ( !m_selectedTypeFilterIndices.empty() )
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Type Filters:" );
        }

        int32_t selectedFilterToRemove = InvalidIndex;

        for ( int32_t i = 0; i < (int32_t) m_selectedTypeFilterIndices.size(); i++ )
        {
            TypeFilter const& filter = m_allPossibleTypeFilters[m_selectedTypeFilterIndices[i]];

            ImGui::SameLine();
            if ( ImGui::Button( filter.m_extension.c_str() ) )
            {
                selectedFilterToRemove = i;
            }
            ImGuiX::ItemTooltip( filter.m_friendlyName.c_str() );
        }

        if ( selectedFilterToRemove != InvalidIndex )
        {
            m_selectedTypeFilterIndices.erase( m_selectedTypeFilterIndices.begin() + selectedFilterToRemove );
            m_filterUpdated = true;
        }
    }

    // Folders
    //-------------------------------------------------------------------------

    void ResourceBrowserEditorTool::RebuildFolderTreeView( TreeListViewItem* pRootItem )
    {
        EE_ASSERT( m_pToolsContext->m_pFileRegistry->IsFileSystemCacheBuilt() );
        auto pDataDirectory = m_pToolsContext->m_pFileRegistry->GetRawResourceDirectoryEntry();

        //-------------------------------------------------------------------------

        pRootItem->DestroyChildren();

        for ( auto const& childDirectory : pDataDirectory->m_directories )
        {
            pRootItem->CreateChild<ResourceBrowserTreeItem>( *m_pToolsContext, &childDirectory );
        }

        //-------------------------------------------------------------------------

        UpdateFolderTreeVisibility();
    }

    void ResourceBrowserEditorTool::UpdateFolderTreeVisibility()
    {
        auto VisibilityFunc = [this] ( TreeListViewItem const* pTreeItem )
        {
            // Type filter
            //-------------------------------------------------------------------------

            bool containsVisibleFile = false;
            auto pItem = static_cast<ResourceBrowserTreeItem const*>( pTreeItem );

            FileRegistry::DirectoryInfo const* pDirectoryInfo = m_pToolsContext->m_pFileRegistry->FindDirectoryEntry( pItem->GetFilePath() );
            for ( FileRegistry::FileInfo const* pFile : pDirectoryInfo->m_files )
            {
                if ( DoesFileMatchFilter( pFile ) )
                {
                    containsVisibleFile = true;
                    break;
                }
            }

            //-------------------------------------------------------------------------

            return containsVisibleFile;
        };

        //-------------------------------------------------------------------------

        m_folderTreeView.UpdateItemVisibility( VisibilityFunc, true );
    }

    void ResourceBrowserEditorTool::SetSelectedFolder( DataPath const& newFolderPath )
    {
        m_selectedFolder = newFolderPath;
        m_selectedFile.Clear();
        GenerateFileList();
    }

    void ResourceBrowserEditorTool::DrawFolderView( UpdateContext const& context )
    {
        TreeListViewContext treeViewContext;
        treeViewContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildFolderTreeView( pRootItem ); };
        treeViewContext.m_drawItemContextMenuFunction = [this] ( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) { DrawFolderContextMenu( selectedItemsWithContextMenus ); };

        if ( m_folderTreeView.UpdateAndDraw( treeViewContext ) )
        {
            TVector<TreeListViewItem*> const& selection = m_folderTreeView.GetSelection();
            if ( selection.empty() )
            {
                SetSelectedFolder( DataPath() );
            }
            else
            {
                SetSelectedFolder( static_cast<ResourceBrowserTreeItem const*>( selection.back() )->GetDataPath() );
            }
        }
    }

    void ResourceBrowserEditorTool::DrawFolderContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus )
    {
        auto pResourceItem = (ResourceBrowserTreeItem*) selectedItemsWithContextMenus[0];

        //-------------------------------------------------------------------------

        if ( ImGui::MenuItem( "Open In Explorer" ) )
        {
            Platform::Win32::OpenInExplorer( pResourceItem->GetFilePath() );
        }

        if ( ImGui::MenuItem( "Copy File Path" ) )
        {
            ImGui::SetClipboardText( pResourceItem->GetFilePath().c_str() );
        }

        if ( ImGui::MenuItem( "Copy Resource Path" ) )
        {
            ImGui::SetClipboardText( pResourceItem->GetDataPath().c_str() );
        }

        ImGui::Separator();

        if ( ImGui::BeginMenu( "Create New Descriptor" ) )
        {
            DrawDescriptorMenuCategory( pResourceItem->GetFilePath(), m_categorizedDescriptorTypes.GetRootCategory() );
            ImGui::EndMenu();
        }
    }

    // Files
    //-------------------------------------------------------------------------

    bool ResourceBrowserEditorTool::DoesFileMatchFilter( FileRegistry::FileInfo const* pFile, bool applyNameFilter )
    {
        bool isVisible = true;

        // Raw files
        if ( pFile->m_fileType == FileRegistry::FileType::Unknown )
        {
            isVisible = m_showRawFiles;
        }
        // Data file
        else if ( pFile->IsDataFile() )
        {
            if ( m_selectedTypeFilterIndices.empty() )
            {
                isVisible = true;
            }
            else // Apply filters
            {
                isVisible = false;

                for ( int32_t selectedFilterIdx : m_selectedTypeFilterIndices )
                {
                    if ( m_allPossibleTypeFilters[selectedFilterIdx].m_extension == pFile->m_extension )
                    {
                        isVisible = true;
                        break;
                    }
                }
            }
        }
        else // Resource/Entity Descriptor file
        {
            if ( m_selectedTypeFilterIndices.empty() )
            {
                isVisible = true;
            }
            else // Apply filters
            {
                isVisible = false;

                for ( int32_t selectedFilterIdx : m_selectedTypeFilterIndices )
                {
                    if ( m_allPossibleTypeFilters[selectedFilterIdx].m_resourceTypeID == pFile->GetResourceTypeID() )
                    {
                        isVisible = true;
                        break;
                    }
                }
            }
        }

        // Text filter
        //-------------------------------------------------------------------------

        if ( applyNameFilter && isVisible )
        {
            isVisible = m_filter.MatchesFilter( pFile->m_dataPath.GetFilename() );
        }

        //-------------------------------------------------------------------------

        return isVisible;
    }

    void ResourceBrowserEditorTool::GenerateFileList()
    {
        m_fileList.clear();
        m_sortedFileListIndices.clear();

        DataPath const previouslySelectedFile = m_selectedFile;

        //-------------------------------------------------------------------------

        TVector<FileRegistry::FileInfo const*> files;
        files.reserve( 25000 );
        if ( m_filter.HasFilterSet() )
        {
            FileRegistry::DirectoryInfo const* pDirectoryInfo = m_pToolsContext->m_pFileRegistry->FindDirectoryEntry( DataPath( "Data://" ) );
            pDirectoryInfo->GetAllFiles( files, true );
        }
        else
        {
            FileRegistry::DirectoryInfo const* pDirectoryInfo = m_pToolsContext->m_pFileRegistry->FindDirectoryEntry( m_selectedFolder );
            if ( pDirectoryInfo != nullptr )
            {
                for ( FileRegistry::FileInfo const* pFileInfo : pDirectoryInfo->m_files )
                {
                    files.emplace_back( pFileInfo );
                }
            }
        }

        //-------------------------------------------------------------------------

        for ( FileRegistry::FileInfo const* pFileInfo : files )
        {
            if ( DoesFileMatchFilter( pFileInfo, true ) )
            {
                m_fileList.emplace_back( *pFileInfo );
                m_sortedFileListIndices.emplace_back( (int32_t) m_fileList.size() - 1 );
            }
        }

        SortFileList();

        //-------------------------------------------------------------------------

        if ( previouslySelectedFile.IsValid() )
        {
            SetSelectedFile( previouslySelectedFile, true );
        }
    }

    void ResourceBrowserEditorTool::SortFileList()
    {
        auto sortPredicate = [this] ( int32_t a, int32_t b )
        {
            FileRegistry::FileInfo const& lhs = m_fileList[a];
            FileRegistry::FileInfo const& rhs = m_fileList[b];

            switch ( m_sortRule )
            {
                case SortRule::TypeAscending:
                {
                    if ( lhs.m_extension == rhs.m_extension )
                    {
                        return lhs.m_dataPath.GetString() < rhs.m_dataPath.GetString();
                    }
                    else
                    {
                        return lhs.m_extension < rhs.m_extension;
                    }
                }
                break;

                case SortRule::TypeDescending:
                {
                    if ( lhs.m_extension == rhs.m_extension )
                    {
                        return lhs.m_dataPath.GetString() < rhs.m_dataPath.GetString();
                    }
                    else
                    {
                        return lhs.m_extension > rhs.m_extension;
                    }
                }
                break;

                case SortRule::NameAscending:
                {
                    return lhs.m_dataPath.GetString() < rhs.m_dataPath.GetString();
                }
                break;

                case SortRule::NameDescending:
                {
                    return lhs.m_dataPath.GetString() > rhs.m_dataPath.GetString();
                }
                break;
            }

            return a < b;
        };

        eastl::sort( m_sortedFileListIndices.begin(), m_sortedFileListIndices.end(), sortPredicate );
    }

    void ResourceBrowserEditorTool::SetSelectedFile( DataPath const& filePath, bool setFocus )
    {
        m_selectedFile.Clear();

        if ( !filePath.IsValid() || filePath.IsDirectoryPath() )
        {
            return;
        }

        for ( auto const& fileInfo : m_fileList )
        {
            if ( fileInfo.m_dataPath == m_navigationRequest.m_path )
            {
                m_selectedFile = m_navigationRequest.m_path;
                m_setFocusToSelectedItem = setFocus;
                break;
            }
        }
    }

    void ResourceBrowserEditorTool::DrawFileView( UpdateContext const& context )
    {
        if ( ImGui::BeginTable( "TableItems", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable, ImGui::GetContentRegionAvail() - ImVec2( 0, 1 ) ) )
        {
            ImGui::TableSetupScrollFreeze( 0, 1 );
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortAscending | ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_PreferSortAscending | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 50 );
            ImGui::TableHeadersRow();

            // Sort table contents
            //-------------------------------------------------------------------------

            if ( ImGuiTableSortSpecs* pSortSpecs = ImGui::TableGetSortSpecs() )
            {
                if ( pSortSpecs->SpecsDirty )
                {
                    bool const isAscending = pSortSpecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
                    if ( pSortSpecs->Specs[0].ColumnIndex == 1 )
                    {
                        m_sortRule = isAscending ? SortRule::TypeAscending : SortRule::TypeDescending;
                    }
                    else if ( pSortSpecs->Specs[0].ColumnIndex == 2 )
                    {
                        m_sortRule = isAscending ? SortRule::NameAscending : SortRule::NameDescending;
                    }

                    SortFileList();
                    pSortSpecs->SpecsDirty = false;
                }
            }

            // Items
            //-------------------------------------------------------------------------

            for ( int32_t const fileInfoIdx : m_sortedFileListIndices )
            {
                FileRegistry::FileInfo const& fileInfo = m_fileList[fileInfoIdx];

                if ( !m_showRawFiles && fileInfo.m_fileType == FileRegistry::FileType::Unknown )
                {
                    continue;
                }

                ImGui::PushID( fileInfoIdx );

                // Filename
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                if ( fileInfo.IsResourceDescriptorFile() )
                {
                    ImGui::Text( EE_ICON_FILE );
                }
                else if ( fileInfo.IsEntityDescriptorFile() )
                {
                    ImGui::Text( EE_ICON_FILE_IMAGE_OUTLINE );
                }
                else if ( fileInfo.IsDataFile() )
                {
                    ImGui::Text( EE_ICON_FILE_TABLE_OUTLINE );
                }
                else
                {
                    ImGui::TextColored( Colors::LightBlue.ToFloat4(), EE_ICON_FILE_IMPORT_OUTLINE );
                }

                ImGui::SameLine();

                bool const isSelected = ( m_selectedFile == fileInfo.m_dataPath );
                if ( isSelected )
                {
                    ImGui::PushStyleColor( ImGuiCol_Header, ImGuiX::Style::s_colorAccent2 );
                    ImGui::PushStyleColor( ImGuiCol_HeaderHovered, Color( ImGuiX::Style::s_colorAccent2 ).GetScaledColor( 0.8f ) );

                    if ( m_setFocusToSelectedItem )
                    {
                        ImGui::SetKeyboardFocusHere();
                        m_setFocusToSelectedItem = false;
                    }
                }

                InlineString const selectableLabel( InlineString::CtorSprintf(), "%s##%u", fileInfo.m_filePath.GetFilename().c_str(), fileInfo.m_dataPath.GetID() );
                if ( ImGui::Selectable( selectableLabel.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick ) )
                {
                    m_selectedFile = fileInfo.m_dataPath;

                    if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                    {
                        if ( fileInfo.IsResourceDescriptorFile() || fileInfo.IsEntityDescriptorFile() )
                        {
                            m_pToolsContext->TryOpenResource( fileInfo.GetResourceID() );
                        }
                        else if ( fileInfo.IsDataFile() )
                        {
                            m_pToolsContext->TryOpenDataFile( fileInfo.m_dataPath );
                        }
                    }
                }

                if ( isSelected )
                {
                    ImGui::PopStyleColor( 2 );
                }

                if ( ImGui::IsItemClicked( ImGuiMouseButton_Right ) )
                {
                    ImGui::OpenPopup( "FileContextMenu" );
                }

                if ( ImGui::BeginDragDropSource( ImGuiDragDropFlags_None ) )
                {
                    ImGui::SetDragDropPayload( DragAndDrop::s_filePayloadID, (void*) fileInfo.m_dataPath.c_str(), fileInfo.m_dataPath.GetString().length() + 1 );
                    ImGui::Text( fileInfo.m_filePath.GetFilename().c_str() );
                    ImGui::EndDragDropSource();
                }

                // Type
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::Text( fileInfo.m_extension.c_str() );

                // Context Menu
                //-------------------------------------------------------------------------

                if( ImGui::BeginPopup( "FileContextMenu" ) )
                {
                    if ( ImGui::MenuItem( "Open In Explorer" ) )
                    {
                        Platform::Win32::OpenInExplorer( fileInfo.m_filePath );
                    }

                    if ( ImGui::MenuItem( "Copy File Path" ) )
                    {
                        ImGui::SetClipboardText( fileInfo.m_filePath.c_str() );
                    }

                    if ( ImGui::MenuItem( "Copy Resource Path" ) )
                    {
                        ImGui::SetClipboardText( fileInfo.m_filePath.c_str() );
                    }

                    ImGui::Separator();

                    if ( ImGui::MenuItem( EE_ICON_ALERT_OCTAGON" Delete" ) )
                    {
                        m_dialogManager.CreateModalDialog( "Delete", [this, &fileInfo] ( UpdateContext const& context )
                            { return DrawDeleteFileConfirmationDialog( context, fileInfo ); } );
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }
}