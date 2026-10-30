#pragma once
#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Widgets/TreeListView.h"
#include "EngineTools/Import/BulkImporters/BulkImporter.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class EE_ENGINETOOLS_API BulkImporterEditorTool : public EditorTool
    {
    public:

        EE_SINGLETON_EDITOR_TOOL( BulkImporterEditorTool );

        enum class RebuildRequest
        {
            None = 0,
            Full,
            RawFileList
        };

    public:

        BulkImporterEditorTool( ToolsContext const* pToolsContext );
        virtual ~BulkImporterEditorTool();

    private:

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_IMPORT; }
        virtual bool IsSingleWindowTool() const override { return true; }
        virtual bool SupportsMainMenu() const override { return false; }
        virtual void SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;

        void DrawMainWindow( UpdateContext const& context, bool isFocused );

        // Directory Tree
        //-------------------------------------------------------------------------

        void DrawDirectoryView( UpdateContext const& context );
        void RebuildDirectoryTreeView( TreeListViewItem* pRootItem );
        void SetSelectedDirectory( DataPath const& directoryPath );

        // Raw File List
        //-------------------------------------------------------------------------

        void GenerateRawFileList();
        void DrawRawFileView();
        void OnFilesToImportUpdated();

        // Import
        //-------------------------------------------------------------------------

        void DrawImporterView();
        void DrawImporterSettings();
        void DrawImportingView();

        void StartImport();
        void CancelImport();
        void CompleteImport();

        void AsyncImportTask();

    public:

        EventBindingID                                      m_fileRegistryUpdateEventBindingID;
        TaskSystem*                                         m_pTaskSystem = nullptr;

        TVector<BulkImporter*>                              m_importers;
        TVector<FileSystem::Extension>                      m_supportedExtensions;

        TreeListView                                        m_directoryTreeView;
        TreeListViewContext                                 m_directoryTreeViewContext;
        TVector<DataFileRegistry::DirectoryInfo>            m_directoryList;
        DataPath                                            m_selectedDirectory;

        TVector<RawFileInfo>                                m_rawFiles;
        int32_t                                             m_dependencyCheckFileIdx = InvalidIndex;
        TVector<DataPath>                                   m_selectedFiles;
        TVector<DataPath>                                   m_filesToImport;
        RebuildRequest                                      m_rebuildRequest = RebuildRequest::None;

        bool                                                m_isImporting = false;
        AsyncTask                                           m_importTask;
        TVector<BulkImporter*>                              m_activeImporters;
        Log                                                 m_importLog = Log( Log::Mode::ThreadSafe );
        std::atomic<float>                                  m_individualImporterProgress = 0.0f;
        std::atomic<float>                                  m_overallImportProgress = 0.0f;
        std::atomic<char const*>                            m_pCurrentImporterName = nullptr;
        std::atomic<bool>                                   m_cancelSignal = false;
    };
}