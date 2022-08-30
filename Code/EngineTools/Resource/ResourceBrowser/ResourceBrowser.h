#pragma once

#include "EngineTools/Core/Widgets/TreeListView.h"
#include "System/Resource/ResourceTypeID.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
    class ToolsContext;
    class ResourceDescriptorCreator;
    namespace Resource{ class RawFileInspector; }

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ResourceBrowser final: public TreeListView
    {
    public:

        ResourceBrowser( ToolsContext& toolsContext );
        ~ResourceBrowser();

        char const* const GetWindowName() { return "Resource Browser"; }
        bool Draw( UpdateContext const& context );

        void RebuildBrowserTree() { RebuildTree(); }

    private:

        virtual void RebuildTreeInternal() override;
        virtual void DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) override;
        virtual void DrawAdditionalUI() override;

        void OnBrowserItemDoubleClicked( TreeListViewItem* pItem );

        TreeListViewItem& FindOrCreateParentForItem( FileSystem::Path const& path );

        void UpdateVisibility();
        void DrawCreateNewDescriptorMenu( FileSystem::Path const& path );
        void DrawCreationControls( UpdateContext const& context );
        void DrawFilterOptions( UpdateContext const& context );
        bool DrawResourceTypeFilterMenu( float width );

    private:

        ToolsContext&                                       m_toolsContext;
        char                                                m_nameFilterBuffer[256];
        TVector<ResourceTypeID>                             m_typeFilter;
        bool                                                m_showRawFiles = false;
        bool                                                m_showDeleteConfirmationDialog = false;

        EventBindingID                                      m_resourceDatabaseUpdateEventBindingID;

        int32_t                                             m_dataDirectoryPathDepth;
        TVector<FileSystem::Path>                           m_foundPaths;

        ResourceDescriptorCreator*                          m_pResourceDescriptorCreator = nullptr;
        Resource::RawFileInspector*                         m_pRawResourceInspector = nullptr;
        EventBindingID                                      m_onDoubleClickEventID;
    };
}