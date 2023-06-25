#include "ResourceBrowser.h"
#include "ResourceBrowser_DescriptorCreator.h"
#include "EngineTools/Resource/RawFileInspector.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "EngineTools/Core/ToolsContext.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/FileSystem/FileSystemUtils.h"
#include "System/Profiling.h"
#include "System/Platform/PlatformUtils_Win32.h"
#include <eastl/sort.h>
#include "EngineTools/Core/CommonDialogs.h"
#include "EngineTools/Resource/ResourceDatabase.h"

//-------------------------------------------------------------------------

namespace EE
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

        ResourceBrowserTreeItem( TreeListViewItem* pParent, ToolsContext& toolsContext, Resource::ResourceDatabase::DirectoryEntry const* pDirectoryEntry )
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

        ResourceBrowserTreeItem( TreeListViewItem* pParent, ToolsContext& toolsContext, Resource::ResourceDatabase::FileEntry const* pFileEntry )
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

        virtual String GetDisplayName() const override
        {
            String displayName;

            if ( IsDirectory() )
            {
                displayName.sprintf( EE_ICON_FOLDER" %s", GetNameID().c_str() );
            }
            else if( IsResourceFile() )
            {
                displayName.sprintf( EE_ICON_FILE_OUTLINE" %s", GetNameID().c_str() );
            }
            else if ( IsRawFile() )
            {
                displayName.sprintf( EE_ICON_FILE_QUESTION_OUTLINE" %s", GetNameID().c_str() );
            }

            return displayName;
        }

        // File Info
        //-------------------------------------------------------------------------

        inline bool IsFile() const { return m_type == Type::File; }
        inline bool IsDirectory() const { return m_type == Type::Directory; }
        inline FileSystem::Path const& GetFilePath() const { return m_path; }
        inline ResourcePath const& GetResourcePath() const { return m_resourcePath; }

        virtual bool IsDragAndDropSource() const override { return IsFile() && IsResourceFile(); }
        virtual void SetDragAndDropPayloadData() const override { ImGui::SetDragDropPayload( "ResourceFile", (void*) m_resourcePath.c_str(), m_resourcePath.GetString().length() + 1 ); }

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
                else // Try create file inspector
                {
                    m_toolsContext.TryOpenRawResource( m_path );
                }
            }

            return false;
        }

        virtual bool OnDoubleClick() override { return OnUserActivate(); }
        virtual bool OnEnterPressed() override { return OnUserActivate(); }

    protected:

        ToolsContext&                           m_toolsContext;
        StringID                                m_nameID;
        FileSystem::Path                        m_path;
        ResourcePath                            m_resourcePath;
        ResourceTypeID                          m_resourceTypeID;
        Type                                    m_type;
    };
}

//-------------------------------------------------------------------------

namespace EE
{
    ResourceBrowser::ResourceBrowser( ToolsContext& toolsContext )
        : m_toolsContext( toolsContext )
        , m_dataDirectoryPathDepth( m_toolsContext.GetRawResourceDirectory().GetPathDepth() )
    {
        m_treeview.SetFlag( TreeListView::ExpandItemsOnlyViaArrow, true );

        m_resourceDatabaseUpdateEventBindingID = toolsContext.m_pResourceDatabase->OnDatabaseUpdated().Bind( [this] () { m_rebuildTree = true; } );

        // Create descriptor category tree
        //-------------------------------------------------------------------------

        TypeSystem::TypeRegistry const* pTypeRegistry = m_toolsContext.m_pTypeRegistry;
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

    ResourceBrowser::~ResourceBrowser()
    {
        m_toolsContext.m_pResourceDatabase->OnDatabaseUpdated().Unbind( m_resourceDatabaseUpdateEventBindingID );

        EE::Delete( m_pResourceDescriptorCreator );
    }

    //-------------------------------------------------------------------------

    bool ResourceBrowser::UpdateAndDraw( UpdateContext const& context )
    {
        TreeListViewContext treeViewContext;
        treeViewContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildTreeView( pRootItem ); };
        treeViewContext.m_drawItemContextMenuFunction = [this] ( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) { DrawItemContextMenu( selectedItemsWithContextMenus ); };

        if ( m_rebuildTree )
        {
            m_treeview.RebuildTree( treeViewContext, true );
            m_rebuildTree = false;
        }

        //-------------------------------------------------------------------------

        bool isOpen = true;
        if ( ImGui::Begin( GetWindowName(), &isOpen) )
        {
            if ( m_toolsContext.m_pResourceDatabase->IsRebuilding() )
            {
                ImGui::Text( "Resource DB building: " );
                ImGui::SameLine();
                ImGui::ProgressBar( m_toolsContext.m_pResourceDatabase->GetRebuildProgress() );
            }
            else
            {
                DrawCreationControls( context );
                DrawFilterOptions( context );

                //-------------------------------------------------------------------------

                m_treeview.UpdateAndDraw( treeViewContext );
            }
        }
        ImGui::End();

        //-------------------------------------------------------------------------

        DrawDialogs();

        //-------------------------------------------------------------------------

        return isOpen;
    }

    //-------------------------------------------------------------------------

    void ResourceBrowser::RebuildTreeView( TreeListViewItem* pRootItem )
    {
        EE_ASSERT( !m_toolsContext.m_pResourceDatabase->IsRebuilding() );
        auto pDataDirectory = m_toolsContext.m_pResourceDatabase->GetRawResourceDirectoryEntry();

        //-------------------------------------------------------------------------

        pRootItem->DestroyChildren();

        for ( auto const& childDirectory : pDataDirectory->m_directories )
        {
            pRootItem->CreateChild<ResourceBrowserTreeItem>( m_toolsContext, &childDirectory );
        }

        //-------------------------------------------------------------------------

        for ( auto pChildFile : pDataDirectory->m_files )
        {
            pRootItem->CreateChild<ResourceBrowserTreeItem>( m_toolsContext, pChildFile );
        }

        //-------------------------------------------------------------------------

        UpdateVisibility();
    }

    void ResourceBrowser::UpdateVisibility()
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

    void ResourceBrowser::DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus )
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
                m_showDeleteConfirmationDialog = true;
            }
        }
    }

    //-------------------------------------------------------------------------

    bool ResourceBrowser::FindAndSelectResource( ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );

        auto pFoundItem = m_treeview.FindItem( resourceID.GetPathID() );
        if ( pFoundItem == nullptr )
        {
            return false;
        }

        m_treeview.SetSelection( pFoundItem );
        m_treeview.SetViewToSelection();
        return true;
    }

    //-------------------------------------------------------------------------

    void ResourceBrowser::DrawDialogs()
    {
        if ( m_showDeleteConfirmationDialog )
        {
            ImGui::OpenPopup( "Delete Resource" );
            m_showDeleteConfirmationDialog = false;
        }

        ImGui::SetNextWindowSize( ImVec2( 300, 96 ) );
        if ( ImGui::BeginPopupModal( "Delete Resource", nullptr, ImGuiWindowFlags_NoResize ) )
        {
            ImGui::Text( "Are you sure you want to delete this file?" );
            ImGui::Text( "This cannot be undone!" );

            if ( ImGui::Button( "Ok", ImVec2( 143, 0 ) ) )
            {
                auto pResourceItem = (ResourceBrowserTreeItem*) m_treeview.GetSelection()[0];
                FileSystem::Path const fileToDelete = pResourceItem->GetFilePath();
                m_treeview.ClearSelection();
                FileSystem::EraseFile( fileToDelete );
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine( 0, 6 );

            if ( ImGui::Button( "Cancel", ImVec2( 143, 0 ) ) )
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();

            ImGui::EndPopup();
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

    void ResourceBrowser::DrawCreationControls( UpdateContext const& context )
    {
        ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );
        float const availableWidth = ImGui::GetContentRegionAvail().x;
        float const buttonWidth = ( availableWidth - 4.0f ) / 2.0f;

        if ( ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_PLUS" CREATE", ImVec2( buttonWidth, 0 ) ) )
        {
            ImGui::OpenPopup( "CreateNewDescriptor" );
        }
        ImGuiX::ItemTooltip( "Create a new resource descriptor and fill out the settings by hand! This is how you create resources that dont have a source file." );

        if ( ImGui::BeginPopup( "CreateNewDescriptor" ) )
        {
            DrawDescriptorMenuCategory( m_toolsContext.GetRawResourceDirectory(), m_categorizedDescriptorTypes.GetRootCategory() );
            ImGui::EndPopup();
        }
        ImGuiX::ItemTooltip( "Create a resource descriptor based on a source file. This will pop up a helper window to help you with the creation." );

        //-------------------------------------------------------------------------

        ImGui::SameLine( 0, 4 );
        if ( ImGuiX::ColoredButton( ImGuiX::ImColors::OrangeRed, ImGuiX::ImColors::White, EE_ICON_FILE_IMPORT" IMPORT", ImVec2( buttonWidth, 0 ) ) )
        {
            auto const selectedFile = pfd::open_file( "Load Map", m_toolsContext.GetRawResourceDirectory().c_str() ).result();
            if ( !selectedFile.empty() )
            {
                FileSystem::Path selectedFilePath( selectedFile[0].data() );
                if ( !selectedFilePath.IsUnderDirectory( m_toolsContext.GetRawResourceDirectory() ) )
                {
                    pfd::message( "Import Error", "File to import must be within the raw resource folder!", pfd::choice::ok, pfd::icon::error );
                }
                else
                {
                    m_toolsContext.TryOpenRawResource( selectedFilePath );
                }
            }
        }
    }

    void ResourceBrowser::DrawFilterOptions( UpdateContext const& context )
    {
        EE_PROFILE_FUNCTION();

        constexpr static float const buttonWidth = 30;
        bool shouldUpdateVisibility = false;

        // Text Filter
        //-------------------------------------------------------------------------

        if ( m_filter.DrawAndUpdate() )
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

        float const availableWidth = ImGui::GetContentRegionAvail().x;
        float const filterWidth = availableWidth - ( 2 * ( buttonWidth + ImGui::GetStyle().ItemSpacing.x ) );
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

    bool ResourceBrowser::DrawResourceTypeFilterMenu( float width )
    {
        bool requiresVisibilityUpdate = false;

        ImGui::SetNextItemWidth( width );
        if ( ImGui::BeginCombo( "##ResourceTypeFilters", "Resource Filters", ImGuiComboFlags_HeightLarge ) )
        {
            if ( ImGui::Checkbox( "Raw Files", &m_showRawFiles ) )
            {
                requiresVisibilityUpdate = true;
            }

            ImGui::Separator();

            for ( auto const& resourceInfo : m_toolsContext.m_pTypeRegistry->GetRegisteredResourceTypes() )
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

    void ResourceBrowser::DrawDescriptorMenuCategory( FileSystem::Path const& path, Category<TypeSystem::TypeInfo const*> const& category )
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
                    m_pResourceDescriptorCreator = EE::New<ResourceDescriptorCreator>( &m_toolsContext, item.m_data->m_ID, path );
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