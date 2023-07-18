#pragma once
#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Core/Widgets/TreeListView.h"
#include "EngineTools/Core/CategoryTree.h"
#include "Base/Resource/ResourceTypeID.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
    class ToolsContext;
    class ResourceID;
    namespace TypeSystem { class TypeInfo; }
}

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceDescriptorCreator;
    class RawFileInspector;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ResourceBrowserEditorTool : public EditorTool
    {
    public:

        EE_SINGLETON_EDITOR_TOOL( ResourceBrowserEditorTool );

    public:

        ResourceBrowserEditorTool( ToolsContext const* pToolsContext );
        ~ResourceBrowserEditorTool();

        // Find, select and focus on a specified resource - returns true if successful
        void TryFindAndSelectResource( ResourceID const& resourceID ) { EE_ASSERT( resourceID.IsValid() ); m_navigationRequest = resourceID; }

    private:

        virtual bool IsSingleWindowTool() const override { return true; }
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_FORMAT_LIST_BULLETED_TYPE; }
        virtual char const* GetDockingUniqueTypeName() const override { return "Resource Browser"; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;
        virtual void DrawDialogs( UpdateContext const& context ) override;

        void DrawBrowserWindow( UpdateContext const& context, bool isFocused );

        void UpdateVisibility();
        void DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus );
        void RebuildTreeView( TreeListViewItem* pRootItem );

        // UI
        //-------------------------------------------------------------------------

        void DrawCreationControls( UpdateContext const& context );
        void DrawFilterOptions( UpdateContext const& context );
        bool DrawResourceTypeFilterMenu( float width );
        void DrawDescriptorMenuCategory( FileSystem::Path const& path, Category<TypeSystem::TypeInfo const*> const& category );

    private:

        ImGuiX::FilterWidget                                m_filter;
        TVector<ResourceTypeID>                             m_typeFilter;
        bool                                                m_showRawFiles = false;
        bool                                                m_showDeleteConfirmationDialog = false;

        EventBindingID                                      m_resourceDatabaseUpdateEventBindingID;

        int32_t                                             m_dataDirectoryPathDepth;
        TVector<FileSystem::Path>                           m_foundPaths;

        CategoryTree<TypeSystem::TypeInfo const*>           m_categorizedDescriptorTypes;
        ResourceDescriptorCreator*                          m_pResourceDescriptorCreator = nullptr;

        TreeListView                                        m_treeview;
        ResourceID                                          m_navigationRequest;
        bool                                                m_rebuildTree = false;
    };
}