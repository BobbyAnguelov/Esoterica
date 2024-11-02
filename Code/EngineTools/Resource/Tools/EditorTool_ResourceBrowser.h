#pragma once
#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Widgets/TreeListView.h"
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
        struct TypeFilter
        {
            String                  m_friendlyName;
            FileSystem::Extension   m_extension;
            ResourceTypeID          m_resourceTypeID;
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
        void DrawDescriptorMenuCategory( FileSystem::Path const& path, Category<TypeSystem::TypeInfo const*> const& category );
        bool DrawDeleteFileConfirmationDialog( UpdateContext const& context, FileRegistry::FileInfo const& fileToDelete );

        void DrawControlRow( UpdateContext const& context );
        void DrawResourceTypeFilterRow( UpdateContext const& context );

        // Folders
        //-------------------------------------------------------------------------

        void DrawFolderView( UpdateContext const& context );
        void DrawFolderContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus );

        void RebuildFolderTreeView( TreeListViewItem* pRootItem );
        void UpdateFolderTreeVisibility();
        void SetSelectedFolder( DataPath const& newFolderPath );

        // Files
        //-------------------------------------------------------------------------

        void DrawFileView( UpdateContext const& context );

        bool DoesFileMatchFilter( FileRegistry::FileInfo const* pFile, bool applyNameFilter = false );
        void GenerateFileList();
        void SortFileList();
        void SetSelectedFile( DataPath const& filePath, bool setFocus );

    private:

        ImGuiX::FilterWidget                                m_filter;
        TVector<TypeFilter>                                 m_allPossibleTypeFilters;
        TVector<int32_t>                                    m_selectedTypeFilterIndices;
        bool                                                m_showRawFiles = false;
        bool                                                m_filterUpdated = false;
        bool                                                m_updateFolderAndFileViews = false;
        SortRule                                            m_sortRule = SortRule::NameAscending;

        EventBindingID                                      m_resourceDatabaseUpdateEventBindingID;

        int32_t                                             m_dataDirectoryPathDepth;
        TVector<FileSystem::Path>                           m_foundPaths;

        CategoryTree<TypeSystem::TypeInfo const*>           m_categorizedDescriptorTypes;
        ResourceDescriptorCreator*                          m_pResourceDescriptorCreator = nullptr;

        NavigationRequest                                   m_navigationRequest;
        bool                                                m_setFocusToSelectedItem = false;

        TreeListView                                        m_folderTreeView;
        DataPath                                            m_selectedFolder;

        TVector<FileRegistry::FileInfo>                     m_fileList;
        TVector<int32_t>                                    m_sortedFileListIndices;
        DataPath                                            m_selectedFile;
    };
}