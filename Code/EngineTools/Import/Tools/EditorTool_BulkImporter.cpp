#include "EditorTool_BulkImporter.h"
#include "EngineTools/Import/BulkImporters/BulkImporter_Render.h"
#include "EngineTools/Import/BulkImporters/BulkImporter_Animation.h"
#include "EngineTools/Import/BulkImporters/BulkImporter_Physics.h"
#include "EngineTools/FileSystem/DataFileRegistry.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Engine/UpdateContext.h"
#include "Base/Threading/TaskSystem.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class BulkImporterDirectoryTreeItem final : public TreeListViewItem
    {

    public:

        BulkImporterDirectoryTreeItem( TreeListViewItem* pParent, ToolsContext const& toolsContext, DataFileRegistry::DirectoryInfo const* pDirectoryEntry )
            : TreeListViewItem( pParent )
            , m_toolsContext( toolsContext )
            , m_nameID( pDirectoryEntry->m_filePath.GetDirectoryName() )
            , m_path( pDirectoryEntry->m_filePath )
            , m_dataPath( pDirectoryEntry->m_dataPath )
            , m_isRootPath( m_dataPath.IsRootPath() )
            , m_isLeaf( pDirectoryEntry->m_directories.empty() )
        {
            EE_ASSERT( m_path.IsValid() );
            EE_ASSERT( m_dataPath.IsValid() );

            //-------------------------------------------------------------------------

            for ( auto const& pChildDirectory : pDirectoryEntry->m_directories )
            {
                CreateChild<BulkImporterDirectoryTreeItem>( m_toolsContext, pChildDirectory );
            }
        }

        virtual uint64_t GetUniqueID() const override { return m_dataPath.GetID(); }
        virtual bool HasContextMenu() const override { return true; }
        virtual bool IsActivatable() const override { return false; }
        virtual bool HasIcon() const override { return true; }
        virtual char const* GetIcon() const override { return m_isRootPath ? EE_ICON_DATABASE : EE_ICON_FOLDER; }
        virtual bool IsLeaf() const override { return m_isLeaf; }
        virtual Color GetIconColor() const override { return m_isRootPath ? Colors::LightSkyBlue : Colors::SandyBrown; }
        virtual char const* GetDisplayName() const override { return m_nameID.c_str(); }

        // File System Info
        //-------------------------------------------------------------------------

        inline FileSystem::Path const& GetFilePath() const { return m_path; }
        inline DataPath const& GetDataPath() const { return m_dataPath; }

        // Signals
        //-------------------------------------------------------------------------

        bool OnUserActivate()
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

        virtual bool OnDoubleClick() override { return OnUserActivate(); }
        virtual bool OnEnterPressed() override { return OnUserActivate(); }

    protected:

        ToolsContext const&                     m_toolsContext;
        StringID                                m_nameID;
        FileSystem::Path                        m_path;
        DataPath                                m_dataPath;
        bool                                    m_isRootPath = false;
        bool                                    m_isLeaf = false;
    };

    //-------------------------------------------------------------------------

    BulkImporterEditorTool::BulkImporterEditorTool( ToolsContext const* pToolsContext )
        : EditorTool( pToolsContext, "Bulk Importer" )
        , m_importTask( 1, [this] ( TaskSetPartition range, uint32_t threadnum ) { AsyncImportTask(); } )
    {
        m_fileRegistryUpdateEventBindingID = m_pToolsContext->m_pDataFileRegistry->OnFileSystemCacheUpdated().Bind( [this] () { m_rebuildRequest = RebuildRequest::Full; } );

        //-------------------------------------------------------------------------

        m_importers.emplace_back( EE::New<StaticMeshBulkImporter>( m_pToolsContext ) );
        m_importers.emplace_back( EE::New<SkeletalMeshBulkImporter>( m_pToolsContext ) );
        m_importers.emplace_back( EE::New<PhysicsMeshBulkImporter>( m_pToolsContext ) );
        m_importers.emplace_back( EE::New<AnimationSkeletonBulkImporter>( m_pToolsContext ) );
        m_importers.emplace_back( EE::New<AnimationClipBulkImporter>( m_pToolsContext ) );

        for ( auto pImporter : m_importers )
        {
            pImporter->Initialize();

            for ( FileSystem::Extension const& ext : pImporter->GetSupportedFileExtensions() )
            {
                VectorEmplaceBackUnique( m_supportedExtensions, ext );
            }
        }

        //-------------------------------------------------------------------------

        m_directoryTreeView.SetFlag( TreeListView::ExpandItemsOnlyViaArrow, true );
        m_directoryTreeView.SetFlag( TreeListView::ExpandItemsWithDoubleClick, true );
        m_directoryTreeView.SetFlag( TreeListView::UseSmallFont, false );
        m_directoryTreeView.SetFlag( TreeListView::SortTree, true );
        m_directoryTreeView.SetFlag( TreeListView::ShowBranchesFirst, false );

        auto RebuildDirectoryTreeView = [this] ( TreeListViewItem * pRootItem )
        {
            EE_ASSERT( m_pToolsContext->m_pDataFileRegistry->IsFileSystemCacheBuilt() );
            auto pDataDirectory = m_pToolsContext->m_pDataFileRegistry->GetRawResourceDirectoryEntry();

            //-------------------------------------------------------------------------

            pRootItem->DestroyChildren();
            auto pDataDirItem = pRootItem->CreateChild<BulkImporterDirectoryTreeItem>( *m_pToolsContext, pDataDirectory );
            pDataDirItem->SetExpanded( false, true );
            pDataDirItem->SetExpanded( true, false );
        };

        m_directoryTreeViewContext.m_rebuildTreeFunction = RebuildDirectoryTreeView;
    }

    BulkImporterEditorTool::~BulkImporterEditorTool()
    {
        m_pToolsContext->m_pDataFileRegistry->OnFileSystemCacheUpdated().Unbind( m_fileRegistryUpdateEventBindingID );

        //-------------------------------------------------------------------------

        for ( auto pImporter : m_importers )
        {
            pImporter->Shutdown();
            EE::Delete( pImporter );
        }
        m_importers.clear();
    }

    void BulkImporterEditorTool::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Importer" ).c_str(), dockspaceID );
    }

    void BulkImporterEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        CreateToolWindow( "Importer", [this] ( UpdateContext const& context, bool isFocused ) { DrawMainWindow( context, isFocused ); } );

        //-------------------------------------------------------------------------

        m_pTaskSystem = context.GetSystem<TaskSystem>();

        //-------------------------------------------------------------------------

        m_directoryTreeView.RebuildTree( m_directoryTreeViewContext );
    }

    void BulkImporterEditorTool::Shutdown( UpdateContext const& context )
    {
        if ( m_isImporting )
        {
            CancelImport();
        }

        m_selectedDirectory.Clear();
        m_rawFiles.clear();
        m_selectedFiles.clear();
        m_filesToImport.clear();
        m_rebuildRequest = RebuildRequest::None;
        m_directoryList.clear();

        m_pTaskSystem = nullptr;

        EditorTool::Shutdown( context );
    }

    void BulkImporterEditorTool::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        if ( !m_isImporting )
        {
            if ( m_rebuildRequest != RebuildRequest::None )
            {
                if ( m_rebuildRequest == RebuildRequest::Full )
                {
                    TreeListViewContext treeViewContext;
                    treeViewContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildDirectoryTreeView( pRootItem ); };
                    m_directoryTreeView.RebuildTree( treeViewContext );
                }

                GenerateRawFileList();
                m_rebuildRequest = RebuildRequest::None;
            }
        }

        //-------------------------------------------------------------------------

        if ( m_rebuildRequest == RebuildRequest::None && m_pToolsContext->m_pDataFileRegistry->IsDependencyCacheBuilt() )
        {
            int32_t const numFilesToInspect = (int32_t) m_rawFiles.size();
            if ( numFilesToInspect > 0 && m_dependencyCheckFileIdx < numFilesToInspect )
            {
                if ( m_dependencyCheckFileIdx == InvalidIndex )
                {
                    m_dependencyCheckFileIdx = 0;
                }

                int32_t const endInspectedIndex = Math::Min( m_dependencyCheckFileIdx + 5, numFilesToInspect );
                for ( m_dependencyCheckFileIdx; m_dependencyCheckFileIdx < endInspectedIndex; m_dependencyCheckFileIdx++ )
                {
                    RawFileInfo& rawFileInfo = m_rawFiles[m_dependencyCheckFileIdx];
                    m_pToolsContext->m_pDataFileRegistry->GetAllResourcesThatDependOnFile( rawFileInfo.m_path, rawFileInfo.m_dependentResources );
                    rawFileInfo.m_hasRunDependencyCheck = true;
                }
            }
        }
    }

    void BulkImporterEditorTool::DrawMainWindow( UpdateContext const& context, bool isFocused )
    {
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
            ImGui::BeginDisabled( m_isImporting );
            DrawDirectoryView( context );
            ImGui::EndDisabled();

            ImGui::TableNextColumn();
            ImGui::BeginDisabled( m_isImporting );
            DrawRawFileView();
            ImGui::EndDisabled();

            ImGui::TableNextColumn();
            DrawImporterView();

            ImGui::EndTable();
        }
    }

    void BulkImporterEditorTool::DrawDirectoryView( UpdateContext const& context )
    {
        TreeListViewContext treeViewContext;
        treeViewContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildDirectoryTreeView( pRootItem ); };

        if ( m_directoryTreeView.UpdateAndDraw( treeViewContext ) )
        {
            TVector<TreeListViewItem*> const& selection = m_directoryTreeView.GetSelection();
            if ( selection.empty() )
            {
                SetSelectedDirectory( DataPath() );
            }
            else
            {
                SetSelectedDirectory( static_cast<BulkImporterDirectoryTreeItem const*>( selection.back() )->GetDataPath() );
            }
        }
    }

    void BulkImporterEditorTool::RebuildDirectoryTreeView( TreeListViewItem* pRootItem )
    {
        EE_ASSERT( m_pToolsContext->m_pDataFileRegistry->IsFileSystemCacheBuilt() );
        auto pDataDirectory = m_pToolsContext->m_pDataFileRegistry->GetRawResourceDirectoryEntry();

        //-------------------------------------------------------------------------

        pRootItem->DestroyChildren();
        auto pDataDirItem = pRootItem->CreateChild<BulkImporterDirectoryTreeItem>( *m_pToolsContext, pDataDirectory );
        pDataDirItem->SetExpanded( false, true );
        pDataDirItem->SetExpanded( true, false );
    }

    void BulkImporterEditorTool::SetSelectedDirectory( DataPath const& directoryPath )
    {
        m_selectedDirectory = directoryPath;

        // Do not override a previous full rebuild request
        if ( m_rebuildRequest == RebuildRequest::None )
        {
            m_rebuildRequest = RebuildRequest::RawFileList;
        }
    }

    void BulkImporterEditorTool::GenerateRawFileList()
    {
        DataFileRegistry::DirectoryInfo const* pDirectoryInfo = m_pToolsContext->m_pDataFileRegistry->FindDirectoryEntry( m_selectedDirectory );
        if ( pDirectoryInfo == nullptr )
        {
            pDirectoryInfo = m_pToolsContext->m_pDataFileRegistry->FindDirectoryEntry( DataPath( "Data://" ) );
        }

        TVector<DataFileRegistry::FileInfo const*> files;
        files.reserve( 25000 );

        pDirectoryInfo->GetAllFiles( files, true );

        //-------------------------------------------------------------------------

        m_rawFiles.clear();
        m_dependencyCheckFileIdx = InvalidIndex;

        for ( DataFileRegistry::FileInfo const* pFileInfo : files )
        {
            if ( !pFileInfo->IsUnknownFileType() )
            {
                continue;
            }

            if ( !VectorContains( m_supportedExtensions, pFileInfo->m_extension ) )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            auto& rawFileInfo = m_rawFiles.emplace_back();
            rawFileInfo.m_path = pFileInfo->m_dataPath;
            rawFileInfo.m_filePath = pFileInfo->m_filePath;
            rawFileInfo.m_extension = pFileInfo->m_extension;
        }

        auto SortPredicate = [] ( RawFileInfo const& a, RawFileInfo const& b )
        {
            return _stricmp( a.m_path.c_str(), b.m_path.c_str() ) < 0;
        };

        eastl::sort( m_rawFiles.begin(), m_rawFiles.end(), SortPredicate );

        // Update selected and import file lists
        //-------------------------------------------------------------------------

        TVector<DataPath> tempSelectedFiles;
        TVector<DataPath> tempFilesToImport;

        tempSelectedFiles.swap( m_selectedFiles );
        tempFilesToImport.swap( m_filesToImport );

        for ( auto const& path : tempSelectedFiles )
        {
            if ( VectorContains( m_rawFiles, path ) )
            {
                m_selectedFiles.emplace_back( path );
            }
        }

        for ( auto const& path : tempFilesToImport )
        {
            if ( VectorContains( m_rawFiles, path ) )
            {
                m_filesToImport.emplace_back( path );
            }
        }

        OnFilesToImportUpdated();
    }

    void BulkImporterEditorTool::OnFilesToImportUpdated()
    {
        TVector<RawFileInfo const*> filesToImport;

        for ( auto const& dp : m_filesToImport )
        {
            RawFileInfo* pFoundFile = VectorFind( m_rawFiles, dp );
            if ( pFoundFile != nullptr )
            {
                filesToImport.emplace_back( pFoundFile );
            }
        }

        for ( auto pImporter : m_importers )
        {
            pImporter->SetRawFiles( filesToImport );
        }
    }

    void Import::BulkImporterEditorTool::DrawRawFileView()
    {
        int32_t const numFiles = (int32_t) m_rawFiles.size();

        auto ApplySelectionRequests = [this, numFiles] ( ImGuiMultiSelectIO* pMSIO )
        {
            for ( ImGuiSelectionRequest const& req : pMSIO->Requests )
            {
                if ( req.Type == ImGuiSelectionRequestType_SetAll )
                {
                    m_selectedFiles.clear();

                    if ( req.Selected )
                    {
                        for ( int32_t i = 0; i < numFiles; i++ )
                        {
                            m_selectedFiles.emplace_back( m_rawFiles[i].m_path );
                        }
                    }
                }
                else if ( req.Type == ImGuiSelectionRequestType_SetRange )
                {
                    if ( req.Selected )
                    {
                        for ( int64_t i = req.RangeFirstItem; i <= req.RangeLastItem; i++ )
                        {
                            VectorEmplaceBackUnique( m_selectedFiles, m_rawFiles[i].m_path );
                        }
                    }
                    else
                    {
                        for ( int64_t i = req.RangeFirstItem; i <= req.RangeLastItem; i++ )
                        {
                            m_selectedFiles.erase_first_unsorted( m_rawFiles[i].m_path );
                        }
                    }
                }
            }
        };

        //-------------------------------------------------------------------------

        char const* const importedReferencesTest = EE_ICON_IMPORT" Imported As";
        float const importedReferencesColumnWidth = ImGuiX::CalculateButtonWidth( importedReferencesTest );

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 8, 4 ) );
        ImGui::PushStyleColor( ImGuiCol_Header, Colors::Transparent );
        ImGui::PushStyleColor( ImGuiCol_HeaderHovered, Colors::Transparent );
        ImGui::PushStyleColor( ImGuiCol_HeaderActive, Colors::Transparent );
        if ( ImGui::BeginTable( "TableItems", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable, ImGui::GetContentRegionAvail() - ImVec2( 0, 1 ) ) )
        {
            ImGui::TableSetupScrollFreeze( 0, 1 );
            ImGui::TableSetupColumn( "##Selected", ImGuiTableColumnFlags_PreferSortAscending | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, ImGui::GetFrameHeight() );
            ImGui::TableSetupColumn( "Path", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortAscending | ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Imported", ImGuiTableColumnFlags_PreferSortAscending | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, importedReferencesColumnWidth );

            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 8, 8 ) );
            ImGui::TableHeadersRow();
            ImGui::PopStyleVar();

            //-------------------------------------------------------------------------

            ImGuiMultiSelectFlags const selectionFlags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_ClearOnClickVoid;
            ImGuiMultiSelectIO* pMSIO = ImGui::BeginMultiSelect( selectionFlags, -1, numFiles );
            ApplySelectionRequests( pMSIO );

            //-------------------------------------------------------------------------

            ImGuiListClipper clipper;
            clipper.Begin( numFiles );
            while ( clipper.Step() )
            {
                for ( int32_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                {
                    ImGui::PushID( i );

                    RawFileInfo const* pRawFileInfo = &m_rawFiles[i];

                    ImGui::TableNextRow();

                    bool isSelected = VectorContains( m_selectedFiles, pRawFileInfo->m_path );
                    if ( isSelected )
                    {
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, ImGuiX::Style::s_colorGray1 );
                    }

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();
                    bool isSelectedForImport = VectorContains( m_filesToImport, pRawFileInfo->m_path );
                    if ( ImGuiX::Checkbox( "##Import", &isSelectedForImport ) )
                    {
                        if ( isSelectedForImport )
                        {
                            VectorEmplaceBackUnique( m_filesToImport, pRawFileInfo->m_path );
                        }
                        else
                        {
                            m_filesToImport.erase_first_unsorted( pRawFileInfo->m_path );
                        }

                        //-------------------------------------------------------------------------

                        for ( DataPath const& selectedFilePath : m_selectedFiles )
                        {
                            if ( isSelectedForImport )
                            {
                                VectorEmplaceBackUnique( m_filesToImport, selectedFilePath );
                            }
                            else
                            {
                                m_filesToImport.erase_first_unsorted( selectedFilePath );
                            }
                        }

                        OnFilesToImportUpdated();
                    }

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    ImGui::SetNextItemSelectionUserData( i );
                    ImGui::Selectable( pRawFileInfo->m_path.c_str(), isSelected, ImGuiSelectableFlags_AllowOverlap | ImGuiSelectableFlags_SpanAllColumns );
                    if ( ImGui::IsItemHovered() && !isSelected )
                    {
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, ImGuiX::Style::s_colorGray0 );
                    }

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();

                    if ( !pRawFileInfo->m_hasRunDependencyCheck )
                    {
                        ImGuiX::DrawSpinner( "Waiting for dependency check..." );
                    }
                    else
                    {
                        if ( pRawFileInfo->m_dependentResources.empty() )
                        {
                            ImGui::Text( "Not Imported" );
                        }
                        else
                        {
                            ImGui::PopStyleColor( 3 );

                            auto ShowImportedResourceMenu = [this, pRawFileInfo] ()
                            {
                                for ( DataPath const& importedResource : pRawFileInfo->m_dependentResources )
                                {
                                    if ( ImGui::MenuItem( importedResource.c_str() ) )
                                    {
                                        m_pToolsContext->TryOpenResource( ResourceID( importedResource ) );
                                    }
                                }
                            };

                            ImGuiX::DropDownButton( importedReferencesTest, ShowImportedResourceMenu );

                            ImGui::PushStyleColor( ImGuiCol_Header, Colors::Transparent );
                            ImGui::PushStyleColor( ImGuiCol_HeaderHovered, Colors::Transparent );
                            ImGui::PushStyleColor( ImGuiCol_HeaderActive, Colors::Transparent );
                        }
                    }

                    ImGui::PopID();
                }
            }

            //-------------------------------------------------------------------------

            pMSIO = ImGui::EndMultiSelect();
            ApplySelectionRequests( pMSIO );

            //-------------------------------------------------------------------------

            ImGui::EndTable();
        }
        ImGui::PopStyleColor( 3 );
        ImGui::PopStyleVar();
    }

    void Import::BulkImporterEditorTool::DrawImporterView()
    {
        if ( m_isImporting )
        {
            DrawImportingView();
        }
        else
        {
            DrawImporterSettings();
        }
    }

    void BulkImporterEditorTool::DrawImporterSettings()
    {
        char const* const noFilesToImportLabel = "No files selected for import";
        ImVec2 textSize = ImGui::CalcTextSize( noFilesToImportLabel );

        if ( m_filesToImport.empty() )
        {
            ImGui::SameLine( ( ImGui::GetContentRegionAvail().x - textSize.x ) / 2 );
            ImGui::Text( noFilesToImportLabel );
            return;
        }

        char const* const noImportersLabel = "No valid importers for file types";
        textSize = ImGui::CalcTextSize( noImportersLabel );

        bool canImport = false;
        bool allImportersDisabled = true;
        for ( auto pImporter : m_importers )
        {
            if ( pImporter->HasFilesToImport() )
            {
                canImport = true;
            }

            if ( pImporter->IsEnabledAndReadyForImport() )
            {
                allImportersDisabled = false;
            }
        }

        if ( !canImport )
        {
            ImGui::SameLine( ( ImGui::GetContentRegionAvail().x - textSize.x ) / 2 );
            ImGui::Text( noImportersLabel );
            return;
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginChild( "##ImportSettings", ImGui::GetContentRegionAvail() - ImVec2( 0, ImGui::GetFrameHeightWithSpacing() ) ) )
        {
            for ( auto pImporter : m_importers )
            {
                pImporter->Draw();
            }
        }
        ImGui::EndChild();

        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( allImportersDisabled );
        if ( ImGuiX::IconButtonColored( EE_ICON_IMPORT, "Import", Colors::Green, Colors::White, ImGuiX::Style::s_colorText, ImVec2( -1, 0 ), true ) )
        {
            StartImport();
        }
        ImGui::EndDisabled();
    }

    void BulkImporterEditorTool::DrawImportingView()
    {
        if ( ImGui::BeginChild( "##Importing", ImGui::GetContentRegionAvail() - ImVec2( 0, ImGui::GetFrameHeightWithSpacing() ) ) )
        {
            ImGui::SeparatorText( "Import Progress" );

            constexpr static char const* const pOverallProgressLabel = "Overall Progress";
            ImGui::Text( pOverallProgressLabel );
            ImGui::ProgressBar( m_overallImportProgress, ImVec2( -1, 0 ) );

            InlineString const importerNameStr( InlineString::CtorSprintf(), "Importer: %s", m_pCurrentImporterName.load() );
            ImGui::Text( importerNameStr.c_str() );
            ImGui::ProgressBar( m_individualImporterProgress, ImVec2( -1, 0 ) );

            //-------------------------------------------------------------------------

            ImGui::SeparatorText( "Import Log" );

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
                if ( ImGui::BeginTable( "Log", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImGui::GetContentRegionAvail() ) )
                {
                    ImGui::TableSetupColumn( "##severity", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize( EE_ICON_CHECK ).x );
                    ImGui::TableSetupColumn( "Path", ImGuiTableColumnFlags_WidthStretch, 0.4f );
                    ImGui::TableSetupColumn( "Message", ImGuiTableColumnFlags_WidthStretch, 0.6f );

                    // This will copy all log entries 
                    // - if this is slow then transfer the log entries from the import log to a low cached log each UI update
                    for ( Log::Entry const& entry : m_importLog.GetLogEntriesThreadsafe() )
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGuiX::DrawSeverityIcon( entry.m_severity );

                        ImGui::TableNextColumn();
                        RawFileInfo const* pInfo = (RawFileInfo *) entry.m_userData;
                        ImGui::Text( "%s", ( pInfo != nullptr ) ? pInfo->m_path.c_str() : "No file info provided via user info" );

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted( entry.m_message.c_str() );
                    }

                    ImGui::EndTable();
                }
            }
        }
        ImGui::EndChild();

        //-------------------------------------------------------------------------

        if ( m_importTask.GetIsComplete() )
        {
            if ( ImGuiX::IconButtonColored( EE_ICON_CHECK, "Ok", ImGuiX::Style::s_colorGray1, Colors::Lime, ImGuiX::Style::s_colorText, ImVec2( -1, 0 ), true ) )
            {
                CompleteImport();
            }
        }
        else
        {
            if ( ImGuiX::IconButtonColored( EE_ICON_CANCEL, "Stop Importing", ImGuiX::Style::s_colorGray1, Colors::Red, Colors::White, ImVec2( -1, 0 ), true ) )
            {
                CancelImport();
            }
        }
    }

    void BulkImporterEditorTool::StartImport()
    {
        EE_ASSERT( m_importTask.GetIsComplete() );
        EE_ASSERT( !m_isImporting );
        EE_ASSERT( m_activeImporters.empty() );

        for ( auto pImporter : m_importers )
        {
            if ( pImporter->IsEnabledAndReadyForImport() )
            {
                m_activeImporters.emplace_back( pImporter );
            }
        }

        EE_ASSERT( !m_activeImporters.empty() );

        //-------------------------------------------------------------------------

        m_pCurrentImporterName = m_activeImporters[0]->GetTitle();
        m_cancelSignal = false;
        m_individualImporterProgress = 0.0f;
        m_overallImportProgress = 0.0f;
        m_importLog.Clear();
        m_pTaskSystem->ScheduleTask( &m_importTask );
        m_isImporting = true;
    }

    void BulkImporterEditorTool::AsyncImportTask()
    {
        int32_t const numActiveImporters = (int32_t) m_activeImporters.size();
        for ( int32_t i = 0; i < numActiveImporters; i++ )
        {
            if ( m_cancelSignal )
            {
                return;
            }

            m_pCurrentImporterName = m_activeImporters[i]->GetTitle();
            m_overallImportProgress = float( i ) / numActiveImporters;
            m_activeImporters[i]->Import( m_importLog, m_individualImporterProgress, m_cancelSignal );
        }

        m_overallImportProgress = 1.0f;
    }

    void BulkImporterEditorTool::CancelImport()
    {
        m_cancelSignal = true;
        m_pTaskSystem->WaitForTask( &m_importTask );
    }

    void BulkImporterEditorTool::CompleteImport()
    {
        m_activeImporters.clear();
        m_individualImporterProgress = 0.0f;
        m_overallImportProgress = 0.0f;
        m_pCurrentImporterName = nullptr;
        m_cancelSignal = false;
        m_importLog.Clear();
        m_isImporting = false;
    }
}