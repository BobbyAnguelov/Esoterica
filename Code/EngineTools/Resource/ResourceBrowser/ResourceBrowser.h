#pragma once

#include "EngineTools/Core/Widgets/TreeListView.h"
#include "EngineTools/Core/CategoryTree.h"
#include "System/Resource/ResourceTypeID.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
    class ToolsContext;
    class ResourceDescriptorCreator;
    namespace Resource{ class RawFileInspector; }
    namespace TypeSystem { class TypeInfo; }

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ResourceBrowser final
    {
    public:

        ResourceBrowser( ToolsContext& toolsContext );
        ~ResourceBrowser();

        char const* GetWindowName() { return "Resource Browser"; }

        // Returns true if the browser is still open
        bool UpdateAndDraw( UpdateContext const& context );

    private:

        void UpdateVisibility();
        void DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus );
        void RebuildTreeView( TreeListViewItem* pRootItem );

        // UI
        //-------------------------------------------------------------------------

        void DrawDialogs();
        void DrawCreationControls( UpdateContext const& context );
        void DrawFilterOptions( UpdateContext const& context );
        bool DrawResourceTypeFilterMenu( float width );
        void DrawDescriptorMenuCategory( FileSystem::Path const& path, Category<TypeSystem::TypeInfo const*> const& category );

    private:

        ToolsContext&                                       m_toolsContext;
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
        bool                                                m_rebuildTree = false;
    };
}