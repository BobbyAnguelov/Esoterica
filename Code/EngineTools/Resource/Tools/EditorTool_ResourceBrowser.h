#pragma once
#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Widgets/TreeListView.h"
#include "Base/Utils/CategoryTree.h"
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
    class ResourceDataFileCreator;
    class RawFileInspector;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ResourceBrowserEditorTool : public EditorTool
    {
        struct TypeFilter
        {
            String                  m_friendlyName;
            FileSystem::Extension   m_extension;
            ResourceTypeID          m_resourceTypeID;
            Color                   m_color;
        };

        struct NavigationRequest
        {
            inline bool IsSet() const { return m_path.IsValid(); }
            inline void Clear() { m_path.Clear(); }

        public:

            DataPath    m_path;
        };

        enum class SortRule
        {
            TypeAscending,
            TypeDescending,
            NameAscending,
            NameDescending
        };

        constexpr static char const * const s_directoryInfoContextMenu = "DirectoryContextMenu";
        constexpr static char const * const s_fileInfoContextMenu = "FileContextMenu";

    public:

        EE_SINGLETON_EDITOR_TOOL( ResourceBrowserEditorTool );

    public:

        ResourceBrowserEditorTool( ToolsContext const* pToolsContext );
        ~ResourceBrowserEditorTool();

        // Try to find, select and focus on a specified data path
        void TryFindAndSelectFile( DataPath const& path )
        { 
            EE_ASSERT( path.IsValid() );
            m_navigationRequest.m_path = path;
        }

    private:

        virtual bool IsSingleWindowTool() const override { return true; }
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_FORMAT_LIST_BULLETED_TYPE; }
        virtual bool SupportsMainMenu() const override { return false; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;

        void DrawBrowserWindow( UpdateContext const& context, bool isFocused );
        void HandleNavigationRequest();

        // UI
        //-------------------------------------------------------------------------

        void DrawCreationControls( UpdateContext const& context );
        void DrawCreateDescriptorMenuCategory( FileSystem::Path const& startingPath, Category<TypeSystem::TypeInfo const*> const& category );

        void DrawResourceTypeFilterRow( UpdateContext const& context );

        // Directory Tree
        //-------------------------------------------------------------------------

        void DrawDirectoryView( UpdateContext const& context );
        void DrawDirectoryTreeContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus );

        void RebuildDirectoryTreeView( TreeListViewItem* pRootItem );
        void SetSelectedDirectory( DataPath const& newFolderPath );

        // Files
        //-------------------------------------------------------------------------

        void DrawFileView( UpdateContext const& context );

        bool DoesFileMatchFilter( FileRegistry::FileInfo const* pFile, bool applyNameFilter = false );
        void GenerateDirectoryContentsList();
        void SortFileList();
        void SetSelectedFile( DataPath const& filePath, bool setFocus );
        void DrawDirectoryInfoContextMenu( FileRegistry::DirectoryInfo const& directoryInfo, bool isFileListView );
        void DrawFileInfoContextMenu( FileRegistry::FileInfo const& fileInfo );

    private:

        ImGuiX::FilterWidget                                m_filter;
        TVector<TypeFilter>                                 m_allPossibleTypeFilters;
        TVector<int32_t>                                    m_selectedTypeFilterIndices;
        bool                                                m_showRawFiles = false;
        bool                                                m_filterUpdated = false;
        bool                                                m_refreshRequested = false;
        SortRule                                            m_sortRule = SortRule::NameAscending;

        EventBindingID                                      m_resourceDatabaseUpdateEventBindingID;

        int32_t                                             m_dataDirectoryPathDepth;
        TVector<FileSystem::Path>                           m_foundPaths;

        CategoryTree<TypeSystem::TypeInfo const*>           m_categorizedDescriptorTypes;

        NavigationRequest                                   m_navigationRequest;
        bool                                                m_setFocusToSelectedItem = false;

        TreeListView                                        m_directoryTreeView;
        DataPath                                            m_selectedDirectory;

        // Selected Directory Contents
        //-------------------------------------------------------------------------

        TVector<FileRegistry::DirectoryInfo>                m_directoryList;
        TVector<FileRegistry::FileInfo>                     m_fileList;
        TVector<int32_t>                                    m_sortedFileListIndices;
        DataPath                                            m_selectedItem;
    };
}