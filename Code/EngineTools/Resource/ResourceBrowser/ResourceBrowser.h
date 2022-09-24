#pragma once

#include "EngineTools/Core/Widgets/TreeListView.h"
#include "EngineTools/Core/Helpers/CategoryTree.h"
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

    class EE_ENGINETOOLS_API ResourceBrowser final: public TreeListView
    {
    public:

        ResourceBrowser( ToolsContext& toolsContext );
        ~ResourceBrowser();

        char const* const GetWindowName() { return "Resource Browser"; }

        // Returns true if the browser is still open
        bool UpdateAndDraw( UpdateContext const& context );

    private:

        // Tree
        //-------------------------------------------------------------------------

        virtual void RebuildTreeUserFunction() override;
        virtual void DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) override;
        void OnBrowserItemDoubleClicked( TreeListViewItem* pItem );

        // Update visual tree item visibility based on the user filter
        void UpdateVisibility();

        // UI
        //-------------------------------------------------------------------------

        void DrawDialogs();
        void DrawCreationControls( UpdateContext const& context );
        void DrawFilterOptions( UpdateContext const& context );
        bool DrawResourceTypeFilterMenu( float width );
        void DrawDescriptorMenuCategory( FileSystem::Path const& path, Category<TypeSystem::TypeInfo const*> const& category );

    private:

        ToolsContext&                                       m_toolsContext;
        char                                                m_nameFilterBuffer[256];
        TVector<ResourceTypeID>                             m_typeFilter;
        bool                                                m_showRawFiles = false;
        bool                                                m_showDeleteConfirmationDialog = false;

        EventBindingID                                      m_resourceDatabaseUpdateEventBindingID;

        int32_t                                             m_dataDirectoryPathDepth;
        TVector<FileSystem::Path>                           m_foundPaths;

        CategoryTree<TypeSystem::TypeInfo const*>           m_categorizedDescriptorTypes;
        ResourceDescriptorCreator*                          m_pResourceDescriptorCreator = nullptr;
        Resource::RawFileInspector*                         m_pRawResourceInspector = nullptr;
        EventBindingID                                      m_onDoubleClickEventID;
    };
}