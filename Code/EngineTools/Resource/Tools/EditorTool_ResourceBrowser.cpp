#include "EditorTool_ResourceBrowser.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/FileSystem/FileRegistry.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Core/CommonToolTypes.h"
#include "EngineTools/Core/DialogManager.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/TypeSystem/DataFileInfo.h"
#include "Base/TypeSystem/ResourceInfo.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include <eastl/sort.h>
#include "EngineTools/Resource/Dialogs/EditorDialog_FileSystemAction.h"

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
            , m_dataPath( pDirectoryEntry->m_dataPath )
            , m_isRootPath( m_dataPath.IsRootPath() )
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

        virtual uint64_t GetUniqueID() const override { return m_dataPath.GetID(); }
        virtual bool HasContextMenu() const override { return true; }
        virtual bool IsActivatable() const override { return false; }

        virtual bool HasIcon() const override { return true; }

        virtual char const* GetIcon() const override 
        {
            return m_isRootPath ? EE_ICON_DATABASE : EE_ICON_FOLDER;
        }

        virtual Color GetIconColor() const override
        {
            return m_isRootPath ? Colors::LightSkyBlue : Colors::SandyBrown;
        }

        virtual char const* GetDisplayName() const override { return m_nameID.c_str(); }

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
        bool                                    m_isRootPath = false;
    };

    //-------------------------------------------------------------------------

    ResourceBrowserEditorTool::ResourceBrowserEditorTool( ToolsContext const* pToolsContext )
        : EditorTool( pToolsContext, "Resource Browser" )
        , m_dataDirectoryPathDepth( m_pToolsContext->GetSourceDataDirectory().GetPathDepth() )
    {
        m_directoryTreeView.SetFlag( TreeListView::ExpandItemsOnlyViaArrow, true );
        m_directoryTreeView.SetFlag( TreeListView::ExpandItemsWithDoubleClick, true );
        m_directoryTreeView.SetFlag( TreeListView::UseSmallFont, false );
        m_directoryTreeView.SetFlag( TreeListView::SortTree, true );
        m_directoryTreeView.SetFlag( TreeListView::ShowBranchesFirst, false );

        m_filter.SetFilterHelpText( "Search..." );

        // Rebuild the tree whenever the file registry updates
        m_resourceDatabaseUpdateEventBindingID = m_pToolsContext->m_pFileRegistry->OnFileSystemCacheUpdated().Bind( [this] () { m_refreshRequested = true; } );

        // Create descriptor category tree
        //-------------------------------------------------------------------------

        TypeSystem::TypeRegistry const* pTypeRegistry = m_pToolsContext->m_pTypeRegistry;

        TVector<TypeSystem::TypeInfo const*> descriptorTypeInfos = pTypeRegistry->GetAllDerivedTypes( Resource::ResourceDescriptor::GetStaticTypeID(), false, false );
        for ( auto pTypeInfo : descriptorTypeInfos )
        {
            auto pRD = pTypeInfo->GetDefaultInstance<Resource::ResourceDescriptor>();
            if ( !pRD->IsUserCreateableDescriptor() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            String category = pTypeInfo->m_ID.c_str();
            StringUtils::ReplaceAllOccurrencesInPlace( category, "::", "/" );
            StringUtils::ReplaceAllOccurrencesInPlace( category, "EE/", "" );
            size_t const foundIdx = category.find_last_of( '/' );
            if ( foundIdx != String::npos )
            {
                category = category.substr( 0, foundIdx );
            }
            else
            {
                category.clear();
            }

            auto pResourceInfo = pTypeRegistry->GetResourceInfo( pRD->GetCompiledResourceTypeID() );
            EE_ASSERT( pResourceInfo != nullptr );
            m_categorizedDescriptorTypes.AddItem( category, pResourceInfo->m_friendlyName.c_str(), pTypeInfo );
        }

        THashMap<TypeSystem::TypeID, TypeSystem::DataFileInfo*> const& dataFileInfos = pTypeRegistry->GetRegisteredDataFileTypes();
        for ( auto const& pair : dataFileInfos )
        {
            TypeSystem::TypeInfo const* pTypeInfo = pTypeRegistry->GetTypeInfo( pair.second->m_typeID );

            //-------------------------------------------------------------------------

            String category = pTypeInfo->m_ID.c_str();
            StringUtils::ReplaceAllOccurrencesInPlace( category, "::", "/" );
            StringUtils::ReplaceAllOccurrencesInPlace( category, "EE/", "" );
            size_t const foundIdx = category.find_last_of( '/' );
            EE_ASSERT( foundIdx != String::npos );
            category = category.substr( 0, foundIdx );

            m_categorizedDescriptorTypes.AddItem( category, pair.second->m_friendlyName.c_str(), pTypeInfo );
        }
    }

    ResourceBrowserEditorTool::~ResourceBrowserEditorTool()
    {
        m_pToolsContext->m_pFileRegistry->OnFileSystemCacheUpdated().Unbind( m_resourceDatabaseUpdateEventBindingID );
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
            typeFilter.m_color = resourceInfo.second->m_color;
            m_allPossibleTypeFilters.emplace_back( typeFilter );
        }

        for ( auto const& dataFileInfo : m_pToolsContext->m_pTypeRegistry->GetRegisteredDataFileTypes() )
        {
            TypeFilter typeFilter;
            typeFilter.m_friendlyName = dataFileInfo.second->m_friendlyName;
            typeFilter.m_resourceTypeID = ResourceTypeID();
            typeFilter.m_extension = dataFileInfo.second->m_extension.ToFileSystemExtension();
            typeFilter.m_color = Colors::LightBlue;
            m_allPossibleTypeFilters.emplace_back( typeFilter );
        }

        auto SortPredicate = [] ( TypeFilter const& a, TypeFilter const& b )
        {
            return a.m_extension < b.m_extension;
        };

        eastl::sort( m_allPossibleTypeFilters.begin(), m_allPossibleTypeFilters.end(), SortPredicate );

        m_navigationRequest.m_path = DataPath( DataPath::s_pathPrefix );

        m_refreshRequested = true;
    }

    void ResourceBrowserEditorTool::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        if ( m_pToolsContext->m_pFileRegistry->IsBuildingCaches() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        TreeListViewContext treeViewContext;
        treeViewContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildDirectoryTreeView( pRootItem ); };
        treeViewContext.m_drawItemContextMenuFunction = [this] ( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) { DrawDirectoryTreeContextMenu( selectedItemsWithContextMenus ); };

        if ( m_refreshRequested )
        {
            m_directoryTreeView.RebuildTree( treeViewContext, true );
            GenerateDirectoryContentsList();
            m_refreshRequested = false;
            m_filterUpdated = false;
        }
        else if ( m_filterUpdated )
        {
            GenerateDirectoryContentsList();
            m_filterUpdated = false;
        }

        //-------------------------------------------------------------------------

        if ( m_navigationRequest.IsSet() )
        {
            HandleNavigationRequest();
            m_navigationRequest.Clear();
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
            // Folders
            //-------------------------------------------------------------------------

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 0 ) );
            ImGui::BeginChild( "left pane", ImVec2( 150, 0 ), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX | ImGuiChildFlags_AlwaysUseWindowPadding );
            ImGui::PopStyleVar();
            {
                {
                    ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );

                    float const importButtonWidth = ImGuiX::CalculateButtonWidth( EE_ICON_IMPORT" IMPORT" );
                    float const createButtonWidth = ImGui::GetContentRegionAvail().x - importButtonWidth - ImGui::GetStyle().ItemSpacing.x;
                    if ( ImGuiX::IconButtonColored( EE_ICON_PLUS, "CREATE", Colors::Green, Colors::White, Colors::White, ImVec2( createButtonWidth, 0 ) ) )
                    {
                        ImGui::OpenPopup( "CreateNewDescriptor" );
                    }
                    ImGuiX::ItemTooltip( "Create a new resource descriptor and fill out the settings by hand! This is how you create resources that dont have a source file." );

                    if ( ImGui::BeginPopup( "CreateNewDescriptor" ) )
                    {
                        DrawCreateDescriptorMenuCategory( m_pToolsContext->GetSourceDataDirectory(), m_categorizedDescriptorTypes.GetRootCategory() );
                        ImGui::EndPopup();
                    }
                    ImGuiX::ItemTooltip( "Create a resource descriptor based on a source file. This will pop up a helper window to help you with the creation." );

                    //-------------------------------------------------------------------------

                    ImGui::SameLine();
                    if ( ImGuiX::IconButton( EE_ICON_IMPORT, "IMPORT", Colors::White ) )
                    {
                        m_pToolsContext->OpenResourceImporter();
                    }
                }

                //-------------------------------------------------------------------------

                DrawDirectoryView( context );
            }
            ImGui::EndChild();
            ImGui::SameLine( 0, 0 );

            // Files
            //-------------------------------------------------------------------------

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 0 ) );
            ImGui::BeginChild( "item view", ImVec2( 0, 0 ), ImGuiChildFlags_AlwaysUseWindowPadding );
            ImGui::PopStyleVar();
            {
                if ( ImGuiX::ToggleButton( EE_ICON_RAW, EE_ICON_RAW_OFF, m_showRawFiles, ImVec2( ImGuiX::Style::s_iconButtonWidth, 0 ) ) )
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
                ImGuiX::DropDownIconButton( EE_ICON_FILTER, "##Resource Filters", DrawFiltersMenu, ImGuiX::Style::s_colorText, ImVec2( ImGuiX::Style::s_iconButtonWidth, 0 ) );

                ImGui::SameLine();
                m_filterUpdated |= m_filter.UpdateAndDraw();

                DrawResourceTypeFilterRow( context );

                //-------------------------------------------------------------------------

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

        // Select folder
        //-------------------------------------------------------------------------

        DataPath const directoryPath = m_navigationRequest.m_path.IsDirectoryPath() ? m_navigationRequest.m_path : m_navigationRequest.m_path.GetParentDirectory();
        if ( !directoryPath.IsValid() )
        {
            return;
        }

        auto pFoundItem = m_directoryTreeView.FindItem( directoryPath.GetID() );
        if ( pFoundItem == nullptr )
        {
            return;
        }

        m_directoryTreeView.SetSelection( pFoundItem );
        m_directoryTreeView.SetViewToSelection();
        SetSelectedDirectory( directoryPath );

        // Select file
        //-------------------------------------------------------------------------

        if ( m_navigationRequest.m_path.IsFilePath() )
        {
            SetSelectedFile( m_navigationRequest.m_path, true );
        }
    }

    //-------------------------------------------------------------------------

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
            DrawCreateDescriptorMenuCategory( m_pToolsContext->GetSourceDataDirectory(), m_categorizedDescriptorTypes.GetRootCategory() );
            ImGui::EndPopup();
        }
        ImGuiX::ItemTooltip( "Create a resource descriptor based on a source file. This will pop up a helper window to help you with the creation." );
    }

    void ResourceBrowserEditorTool::DrawCreateDescriptorMenuCategory( FileSystem::Path const& startingPath, Category<TypeSystem::TypeInfo const*> const& category )
    {
        auto DrawItems = [this, &startingPath, &category] ()
        {
            for ( auto const& childCategory : category.m_childCategories )
            {
                DrawCreateDescriptorMenuCategory( startingPath, childCategory );
            }

            for ( auto const& item : category.m_items )
            {
                if ( ImGui::MenuItem( item.m_name.c_str() ) )
                {
                    m_pToolsContext->TryCreateNewResourceDescriptorOrDataFile( item.m_data->m_ID, startingPath );
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

    void ResourceBrowserEditorTool::DrawResourceTypeFilterRow( UpdateContext const& context )
    {
        ImGuiX::ScopedFont sf( ImGuiX::Font::SmallBold );
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 2 ) );

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
            if ( ImGuiX::ButtonColored( filter.m_extension.c_str(), filter.m_color, Colors::Black ) )
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

        ImGui::PopStyleVar();
    }

    // Directories
    //-------------------------------------------------------------------------

    void ResourceBrowserEditorTool::RebuildDirectoryTreeView( TreeListViewItem* pRootItem )
    {
        EE_ASSERT( m_pToolsContext->m_pFileRegistry->IsFileSystemCacheBuilt() );
        auto pDataDirectory = m_pToolsContext->m_pFileRegistry->GetRawResourceDirectoryEntry();

        //-------------------------------------------------------------------------

        pRootItem->DestroyChildren();
        auto pDataDirItem = pRootItem->CreateChild<ResourceBrowserTreeItem>( *m_pToolsContext, pDataDirectory );
        pDataDirItem->SetExpanded( false, true );
        pDataDirItem->SetExpanded( true, false );
    }

    void ResourceBrowserEditorTool::SetSelectedDirectory( DataPath const& newFolderPath )
    {
        m_selectedDirectory = newFolderPath;
        m_selectedItem.Clear();
        GenerateDirectoryContentsList();
    }

    void ResourceBrowserEditorTool::DrawDirectoryView( UpdateContext const& context )
    {
        TreeListViewContext treeViewContext;
        treeViewContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildDirectoryTreeView( pRootItem ); };
        treeViewContext.m_drawItemContextMenuFunction = [this] ( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) { DrawDirectoryTreeContextMenu( selectedItemsWithContextMenus ); };

        if ( m_directoryTreeView.UpdateAndDraw( treeViewContext ) )
        {
            TVector<TreeListViewItem*> const& selection = m_directoryTreeView.GetSelection();
            if ( selection.empty() )
            {
                SetSelectedDirectory( DataPath() );
            }
            else
            {
                SetSelectedDirectory( static_cast<ResourceBrowserTreeItem const*>( selection.back() )->GetDataPath() );
            }
        }
    }

    void ResourceBrowserEditorTool::DrawDirectoryTreeContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus )
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
            DrawCreateDescriptorMenuCategory( pResourceItem->GetFilePath(), m_categorizedDescriptorTypes.GetRootCategory() );
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

    void ResourceBrowserEditorTool::GenerateDirectoryContentsList()
    {
        m_fileList.clear();
        m_directoryList.clear();
        m_sortedFileListIndices.clear();

        DataPath const previouslySelectedFile = m_selectedItem;

        //-------------------------------------------------------------------------

        TVector<FileRegistry::FileInfo const*> files;
        files.reserve( 25000 );
        if ( m_filter.HasFilterSet() )
        {
            FileRegistry::DirectoryInfo const* pDirectoryInfo = m_pToolsContext->m_pFileRegistry->FindDirectoryEntry( m_selectedDirectory );
            if ( pDirectoryInfo == nullptr )
            {
                pDirectoryInfo = m_pToolsContext->m_pFileRegistry->FindDirectoryEntry( DataPath( "Data://" ) );
            }

            pDirectoryInfo->GetAllFiles( files, true );
        }
        else
        {
            FileRegistry::DirectoryInfo const* pDirectoryInfo = m_pToolsContext->m_pFileRegistry->FindDirectoryEntry( m_selectedDirectory );
            if ( pDirectoryInfo != nullptr )
            {
                for ( FileRegistry::FileInfo const* pFileInfo : pDirectoryInfo->m_files )
                {
                    files.emplace_back( pFileInfo );
                }

                for ( FileRegistry::DirectoryInfo const &subDir : pDirectoryInfo->m_directories )
                {
                    m_directoryList.emplace_back( subDir );
                }
            }

            auto SortPredicate = [] ( FileRegistry::DirectoryInfo const& a, FileRegistry::DirectoryInfo const& b )
            {
                return _stricmp( a.m_dataPath.c_str(), b.m_dataPath.c_str() ) < 0;
            };

            eastl::sort( m_directoryList.begin(), m_directoryList.end(), SortPredicate );
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
        auto SortPredicate = [this] ( int32_t a, int32_t b )
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

        eastl::sort( m_sortedFileListIndices.begin(), m_sortedFileListIndices.end(), SortPredicate );
    }

    void ResourceBrowserEditorTool::SetSelectedFile( DataPath const& filePath, bool setFocus )
    {
        m_selectedItem.Clear();

        if ( !filePath.IsValid() || filePath.IsDirectoryPath() )
        {
            return;
        }

        for ( auto const& fileInfo : m_fileList )
        {
            if ( fileInfo.m_dataPath == m_navigationRequest.m_path )
            {
                m_selectedItem = m_navigationRequest.m_path;
                m_setFocusToSelectedItem = setFocus;
                break;
            }
        }
    }

    void ResourceBrowserEditorTool::DrawFileView( UpdateContext const& context )
    {
        bool itemContextMenuRequested = false;
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 8, 0 ) );
        if ( ImGui::BeginTable( "TableItems", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable, ImGui::GetContentRegionAvail() - ImVec2( 0, 1 ) ) )
        {
            ImGui::TableSetupScrollFreeze( 0, 1 );
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortAscending | ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_PreferSortAscending | ImGuiTableColumnFlags_WidthFixed );

            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 8, 8 ) );
            ImGui::TableHeadersRow();
            ImGui::PopStyleVar();

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
                    else if ( pSortSpecs->Specs[0].ColumnIndex == 0 )
                    {
                        m_sortRule = isAscending ? SortRule::NameAscending : SortRule::NameDescending;
                    }

                    SortFileList();
                    pSortSpecs->SpecsDirty = false;
                }
            }

            // Up
            //-------------------------------------------------------------------------

            bool const isTextFilterSet = m_filter.HasFilterSet();
            if ( m_selectedDirectory.IsValid() && m_selectedDirectory != DataPath( "Data://" ) && !isTextFilterSet )
            {
                DataPath const parentPath = m_selectedDirectory.GetParentDirectory();
                bool const isSelected = ( m_selectedItem == parentPath );

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();

                ImGui::Text( EE_ICON_ARROW_UP_LEFT_BOLD );
                ImGui::SameLine();
                InlineString const selectableLabel( InlineString::CtorSprintf(), "...##%u", parentPath.GetID() );
                if ( ImGui::Selectable( selectableLabel.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick ) )
                {
                    m_selectedItem = parentPath;

                    if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                    {
                        m_navigationRequest.m_path = parentPath;
                    }
                }

                ImGui::TableNextColumn();
                ImGui::Text( "Directory" );
            }

            // Directory
            //-------------------------------------------------------------------------

            for ( FileRegistry::DirectoryInfo const& directoryInfo : m_directoryList )
            {
                bool const isSelected = ( m_selectedItem == directoryInfo.m_dataPath );

                ImGui::PushID( directoryInfo.m_dataPath.c_str() );

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();

                ImGui::TextColored( Colors::SandyBrown, EE_ICON_FOLDER );

                if ( isSelected )
                {
                    ImGui::PushStyleColor( ImGuiCol_Text, ImGuiX::Style::s_colorAccent1 );
                }

                ImGui::SameLine();
                InlineString const selectableLabel( InlineString::CtorSprintf(), "%s##%u", directoryInfo.m_dataPath.GetDirectoryName().c_str(), directoryInfo.m_dataPath.GetID() );
                if ( ImGui::Selectable( selectableLabel.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick ) )
                {
                    m_selectedItem = directoryInfo.m_dataPath;

                    if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                    {
                        m_navigationRequest.m_path = directoryInfo.m_dataPath;
                    }
                }

                if ( isSelected )
                {
                    ImGui::PopStyleColor( 1 );
                }

                if ( ImGui::IsItemClicked( ImGuiMouseButton_Right ) )
                {
                    ImGui::OpenPopup( s_directoryInfoContextMenu );
                    m_selectedItem = directoryInfo.m_dataPath;
                    itemContextMenuRequested = true;
                }

                ImGui::TableNextColumn();
                ImGui::Text( "Directory" );

                // Directory Context Menu
                //-------------------------------------------------------------------------

                if ( ImGui::BeginPopup( s_directoryInfoContextMenu ) )
                {
                    DrawDirectoryInfoContextMenu( directoryInfo, true );
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }

            // Files
            //-------------------------------------------------------------------------

            for ( int32_t const fileInfoIdx : m_sortedFileListIndices )
            {
                FileRegistry::FileInfo const& fileInfo = m_fileList[fileInfoIdx];

                if ( !m_showRawFiles && fileInfo.m_fileType == FileRegistry::FileType::Unknown )
                {
                    continue;
                }

                ImGui::PushID( fileInfoIdx );
                bool const isSelected = ( m_selectedItem == fileInfo.m_dataPath );

                // Filename
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();

                auto const pResourceInfo = fileInfo.IsResourceDescriptorFile() ? m_pToolsContext->m_pTypeRegistry->GetResourceInfo( fileInfo.GetResourceTypeID() ) : nullptr;
                if ( fileInfo.IsResourceDescriptorFile() )
                {
                    ImGui::TextColored( pResourceInfo->m_color.ToFloat4(), EE_ICON_FILE );
                }
                else if ( fileInfo.IsDataFile() )
                {
                    ImGui::TextColored( Colors::LightBlue.ToFloat4(), EE_ICON_CUBE_OUTLINE );
                }
                else
                {
                    ImGui::Text( EE_ICON_FILE_OUTLINE );
                }

                if ( isSelected )
                {
                    ImGui::PushStyleColor( ImGuiCol_Text, ImGuiX::Style::s_colorAccent1 );

                    if ( m_setFocusToSelectedItem )
                    {
                        ImGui::SetKeyboardFocusHere();
                        m_setFocusToSelectedItem = false;
                    }
                }

                ImGui::SameLine();
                InlineString const selectableLabel( InlineString::CtorSprintf(), "%s##%u", isTextFilterSet ? fileInfo.m_dataPath.GetString().substr( 7 ).c_str() : fileInfo.m_filePath.GetFilename().c_str(), fileInfo.m_dataPath.GetID() );
                if ( ImGui::Selectable( selectableLabel.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick ) )
                {
                    m_selectedItem = fileInfo.m_dataPath;

                    if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                    {
                        if ( fileInfo.IsResourceDescriptorFile() )
                        {
                            m_pToolsContext->TryOpenResource( fileInfo.GetResourceID() );
                        }
                        else if ( fileInfo.IsDataFile() )
                        {
                            m_pToolsContext->TryOpenDataFile( fileInfo.m_dataPath );
                        }
                    }
                }

                if ( ImGui::BeginDragDropSource( ImGuiDragDropFlags_None ) )
                {
                    ImGui::SetDragDropPayload( DragAndDrop::s_filePayloadID, (void*) fileInfo.m_dataPath.c_str(), fileInfo.m_dataPath.GetString().length() + 1 );
                    ImGui::Text( fileInfo.m_filePath.GetFilename().c_str() );
                    ImGui::EndDragDropSource();
                }

                if ( isSelected )
                {
                    ImGui::PopStyleColor( 1 );
                }

                if ( ImGui::IsItemClicked( ImGuiMouseButton_Right ) )
                {
                    m_selectedItem = fileInfo.m_dataPath;
                    ImGui::OpenPopup( s_fileInfoContextMenu );
                    itemContextMenuRequested = true;
                }

                // Type
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();

                if ( fileInfo.IsResourceDescriptorFile() )
                {
                    ImGui::Text( pResourceInfo->m_friendlyName.c_str() );
                }
                else if ( fileInfo.IsDataFile() )
                {
                    ImGui::Text( "Data File" );
                }
                else
                {
                    ImGui::Text( fileInfo.m_extension.c_str() );
                }

                // File Context Menu
                //-------------------------------------------------------------------------

                if( ImGui::BeginPopup( s_fileInfoContextMenu ) )
                {
                    DrawFileInfoContextMenu( fileInfo );
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }

            ImGui::EndTable();

            // Parent Folder Context Menu
            //-------------------------------------------------------------------------

            if ( !itemContextMenuRequested && ImGui::IsItemClicked( ImGuiMouseButton_Right ) )
            {
                ImGui::OpenPopup( s_directoryInfoContextMenu );
            }

            if ( ImGui::BeginPopup( s_directoryInfoContextMenu ) )
            {
                FileRegistry::DirectoryInfo const* pDirectoryInfo = m_pToolsContext->m_pFileRegistry->FindDirectoryEntry( m_selectedDirectory );
                if ( pDirectoryInfo != nullptr )
                {
                    DrawDirectoryInfoContextMenu( *pDirectoryInfo, false );
                }
                else
                {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }
        ImGui::PopStyleVar();
    }

    void ResourceBrowserEditorTool::DrawDirectoryInfoContextMenu( FileRegistry::DirectoryInfo const& directoryInfo, bool isFileListView )
    {
        if ( ImGui::MenuItem( EE_ICON_OPEN_IN_NEW" Open In Explorer" ) )
        {
            Platform::Win32::OpenInExplorer( directoryInfo.m_filePath );
        }

        if ( ImGui::MenuItem( EE_ICON_FILE" Copy File Path" ) )
        {
            ImGui::SetClipboardText( directoryInfo.m_filePath );
        }

        if ( ImGui::MenuItem( EE_ICON_DATABASE" Copy Data Path" ) )
        {
            ImGui::SetClipboardText( directoryInfo.m_dataPath.c_str() );
        }

        //-------------------------------------------------------------------------

        if ( !isFileListView )
        {
            ImGui::Separator();

            if ( ImGui::BeginMenu( "Create New Descriptor" ) )
            {
                DrawCreateDescriptorMenuCategory( directoryInfo.m_filePath, m_categorizedDescriptorTypes.GetRootCategory() );
                ImGui::EndMenu();
            }
        }

        //-------------------------------------------------------------------------

        ImGui::Separator();

        if ( ImGui::MenuItem( EE_ICON_RENAME" Rename" ) )
        {
            m_pToolsContext->m_pDialogManager->StartModalDialog<FileSystemActionDialog>( FileSystemActionDialog::Action::Rename, m_pToolsContext, directoryInfo.m_dataPath );
        }

        if ( ImGui::MenuItem( EE_ICON_ALERT_OCTAGON" Delete" ) )
        {
            m_pToolsContext->m_pDialogManager->StartModalDialog<FileSystemActionDialog>( FileSystemActionDialog::Action::Delete, m_pToolsContext, directoryInfo.m_dataPath );
        }
    }

    void ResourceBrowserEditorTool::DrawFileInfoContextMenu( FileRegistry::FileInfo const& fileInfo )
    {
        if ( ImGui::MenuItem( EE_ICON_OPEN_IN_APP" Open" ) )
        {
            if ( fileInfo.IsResourceDescriptorFile() )
            {
                m_pToolsContext->TryOpenResource( fileInfo.GetResourceID() );
            }
            else if ( fileInfo.IsDataFile() )
            {
                m_pToolsContext->TryOpenDataFile( fileInfo.m_dataPath );
            }
        }

        if ( ImGui::MenuItem( EE_ICON_OPEN_IN_NEW" Open In Explorer" ) )
        {
            Platform::Win32::OpenInExplorer( fileInfo.m_filePath );
        }

        ImGui::Separator();

        if ( ImGui::MenuItem( EE_ICON_FILE" Copy File Path" ) )
        {
            ImGui::SetClipboardText( fileInfo.m_filePath.c_str() );
        }

        if ( ImGui::MenuItem( EE_ICON_DATABASE" Copy Data Path" ) )
        {
            ImGui::SetClipboardText( fileInfo.m_dataPath.c_str() );
        }

        ImGui::Separator();

        if ( fileInfo.IsResourceDescriptorFile() )
        {
            if ( ImGui::MenuItem( EE_ICON_GRAPH" Show Dependencies" ) )
            {
                m_pToolsContext->ShowResourceDependencies( fileInfo.GetResourceID() );
            }
        }

        ImGui::Separator();

        if ( ImGui::MenuItem( EE_ICON_RENAME" Rename" ) )
        {
            m_pToolsContext->m_pDialogManager->StartModalDialog<FileSystemActionDialog>( FileSystemActionDialog::Action::Rename, m_pToolsContext, fileInfo.m_dataPath );
        }

        if ( ImGui::MenuItem( EE_ICON_ALERT_OCTAGON" Delete" ) )
        {
            m_pToolsContext->m_pDialogManager->StartModalDialog<FileSystemActionDialog>( FileSystemActionDialog::Action::Delete, m_pToolsContext, fileInfo.m_dataPath );
        }
    }
}