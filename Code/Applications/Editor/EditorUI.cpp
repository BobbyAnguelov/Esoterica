#include "EditorUI.h"
#include "EditorTool_GamePreviewer.h"
#include "Game/ToolsUI/GameDebugUI.h"
#include "EngineTools/MapEditor/MapEditor.h"
#include "EngineTools/Resource/Tools/EditorTool_ResourceBrowser.h"
#include "EngineTools/Resource/Tools/EditorTool_ResourceSystem.h"
#include "EngineTools/Resource/Tools/EditorTool_ResourceImporter.h"
#include "EngineTools/Resource/Tools/EditorTool_ResourceDependencyViewer.h"
#include "EngineTools/Resource/Tools/EditorTool_ResourceBulkUpdate.h"
#include "EngineTools/Resource/Dialogs/EditorDialog_DataFileCreator.h"
#include "EngineTools/Core/Tools/EditorTool_SystemLog.h"
#include "EngineTools/Core/Tools/EditorTool_SystemSettings.h"
#include "EngineTools/Core/Tools/EditorTool_MemoryTracker.h"
#include "EngineTools/Core/ToolsEmbeddedResources.inl"
#include "EngineTools/Core/Test/UITest.h"
#include "EngineTools/Core/SystemDialogs.h"
#include "Engine/Camera/Components/Component_ToolsCamera.h"
#include "Engine/Camera/Systems/WorldSystem_Camera.h"
#include "Engine/ToolsUI/EngineDebugUI.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Render/Imgui/ImguiImageCache.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/Resource/Settings/Settings_Resource.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/ThirdParty/implot/implot.h"

//-------------------------------------------------------------------------

namespace EE
{
    EditorUI::~EditorUI()
    {
        EE_ASSERT( m_editorTools.empty() );
        EE_ASSERT( m_pMapEditor == nullptr );
        EE_ASSERT( m_pGamePreviewer == nullptr );

        EE_ASSERT( m_pWorldManager == nullptr );
    }

    void EditorUI::SetStartupMap( ResourceID const& mapID )
    {
        EE_ASSERT( mapID.IsValid() );

        if ( mapID.GetResourceTypeID() == EntityModel::EntityMapDescriptor::GetStaticResourceTypeID() )
        {
            m_startupMapResourceID = mapID;
        }
        else
        {
            EE_LOG_ERROR( LogCategory::Tools, "Editor - Invalid startup map resource supplied: %s", m_startupMapResourceID.c_str() );
        }
    }

    void EditorUI::Initialize( UpdateContext const& context, ImGuiX::ImageCache* pImageCache )
    {
        m_pDialogManager = &m_dialogManager;

        // ImGui
        //-------------------------------------------------------------------------

        m_editorWindowClass.ClassId = ImHashStr( "EditorWindowClass" );
        m_editorWindowClass.DockingAllowUnclassed = false;
        m_editorWindowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoAutoMerge;
        m_editorWindowClass.ViewportFlagsOverrideClear = ImGuiViewportFlags_NoDecoration | ImGuiViewportFlags_NoTaskBarIcon;
        m_editorWindowClass.ParentViewportId = 0; // Top level window
        m_editorWindowClass.DockingAllowUnclassed = false;
        m_editorWindowClass.DockingAlwaysTabBar = true;

        // Systems
        //-------------------------------------------------------------------------

        m_pTypeRegistry = context.GetSystem<TypeSystem::TypeRegistry>();
        m_pSystemRegistry = context.GetSystemRegistry();
        m_pWorldManager = context.GetSystem<EntityWorldManager>();
        auto pTaskSystem = context.GetSystem<TaskSystem>();

        // Resources
        //-------------------------------------------------------------------------

        auto pResourceSystem = context.GetSystem<Resource::ResourceSystem>();
        m_fileRegistry.Initialize( m_pTypeRegistry, pTaskSystem, pResourceSystem->GetSettings().m_sourceDataDirectoryPath, pResourceSystem->GetSettings().m_compiledResourceDirectoryPath );
        m_resourceDeletedEventID = m_fileRegistry.OnFileDeleted().Bind( [this] ( DataPath const& dataPath ) { OnFileDeleted( dataPath ); } );
        m_pFileRegistry = &m_fileRegistry;

        // Icons/Images
        //-------------------------------------------------------------------------

        EE_ASSERT( pImageCache != nullptr );
        m_pImageCache = pImageCache;
        m_editorIcon = m_pImageCache->LoadImageFromMemoryBase64( g_encodedDataGreen, 3328 );

        // Map Editor
        //-------------------------------------------------------------------------

        auto& request = m_toolOperations.emplace_back();
        request.m_type = ToolOperation::CreateMapEditor;

        // Create default editor tools
        //-------------------------------------------------------------------------

        CreateTool<SystemLogEditorTool>( this );
        CreateTool<Resource::ResourceBrowserEditorTool>( this );
    }

    void EditorUI::Shutdown( UpdateContext const& context )
    {
        // Map Editor
        //-------------------------------------------------------------------------

        m_pMapEditor->OnGamePreviewStartRequested().Unbind( m_gamePreviewStartRequestEventBindingID );
        m_pMapEditor->OnGamePreviewStopRequested().Unbind( m_gamePreviewStopRequestEventBindingID );

        EE_ASSERT( m_pMapEditor != nullptr );
        m_pMapEditor = nullptr;
        m_pGamePreviewer = nullptr;

        // Editor Tools
        //-------------------------------------------------------------------------

        for ( auto& creationRequest : m_toolOperations )
        {
            if ( creationRequest.m_type == ToolOperation::InitializeTool )
            {
                EE::Delete( creationRequest.m_pEditorTool );
            }
        }

        while ( !m_editorTools.empty() )
        {
            DestroyTool( context, m_editorTools[0], true );
        }

        // Misc
        //-------------------------------------------------------------------------

        m_pImageCache->UnloadImage( m_editorIcon );

        // Resources
        //-------------------------------------------------------------------------

        m_pFileRegistry = nullptr;
        m_fileRegistry.OnFileDeleted().Unbind( m_resourceDeletedEventID );
        m_fileRegistry.Shutdown();

        // Systems
        //-------------------------------------------------------------------------

        m_pWorldManager = nullptr;
        m_pTypeRegistry = nullptr;
    }

    //-------------------------------------------------------------------------
    // Tools Context
    //-------------------------------------------------------------------------

    bool EditorUI::TryOpenDataFile( DataPath const& path ) const
    {
        EE_ASSERT( path.IsValid() && path.IsFilePath() );

        // Handle maps explicitly
        //-------------------------------------------------------------------------

        ResourceID const resourceID( path );
        ResourceTypeID const resourceTypeID = resourceID.GetResourceTypeID();
        if ( resourceTypeID == EntityModel::EntityMapDescriptor::GetStaticResourceTypeID() )
        {
            // Map is already open, so nothing to do
            if ( m_pMapEditor->GetLoadedMap() == resourceID )
            {
                return true;
            }

            // Stop preview when switching map
            if ( m_pGamePreviewer != nullptr && !IsToolQueuedForDestruction( m_pGamePreviewer ) )
            {
                m_toolOperations.emplace_back( ToolOperation( m_pGamePreviewer, ToolOperation::DestroyTool ) );
            }
        }

        ToolOperation& request = m_toolOperations.emplace_back();
        request.m_type = ToolOperation::OpenFile;
        request.m_path = path;
        return true;
    }

    bool EditorUI::TryFindInResourceBrowser( DataPath const& path ) const
    {
        auto pResourceBrowser = GetTool<Resource::ResourceBrowserEditorTool>();
        if ( pResourceBrowser == nullptr )
        {
            pResourceBrowser = const_cast<EditorUI*>( this )->CreateTool<Resource::ResourceBrowserEditorTool>( this );
        }

        //-------------------------------------------------------------------------

        pResourceBrowser->TryFindAndSelectFile( path );
        return true;
    }

    void EditorUI::TryCreateNewResourceDescriptorOrDataFile( TypeSystem::TypeID typeID, FileSystem::Path const& startingDir ) const
    {
        const_cast<DialogManager&>( m_dialogManager ).StartModalDialog<Resource::ResourceDataFileCreatorDialog>( this, typeID, startingDir.IsValid() ? startingDir : m_fileRegistry.GetSourceDataDirectoryPath() );

        /*Resource::ResourceDataFileCreator*& pDataFileCreator = const_cast<Resource::ResourceDataFileCreator*&>( m_pResourceDataFileCreator );
        EE::Delete( pDataFileCreator );
        pDataFileCreator = EE::New<Resource::ResourceDataFileCreator>( this, typeID, startingDir.IsValid() ? startingDir : m_fileRegistry.GetSourceDataDirectoryPath() );*/
    }

    void EditorUI::OpenResourceImporter() const
    {
        const_cast<EditorUI*>( this )->CreateTool<Resource::ResourceImporterEditorTool>( this );
    }

    void EditorUI::ShowResourceDependencies( ResourceID const& resourceID ) const
    {
        Resource::ResourceDependencyViewerEditorTool* pDependencyViewer = GetTool<Resource::ResourceDependencyViewerEditorTool>();
        if ( pDependencyViewer == nullptr )
        {
            pDependencyViewer = const_cast<EditorUI*>( this )->CreateTool<Resource::ResourceDependencyViewerEditorTool>( this );
        }

        pDependencyViewer->ShowDependenciesForResourceID( resourceID );
    }

    //-------------------------------------------------------------------------
    // Title bar
    //-------------------------------------------------------------------------

    void EditorUI::GetBorderlessTitleBarInfo( Math::ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const
    {
        outTitlebarRect = m_titleBar.GetScreenRectangle();
        isInteractibleWidgetHovered = ImGui::IsAnyItemHovered();
    }

    void EditorUI::DrawTitleBarMenu( UpdateContext const& context )
    {
        ImGui::Image( m_editorIcon.m_ID, m_editorIcon.m_size );
        ImGuiX::TextTooltip( "Esoterica Editor" );

        //-------------------------------------------------------------------------

        constexpr float const verticalOffset = -4;

        ImGui::SameLine();
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + verticalOffset );

        if ( ImGui::BeginMenu( "Editor" ) )
        {
            ImGui::MenuItem( "UI Test Window", nullptr, &m_isUITestWindowOpen, !m_isUITestWindowOpen );

            ImGui::Separator();

            ImGui::MenuItem( "ImGui Demo Window", nullptr, &m_isImguiDemoWindowOpen, !m_isImguiDemoWindowOpen );
            ImGui::MenuItem( "ImGui Plot Demo Window", nullptr, &m_isImguiPlotDemoWindowOpen, !m_isImguiPlotDemoWindowOpen );

            ImGui::EndMenu();
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + verticalOffset );

        if ( ImGui::BeginMenu( "Resources" ) )
        {
            bool isResourceImporterOpen = GetTool<Resource::ResourceImporterEditorTool>() != nullptr;
            if ( ImGui::MenuItem( "Resource Importer", nullptr, &isResourceImporterOpen, !isResourceImporterOpen ) )
            {
                CreateTool<Resource::ResourceImporterEditorTool>( this );
            }

            bool isResourceBrowserOpen = GetTool<Resource::ResourceBrowserEditorTool>() != nullptr;
            if ( ImGui::MenuItem( "Resource Browser", nullptr, &isResourceBrowserOpen, !isResourceBrowserOpen ) )
            {
                CreateTool<Resource::ResourceBrowserEditorTool>( this );
            }

            bool isResourceDependencyViewOpen = GetTool<Resource::ResourceDependencyViewerEditorTool>() != nullptr;
            if ( ImGui::MenuItem( "Resource Dependency Viewer", nullptr, &isResourceDependencyViewOpen, !isResourceDependencyViewOpen ) )
            {
                CreateTool<Resource::ResourceDependencyViewerEditorTool>( this );
            }

            bool isResourceSystemOpen = GetTool<Resource::ResourceSystemEditorTool>() != nullptr;
            if ( ImGui::MenuItem( "Resource System Info", nullptr, &isResourceSystemOpen, !isResourceSystemOpen ) )
            {
                CreateTool<Resource::ResourceSystemEditorTool>( this );
            }

            bool isResourceBulkUpdateOpen = GetTool<Resource::ResourceBulkUpdateEditorTool>() != nullptr;
            if ( ImGui::MenuItem( "Resource Bulk Update", nullptr, &isResourceBulkUpdateOpen, !isResourceBulkUpdateOpen ) )
            {
                CreateTool<Resource::ResourceBulkUpdateEditorTool>( this );
            }

            //-------------------------------------------------------------------------

            ImGui::Separator();

            if ( ImGui::MenuItem( "Rebuild File Registry" ) )
            {
                m_fileRegistry.RequestRebuild();
            }

            ImGui::EndMenu();
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + verticalOffset );

        if ( ImGui::BeginMenu( "System" ) )
        {
            bool isMemoryTrackerOpen = GetTool<MemoryTrackerEditorTool>() != nullptr;
            if ( ImGui::MenuItem( "Memory Tracker", nullptr, &isMemoryTrackerOpen, !isMemoryTrackerOpen ) )
            {
                CreateTool<MemoryTrackerEditorTool>( this );
            }

            bool isLogOpen = GetTool<SystemLogEditorTool>() != nullptr;
            if ( ImGui::MenuItem( "System Log", nullptr, &isLogOpen, !isLogOpen ) )
            {
                CreateTool<SystemLogEditorTool>( this );
            }

            bool isSettingsOpen = GetTool<SystemSettingsEditorTool>() != nullptr;
            if ( ImGui::MenuItem( "System Settings", nullptr, &isSettingsOpen, !isSettingsOpen ) )
            {
                CreateTool<SystemSettingsEditorTool>( this );
            }

            ImGui::EndMenu();
        }
    }

    void EditorUI::DrawTitleBarInfoStats( UpdateContext const& context )
    {
        FrameLimiterWidget frameLimiter;
        PerformanceStatsWidget performanceStats( context );
        SystemLogSummaryWidget logSummary;

        float const availableSpace = ImGui::GetContentRegionAvail().x;
        float const requiredOffset = availableSpace - logSummary.m_size.x - frameLimiter.m_width - performanceStats.m_totalSize.x - ImGui::GetFrameHeight() - ( 4 * ImGui::GetStyle().ItemSpacing.x );
        if ( requiredOffset > 0 )
        {
            ImGui::Dummy( ImVec2( requiredOffset, 0 ) );
            ImGui::SameLine();
        }

        constexpr float const verticalOffset = 4;
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + verticalOffset );

        if ( logSummary.Draw() )
        {
            CreateTool<SystemLogEditorTool>( this );
        }
        ImGui::SameLine();

        auto pResourceSystem = m_pSystemRegistry->GetSystem<Resource::ResourceSystem>();
        if ( pResourceSystem->IsBusy() )
        {
            if ( ImGuiX::DrawSpinner( "##RS", Colors::LimeGreen ) )
            {
                CreateTool<Resource::ResourceSystemEditorTool>( this );
            }
            ImGuiX::TextTooltip( "Resource System: Busy" );
        }
        else
        {
            if ( ImGuiX::FlatButton( EE_ICON_SLEEP, ImVec2( ImGui::GetFrameHeight(), ImGui::GetFrameHeight() ) ) )
            {
                CreateTool<Resource::ResourceSystemEditorTool>( this );
            }
            ImGuiX::TextTooltip( "Resource System: Idle" );
        }

        ImGui::SameLine();
        frameLimiter.Draw( const_cast<UpdateContext&>( context ) );

        ImGui::SameLine();
        performanceStats.Draw();
    }

    //-------------------------------------------------------------------------
    // Update
    //-------------------------------------------------------------------------

    void EditorUI::StartFrame( UpdateContext const& context )
    {
        UpdateStage const updateStage = context.GetUpdateStage();
        EE_ASSERT( updateStage == UpdateStage::FrameStart );

        //-------------------------------------------------------------------------
        // Resource Systems
        //-------------------------------------------------------------------------

        m_fileRegistry.Update();

        //-------------------------------------------------------------------------
        // Editor Tool Management
        //-------------------------------------------------------------------------

        // Destroy all required editor tools
        // We needed to defer this to the start of the update since we may have references resources that we might unload (i.e. textures)
        for ( size_t i = 0; i < m_toolOperations.size(); i++ )
        {
            if ( m_toolOperations[i].m_type == ToolOperation::DestroyTool )
            {
                ExecuteToolOperation( context, m_toolOperations[i] );
                m_toolOperations.erase( m_toolOperations.begin() + i );
                i--;
            }
        }

        // Execute all other editor tool operations
        for ( size_t i = 0; i < m_toolOperations.size(); i++ )
        {
            if ( m_toolOperations[i].m_type != ToolOperation::DestroyTool )
            {
                ExecuteToolOperation( context, m_toolOperations[i] );
                m_toolOperations.erase( m_toolOperations.begin() + i );
                i--;
            }
        }

        //-------------------------------------------------------------------------
        // Title Bar
        //-------------------------------------------------------------------------

        auto TitleBarLeftContents = [this, &context] ()
        {
            DrawTitleBarMenu( context );
        };

        auto TitleBarRightContents = [this, &context] ()
        {
            DrawTitleBarInfoStats( context );
        };

        m_titleBar.Draw( TitleBarLeftContents, 400, TitleBarRightContents, 800 );

        //-------------------------------------------------------------------------
        // Create main dock window
        //-------------------------------------------------------------------------

        ImGuiID const dockspaceID = ImGui::GetID( "EditorDockSpace" );

        ImGuiViewport const* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos( viewport->WorkPos );
        ImGui::SetNextWindowSize( viewport->WorkSize );
        ImGui::SetNextWindowViewport( viewport->ID );

        ImGuiWindowFlags const windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
        ImGui::Begin( "EditorDockSpaceWindow", nullptr, windowFlags );
        ImGui::PopStyleVar( 3 );
        {
            // Create initial layout
            if ( !ImGui::DockBuilderGetNode( dockspaceID ) )
            {
                ImGui::DockBuilderAddNode( dockspaceID, ImGuiDockNodeFlags_DockSpace );
                ImGui::DockBuilderSetNodeSize( dockspaceID, ImGui::GetContentRegionAvail() );

                ImGuiID leftDockID = 0, rightDockID = 0, bottomDockID = 0;
                ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.1f, &bottomDockID, &leftDockID );
                ImGui::DockBuilderSplitNode( leftDockID, ImGuiDir_Left, 0.25f, &leftDockID, &rightDockID );
                ImGui::DockBuilderFinish( dockspaceID );

                //-------------------------------------------------------------------------

                auto pSystemLog = GetTool<SystemLogEditorTool>();
                ImGui::DockBuilderDockWindow( pSystemLog->GetDisplayName(), bottomDockID );

                auto pResourceBrowser = GetTool<Resource::ResourceBrowserEditorTool>();
                ImGui::DockBuilderDockWindow( pResourceBrowser->GetDisplayName(), leftDockID );

                ImGui::DockBuilderDockWindow( m_pMapEditor->m_windowName.c_str(), rightDockID );
            }

            // Create the actual dock space
            ImGui::PushStyleVar( ImGuiStyleVar_TabRounding, 0 );
            ImGui::DockSpace( dockspaceID, viewport->WorkSize, 0, &m_editorWindowClass );
            ImGui::PopStyleVar( 1 );

            //-------------------------------------------------------------------------

            m_dialogManager.DrawDialog();
        }
        ImGui::End();

        //-------------------------------------------------------------------------
        // Draw editor windows
        //-------------------------------------------------------------------------

        if ( m_isImguiDemoWindowOpen )
        {
            ImGui::ShowDemoWindow( &m_isImguiDemoWindowOpen );
        }

        if ( m_isImguiPlotDemoWindowOpen )
        {
            ImPlot::ShowDemoWindow( &m_isImguiPlotDemoWindowOpen );
        }

        if ( m_isUITestWindowOpen )
        {
            DrawUITestWindow( this, &m_isUITestWindowOpen );
        }

        //-------------------------------------------------------------------------
        // Draw open editor tools
        //-------------------------------------------------------------------------

        EditorTool* pEditorToolToClose = nullptr;

        // Switch focus
        if ( !m_focusTargetWindowName.empty() )
        {
            ImGuiWindow* pWindow = ImGui::FindWindowByName( m_focusTargetWindowName.c_str() );
            if ( pWindow == nullptr || pWindow->DockNode == nullptr || pWindow->DockNode->TabBar == nullptr )
            {
                m_focusTargetWindowName.clear();
                return;
            }

            ImGuiID tabID = 0;
            for ( int32_t i = 0; i < pWindow->DockNode->TabBar->Tabs.Size; i++ )
            {
                ImGuiTabItem* pTab = &pWindow->DockNode->TabBar->Tabs[i];
                if ( pTab->Window->ID == pWindow->ID )
                {
                    tabID = pTab->ID;
                    break;
                }
            }

            if ( tabID != 0 )
            {
                pWindow->DockNode->TabBar->NextSelectedTabId = tabID;
                ImGui::SetWindowFocus( m_focusTargetWindowName.c_str() );
            }

            m_focusTargetWindowName.clear();
        }

        // Update the location for all editor tools
        for ( auto pEditorTool : m_editorTools )
        {
            if ( !SubmitToolMainWindow( context, pEditorTool, dockspaceID ) )
            {
                pEditorToolToClose = pEditorTool;
            }
        }

        // Draw all editor tools
        for ( auto pEditorTool : m_editorTools )
        {
            // If we've been asked to close, no point drawing
            if ( pEditorTool == pEditorToolToClose )
            {
                continue;
            }

            // The game previewer is special and is handled separately
            if ( pEditorTool == m_pGamePreviewer )
            {
                continue;
            }

            // Dont draw any editor tools queued for destructor
            if ( IsToolQueuedForDestruction( pEditorTool ) )
            {
                continue;
            }

            DrawToolContents( context, pEditorTool );
        }

        // Did we get a close request?
        if ( pEditorToolToClose != nullptr )
        {
            // We need to defer this to the start of the update since we may have references resources that we might unload (i.e. textures)
            QueueDestroyTool( pEditorToolToClose );
        }
    }

    void EditorUI::EndFrame( UpdateContext const& context )
    {
        // Game previewer needs to be drawn at the end of the frames since then all the game simulation data will be correct and all the debug tools will be accurate
        if ( m_pGamePreviewer != nullptr && !IsToolQueuedForDestruction( m_pGamePreviewer ) )
        {
            DrawToolContents( context, m_pGamePreviewer );
            m_pGamePreviewer->DrawEngineDebugUI( context );
        }
    }

    void EditorUI::Update( UpdateContext const& context )
    {
        for ( auto pEditorTool : m_editorTools )
        {
            if ( pEditorTool->HasEntityWorld() )
            {
                EntityWorldUpdateContext updateContext( context, pEditorTool->GetEntityWorld() );
                pEditorTool->WorldUpdate( updateContext );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Hot Reload
    //-------------------------------------------------------------------------

    void EditorUI::HotReload_UnloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded )
    {
        for ( auto pEditorTool : m_editorTools )
        {
            pEditorTool->HotReload_UnloadResources( usersToReload, resourcesToBeReloaded );
        }

        if ( m_pGamePreviewer != nullptr )
        {
            m_pGamePreviewer->m_pDebugUI->HotReload_UnloadResources( usersToReload, resourcesToBeReloaded );
        }
    }

    void EditorUI::HotReload_ReloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded )
    {
        for ( auto pEditorTool : m_editorTools )
        {
            // Auto destroy any editor tools that had a problem loading their descriptor i.e. they were externally corrupted.
            if ( !pEditorTool->HotReload_ReloadResources( usersToReload, resourcesToBeReloaded ) )
            {
                MessageDialog::Error( "Error Loading Data File", "There was an error reloading the data file for editor tool: %s! Please check the log for details.", pEditorTool->GetDisplayName() );
                QueueDestroyTool( pEditorTool );
            }
        }

        // Hot-reload game debug views if we are currently running a game preview
        if ( m_pGamePreviewer != nullptr )
        {
            m_pGamePreviewer->m_pDebugUI->HotReload_ReloadResources( usersToReload, resourcesToBeReloaded );
        }
    }

    //-------------------------------------------------------------------------
    // Resource Management
    //-------------------------------------------------------------------------

    void EditorUI::OnFileDeleted( DataPath const& dataPath )
    {
        EE_ASSERT( dataPath.IsValid() );

        DataPath const dataPathParent = dataPath.GetPathWithoutSubFilename();

        for ( auto pEditorTool : m_editorTools )
        {
            // Destroy any open tool windows for the resource
            if ( pEditorTool->IsEditingFile( dataPathParent ) )
            {
                QueueDestroyTool( pEditorTool );
            }
            else if ( pEditorTool->HasDependencyOnFile( dataPathParent ) )
            {
                QueueDestroyTool( pEditorTool );
            }

            // Notify the game to reload the resource (i.e. fail to load)
            if ( ResourceID::IsValidResourceIDString( dataPath.GetString() ) )
            {
                ResourceID resourceID = dataPath;
                auto pResourceSystem = m_pSystemRegistry->GetSystem<Resource::ResourceSystem>();
                pResourceSystem->RequestResourceHotReload( resourceID );
            }
        }
    }

    //-------------------------------------------------------------------------
    // EditorTool Management
    //-------------------------------------------------------------------------

    InlineString EditorUI::GetEditorToolName( uint64_t toolID ) const
    {
        InlineString name;

        for ( EditorTool const* pTool : m_editorTools )
        {
            if ( pTool->GetID() == toolID )
            {
                name = pTool->GetName();
                break;
            }
        }

        return name;
    }

    bool EditorUI::ExecuteToolOperation( UpdateContext const& context, ToolOperation& request )
    {
        // Initialize EditorTool
        //-------------------------------------------------------------------------

        if ( request.m_type == ToolOperation::InitializeTool )
        {
            request.m_pEditorTool->Initialize( context );
            m_editorTools.emplace_back( request.m_pEditorTool );
            return true;
        }

        // Destroy Tool
        //-------------------------------------------------------------------------

        if ( request.m_type == ToolOperation::DestroyTool )
        {
            DestroyTool( context, request.m_pEditorTool, false );
            return true;
        }

        // Map editor
        //-------------------------------------------------------------------------

        else if ( request.m_type == ToolOperation::CreateMapEditor )
        {
            // Destroy the default created game world
            m_pWorldManager->DestroyWorld( m_pWorldManager->GetWorlds()[0] );

            // Create a new editor world for the map editor editor tool
            auto pMapEditorWorld = m_pWorldManager->CreateWorld( EntityWorldType::Tools );

            // Create the map editor editor tool
            m_pMapEditor = EE::New<EntityModel::MapEditor>( this, pMapEditorWorld );
            m_pMapEditor->Initialize( context );
            m_editorTools.emplace_back( m_pMapEditor );

            m_gamePreviewStartRequestEventBindingID = m_pMapEditor->OnGamePreviewStartRequested().Bind( [this] ( UpdateContext const& context ) { CreateGamePreviewTool( context ); } );
            m_gamePreviewStopRequestEventBindingID = m_pMapEditor->OnGamePreviewStopRequested().Bind( [this] ( UpdateContext const& context ) { DestroyGamePreviewTool( context ); } );

            // Load startup map
            if ( m_startupMapResourceID.IsValid() )
            {
                EE_ASSERT( m_startupMapResourceID.GetResourceTypeID() == EntityModel::EntityMapDescriptor::GetStaticResourceTypeID() );
                m_pMapEditor->LoadMap( m_startupMapResourceID );
            }

            return true;
        }

        // Game previewer
        //-------------------------------------------------------------------------

        else if ( request.m_type == ToolOperation::CreateGamePreview )
        {
            auto pPreviewWorld = m_pWorldManager->CreateWorld( EntityWorldType::Game );
            m_pGamePreviewer = EE::New<GamePreviewer>( this, pPreviewWorld );
            m_pGamePreviewer->Initialize( context );
            m_pGamePreviewer->LoadMapToPreview( m_pMapEditor->GetLoadedMap() );
            m_editorTools.emplace_back( m_pGamePreviewer );

            m_pMapEditor->NotifyGamePreviewStarted();

            return true;
        }

        // Resource
        //-------------------------------------------------------------------------

        else if ( request.m_type == ToolOperation::OpenFile )
        {
            EE_ASSERT( request.m_path.IsValid() );

            // Don't try to open files that don't exist
            FileSystem::Path const path = request.m_path.GetFileSystemPath( m_fileRegistry.GetSourceDataDirectoryPath() );
            if ( !FileSystem::Exists( path ) )
            {
                return false;
            }

            // Check if we already have a editor tool open for this file, if so then switch focus to it
            for ( auto pEditorTool : m_editorTools )
            {
                if ( pEditorTool->IsEditingFile( request.m_path ) )
                {
                    m_focusTargetWindowName = pEditorTool->m_windowName;
                    return true;
                }
            }

            // Handle maps explicitly
            //-------------------------------------------------------------------------

            ResourceID const resourceID( request.m_path );
            ResourceTypeID const resourceTypeID = resourceID.GetResourceTypeID();
            if ( resourceTypeID == EntityModel::EntityMapDescriptor::GetStaticResourceTypeID() )
            {
                m_pMapEditor->LoadMap( resourceID );
                m_focusTargetWindowName = m_pMapEditor->m_windowName;
                return true;
            }

            // Create new editor
            //-------------------------------------------------------------------------

            auto CreateToolWorld = [this] ()
            {
                EntityWorld* pEditorToolWorld = m_pWorldManager->CreateWorld( EntityWorldType::Tools );
                return pEditorToolWorld;
            };

            EditorTool* pCreatedTool = EditorToolFactory::TryCreateEditor( this, request.m_path, CreateToolWorld );
            if ( pCreatedTool != nullptr )
            {
                m_editorTools.emplace_back( pCreatedTool );

                // Initialize editor tool
                pCreatedTool->Initialize( context );
                EE_ASSERT( pCreatedTool->WasInitialized() ); // Ensure that the base initialize method was called!

                // Check if the data file was correctly loaded, if not schedule this editor tool to be destroyed
                if ( pCreatedTool->IsDataFileTool() )
                {
                    auto pDataFileEditor = static_cast<DataFileEditor*>( pCreatedTool );
                    if ( !pDataFileEditor->IsDataFileLoaded() )
                    {
                        MessageDialog::Error( "Error Loading Data File", "There was an error loading the data file for %s! Please check the log for details.", request.m_path.c_str() );
                        QueueDestroyTool( pCreatedTool );
                        return false;
                    }
                }

                return true;
            }

            return false;
        }
        else
        {
            EE_UNREACHABLE_CODE();
        }

        //-------------------------------------------------------------------------

        return false;
    }

    bool EditorUI::IsToolQueuedForDestruction( EditorTool* pEditorTool ) const
    {
        for ( auto const& operation : m_toolOperations )
        {
            if ( operation.m_type == ToolOperation::DestroyTool && operation.m_pEditorTool == pEditorTool )
            {
                return true;
            }
        }

        return false;
    }

    bool EditorUI::HasOpenAndDirtyTools() const
    {
        for ( auto pEditorTool : m_editorTools )
        {
            if ( pEditorTool->IsDirty() )
            {
                return true;
            }
        }

        return false;
    }

    void EditorUI::SaveAllDirtyTools()
    {
        for ( auto pEditorTool : m_editorTools )
        {
            if ( pEditorTool->IsDirty() )
            {
                pEditorTool->Save();
            }
        }
    }

    void EditorUI::DestroyTool( UpdateContext const& context, EditorTool* pEditorTool, bool isEditorShutdown )
    {
        EE_ASSERT( m_pMapEditor != pEditorTool );
        EE_ASSERT( pEditorTool != nullptr );

        auto foundToolIter = eastl::find( m_editorTools.begin(), m_editorTools.end(), pEditorTool );
        EE_ASSERT( foundToolIter != m_editorTools.end() );

        bool const isGamePreviewerTool = m_pGamePreviewer == pEditorTool;

        // Save unsaved changes
        //-------------------------------------------------------------------------

        if ( pEditorTool->SupportsSaving() && pEditorTool->IsDirty() )
        {
            if ( isEditorShutdown )
            {
                if ( MessageDialog::Confirmation( Severity::Warning, "Unsaved Changes", "You have unsaved changes!\nDo you wish to save these changes before closing?" ) )
                {
                    pEditorTool->Save();
                }
            }
            else
            {
                MessageDialog::Result const result = MessageDialog::ShowEx( Severity::Warning, MessageDialog::Type::YesNoCancel, "Unsaved Changes", "You have unsaved changes!\nDo you wish to save these changes before closing?" );
                if ( result == MessageDialog::Result::Yes )
                {
                    if ( !pEditorTool->Save() )
                    {
                        return;
                    }
                }
                else if ( result == MessageDialog::Result::Cancel )
                {
                    return;
                }
            }
        }

        //-------------------------------------------------------------------------

        // Clear last active tool
        if ( m_pLastActiveTool == pEditorTool )
        {
            m_pLastActiveTool = nullptr;
        }

        // Get the world before we destroy the editor tool
        auto pEditorToolWorld = pEditorTool->GetEntityWorld();

        // Destroy editor tool
        if ( pEditorTool->WasInitialized() )
        {
            pEditorTool->Shutdown( context );
        }
        EE::Delete( pEditorTool );
        m_editorTools.erase( foundToolIter );

        // Clear the game previewer editor tool ptr if we just destroyed it
        if ( isGamePreviewerTool )
        {
            m_pMapEditor->NotifyGamePreviewEnded();
            m_pGamePreviewer = nullptr;
        }

        // Destroy preview world and render target
        if ( pEditorToolWorld )
        {
            m_pWorldManager->DestroyWorld( pEditorToolWorld );
        }
    }

    void EditorUI::QueueDestroyTool( EditorTool* pEditorTool )
    {
        EE_ASSERT( pEditorTool != nullptr );
        EE_ASSERT( m_pMapEditor != pEditorTool );
        EE_ASSERT( !IsToolQueuedForDestruction( pEditorTool ) );

        m_toolOperations.emplace_back( ToolOperation( pEditorTool, ToolOperation::DestroyTool ) );
    }

    void EditorUI::ToolLayoutCopy( EditorTool* pEditorTool )
    {
        ImGuiID sourceToolID = pEditorTool->m_previousDockspaceID;
        ImGuiID destinationToolID = pEditorTool->m_currentDockspaceID;
        IM_ASSERT( sourceToolID != 0 );
        IM_ASSERT( destinationToolID != 0 );

        // Helper to build an array of strings pointer into the same contiguous memory buffer.
        struct ContiguousStringArrayBuilder
        {
            void AddEntry( const char* data, size_t dataLength )
            {
                int32_t const bufferSize = (int32_t) m_buffer.size();
                m_offsets.push_back( bufferSize );
                int32_t const offset = bufferSize;
                m_buffer.resize( bufferSize + (int32_t) dataLength );
                memcpy( m_buffer.data() + offset, data, dataLength );
            }

            void BuildPointerArray( ImVector<const char*>& outArray )
            {
                outArray.resize( (int32_t) m_offsets.size() );
                for ( int32_t n = 0; n < (int32_t) m_offsets.size(); n++ )
                {
                    outArray[n] = m_buffer.data() + m_offsets[n];
                }
            }

            TVector<char>       m_buffer;
            TVector<int32_t>    m_offsets;
        };

        // Build an array of remapped names
        ContiguousStringArrayBuilder namePairsBuilder;

        // Iterate tool windows
        for ( auto& pToolWindow : pEditorTool->m_toolWindows )
        {
            InlineString const sourceToolWindowName = EditorTool::GetToolWindowName( pToolWindow->m_name.c_str(), sourceToolID );
            InlineString const destinationToolWindowName = EditorTool::GetToolWindowName( pToolWindow->m_name.c_str(), destinationToolID );
            namePairsBuilder.AddEntry( sourceToolWindowName.c_str(), sourceToolWindowName.length() + 1 );
            namePairsBuilder.AddEntry( destinationToolWindowName.c_str(), destinationToolWindowName.length() + 1 );
        }

        // Perform the cloning
        if ( ImGui::DockContextFindNodeByID( ImGui::GetCurrentContext(), sourceToolID ) )
        {
            // Build the same array with char* pointers at it is the input of DockBuilderCopyDockspace() (may change its signature?)
            ImVector<const char*> windowRemapPairs;
            namePairsBuilder.BuildPointerArray( windowRemapPairs );

            ImGui::DockBuilderCopyDockSpace( sourceToolID, destinationToolID, &windowRemapPairs );
            ImGui::DockBuilderFinish( destinationToolID );
        }
    }

    bool EditorUI::SubmitToolMainWindow( UpdateContext const& context, EditorTool* pEditorTool, ImGuiID editorDockspaceID )
    {
        EE_ASSERT( pEditorTool != nullptr && pEditorTool->WasInitialized() );
        IM_ASSERT( editorDockspaceID != 0 );

        bool isToolStillOpen = true;
        bool* pIsToolOpen = ( pEditorTool == m_pMapEditor ) ? nullptr : &isToolStillOpen; // Prevent closing the map-editor editor tool

        // Top level editors can only be docked with each others (we dont allow docking the game preview window)
        if ( pEditorTool != m_pGamePreviewer )
        {
            ImGui::SetNextWindowClass( &m_editorWindowClass );
            if ( pEditorTool->m_desiredDockID != 0 )
            {
                ImGui::SetNextWindowDockID( pEditorTool->m_desiredDockID );
                pEditorTool->m_desiredDockID = 0;
            }
            else
            {
                ImGui::SetNextWindowDockID( editorDockspaceID, ImGuiCond_FirstUseEver );
            }
        }

        // Window flags
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;

        if ( pEditorTool->SupportsMainMenu() )
        {
            windowFlags |= ImGuiWindowFlags_MenuBar;
        }

        if ( pEditorTool->IsDirty() )
        {
            windowFlags |= ImGuiWindowFlags_UnsavedDocument;
        }

        //-------------------------------------------------------------------------

        ImGuiWindow* pCurrentWindow = ImGui::FindWindowByName( pEditorTool->m_windowName.c_str() );
        bool const isVisible = pCurrentWindow != nullptr && !pCurrentWindow->Hidden;

        // Create top level editor tab/window
        ImGui::SetNextWindowSizeConstraints( ImVec2( 128, 128 ), ImVec2( FLT_MAX, FLT_MAX ) );

        if ( pEditorTool == m_pGamePreviewer )
        {
            ImGui::SetNextWindowSize( Float2( m_pGamePreviewer->GetRequiredWindowSize() ), ImGuiCond_Always );
            windowFlags |= ImGuiWindowFlags_NoResize;
            windowFlags |= ImGuiWindowFlags_AlwaysAutoResize;
            windowFlags |= ImGuiWindowFlags_NoDocking;
        }
        else
        {
            ImGui::SetNextWindowSize( ImVec2( 1024, 768 ), ImGuiCond_FirstUseEver );
        }

        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1.0f );
        ImGui::Begin( pEditorTool->m_windowName.c_str(), pIsToolOpen, windowFlags );
        ImGui::PopStyleVar();

        //-------------------------------------------------------------------------

        // Store last focused document
        bool const isFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_DockHierarchy );
        if ( isFocused )
        {
            m_pLastActiveTool = pEditorTool;
        }

        // Set WindowClass based on per-document ID, so tabs from Document A are not dockable in Document B etc. We could be using any ID suiting us, e.g. &pEditorTool
        // We also set ParentViewportId to request the platform back-end to set parent/child relationship at the windowing level.
        pEditorTool->m_toolWindowClass.ClassId = pEditorTool->m_ID;
        pEditorTool->m_toolWindowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoTaskBarIcon | ImGuiViewportFlags_NoDecoration;
        pEditorTool->m_toolWindowClass.ParentViewportId = ImGui::GetWindowViewport()->ID; // Make child of the top-level editor window
        pEditorTool->m_toolWindowClass.DockingAllowUnclassed = true;

        // Track LocationID change so we can fork/copy the layout data according to where the window is going + reference count
        // LocationID ~~ (DockId != 0 ? DockId : DocumentID) // When we are in a loose floating window we use our own document id instead of the dock id
        pEditorTool->m_currentDockID = ImGui::GetWindowDockID();
        pEditorTool->m_previousLocationID = pEditorTool->m_currentLocationID;
        pEditorTool->m_currentLocationID = pEditorTool->m_currentDockID != 0 ? pEditorTool->m_currentDockID : pEditorTool->m_ID;

        // Dockspace ID ~~ Hash of LocationID + DocType
        // So all editors of a same type inside a same tab-bar will share the same layout.
        // We will also use this value as a suffix to create window titles, but we could perfectly have an indirection to allocate and use nicer names for window names (e.g. 0001, 0002).
        pEditorTool->m_previousDockspaceID = pEditorTool->m_currentDockspaceID;
        pEditorTool->m_currentDockspaceID = pEditorTool->CalculateDockspaceID();
        EE_ASSERT( pEditorTool->m_currentDockspaceID != 0 );

        ImGui::End();

        //-------------------------------------------------------------------------

        return isToolStillOpen;
    }

    void EditorUI::DrawToolContents( UpdateContext const& context, EditorTool* pEditorTool )
    {
        EE_ASSERT( pEditorTool != nullptr && pEditorTool->WasInitialized() );

        auto pWorld = pEditorTool->GetEntityWorld();

        //-------------------------------------------------------------------------

        // This is the second Begin(), as SubmitToolWindow() has already done one
        // (Therefore only the p_open and flags of the first call to Begin() applies)
        ImGui::Begin( pEditorTool->m_windowName.c_str() );
        int32_t const beginCount = ImGui::GetCurrentWindow()->BeginCount;
        IM_ASSERT( beginCount == 2 );

        // Fork settings when extracting to a new location, or Overwrite settings when docking back into an existing location
        if ( pEditorTool->m_previousLocationID != 0 && pEditorTool->m_previousLocationID != pEditorTool->m_currentLocationID )
        {
            // Count references to tell if we should Copy or Move the layout.
            int32_t previousDockspaceRefCount = 0;
            int32_t currentDockspaceRefCount = 0;
            for ( int32_t i = 0; i < (int32_t) m_editorTools.size(); i++ )
            {
                EditorTool* pOtherTool = m_editorTools[i];

                if ( pOtherTool->m_currentDockspaceID == pEditorTool->m_previousDockspaceID )
                {
                    previousDockspaceRefCount++;
                }

                if ( pOtherTool->m_currentDockspaceID == pEditorTool->m_currentDockspaceID )
                {
                    currentDockspaceRefCount++;
                }
            }

            // Fork or overwrite settings
            // FIXME: should be able to do a "move window but keep layout" if curr_dockspace_ref_count > 1.
            // FIXME: when moving, delete settings of old windows
            ToolLayoutCopy( pEditorTool );

            if ( previousDockspaceRefCount == 0 )
            {
                ImGui::DockBuilderRemoveNode( pEditorTool->m_previousDockspaceID );

                // Delete settings of old windows
                // Rely on window name to ditch their .ini settings forever..
                char windowSuffix[16];
                ImFormatString( windowSuffix, IM_ARRAYSIZE( windowSuffix ), "##%08X", pEditorTool->m_previousDockspaceID );
                size_t windowSuffixLength = strlen( windowSuffix );
                ImGuiContext& g = *GImGui;
                for ( ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk( settings ) )
                {
                    if ( settings->ID == 0 )
                    {
                        continue;
                    }

                    char const* pWindowName = settings->GetName();
                    size_t windowNameLength = strlen( pWindowName );
                    if ( windowNameLength >= windowSuffixLength )
                    {
                        if ( strcmp( pWindowName + windowNameLength - windowSuffixLength, windowSuffix ) == 0 ) // Compare suffix
                        {
                            ImGui::ClearWindowSettings( pWindowName );
                        }
                    }
                }
            }
        }
        else if ( pEditorTool->m_requiresLayoutReset || ImGui::DockBuilderGetNode( pEditorTool->m_currentDockspaceID ) == nullptr )
        {
            ImVec2 dockspaceSize = ImGui::GetContentRegionAvail();
            dockspaceSize.x = Math::Max( dockspaceSize.x, 1.0f );
            dockspaceSize.y = Math::Max( dockspaceSize.y, 1.0f );

            ImGui::DockBuilderAddNode( pEditorTool->m_currentDockspaceID, ImGuiDockNodeFlags_DockSpace );
            ImGui::DockBuilderSetNodeSize( pEditorTool->m_currentDockspaceID, dockspaceSize );
            ImGui::DockBuilderRemoveNodeChildNodes( pEditorTool->m_currentDockspaceID );
            if ( !pEditorTool->IsSingleWindowTool() )
            {
                // Always dock viewport into central window
                if ( pEditorTool->SupportsViewport() )
                {
                    ImGui::DockBuilderDockWindow( pEditorTool->GetToolWindowName( EditorTool::s_viewportWindowName ).c_str(), pEditorTool->m_currentDockspaceID );
                }

                pEditorTool->SetupDockingLayout( pEditorTool->m_currentDockspaceID, dockspaceSize );
            }
            else
            {
                ImGui::DockBuilderDockWindow( pEditorTool->GetToolWindowName( pEditorTool->m_toolWindows[0]->m_name ).c_str(), pEditorTool->m_currentDockspaceID );
            }
            ImGui::DockBuilderFinish( pEditorTool->m_currentDockspaceID );

            pEditorTool->m_requiresLayoutReset = false;
        }

        // FIXME-DOCK: This is a little tricky to explain but we currently need this to use the pattern of sharing a same dockspace between tabs of a same tab bar
        bool isVisible = true;
        if ( ImGui::GetCurrentWindow()->Hidden )
        {
            isVisible = false;
        }

        // Update editor tool
        //-------------------------------------------------------------------------

        bool const isLastFocusedTool = ( m_pLastActiveTool == pEditorTool );
        pEditorTool->PreDrawUpdate( context, isVisible, isLastFocusedTool );
        pEditorTool->Update( context, isVisible, isLastFocusedTool );
        pEditorTool->m_isViewportFocused = false;
        pEditorTool->m_isViewportHovered = false;

        bool const isGamePreviewerTool = pEditorTool == m_pGamePreviewer;

        // Check Visibility
        //-------------------------------------------------------------------------

        if ( !isVisible )
        {
            // Keep alive document dockspace so windows that are docked into it but which visibility are not linked to the dockspace visibility won't get undocked.
            ImGui::DockSpace( pEditorTool->m_currentDockspaceID, ImVec2( 0, 0 ), ImGuiDockNodeFlags_KeepAliveOnly, &pEditorTool->m_toolWindowClass );
            ImGui::End();

            // Suspend world updates for hidden windows
            if ( pEditorTool->HasEntityWorld() && pEditorTool != m_pGamePreviewer )
            {
                pWorld->SuspendUpdates();
            }

            return;
        }

        // Draw EditorTool Menu
        //-------------------------------------------------------------------------

        if ( pEditorTool->SupportsMainMenu() )
        {
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 8, 16 ) );
            if ( ImGui::BeginMenuBar() )
            {
                pEditorTool->DrawMainMenu( context );
                ImGui::EndMenuBar();
            }
            ImGui::PopStyleVar( 1 );
        }

        // Draw EditorTool Toolbar
        //-------------------------------------------------------------------------

        if ( pEditorTool->SupportsToolbar() )
        {
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() - ImGui::GetStyle().WindowPadding.y );
            if ( ImGui::BeginChild( "Toolbar", ImVec2( ImGui::GetContentRegionAvail().x, 0 ), ImGuiChildFlags_AutoResizeY ) )
            {
                pEditorTool->DrawToolbar( context );
            }
            ImGui::EndChild();
        }

        // Submit the dockspace node and end window
        //-------------------------------------------------------------------------

        ImGuiDockNodeFlags dockingFlags = ImGuiDockNodeFlags_None;

        if ( pEditorTool->IsSingleWindowTool() )
        {
            dockingFlags |= ImGuiDockNodeFlags_AutoHideTabBar | ImGuiDockNodeFlags_NoUndocking | ImGuiDockNodeFlags_NoDockingSplit;
        }

        ImGui::DockSpace( pEditorTool->m_currentDockspaceID, ImVec2( 0, 0 ), dockingFlags, &pEditorTool->m_toolWindowClass );
        ImGui::End();

        // Manage World state
        //-------------------------------------------------------------------------

        if ( pEditorTool->HasEntityWorld() && ( pEditorTool != m_pMapEditor || m_pGamePreviewer == nullptr ) )
        {
            pWorld->ResumeUpdates();
        }

        // Draw editor tool windows
        //-------------------------------------------------------------------------
        // Note: viewports will always be drawn before other windows

        for ( auto& pToolWindow : pEditorTool->m_toolWindows )
        {
            if ( !pToolWindow->m_isOpen )
            {
                continue;
            }

            if ( pToolWindow->IsHidden() )
            {
                continue;
            }

            InlineString const toolWindowName = EditorTool::GetToolWindowName( pToolWindow->m_name.c_str(), pEditorTool->m_currentDockspaceID );

            // When multiple documents are open, floating tools only appear for focused one
            if ( !isLastFocusedTool )
            {
                if ( ImGuiWindow* pWindow = ImGui::FindWindowByName( toolWindowName.c_str() ) )
                {
                    ImGuiDockNode* pWindowDockNode = pWindow->DockNode;
                    if ( pWindowDockNode == nullptr && pWindow->DockId != 0 )
                    {
                        pWindowDockNode = ImGui::DockContextFindNodeByID( ImGui::GetCurrentContext(), pWindow->DockId );
                    }

                    if ( pWindowDockNode == nullptr || ImGui::DockNodeGetRootNode( pWindowDockNode )->ID != pEditorTool->m_currentDockspaceID )
                    {
                        continue;
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( pToolWindow->m_pViewport != nullptr )
            {
                EE_ASSERT( pEditorTool->SupportsViewport() );
                EE_ASSERT( pEditorTool->m_pCamera != nullptr );
                EE_ASSERT( pEditorTool->HasEntityWorld() && pWorld != nullptr );

                // Update camera
                //-------------------------------------------------------------------------

                auto pCameraSystem = pEditorTool->GetEntityWorld()->GetWorldSystem<CameraSystem>();
                if ( pCameraSystem != nullptr && pCameraSystem->GetActiveCamera() == pEditorTool->m_pCamera )
                {
                    EntityWorldUpdateContext updateContext( context, pEditorTool->GetEntityWorld() );
                    pCameraSystem->UpdateToolsCamera( updateContext );
                    pEditorTool->GetEntityWorld()->UpdateViewportViewVolumes();
                }

                // Draw viewport window
                //-------------------------------------------------------------------------

                ImGuiWindowFlags viewportWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus;
                ImGui::SetNextWindowClass( &pEditorTool->m_toolWindowClass );
                ImGui::SetNextWindowSizeConstraints( ImVec2( 128, 128 ), ImVec2( FLT_MAX, FLT_MAX ) );

                if ( isGamePreviewerTool )
                {
                    viewportWindowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration;
                }

                if ( ImGui::Begin( toolWindowName.c_str(), nullptr, viewportWindowFlags ) )
                {
                    // Ensure viewport is the correct desired size
                    if ( isGamePreviewerTool )
                    {
                        Int2 const actualSize = Float2( ImGui::GetContentRegionAvail() );
                        Int2 const delta = m_pGamePreviewer->GetDesiredViewportSize() - actualSize;
                        m_pGamePreviewer->SetRequiredWindowSize( m_pGamePreviewer->GetRequiredWindowSize() + delta );
                    }

                    pEditorTool->m_isViewportFocused = ImGui::IsWindowFocused();
                    pEditorTool->m_isViewportHovered = ImGui::IsWindowHovered();
                    pEditorTool->DrawViewportWindow( context, pToolWindow->m_pViewport );
                }
                ImGui::End();
            }
            else // Draw the tool window
            {
                ImGuiWindowFlags toolWindowFlags = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus;
                if ( pToolWindow->m_disableScrolling )
                {
                    toolWindowFlags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
                }

                ImGui::SetNextWindowClass( &pEditorTool->m_toolWindowClass );

                ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
                if ( pEditorTool->IsSingleWindowTool() )
                {
                    windowPadding = ImVec2( 0, 0 );
                }
                else if ( pToolWindow->HasUserSpecifiedWindowPadding() )
                {
                    windowPadding = pToolWindow->m_windowPadding;
                }

                ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, windowPadding );
                bool const drawToolWindow = ImGui::Begin( toolWindowName.c_str(), &pToolWindow->m_isOpen, toolWindowFlags );
                ImGui::PopStyleVar();

                if ( drawToolWindow )
                {
                    bool const isToolWindowFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_DockHierarchy );
                    pToolWindow->m_drawFunction( context, isToolWindowFocused );

                    // Hide docking UI
                    if ( pEditorTool->IsSingleWindowTool() )
                    {
                        if ( auto pDockNode = ImGui::GetWindowDockNode() )
                        {
                            pDockNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
                            pDockNode->LocalFlags |= ImGuiDockNodeFlags_NoDockingOverMe;
                            pDockNode->LocalFlags |= ImGuiDockNodeFlags_NoDockingOverOther;
                        }
                    }
                }

                ImGui::End();
            }
        }

        pEditorTool->PostDrawUpdate( context, isVisible, pEditorTool->m_isViewportFocused );
    }

    void EditorUI::CreateGamePreviewTool( UpdateContext const& context )
    {
        EE_ASSERT( m_pGamePreviewer == nullptr );
        auto& request = m_toolOperations.emplace_back();
        request.m_type = ToolOperation::CreateGamePreview;
    }

    void EditorUI::DestroyGamePreviewTool( UpdateContext const& context )
    {
        EE_ASSERT( m_pGamePreviewer != nullptr );
        QueueDestroyTool( m_pGamePreviewer );
    }
}