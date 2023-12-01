#include "EditorTool_ResourceImporter.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "EngineTools/Resource/ResourceImportSettings.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Core/CommonDialogs.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "Engine/Render/Mesh/StaticMesh.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Physics/PhysicsCollisionMesh.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Serialization/TypeSerialization.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsCollisionMesh.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMesh.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderTexture.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMaterial.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    //-------------------------------------------------------------------------
    // Import Settings
    //-------------------------------------------------------------------------

    class AnimationSettings : public TImportSettings<Animation::AnimationClipResourceDescriptor>
    {
        using TImportSettings::TImportSettings;

        virtual char const* GetName() override { return "Animation"; }

        virtual bool IsVisible() const override
        {
            return m_descriptor.m_animationPath.IsValid();
        }

        virtual void UpdateDescriptorInternal( ResourcePath sourceFileResourcePath, TVector<Import::ImportableItem*> const& selectedItems ) override
        {
            bool const validSelection = !selectedItems.empty() && selectedItems[0]->GetTypeID() == Import::ImportableAnimation::GetStaticTypeID();
            if ( !validSelection )
            {
                m_descriptor.m_animationPath.Clear();
                return;
            }

            //-------------------------------------------------------------------------

            m_descriptor.m_animationPath = sourceFileResourcePath;
            m_descriptor.m_animationName.clear();

            if ( selectedItems.size() > 1 )
            {
                m_descriptor.m_animationName = selectedItems.back()->m_nameID.c_str();
            }
        }
    };

    class SkeletonSettings : public TImportSettings<Animation::SkeletonResourceDescriptor>
    {
        using TImportSettings::TImportSettings;

        virtual char const* GetName() override { return "Skeleton"; }

        virtual bool IsVisible() const override
        {
            return m_descriptor.m_skeletonPath.IsValid();
        }

        virtual void UpdateDescriptorInternal( ResourcePath sourceFileResourcePath, TVector<Import::ImportableItem*> const& selectedItems ) override
        {
            bool const validSelection = !selectedItems.empty() && selectedItems[0]->GetTypeID() == Import::ImportableSkeleton::GetStaticTypeID();
            if ( !validSelection )
            {
                m_descriptor.m_skeletonPath.Clear();
                return;
            }

            //-------------------------------------------------------------------------

            m_descriptor.m_skeletonPath = sourceFileResourcePath;
            m_descriptor.m_skeletonRootBoneName.clear();

            if ( selectedItems.size() > 1 )
            {
                m_descriptor.m_skeletonRootBoneName = selectedItems.back()->m_nameID.c_str();
            }
        }
    };

    class TextureSettings : public TImportSettings<Render::TextureResourceDescriptor>
    {
        using TImportSettings::TImportSettings;

        virtual char const* GetName() override { return "Texture"; }

        virtual bool IsVisible() const override
        {
            return m_descriptor.m_path.IsValid();
        }

        virtual void UpdateDescriptorInternal( ResourcePath sourceFileResourcePath, TVector<Import::ImportableItem*> const& selectedItems ) override
        {
            bool const validSelection = !selectedItems.empty() && selectedItems[0]->GetTypeID() == Import::ImportableImage::GetStaticTypeID();
            if ( !validSelection )
            {
                m_descriptor.m_path.Clear();
                return;
            }

            //-------------------------------------------------------------------------

            m_descriptor.m_path = sourceFileResourcePath;
        }
    };

    class MeshImportSettings : public ImportSettings
    {

    protected:

        MeshImportSettings( ToolsContext const* pToolsContext, bool restrictToSkeletalMeshes )
            : ImportSettings( pToolsContext )
            , m_restrictToSkeletalMeshes( restrictToSkeletalMeshes )
        {}

        virtual bool IsVisible() const override
        {
            return Cast<Render::MeshResourceDescriptor>( GetDescriptor() )->m_meshPath.IsValid();
        }

        virtual void UpdateDescriptorInternal( ResourcePath sourceFileResourcePath, TVector<Import::ImportableItem*> const& selectedItems ) override
        {
            auto pMeshDescriptor = Cast<Render::MeshResourceDescriptor>( GetDescriptor() );

            bool validSelection = !selectedItems.empty() && selectedItems[0]->GetTypeID() == Import::ImportableMesh::GetStaticTypeID();

            if ( validSelection && m_restrictToSkeletalMeshes )
            {
                validSelection = false;

                for ( auto pItem : selectedItems )
                {
                    auto pImportableMeshItem = Cast<Import::ImportableMesh>( pItem );
                    if ( pImportableMeshItem->m_isSkeletalMesh )
                    {
                        validSelection = true;
                        break;
                    }
                }
            }

            if ( !validSelection )
            {
                pMeshDescriptor->m_meshPath.Clear();
                pMeshDescriptor->m_meshesToInclude.clear();
                return;
            }

            //-------------------------------------------------------------------------

            pMeshDescriptor->m_meshPath = sourceFileResourcePath;
            pMeshDescriptor->m_meshesToInclude.clear();

            if ( selectedItems.size() > 1 )
            {
                for ( auto pItem : selectedItems )
                {
                    if ( m_restrictToSkeletalMeshes )
                    {
                        auto pImportableMeshItem = Cast<Import::ImportableMesh>( pItem );
                        if ( pImportableMeshItem->m_isSkeletalMesh )
                        {
                            pMeshDescriptor->m_meshesToInclude.emplace_back( pItem->m_nameID.c_str() );
                        }
                    }
                    else // All meshes allowed
                    {
                        pMeshDescriptor->m_meshesToInclude.emplace_back( pItem->m_nameID.c_str() );
                    }
                }
            }
        }

        virtual bool HasExtraOptions() const override { return true; }

        virtual float GetExtraOptionsRequiredHeight() const override { return 30; }

        virtual void DrawExtraOptions()
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Setup Materials: " );
            ImGui::SameLine();
            ImGuiX::Checkbox( "##Set materials", &m_setupMaterials );
            ImGui::SameLine();
            ImGuiX::HelpMarker( "Will try to automatically set materials for this mesh descriptor.\n\nFirst it will look for a material with a matching name in the current directory, if that fails we will create an empty material descriptor with a matching name and use that instead." );
        }

        virtual bool PreSaveDescriptor( FileSystem::Path const descriptorSavePath ) override
        {
            if ( !m_setupMaterials )
            {
                return true;
            }

            //-------------------------------------------------------------------------

            auto* pMeshDescriptor = Cast<Render::MeshResourceDescriptor>( GetDescriptor() );
            pMeshDescriptor->m_materials.clear();

            EE_ASSERT( descriptorSavePath.IsValid() && descriptorSavePath.IsFilePath() );
            FileSystem::Path const destinationDirectory = descriptorSavePath.GetParentDirectory();

            auto AddMaterial = [&] ( Import::ImportableMesh const& meshInfo )
            {
                FileSystem::Path materialPath;
                if ( meshInfo.m_materialID.IsValid() )
                {
                    materialPath = FileSystem::Path( destinationDirectory.ToString() + meshInfo.m_materialID.c_str() + "." + Render::Material::GetStaticResourceTypeID().ToString().c_str() );
                }

                //-------------------------------------------------------------------------

                if ( materialPath.IsValid() )
                {
                    if ( !materialPath.Exists() )
                    {
                        Render::MaterialResourceDescriptor materialDesc;
                        TrySaveDescriptorToDisk( materialPath );
                    }

                    pMeshDescriptor->m_materials.emplace_back( ResourcePath::FromFileSystemPath( m_pToolsContext->GetRawResourceDirectory(), materialPath ) );
                }
                else
                {
                    pMeshDescriptor->m_materials.emplace_back( ResourcePath() );
                }
            };

            //-------------------------------------------------------------------------

            TInlineVector<StringID, 25> uniqueMaterialIDs;

            /*for ( auto const& meshInfo : m_meshes )
            {
                if ( !meshInfo.m_isSelected )
                {
                    continue;
                }

                if ( pMeshDescriptor->m_mergeSectionsByMaterial )
                {
                    if ( VectorContains( uniqueMaterialIDs, meshInfo.m_materialID ) )
                    {
                        continue;
                    }

                    uniqueMaterialIDs.emplace_back( meshInfo.m_materialID );
                }

                AddMaterial( meshInfo );
            }*/

            return true;
        }

        virtual void PostSaveDescriptor( FileSystem::Path const descriptorSavePath ) override
        {
            auto* pMeshDescriptor = Cast<Render::MeshResourceDescriptor>( GetDescriptor() );
            pMeshDescriptor->m_materials.clear();
        }

    protected:

        bool            m_setupMaterials = true;
        bool const      m_restrictToSkeletalMeshes = false;
    };

    class SkeletalMeshSettings : public MeshImportSettings
    {
    public:

        SkeletalMeshSettings( ToolsContext const* pToolsContext ) : MeshImportSettings( pToolsContext, true )
        {
            m_propertyGrid.SetTypeToEdit( &m_descriptor );
        }

        virtual char const* GetName() override { return "Skeletal Mesh"; }
        virtual ResourceDescriptor const* GetDescriptor() const override { return &m_descriptor; }

    protected:

        Render::SkeletalMeshResourceDescriptor      m_descriptor;
        bool                                        m_setupMaterials = true;
    };

    class StaticMeshSettings : public MeshImportSettings
    {
    public:

        StaticMeshSettings( ToolsContext const* pToolsContext ) : MeshImportSettings( pToolsContext, false )
        {
            m_propertyGrid.SetTypeToEdit( &m_descriptor );
        }

        virtual char const* GetName() override { return "Static Mesh"; }
        virtual ResourceDescriptor const* GetDescriptor() const override { return &m_descriptor; }

    protected:

        Render::StaticMeshResourceDescriptor m_descriptor;
    };

    class CollisionMeshSettings : public TImportSettings<Physics::PhysicsCollisionMeshResourceDescriptor>
    {
        using TImportSettings::TImportSettings;

        virtual char const* GetName() override { return "Collision"; }

        virtual bool IsVisible() const override
        {
            return m_descriptor.m_sourcePath.IsValid();
        }

        virtual void UpdateDescriptorInternal( ResourcePath sourceFileResourcePath, TVector<Import::ImportableItem*> const& selectedItems ) override
        {
            bool const validSelection = !selectedItems.empty() && selectedItems[0]->GetTypeID() == Import::ImportableMesh::GetStaticTypeID();
            if ( !validSelection )
            {
                m_descriptor.m_sourcePath.Clear();
                m_descriptor.m_meshesToInclude.clear();
                return;
            }

            //-------------------------------------------------------------------------

            m_descriptor.m_sourcePath = sourceFileResourcePath;
            m_descriptor.m_meshesToInclude.clear();

            if ( selectedItems.size() > 1 )
            {
                for ( auto pItem : selectedItems )
                {
                    m_descriptor.m_meshesToInclude.emplace_back( pItem->m_nameID.c_str() );
                }
            }
        }
    };

    //-------------------------------------------------------------------------
    // Tree Item
    //-------------------------------------------------------------------------

    class ImporterTreeItem final : public TreeListViewItem
    {
    public:

        enum class Type
        {
            Directory = 0,
            File,
        };

    public:

        ImporterTreeItem( TreeListViewItem* pParent, ToolsContext const& toolsContext, Resource::ResourceDatabase::DirectoryEntry const* pDirectoryEntry )
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
                CreateChild<ImporterTreeItem>( m_toolsContext, &childDirectory );
            }

            //-------------------------------------------------------------------------

            for ( auto pChildFile : pDirectoryEntry->m_files )
            {
                if ( pChildFile->HasDescriptor() )
                {
                    continue;
                }

                TInlineString<6> const extension = pChildFile->m_filePath.GetExtensionAsString();
                if ( extension.empty() )
                {
                    continue;
                }

                if ( Import::IsImportableFileType( extension ) )
                {
                    CreateChild<ImporterTreeItem>( m_toolsContext, pChildFile );
                }
            }

        }

        ImporterTreeItem( TreeListViewItem* pParent, ToolsContext const& toolsContext, Resource::ResourceDatabase::FileEntry const* pFileEntry )
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

        void RemoveEmptyDirectories()
        {
            for ( int32_t i = (int32_t) m_children.size() - 1; i >= 0; i-- )
            {
                auto pImporterChildItem = static_cast<ImporterTreeItem*>( m_children[i] );
                if ( pImporterChildItem->IsDirectory() )
                {
                    // Remove any child empty directories
                    pImporterChildItem->RemoveEmptyDirectories();

                    // If this is now an empty directory
                    if ( pImporterChildItem->m_children.empty() )
                    {
                        EE::Delete( m_children[i] );
                        m_children.erase( m_children.begin() + i );
                    }
                }
            }
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

        // File Info
        //-------------------------------------------------------------------------

        inline bool IsFile() const { return m_type == Type::File; }
        inline bool IsDirectory() const { return m_type == Type::Directory; }
        inline FileSystem::Path const& GetFilePath() const { return m_path; }
        inline ResourcePath const& GetResourcePath() const { return m_resourcePath; }

        // Resource Info
        //-------------------------------------------------------------------------

        inline bool IsRawFile() const { return IsFile() && !m_resourceTypeID.IsValid(); }
        inline bool IsResourceFile() const { return IsFile() && m_resourceTypeID.IsValid(); }
        inline ResourceID GetResourceID() const { EE_ASSERT( IsResourceFile() ); return ResourceID( m_resourcePath ); }
        inline ResourceTypeID const& GetResourceTypeID() const { EE_ASSERT( IsResourceFile() ); return m_resourceTypeID; }

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
    // Raw File
    //-------------------------------------------------------------------------

    void ResourceImporterEditorTool::SelectedRawFile::Clear()
    {
        m_resourcePath.Clear();
        m_filePath.Clear();
        m_dependentResources.clear();
        m_warnings.clear();
        m_errors.clear();

        for ( auto& pImportableData : m_importableItems )
        {
            EE::Delete( pImportableData );
        }

        m_importableItems.clear();
        m_inspectionResult = Import::InspectionResult::Uninspected;
    }

    //-------------------------------------------------------------------------
    // Importer Tool
    //-------------------------------------------------------------------------

    ResourceImporterEditorTool::ResourceImporterEditorTool( ToolsContext const* pToolsContext )
        : EditorTool( pToolsContext, "Resource Importer" )
    {
        m_treeview.SetFlag( TreeListView::ExpandItemsOnlyViaArrow, true );
        m_treeview.SetFlag( TreeListView::UseSmallFont, false );
        m_treeview.SetFlag( TreeListView::SortTree, true );

        m_resourceDatabaseUpdateEventBindingID = m_pToolsContext->m_pResourceDatabase->OnFileSystemCacheUpdated().Bind( [this] () { OnResourceDatabaseUpdated(); } );

        // Create import settings
        //-------------------------------------------------------------------------

        m_importSettings.emplace_back( EE::New<AnimationSettings>( m_pToolsContext ) );
        m_importSettings.emplace_back( EE::New<SkeletonSettings>( m_pToolsContext ) );
        m_importSettings.emplace_back( EE::New<CollisionMeshSettings>( m_pToolsContext ) );
        m_importSettings.emplace_back( EE::New<StaticMeshSettings>( m_pToolsContext ) );
        m_importSettings.emplace_back( EE::New<SkeletalMeshSettings>( m_pToolsContext ) );
        m_importSettings.emplace_back( EE::New<TextureSettings>( m_pToolsContext ) );
    }

    ResourceImporterEditorTool::~ResourceImporterEditorTool()
    {
        // Destroy import settings
        //-------------------------------------------------------------------------

        m_pActiveSettings = nullptr;

        for ( auto pImportSettings : m_importSettings )
        {
            EE::Delete( pImportSettings );
        }
        m_importSettings.clear();

        //-------------------------------------------------------------------------

        m_selectedFile.Clear();
        m_pToolsContext->m_pResourceDatabase->OnFileSystemCacheUpdated().Unbind( m_resourceDatabaseUpdateEventBindingID );
    }

    void ResourceImporterEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        CreateToolWindow( "Importer", [this] ( UpdateContext const& context, bool isFocused ) { DrawImporterWindow( context, isFocused ); } );
    }

    void ResourceImporterEditorTool::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Importer" ).c_str(), dockspaceID );
    }

    void ResourceImporterEditorTool::Update( UpdateContext const& context, bool isVisible, bool isFocused )
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
    }

    //-------------------------------------------------------------------------

    void ResourceImporterEditorTool::OnResourceDatabaseUpdated()
    {
        m_rebuildTree = true;

        if ( m_selectedFile.IsSet() )
        {
            m_selectedFile.m_dependentResources = m_pToolsContext->m_pResourceDatabase->GetAllDependentResources( m_selectedFile.m_resourcePath );
        }
    }

    void ResourceImporterEditorTool::RebuildTreeView( TreeListViewItem* pRootItem )
    {
        EE_ASSERT( m_pToolsContext->m_pResourceDatabase->IsFileSystemCacheBuilt() );
        auto pDataDirectory = m_pToolsContext->m_pResourceDatabase->GetRawResourceDirectoryEntry();

        //-------------------------------------------------------------------------

        pRootItem->DestroyChildren();

        for ( auto const& childDirectory : pDataDirectory->m_directories )
        {
            pRootItem->CreateChild<ImporterTreeItem>( *m_pToolsContext, &childDirectory );
        }

        //-------------------------------------------------------------------------

        for ( auto pChildFile : pDataDirectory->m_files )
        {
            // Ignore all resource files
            if ( !pChildFile->HasDescriptor() && Import::IsImportableFileType( pChildFile->m_filePath.GetExtension() ) )
            {
                pRootItem->CreateChild<ImporterTreeItem>( *m_pToolsContext, pChildFile );
            }
        }

        //-------------------------------------------------------------------------

        for ( auto const& childDirectory : pRootItem->GetChildren() )
        {
            auto pImporterTreeItem = static_cast<ImporterTreeItem*>( pRootItem );
            pImporterTreeItem->RemoveEmptyDirectories();
        }

        //-------------------------------------------------------------------------

        UpdateVisibility();
    }

    void ResourceImporterEditorTool::UpdateSelectedFile( ImporterTreeItem const* pSelectedItem )
    {
        m_selectedFile.Clear();

        //-------------------------------------------------------------------------

        if ( pSelectedItem != nullptr )
        {
            m_selectedFile.m_resourcePath = pSelectedItem->GetResourcePath();
            m_selectedFile.m_filePath = pSelectedItem->GetFilePath();
            m_selectedFile.m_extension = m_selectedFile.m_filePath.GetExtension();
            m_selectedFile.m_dependentResources = m_pToolsContext->m_pResourceDatabase->GetAllDependentResources( pSelectedItem->GetResourcePath() );

            Import::InspectorContext ctx;
            ctx.m_rawResourceDirectoryPath = m_pToolsContext->GetRawResourceDirectory();
            ctx.m_warningDelegate = [this] ( char const* pWarningStr ) { m_selectedFile.m_warnings.append( pWarningStr ); };
            ctx.m_errorDelegate = [this] ( char const* pErrorStr ) { m_selectedFile.m_errors.append( pErrorStr ); };

            m_selectedFile.m_inspectionResult = InspectFile( ctx, m_selectedFile.m_filePath, m_selectedFile.m_importableItems );

            for ( auto pItem : m_selectedFile.m_importableItems )
            {
                EE_ASSERT( pItem != nullptr && pItem->IsValid() );
            }
        }
    }

    void ResourceImporterEditorTool::UpdateVisibility()
    {
        auto VisibilityFunc = [this] ( TreeListViewItem const* pItem )
        {
            auto pImporterItem = static_cast<ImporterTreeItem const*>( pItem );
            return m_filter.MatchesFilter( pImporterItem->GetResourcePath().c_str() );
        };

        //-------------------------------------------------------------------------

        m_treeview.UpdateItemVisibility( VisibilityFunc );
    }

    //-------------------------------------------------------------------------

    void ResourceImporterEditorTool::DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus )
    {
        auto pResourceItem = (ImporterTreeItem*) selectedItemsWithContextMenus[0];

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
    }

    void ResourceImporterEditorTool::DrawImporterWindow( UpdateContext const& context, bool isFocused )
    {
        // Draw progress bar
        if ( m_pToolsContext->m_pResourceDatabase->IsBuildingCaches() )
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text( m_pToolsContext->m_pResourceDatabase->IsFileSystemCacheBuilt() ? "Building Descriptor Cache: " : "Building File System Cache: " );
            ImGui::SameLine();
            ImGui::ProgressBar( m_pToolsContext->m_pResourceDatabase->GetProgress() );
        }

        //-------------------------------------------------------------------------

        if ( !m_pToolsContext->m_pResourceDatabase->IsFileSystemCacheBuilt() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 8, 4 ) );
        bool const drawTable = ImGui::BeginTable( "ImporterLayout", 3, ImGuiTableFlags_Resizable, ImGui::GetContentRegionAvail() );
        ImGui::PopStyleVar();

        if ( drawTable )
        {
            ImGui::TableSetupColumn( "Tree", ImGuiTableColumnFlags_WidthStretch, 0.2f );
            ImGui::TableSetupColumn( "Info", ImGuiTableColumnFlags_WidthStretch, 0.6f );
            ImGui::TableSetupColumn( "Controls", ImGuiTableColumnFlags_WidthStretch, 0.2f );

            //-------------------------------------------------------------------------

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            DrawFileTree( context );

            ImGui::TableNextColumn();
            DrawFileInfo( context );

            ImGui::TableNextColumn();
            DrawImportSettings( context );

            ImGui::EndTable();
        }
    }

    void ResourceImporterEditorTool::DrawFileTree( UpdateContext const& context )
    {
        constexpr static float const buttonWidth = 30;
        bool shouldUpdateVisibility = false;

        // Text Filter
        //-------------------------------------------------------------------------

        float const availableWidth = ImGui::GetContentRegionAvail().x;
        float const filterWidth = availableWidth - ( 2 * ( buttonWidth + ImGui::GetStyle().ItemSpacing.x ) );

        if ( m_filter.UpdateAndDraw( filterWidth ) )
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

        // Draw file system tree
        //-------------------------------------------------------------------------

        TreeListViewContext treeViewContext;
        treeViewContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildTreeView( pRootItem ); };
        treeViewContext.m_drawItemContextMenuFunction = [this] ( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) { DrawItemContextMenu( selectedItemsWithContextMenus ); };

        if ( m_treeview.UpdateAndDraw( treeViewContext ) )
        {
            ImporterTreeItem* pSelectedItem = nullptr;

            auto const& selection = m_treeview.GetSelection();
            if ( !selection.empty() )
            {
                pSelectedItem = (ImporterTreeItem*) selection[0];
                if ( !pSelectedItem->IsRawFile() )
                {
                    pSelectedItem = nullptr;
                }
            }

            UpdateSelectedFile( pSelectedItem );
        }
    }

    void ResourceImporterEditorTool::DrawFileInfo( UpdateContext const& context )
    {
        if ( !m_selectedFile.IsSet() )
        {
            ImGui::Text( "Nothing Selected" );
            return;
        }

        if ( m_selectedFile.m_inspectionResult == Import::InspectionResult::UnsupportedExtension )
        {
            ImGui::Text( "Unsupported File Type" );
            return;
        }

        // Draw warnings and errors
        //-------------------------------------------------------------------------

        auto DrawIconBox = [] ( char const* pIcon, char const* pText, Color background, Color foreground )
        {
            ImVec2 const tableSize( ImGui::GetContentRegionAvail().x, 0 );

            ImGui::PushStyleColor( ImGuiCol_TableRowBg, background );
            ImGui::PushStyleColor( ImGuiCol_TableBorderStrong, background.GetScaledColor( 0.75f ) );

            if ( ImGui::BeginTable( "Info", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg, tableSize ) )
            {
                ImGui::TableSetupColumn( "Icon", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize( pIcon ).x );
                ImGui::TableSetupColumn( "Text", ImGuiTableColumnFlags_WidthStretch, 1.0f );

                //-------------------------------------------------------------------------

                ImGuiX::ScopedFont const sf( ImGuiX::Font::MediumBold, foreground );

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                {
                    ImGuiX::ScopedFont const sf2( ImGuiX::Font::Medium );
                    ImGui::Text( pIcon );
                }

                ImGui::TableNextColumn();
                {
                    ImGui::Text( pText );
                }

                ImGui::EndTable();
            }
            ImGui::PopStyleColor( 2 );
        };

        bool const hasWarnings = !m_selectedFile.m_warnings.empty();
        if ( hasWarnings )
        {
            DrawIconBox( EE_ICON_ALERT, m_selectedFile.m_warnings.c_str(), Colors::DarkOrange, Colors::White );
        }

        bool const hasErrors = !m_selectedFile.m_errors.empty();
        if ( hasErrors )
        {
            DrawIconBox( EE_ICON_CLOSE_CIRCLE, m_selectedFile.m_errors.c_str(), Colors::FireBrick, Colors::White );
        }

        // Draw any resources that are referencing this file
        //-------------------------------------------------------------------------

        if ( !m_selectedFile.m_dependentResources.empty() )
        {
            ImGuiX::TextSeparator( "Dependent Resources" );

            ImVec2 const tableSize( ImGui::GetContentRegionAvail().x, 0 );
            if ( ImGui::BeginTable( "Info", 2, 0, tableSize ) )
            {
                ImGui::TableSetupColumn( "path", ImGuiTableColumnFlags_WidthStretch, 1.0f );
                ImGui::TableSetupColumn( "button", ImGuiTableColumnFlags_WidthFixed, 210 );

                //-------------------------------------------------------------------------

                for ( auto const& path : m_selectedFile.m_dependentResources )
                {
                    ImGui::PushID( &path );

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::SetNextItemWidth( -1 );
                        ImGui::InputText( "##ID", const_cast<char*>( path.c_str() ), path.GetString().length(), ImGuiInputTextFlags_ReadOnly );
                    }

                    ImGui::TableNextColumn();
                    {
                        if ( ImGuiX::ColoredIconButton( Colors::DarkOrange, Colors::White, Colors::White, EE_ICON_FILE_FIND_OUTLINE, "Browse To", ImVec2( 100, 0 ) ) )
                        {
                            m_pToolsContext->TryFindInResourceBrowser( ResourceID( path ) );
                        }

                        ImGui::SameLine();

                        if ( ImGuiX::ColoredIconButton( Colors::Green, Colors::White, Colors::White, EE_ICON_OPEN_IN_NEW, "Open", ImVec2( 100, 0 ) ) )
                        {
                            m_pToolsContext->TryOpenResource( ResourceID( path ) );
                        }
                    }

                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }

        if ( hasErrors )
        {
            return;
        }

        // Draw Importable Items
        //-------------------------------------------------------------------------

        ImGuiX::TextSeparator( "File Contents" );

        if ( ImGui::BeginTable( "InfoTable", 2, ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg ) )
        {
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_NoHide );
            ImGui::TableSetupColumn( "Info", ImGuiTableColumnFlags_WidthStretch, 0.8f );

            constexpr static int32_t const numImportableTypes = 4;
            constexpr static char const * const headings[numImportableTypes] = { "Animations", "Images", "Meshes", "Skeletons" };
            TypeSystem::TypeInfo const* const importableItemTypes[numImportableTypes] = { Import::ImportableAnimation::s_pTypeInfo, Import::ImportableImage::s_pTypeInfo, Import::ImportableMesh::s_pTypeInfo, Import::ImportableSkeleton::s_pTypeInfo};

            for ( auto i = 0; i < numImportableTypes; i++ )
            {
                // Get all valid items for the specified type
                TVector<Import::ImportableItem*> validItems;
                for ( auto pItem : m_selectedFile.m_importableItems )
                {
                    if ( !pItem->GetTypeInfo()->IsDerivedFrom( importableItemTypes[i]->m_ID ) )
                    {
                        continue;
                    }

                    validItems.emplace_back( pItem );
                }

                //-------------------------------------------------------------------------

                bool hasCreatedHeader = false;
                bool isHeadingOpen = false;

                for ( auto pItem : validItems )
                {
                    // Create Heading
                    if ( !hasCreatedHeader )
                    {
                        hasCreatedHeader = true;

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        isHeadingOpen = ImGui::TreeNodeEx( headings[i], ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow );
                        
                        if ( ImGui::IsItemClicked( ImGuiMouseButton_Left ) )
                        {
                            SelectAllItems( validItems );
                        }
                    }

                    // Print Item
                    if ( isHeadingOpen )
                    {
                        uint32_t treeNodeflags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                        if ( VectorContains( m_selectedImportableItems, pItem ) )
                        {
                            treeNodeflags |= ImGuiTreeNodeFlags_Selected;
                        }

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TreeNodeEx( pItem->m_nameID.c_str(), treeNodeflags );
                        
                        if ( ImGui::IsItemClicked( ImGuiMouseButton_Left ) )
                        {
                            HandleItemSelection( pItem, ImGui::GetIO().KeyCtrl );
                        }

                        ImGui::TableNextColumn();
                        ImGui::Text( pItem->GetDescription().c_str() );
                    }
                }

                // Close header
                if ( hasCreatedHeader && isHeadingOpen )
                {
                    ImGui::TreePop();
                }
            }

            ImGui::EndTable();
        }
    }

    void ResourceImporterEditorTool::DrawImportSettings( UpdateContext const& context )
    {
        ResourcePath createdDescriptorPath;

        m_pActiveSettings = nullptr;

        if ( ImGui::BeginTabBar( "ImportSettings" ) )
        {
            for ( auto pImportSettings : m_importSettings )
            {
                if ( !pImportSettings->IsVisible() )
                {
                    continue;
                }

                if ( ImGui::BeginTabItem( pImportSettings->GetName() ) )
                {
                    EE_ASSERT( m_pActiveSettings == nullptr );
                    m_pActiveSettings = pImportSettings;
                    pImportSettings->UpdateAndDraw( createdDescriptorPath );
                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }
    }

    void ResourceImporterEditorTool::HandleItemSelection( Import::ImportableItem* pItem, bool isMultiSelectionEnabled )
    {
        if ( !m_selectedImportableItems.empty() && ImGui::GetIO().KeyCtrl )
        {
            // If we have a type info mismatch, set the selection to the new item
            if ( pItem->GetTypeInfo() != m_selectedImportableItems[0]->GetTypeInfo() )
            {
                m_selectedImportableItems.clear();
                m_selectedImportableItems.emplace_back( pItem );
            }
            else // Try to modify the selection
            {
                if ( VectorContains( m_selectedImportableItems, pItem ) )
                {
                    m_selectedImportableItems.erase_first( pItem );
                }
                else
                {
                    m_selectedImportableItems.emplace_back( pItem );
                }
            }
        }
        else // Set selection to new item
        {
            m_selectedImportableItems.clear();
            m_selectedImportableItems.emplace_back( pItem );
        }

        OnSelectionChanged();
    }

    void ResourceImporterEditorTool::SelectAllItems( TVector<Import::ImportableItem*> itemsToSelect )
    {
        m_selectedImportableItems.clear();

        // Perform validation
        if ( !itemsToSelect.empty() )
        {
            TypeSystem::TypeInfo const* pTypeInfo = itemsToSelect[0]->GetTypeInfo();
            for ( auto pItem : itemsToSelect )
            {
                if ( pItem->GetTypeInfo() != pTypeInfo )
                {
                    OnSelectionChanged();
                    return;
                }
            }
        }

        // Update selection
        m_selectedImportableItems.insert( m_selectedImportableItems.end(), itemsToSelect.begin(), itemsToSelect.end() );
        OnSelectionChanged();
    }

    void ResourceImporterEditorTool::OnSelectionChanged()
    {
        for ( auto pImportSettings : m_importSettings )
        {
            pImportSettings->UpdateDescriptor( m_selectedFile.m_resourcePath, m_selectedImportableItems );
        }
    }
}