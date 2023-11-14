#include "EditorTool_ResourceBrowser.h"
#include "EngineTools/Resource/ResourceDescriptorCreator.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Core/CommonDialogs.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "EngineTools/Resource/ResourceToolDefines.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/Profiling.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include <eastl/sort.h>

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceBrowserTreeItem final : public TreeListViewItem
    {
    public:

        enum class Type
        {
            Directory = 0,
            File,
        };

    public:

        ResourceBrowserTreeItem( TreeListViewItem* pParent, ToolsContext const& toolsContext, Resource::ResourceDatabase::DirectoryEntry const* pDirectoryEntry )
            : TreeListViewItem( pParent )
            , m_toolsContext( toolsContext )
            , m_nameID( pDirectoryEntry->m_filePath.GetDirectoryName() )
            , m_path( pDirectoryEntry->m_filePath )
            , m_resourcePath( pDirectoryEntry->m_resourcePath )
            , m_type( Type::Directory )
        {
            EE_ASSERT( m_path.IsValid() );
            EE_ASSERT( m_resourcePath.IsValid() );

            //-------------------------------------------------------------------------

            for ( auto const& childDirectory : pDirectoryEntry->m_directories )
            {
                CreateChild<ResourceBrowserTreeItem>( m_toolsContext, &childDirectory );
            }

            //-------------------------------------------------------------------------

            for ( auto pChildFile : pDirectoryEntry->m_files )
            {
                CreateChild<ResourceBrowserTreeItem>( m_toolsContext, pChildFile );
            }

        }

        ResourceBrowserTreeItem( TreeListViewItem* pParent, ToolsContext const& toolsContext, Resource::ResourceDatabase::FileEntry const* pFileEntry )
            : TreeListViewItem( pParent )
            , m_toolsContext( toolsContext )
            , m_nameID( pFileEntry->m_filePath.GetFilename() )
            , m_path( pFileEntry->m_filePath )
            , m_resourcePath( pFileEntry->m_resourceID.GetResourcePath() )
            , m_resourceTypeID( pFileEntry->m_isRegisteredResourceType ? pFileEntry->m_resourceID.GetResourceTypeID() : ResourceTypeID() )
            , m_type( Type::File )
        {
            EE_ASSERT( m_path.IsValid() );
            EE_ASSERT( m_resourcePath.IsValid() );
        }

        virtual StringID GetNameID() const { return m_nameID; }
        virtual uint64_t GetUniqueID() const override { return m_resourcePath.GetID(); }
        virtual bool HasContextMenu() const override { return true; }
        virtual bool IsActivatable() const override { return false; }
        virtual bool IsLeaf() const override { return IsFile(); }

        virtual InlineString GetDisplayName() const override
        {
            InlineString displayName;

            if ( IsDirectory() )
            {
                displayName.sprintf( EE_ICON_FOLDER" %s", GetNameID().c_str() );
            }
            else if ( IsResourceFile() )
            {
                displayName.sprintf( EE_ICON_FILE" %s", GetNameID().c_str() );
            }
            else if ( IsRawFile() )
            {
                displayName.sprintf( EE_ICON_FILE_OUTLINE" %s", GetNameID().c_str() );
            }

            return displayName;
        }

        virtual Color GetDisplayColor( ItemState state ) const override
        {
            if ( IsFile() && IsRawFile() && ( state == TreeListViewItem::None ) )
            {
                return Colors::LightBlue;
            }
            else
            {
                return TreeListViewItem::GetDisplayColor( state );
            }
        }

        // File Info
        //-------------------------------------------------------------------------

        inline bool IsFile() const { return m_type == Type::File; }
        inline bool IsDirectory() const { return m_type == Type::Directory; }
        inline FileSystem::Path const& GetFilePath() const { return m_path; }
        inline ResourcePath const& GetResourcePath() const { return m_resourcePath; }

        virtual bool IsDragAndDropSource() const override { return IsFile() && IsResourceFile(); }
        virtual void SetDragAndDropPayloadData() const override { ImGui::SetDragDropPayload( Resource::DragAndDrop::s_payloadID, (void*) m_resourcePath.c_str(), m_resourcePath.GetString().length() + 1 ); }

        // Resource Info
        //-------------------------------------------------------------------------

        inline bool IsRawFile() const { EE_ASSERT( IsFile() ); return !m_resourceTypeID.IsValid(); }
        inline bool IsResourceFile() const { EE_ASSERT( IsFile() ); return m_resourceTypeID.IsValid(); }
        inline ResourceID GetResourceID() const { EE_ASSERT( IsResourceFile() ); return ResourceID( m_resourcePath ); }
        inline ResourceTypeID const& GetResourceTypeID() const { EE_ASSERT( IsFile() ); return m_resourceTypeID; }

        template<typename T>
        inline bool IsResourceOfType() const
        {
            static_assert( std::is_base_of<Resource::IResource, T>::value, "T must derive from IResource" );
            return m_resourceTypeID == T::GetStaticResourceTypeID();
        }

        // Signals
        //-------------------------------------------------------------------------

        bool OnUserActivate()
        {
            if ( IsDirectory() )
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
            else // Files
            {
                if ( IsResourceFile() )
                {
                    m_toolsContext.TryOpenResource( GetResourceID() );
                }
            }

            return false;
        }

        virtual bool OnDoubleClick() override { return OnUserActivate(); }
        virtual bool OnEnterPressed() override { return OnUserActivate(); }

    protected:

        ToolsContext const&                     m_toolsContext;
        StringID                                m_nameID;
        FileSystem::Path                        m_path;
        ResourcePath                            m_resourcePath;
        ResourceTypeID                          m_resourceTypeID;
        Type                                    m_type;
    };

    //-------------------------------------------------------------------------

    ResourceBrowserEditorTool::ResourceBrowserEditorTool( ToolsContext const* pToolsContext )
        : EditorTool( pToolsContext, "Resource Browser" )
        , m_dataDirectoryPathDepth( m_pToolsContext->GetRawResourceDirectory().GetPathDepth() )
    {
        m_treeview.SetFlag( TreeListView::ExpandItemsOnlyViaArrow, true );
        m_treeview.SetFlag( TreeListView::UseSmallFont, false );
        m_treeview.SetFlag( TreeListView::SortTree, true );

        m_resourceDatabaseUpdateEventBindingID = m_pToolsContext->m_pResourceDatabase->OnFileSystemCacheUpdated().Bind( [this] () { m_rebuildTree = true; } );

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

            auto pResourceInfo = pTypeRegistry->GetResourceInfoForResourceType( pRD->GetCompiledResourceTypeID() );
            m_categorizedDescriptorTypes.AddItem( category, pResourceInfo->m_friendlyName.c_str(), pTypeInfo );
        }
    }

    ResourceBrowserEditorTool::~ResourceBrowserEditorTool()
    {
        m_pToolsContext->m_pResourceDatabase->OnFileSystemCacheUpdated().Unbind( m_resourceDatabaseUpdateEventBindingID );
        EE::Delete( m_pResourceDescriptorCreator );
    }

    void ResourceBrowserEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        CreateToolWindow( "Browser", [this] ( UpdateContext const& context, bool isFocused ) { DrawBrowserWindow( context, isFocused ); } );
    }

    void ResourceBrowserEditorTool::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        if ( m_pToolsContext->m_pResourceDatabase->IsBuildingCaches() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        TreeListViewContext treeViewContext;
        treeViewContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildTreeView( pRootItem ); };
        treeViewContext.m_drawItemContextMenuFunction = [this] ( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) { DrawItemContextMenu( selectedItemsWithContextMenus ); };

        if ( m_rebuildTree )
        {
            m_treeview.RebuildTree( treeViewContext, true );
            m_rebuildTree = false;
        }

        //-------------------------------------------------------------------------

        if ( m_navigationRequest.IsValid() )
        {
            auto pFoundItem = m_treeview.FindItem( m_navigationRequest.GetPathID() );
            if ( pFoundItem != nullptr )
            {
                m_treeview.SetSelection( pFoundItem );
                m_treeview.SetViewToSelection();
            }

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
        if ( m_pToolsContext->m_pResourceDatabase->IsBuildingCaches() )
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text( m_pToolsContext->m_pResourceDatabase->IsFileSystemCacheBuilt() ? "Building Descriptor Cache: " : "Building File System Cache: " );
            ImGui::SameLine();
            ImGui::ProgressBar( m_pToolsContext->m_pResourceDatabase->GetProgress() );
        }

        // Draw file system tree
        if ( m_pToolsContext->m_pResourceDatabase->IsFileSystemCacheBuilt() )
        {
            DrawCreationControls( context );
            DrawFilterOptions( context );

            //-------------------------------------------------------------------------

            TreeListViewContext treeViewContext;
            treeViewContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildTreeView( pRootItem ); };
            treeViewContext.m_drawItemContextMenuFunction = [this] ( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) { DrawItemContextMenu( selectedItemsWithContextMenus ); };

            m_treeview.UpdateAndDraw( treeViewContext );
        }
    }

    void ResourceBrowserEditorTool::RebuildTreeView( TreeListViewItem* pRootItem )
    {
        EE_ASSERT( m_pToolsContext->m_pResourceDatabase->IsFileSystemCacheBuilt() );
        auto pDataDirectory = m_pToolsContext->m_pResourceDatabase->GetRawResourceDirectoryEntry();

        //-------------------------------------------------------------------------

        pRootItem->DestroyChildren();

        for ( auto const& childDirectory : pDataDirectory->m_directories )
        {
            pRootItem->CreateChild<ResourceBrowserTreeItem>( *m_pToolsContext, &childDirectory );
        }

        //-------------------------------------------------------------------------

        for ( auto pChildFile : pDataDirectory->m_files )
        {
            pRootItem->CreateChild<ResourceBrowserTreeItem>( *m_pToolsContext, pChildFile );
        }

        //-------------------------------------------------------------------------

        UpdateVisibility();
    }

    void ResourceBrowserEditorTool::UpdateVisibility()
    {
        auto VisibilityFunc = [this] ( TreeListViewItem const* pItem )
        {
            bool isVisible = true;

            // Type filter
            //-------------------------------------------------------------------------

            auto pDataFileItem = static_cast<ResourceBrowserTreeItem const*>( pItem );
            if ( pDataFileItem->IsFile() )
            {
                if ( pDataFileItem->IsRawFile() )
                {
                    isVisible = m_showRawFiles;
                }
                else // Resource file
                {
                    auto const& resourceTypeID = pDataFileItem->GetResourceTypeID();
                    isVisible = m_typeFilter.empty() || VectorContains( m_typeFilter, resourceTypeID );
                }
            }

            // Text filter
            //-------------------------------------------------------------------------

            if ( isVisible )
            {
                isVisible = m_filter.MatchesFilter( pItem->GetDisplayName() );
            }

            //-------------------------------------------------------------------------

            return isVisible;
        };

        //-------------------------------------------------------------------------

        m_treeview.UpdateItemVisibility( VisibilityFunc );
    }

    void ResourceBrowserEditorTool::DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus )
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
            ImGui::SetClipboardText( pResourceItem->GetResourcePath().c_str() );
        }

        // Directory options
        //-------------------------------------------------------------------------

        if ( pResourceItem->GetFilePath().IsDirectoryPath() )
        {
            ImGui::Separator();

            if ( ImGui::BeginMenu( "Create New Descriptor" ) )
            {
                DrawDescriptorMenuCategory( pResourceItem->GetFilePath(), m_categorizedDescriptorTypes.GetRootCategory() );
                ImGui::EndMenu();
            }
        }

        // File options
        //-------------------------------------------------------------------------

        if ( pResourceItem->GetFilePath().IsFilePath() )
        {
            ImGui::Separator();

            if ( ImGui::MenuItem( EE_ICON_ALERT_OCTAGON" Delete" ) )
            {
                m_dialogManager.CreateModalDialog( "Delete Resource", [this] ( UpdateContext const& context ) { return DrawDeleteConfirmationDialog( context ); } );
            }
        }
    }

    //-------------------------------------------------------------------------

    bool ResourceBrowserEditorTool::DrawDeleteConfirmationDialog( UpdateContext const& context )
    {
        bool shouldClose = false;

        ImGui::Text( "Are you sure you want to delete this file?" );
        ImGui::Text( "This cannot be undone!" );

        if ( ImGui::Button( "Ok", ImVec2( 143, 0 ) ) )
        {
            auto pResourceItem = (ResourceBrowserTreeItem*) m_treeview.GetSelection()[0];
            FileSystem::Path const fileToDelete = pResourceItem->GetFilePath();
            m_treeview.ClearSelection();
            FileSystem::EraseFile( fileToDelete );
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
        float const availableWidth = ImGui::GetContentRegionAvail().x;
        float const buttonWidth = ( availableWidth - 4.0f ) / 2.0f;

        if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, EE_ICON_PLUS" CREATE", ImVec2( buttonWidth, 0 ) ) )
        {
            ImGui::OpenPopup( "CreateNewDescriptor" );
        }
        ImGuiX::ItemTooltip( "Create a new resource descriptor and fill out the settings by hand! This is how you create resources that dont have a source file." );

        if ( ImGui::BeginPopup( "CreateNewDescriptor" ) )
        {
            DrawDescriptorMenuCategory( m_pToolsContext->GetRawResourceDirectory(), m_categorizedDescriptorTypes.GetRootCategory() );
            ImGui::EndPopup();
        }
        ImGuiX::ItemTooltip( "Create a resource descriptor based on a source file. This will pop up a helper window to help you with the creation." );

        //-------------------------------------------------------------------------

        ImGui::SameLine( 0, 4 );
        if ( ImGuiX::ColoredButton( Colors::OrangeRed, Colors::White, EE_ICON_FILE_IMPORT" IMPORT", ImVec2( buttonWidth, 0 ) ) )
        {
            auto const selectedFile = pfd::open_file( "Import File", m_pToolsContext->GetRawResourceDirectory().c_str() ).result();
            if ( !selectedFile.empty() )
            {
                FileSystem::Path selectedFilePath( selectedFile[0].data() );
                if ( !selectedFilePath.IsUnderDirectory( m_pToolsContext->GetRawResourceDirectory() ) )
                {
                    pfd::message( "Import Error", "File to import must be within the raw resource folder!", pfd::choice::ok, pfd::icon::error );
                }
            }
        }
    }

    void ResourceBrowserEditorTool::DrawFilterOptions( UpdateContext const& context )
    {
        EE_PROFILE_FUNCTION();

        constexpr static float const buttonWidth = 30;
        bool shouldUpdateVisibility = false;

        // Text Filter
        //-------------------------------------------------------------------------

        if ( m_filter.UpdateAndDraw() )
        {
            shouldUpdateVisibility = true;

            auto const SetExpansion = [] ( TreeListViewItem* pItem )
            {
                if ( pItem->IsVisible() )
                {
                    pItem->SetExpanded( true );
                }
            };

            m_treeview.ForEachItem( SetExpansion );
        }

        // Type Filter + Controls
        //-------------------------------------------------------------------------

        if ( ImGuiX::ToggleButton( EE_ICON_RAW, EE_ICON_RAW_OFF, m_showRawFiles, ImVec2( buttonWidth, 0 ) ) )
        {
            shouldUpdateVisibility = true;
        }

        float const availableWidth = ImGui::GetContentRegionAvail().x;
        float const filterWidth = availableWidth - ( 3 * ( buttonWidth + ImGui::GetStyle().ItemSpacing.x ) );
        ImGui::SameLine();
        shouldUpdateVisibility |= DrawResourceTypeFilterMenu( filterWidth );

        ImGui::SameLine();
        if ( ImGui::Button( EE_ICON_EXPAND_ALL "##Expand All", ImVec2( buttonWidth, 0 ) ) )
        {
            m_treeview.ForEachItem( [] ( TreeListViewItem* pItem ) { pItem->SetExpanded( true ); } );
        }
        ImGuiX::ItemTooltip( "Expand All" );

        ImGui::SameLine();
        if ( ImGui::Button( EE_ICON_COLLAPSE_ALL "##Collapse ALL", ImVec2( buttonWidth, 0 ) ) )
        {
            m_treeview.ForEachItem( [] ( TreeListViewItem* pItem ) { pItem->SetExpanded( false ); } );
        }
        ImGuiX::ItemTooltip( "Collapse All" );

        //-------------------------------------------------------------------------

        if ( shouldUpdateVisibility )
        {
            UpdateVisibility();
        }
    }

    bool ResourceBrowserEditorTool::DrawResourceTypeFilterMenu( float width )
    {
        bool requiresVisibilityUpdate = false;

        ImGui::SetNextItemWidth( width );
        if ( ImGui::BeginCombo( "##ResourceTypeFilters", "Resource Filters", ImGuiComboFlags_HeightLarge ) )
        {
            for ( auto const& resourceInfo : m_pToolsContext->m_pTypeRegistry->GetRegisteredResourceTypes() )
            {
                bool isChecked = VectorContains( m_typeFilter, resourceInfo.second.m_resourceTypeID );
                if ( ImGui::Checkbox( resourceInfo.second.m_friendlyName.c_str(), &isChecked ) )
                {
                    if ( isChecked )
                    {
                        m_typeFilter.emplace_back( resourceInfo.second.m_resourceTypeID );
                    }
                    else
                    {
                        m_typeFilter.erase_first_unsorted( resourceInfo.second.m_resourceTypeID );
                    }

                    requiresVisibilityUpdate = true;
                }
            }

            ImGui::EndCombo();
        }

        //-------------------------------------------------------------------------

        return requiresVisibilityUpdate;
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
}