#pragma once
#include "EngineTools/Import/RawFileInspector.h"
#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Core/CategoryTree.h"
#include "EngineTools/Widgets/TreeListView.h"

//-------------------------------------------------------------------------

namespace EE
{
    class TreeListViewItem;
}

namespace EE::Resource
{
    class ImportSettings;
    class ImporterTreeItem;
    struct ResourceDescriptor;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ResourceImporterEditorTool : public EditorTool
    {
    public:

        EE_SINGLETON_EDITOR_TOOL( ResourceImporterEditorTool );

    private:

        struct SelectedRawFile
        {
            inline bool IsSet() const { return m_resourcePath.IsValid(); }

            void Clear();

        public:

            DataPath                                    m_resourcePath;
            FileSystem::Path                                m_filePath;
            String                                          m_extension;
            TVector<DataPath>                           m_dependentResources;
            TVector<Import::ImportableItem*>                m_importableItems;
            String                                          m_warnings;
            String                                          m_errors;
            Import::InspectionResult                        m_inspectionResult = Import::InspectionResult::Uninspected;
        };

    public:

        ResourceImporterEditorTool( ToolsContext const* pToolsContext );
        virtual ~ResourceImporterEditorTool();

    private:

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_IMPORT; }
        virtual bool IsSingleWindowTool() const override { return true; }
        virtual bool SupportsMainMenu() const override { return false; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;

        void UpdateVisibility();
        void RebuildTreeView( TreeListViewItem* pRootItem );
        void OnResourceDatabaseUpdated();

        void UpdateSelectedFile( ImporterTreeItem const* pSelectedItem );

        void HandleItemSelection( Import::ImportableItem* pItem, bool isMultiSelectionEnabled );
        void SelectAllItems( TVector<Import::ImportableItem*> itemsToSelect );
        void OnSelectionChanged();

        void DrawImporterWindow( UpdateContext const& context, bool isFocused );
        void DrawFileTree( UpdateContext const& context );
        void DrawFileInfo( UpdateContext const& context );

        void DrawImportSettings( UpdateContext const& context );
        void DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus );

    private:

        ImGuiX::FilterWidget                                m_filter;
        EventBindingID                                      m_resourceDatabaseUpdateEventBindingID;
        int32_t                                             m_dataDirectoryPathDepth;
        TVector<FileSystem::Path>                           m_foundPaths;
        TVector<ImportSettings*>                            m_importSettings;
        ImportSettings*                                     m_pActiveSettings = nullptr;

        TreeListView                                        m_treeview;
        bool                                                m_rebuildTree = true;

        SelectedRawFile                                     m_selectedFile;
        TVector<Import::ImportableItem*>                    m_selectedImportableItems;
    };
}