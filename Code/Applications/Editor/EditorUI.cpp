#include "EditorUI.h"
#include "RenderingSystem.h"
#include "EngineTools/Entity/Workspaces/Workspace_MapEditor.h"
#include "EngineTools/Entity/Workspaces/Workspace_GamePreviewer.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "EngineTools/Core/ToolsEmbeddedResources.inl"
#include "Engine/Physics/Debug/DebugView_Physics.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/DebugViews/DebugView_Resource.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Render/DebugViews/DebugView_Render.h"
#include "System/Imgui/ImguiImageCache.h"
#include "System/Resource/ResourceSystem.h"
#include "System/Resource/ResourceSettings.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/ThirdParty/implot/implot.h"

//-------------------------------------------------------------------------

namespace EE
{
    EditorUI::~EditorUI()
    {
        EE_ASSERT( m_workspaces.empty() );
        EE_ASSERT( m_pMapEditor == nullptr );
        EE_ASSERT( m_pGamePreviewer == nullptr );

        EE_ASSERT( m_pResourceBrowser == nullptr );
        EE_ASSERT( m_pRenderingSystem == nullptr );
        EE_ASSERT( m_pWorldManager == nullptr );
    }

    void EditorUI::SetStartupMap( ResourceID const& mapID )
    {
        EE_ASSERT( mapID.IsValid() );

        if ( mapID.GetResourceTypeID() == EntityModel::SerializedEntityMap::GetStaticResourceTypeID() )
        {
            m_startupMapResourceID = mapID;
        }
        else
        {
            EE_LOG_ERROR( "Editor", "Invalid startup map resource supplied: %s", m_startupMapResourceID.c_str() );
        }
    }

    void EditorUI::Initialize( UpdateContext const& context, ImGuiX::ImageCache* pImageCache )
    {
        // Systems
        //-------------------------------------------------------------------------

        m_pTypeRegistry = context.GetSystem<TypeSystem::TypeRegistry>();
        m_pSystemRegistry = context.GetSystemRegistry();
        m_pWorldManager = context.GetSystem<EntityWorldManager>();
        m_pRenderingSystem = context.GetSystem<Render::RenderingSystem>();
        auto pTaskSystem = context.GetSystem<TaskSystem>();

        // Resources
        //-------------------------------------------------------------------------

        auto pResourceSystem = context.GetSystem<Resource::ResourceSystem>();
        m_resourceDB.Initialize( m_pTypeRegistry, pTaskSystem, pResourceSystem->GetSettings().m_rawResourcePath, pResourceSystem->GetSettings().m_compiledResourcePath );
        m_resourceDeletedEventID = m_resourceDB.OnResourceDeleted().Bind( [this] ( ResourceID const& resourceID ) { OnResourceDeleted( resourceID ); } );
        m_pResourceDatabase = &m_resourceDB;
        m_pResourceBrowser = EE::New<ResourceBrowser>( *this );

        // Icons/Images
        //-------------------------------------------------------------------------

        EE_ASSERT( pImageCache != nullptr );
        m_pImageCache = pImageCache;
        m_editorIcon = m_pImageCache->LoadImageFromMemoryBase64( g_encodedDataGreen, 3328 );

        // Map Editor
        //-------------------------------------------------------------------------

        // Destroy the default created game world
        m_pWorldManager->DestroyWorld( m_pWorldManager->GetWorlds()[0] );

        // Create a new editor world for the map editor workspace
        auto pMapEditorWorld = m_pWorldManager->CreateWorld( EntityWorldType::Tools );
        m_pRenderingSystem->CreateCustomRenderTargetForViewport( pMapEditorWorld->GetViewport(), true );

        // Create the map editor workspace
        m_pMapEditor = EE::New<EntityModel::EntityMapEditor>( this, pMapEditorWorld );
        m_pMapEditor->Initialize( context );
        m_workspaces.emplace_back( m_pMapEditor );

        m_gamePreviewStartRequestEventBindingID = m_pMapEditor->OnGamePreviewStartRequested().Bind( [this] ( UpdateContext const& context ) { CreateGamePreviewWorkspace( context ); } );
        m_gamePreviewStopRequestEventBindingID = m_pMapEditor->OnGamePreviewStopRequested().Bind( [this] ( UpdateContext const& context ) { DestroyGamePreviewWorkspace( context ); } );

        // Load startup map
        if ( m_startupMapResourceID.IsValid() )
        {
            EE_ASSERT( m_startupMapResourceID.GetResourceTypeID() == EntityModel::SerializedEntityMap::GetStaticResourceTypeID() );
            m_pMapEditor->LoadMap( m_startupMapResourceID );
        }
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

        // Workspaces
        //-------------------------------------------------------------------------

        while ( !m_workspaces.empty() )
        {
            DestroyWorkspace( context, m_workspaces[0], true );
        }

        m_workspaces.clear();

        // Misc
        //-------------------------------------------------------------------------

        m_pImageCache->UnloadImage( m_editorIcon );

        // Resources
        //-------------------------------------------------------------------------

        EE::Delete( m_pResourceBrowser );
        m_pResourceDatabase = nullptr;
        m_resourceDB.OnResourceDeleted().Unbind( m_resourceDeletedEventID );
        m_resourceDB.Shutdown();

        // Systems
        //-------------------------------------------------------------------------

        m_pWorldManager = nullptr;
        m_pRenderingSystem = nullptr;
        m_pTypeRegistry = nullptr;
    }

    void EditorUI::TryOpenResource( ResourceID const& resourceID ) const
    {
        if ( resourceID.IsValid() )
        {
            const_cast<EditorUI*>( this )->QueueCreateWorkspace( resourceID );
        }
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
        ImGui::SetCursorPos( ImGui::GetCursorPos() + ImVec2( 2, 4 ) );

        //-------------------------------------------------------------------------

        ImGuiX::Image( m_editorIcon );

        //-------------------------------------------------------------------------

        ImGui::SameLine();

        if ( ImGui::BeginChild( "#HackChild", ImVec2( 0, 0 ), false, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar ) )
        {
            if ( ImGui::BeginMenuBar() )
            {
                if ( ImGui::BeginMenu( "Resource" ) )
                {
                    ImGui::MenuItem( "Resource Browser", nullptr, &m_isResourceBrowserWindowOpen );
                    ImGui::MenuItem( "Resource System Overview", nullptr, &m_isResourceOverviewWindowOpen );
                    ImGui::MenuItem( "Resource Log", nullptr, &m_isResourceLogWindowOpen );
                    ImGui::EndMenu();
                }

                ImGui::SameLine();
                if ( ImGui::BeginMenu( "System" ) )
                {
                    ImGui::MenuItem( "System Log", nullptr, &m_isSystemLogWindowOpen );

                    ImGui::Separator();

                    ImGui::MenuItem( "Imgui UI Test Window", nullptr, &m_isUITestWindowOpen );
                    ImGui::MenuItem( "Imgui Demo Window", nullptr, &m_isImguiDemoWindowOpen );

                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }
        }
        ImGui::EndChild();
    }

    void EditorUI::DrawTitleBarGamePreviewControls( UpdateContext const& context )
    {
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 6 );
        ImGui::BeginDisabled( !m_pMapEditor->HasLoadedMap() );
        if ( m_pGamePreviewer != nullptr )
        {
            if ( ImGuiX::FlatIconButton( EE_ICON_STOP, "Stop Preview##MapEditor", ImGuiX::ImColors::Red, ImVec2( -1, 0 ) ) )
            {
                DestroyGamePreviewWorkspace( context );
            }
        }
        else
        {
            if ( ImGuiX::FlatIconButton( EE_ICON_PLAY, "Preview Loaded Map##MapEditor", ImGuiX::ImColors::Lime, ImVec2( -1, 0 ) ) )
            {
                CreateGamePreviewWorkspace( context );
            }
        }
        ImGui::EndDisabled();
    }

    void EditorUI::DrawTitleBarPerformanceStats( UpdateContext const& context )
    {
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 6 );
        SystemDebugView::DrawFrameLimiterCombo( const_cast<UpdateContext&>( context ) );

        ImGui::SameLine();
        float const currentFPS = 1.0f / context.GetDeltaTime();
        TInlineString<100> const perfStats( TInlineString<100>::CtorSprintf(), "FPS: %3.0f", currentFPS );
        ImGui::Text( perfStats.c_str() );

        ImGui::SameLine();
        float const allocatedMemory = Memory::GetTotalAllocatedMemory() / 1024.0f / 1024.0f;
        TInlineString<100> const memStats( TInlineString<100>::CtorSprintf(), "MEM: %.2fMB", allocatedMemory );
        ImGui::Text( memStats.c_str() );
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

        m_resourceDB.Update();

        //-------------------------------------------------------------------------
        // Workspace Management
        //-------------------------------------------------------------------------

        // Destroy all required workspaces
        // We needed to defer this to the start of the update since we may have references resources that we might unload (i.e. textures)
        for ( auto pWorkspaceToDestroy : m_workspaceDestructionRequests )
        {
            DestroyWorkspace( context, pWorkspaceToDestroy );
        }
        m_workspaceDestructionRequests.clear();

        // Create all workspaces
        for ( auto const& resourceID : m_workspaceCreationRequests )
        {
            TryCreateWorkspace( context, resourceID );
        }
        m_workspaceCreationRequests.clear();

        //-------------------------------------------------------------------------
        // Title Bar
        //-------------------------------------------------------------------------

        auto TitleBarLeftContents = [this, &context] ()
        {
            DrawTitleBarMenu( context );
        };

        auto TitleBarMidContents = [this, &context] ()
        {
            // Do Nothing
        };

        auto TitleBarRightContents = [this, &context] ()
        {
            DrawTitleBarPerformanceStats( context );
        };

        m_titleBar.Draw( TitleBarLeftContents, 210, TitleBarMidContents, 170, TitleBarRightContents, 190 );

        //-------------------------------------------------------------------------
        // Create main dock window
        //-------------------------------------------------------------------------

        m_editorWindowClass.ClassId = ImGui::GetID( "EditorWindowClass" );
        m_editorWindowClass.DockingAllowUnclassed = false;

        ImGuiID const dockspaceID = ImGui::GetID( "EditorDockSpace" );

        ImGuiWindowFlags const windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGuiViewport const* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos( viewport->WorkPos );
        ImGui::SetNextWindowSize( viewport->WorkSize );
        ImGui::SetNextWindowViewport( viewport->ID );

        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
        ImGui::Begin( "EditorDockSpaceWindow", nullptr, windowFlags );
        ImGui::PopStyleVar( 3 );
        {
            if ( !ImGui::DockBuilderGetNode( dockspaceID ) )
            {
                ImGui::DockBuilderAddNode( dockspaceID, ImGuiDockNodeFlags_DockSpace );
                ImGui::DockBuilderSetNodeSize( dockspaceID, ImGui::GetContentRegionAvail() );
                ImGuiID leftDockID = 0, rightDockID = 0;
                ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.25f, &leftDockID, &rightDockID );
                ImGui::DockBuilderFinish( dockspaceID );

                ImGui::DockBuilderDockWindow( m_pResourceBrowser->GetWindowName(), leftDockID );
                ImGui::DockBuilderDockWindow( m_pMapEditor->GetWorkspaceWindowID(), rightDockID );
            }

            // Create the actual dock space
            ImGui::PushStyleVar( ImGuiStyleVar_TabRounding, 0 );
            ImGui::DockSpace( dockspaceID, viewport->WorkSize, ImGuiDockNodeFlags_None, &m_editorWindowClass );
            ImGui::PopStyleVar( 1 );
        }
        ImGui::End();

        //-------------------------------------------------------------------------
        // Draw editor windows
        //-------------------------------------------------------------------------

        if ( m_isResourceBrowserWindowOpen )
        {
            ImGui::SetNextWindowClass( &m_editorWindowClass );
            m_isResourceBrowserWindowOpen = m_pResourceBrowser->UpdateAndDraw( context );
        }

        if ( m_isResourceOverviewWindowOpen )
        {
            ImGui::SetNextWindowClass( &m_editorWindowClass );
            auto pResourceSystem = context.GetSystem<Resource::ResourceSystem>();
            Resource::ResourceDebugView::DrawOverviewWindow( pResourceSystem, &m_isResourceOverviewWindowOpen );
        }

        if ( m_isResourceLogWindowOpen )
        {
            ImGui::SetNextWindowClass( &m_editorWindowClass );
            auto pResourceSystem = context.GetSystem<Resource::ResourceSystem>();
            Resource::ResourceDebugView::DrawLogWindow( pResourceSystem, &m_isResourceLogWindowOpen );
        }

        if ( m_isSystemLogWindowOpen )
        {
            ImGui::SetNextWindowClass( &m_editorWindowClass );
            m_isSystemLogWindowOpen = m_systemLogView.Draw( context );
        }

        if ( m_isImguiDemoWindowOpen )
        {
            ImGui::ShowDemoWindow( &m_isImguiDemoWindowOpen );
            ImPlot::ShowDemoWindow( &m_isImguiDemoWindowOpen );
        }

        if ( m_isUITestWindowOpen )
        {
            DrawUITestWindow();
        }

        //-------------------------------------------------------------------------
        // Draw open workspaces
        //-------------------------------------------------------------------------

        // Reset mouse state, this is updated via the workspaces
        Workspace* pWorkspaceToClose = nullptr;

        // Draw all workspaces
        for ( auto pWorkspace : m_workspaces )
        {
            // The game previewer is special and is handled separately
            if ( pWorkspace == m_pGamePreviewer )
            {
                continue;
            }

            // Dont draw any workspaces queued for destructor
            if ( VectorContains( m_workspaceDestructionRequests, pWorkspace ) )
            {
                continue;
            }

            // Draw the workspaces
            EE_ASSERT( pWorkspace->IsInitialized() );
            ImGui::SetNextWindowClass( &m_editorWindowClass );
            if ( !DrawWorkspaceWindow( context, pWorkspace ) )
            {
                pWorkspaceToClose = pWorkspace;
            }
        }

        // Did we get a close request?
        if ( pWorkspaceToClose != nullptr )
        {
            // We need to defer this to the start of the update since we may have references resources that we might unload (i.e. textures)
            QueueDestroyWorkspace( pWorkspaceToClose );
        }

        //-------------------------------------------------------------------------
        // Handle Warnings/Errors
        //-------------------------------------------------------------------------

        auto const unhandledWarningsAndErrors = Log::GetUnhandledWarningsAndErrors();
        if ( !unhandledWarningsAndErrors.empty() )
        {
            m_isSystemLogWindowOpen = true;
        }
    }

    void EditorUI::EndFrame( UpdateContext const& context )
    {
        // Game previewer needs to be drawn at the end of the frames since then all the game simulation data will be correct and all the debug tools will be accurate
        if ( m_pGamePreviewer != nullptr )
        {
            if ( !DrawWorkspaceWindow( context, m_pGamePreviewer ) )
            {
                QueueDestroyWorkspace( m_pGamePreviewer );
            }
        }
    }

    void EditorUI::Update( UpdateContext const& context )
    {
        for ( auto pWorkspace : m_workspaces )
        {
            EntityWorldUpdateContext updateContext( context, pWorkspace->GetWorld() );
            pWorkspace->PreUpdateWorld( updateContext );
        }
    }

    //-------------------------------------------------------------------------
    // Hot Reload
    //-------------------------------------------------------------------------

    void EditorUI::BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded )
    {
        for ( auto pWorkspace : m_workspaces )
        {
            pWorkspace->BeginHotReload( usersToBeReloaded, resourcesToBeReloaded );
        }
    }

    void EditorUI::EndHotReload()
    {
        for ( auto pWorkspace : m_workspaces )
        {
            pWorkspace->EndHotReload();

            // Auto destroy any workspaces that had a problem loading their descriptor i.e. they were externally corrupted.
            if ( pWorkspace->IsADescriptorWorkspace() && !pWorkspace->IsDescriptorLoaded() )
            {
                InlineString const str( InlineString::CtorSprintf(), "There was an error reloading the descriptor for workspace: %s! Please check the log for details.", pWorkspace->GetDisplayName() );
                pfd::message( "Error Loading Descriptor", str.c_str(), pfd::choice::ok, pfd::icon::error ).result();
                QueueDestroyWorkspace( pWorkspace );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Resource Management
    //-------------------------------------------------------------------------

    void EditorUI::OnResourceDeleted( ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );

        for ( auto pWorkspace : m_workspaces )
        {
            if ( pWorkspace->HasDependencyOnResource( resourceID ) )
            {
                QueueDestroyWorkspace( pWorkspace );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Workspace Management
    //-------------------------------------------------------------------------

    bool EditorUI::TryCreateWorkspace( UpdateContext const& context, ResourceID const& resourceID )
    {
        ResourceTypeID const resourceTypeID = resourceID.GetResourceTypeID();

        // Don't try to open invalid resource IDs
        if ( !m_resourceDB.DoesResourceExist( resourceID ) )
        {
            return false;
        }

        // Handle maps explicitly
        //-------------------------------------------------------------------------

        if ( resourceTypeID == EntityModel::SerializedEntityMap::GetStaticResourceTypeID() )
        {
            m_pMapEditor->LoadMap( resourceID );
            ImGuiX::MakeTabVisible( m_pMapEditor->GetWorkspaceWindowID() );
            return true;
        }

        // Other resource types
        //-------------------------------------------------------------------------

        // Check if we already have a workspace open for this resource, if so then switch focus to it
        for ( auto pWorkspace : m_workspaces )
        {
            if ( pWorkspace->IsWorkingOnResource( resourceID ) )
            {
                ImGuiX::MakeTabVisible( pWorkspace->GetWorkspaceWindowID() );
                return true;
            }
        }

        // Check if we can create a new workspace
        if ( !ResourceWorkspaceFactory::CanCreateWorkspace( this, resourceID ) )
        {
            return false;
        }

        // Create tools world
        auto pWorkspaceWorld = m_pWorldManager->CreateWorld( EntityWorldType::Tools );
        m_pRenderingSystem->CreateCustomRenderTargetForViewport( pWorkspaceWorld->GetViewport(), true );

        // Create workspace
        auto pCreatedWorkspace = ResourceWorkspaceFactory::CreateWorkspace( this, pWorkspaceWorld, resourceID );
        m_workspaces.emplace_back( pCreatedWorkspace );

        // Check if the descriptor was correctly loaded, if not schedule this workspace to be destroyed
        if ( pCreatedWorkspace->IsADescriptorWorkspace() && !pCreatedWorkspace->IsDescriptorLoaded() )
        {
            InlineString const str( InlineString::CtorSprintf(), "There was an error loading the descriptor for %s! Please check the log for details.", resourceID.c_str() );
            pfd::message( "Error Loading Descriptor", str.c_str(), pfd::choice::ok, pfd::icon::error ).result();
            QueueDestroyWorkspace( pCreatedWorkspace );
            return false;
        }

        // Initialize workspace
        pCreatedWorkspace->Initialize( context );
        return true;
    }

    void EditorUI::QueueCreateWorkspace( ResourceID const& resourceID )
    {
        m_workspaceCreationRequests.emplace_back( resourceID );
    }

    void EditorUI::DestroyWorkspace( UpdateContext const& context, Workspace* pWorkspace, bool isEditorShutdown )
    {
        EE_ASSERT( m_pMapEditor != pWorkspace );
        EE_ASSERT( pWorkspace != nullptr );

        auto foundWorkspaceIter = eastl::find( m_workspaces.begin(), m_workspaces.end(), pWorkspace );
        EE_ASSERT( foundWorkspaceIter != m_workspaces.end() );

        if ( pWorkspace->IsDirty() )
        {
            auto messageDialog = pfd::message( "Unsaved Changes", "You have unsaved changes!\nDo you wish to save these changes before closing?", isEditorShutdown ? pfd::choice::yes_no : pfd::choice::yes_no_cancel );
            switch ( messageDialog.result() )
            {
                case pfd::button::yes:
                {
                    if ( !pWorkspace->Save() )
                    {
                        return;
                    }
                }
                break;

                case pfd::button::cancel:
                {
                    return;
                }
                break;

                case pfd::button::no:
                default:
                {
                    // Do Nothing
                }
                break;
            }
        }

        //-------------------------------------------------------------------------

        bool const isGamePreviewerWorkspace = m_pGamePreviewer == pWorkspace;

        // Get the world before we destroy the workspace
        auto pWorkspaceWorld = pWorkspace->GetWorld();

        // Destroy workspace
        if ( pWorkspace->IsInitialized() )
        {
            pWorkspace->Shutdown( context );
        }
        EE::Delete( pWorkspace );
        m_workspaces.erase( foundWorkspaceIter );

        // Clear the game previewer workspace ptr if we just destroyed it
        if ( isGamePreviewerWorkspace )
        {
            m_pMapEditor->NotifyGamePreviewEnded();
            m_pGamePreviewer = nullptr;
        }

        // Destroy preview world and render target
        m_pRenderingSystem->DestroyCustomRenderTargetForViewport( pWorkspaceWorld->GetViewport() );
        m_pWorldManager->DestroyWorld( pWorkspaceWorld );
    }

    void EditorUI::QueueDestroyWorkspace( Workspace* pWorkspace )
    {
        EE_ASSERT( m_pMapEditor != pWorkspace );
        m_workspaceDestructionRequests.emplace_back( pWorkspace );
    }

    bool EditorUI::DrawWorkspaceWindow( UpdateContext const& context, Workspace* pWorkspace )
    {
        EE_ASSERT( pWorkspace != nullptr );

        //-------------------------------------------------------------------------
        // Create Workspace Window
        //-------------------------------------------------------------------------
        // This is an empty window that just contains the dockspace for the workspace

        bool isTabOpen = true;
        bool* pIsTabOpen = ( pWorkspace == m_pMapEditor ) ? nullptr : &isTabOpen; // Prevent closing the map-editor workspace

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;

        if ( pWorkspace->HasWorkspaceToolbar() )
        {
            windowFlags |= ImGuiWindowFlags_MenuBar;
        }

        if ( pWorkspace->IsDirty() )
        {
            windowFlags |= ImGuiWindowFlags_UnsavedDocument;
        }

        ImGui::SetNextWindowSizeConstraints( ImVec2( 128, 128 ), ImVec2( FLT_MAX, FLT_MAX ) );
        ImGui::SetNextWindowSize( ImVec2( 1024, 768 ), ImGuiCond_FirstUseEver );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1.0f );

        bool const shouldDrawWindowContents = ImGui::Begin( pWorkspace->GetWorkspaceWindowID(), pIsTabOpen, windowFlags );
        bool const isFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_DockHierarchy );
        ImGui::PopStyleVar();

        // Draw Workspace Menu
        //-------------------------------------------------------------------------

        if ( pWorkspace->HasWorkspaceToolbar() )
        {
            if ( ImGui::BeginMenuBar() )
            {
                pWorkspace->DrawWorkspaceToolbar( context );
                ImGui::EndMenuBar();
            }
        }

        // Create dockspace
        //-------------------------------------------------------------------------

        ImGuiID const dockspaceID = ImGui::GetID( pWorkspace->GetDockspaceID() );
        ImGuiWindowClass workspaceWindowClass;
        workspaceWindowClass.ClassId = dockspaceID;
        workspaceWindowClass.DockingAllowUnclassed = false;

        if ( !ImGui::DockBuilderGetNode( dockspaceID ) )
        {
            ImGui::DockBuilderAddNode( dockspaceID, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton );
            ImGui::DockBuilderSetNodeSize( dockspaceID, ImGui::GetContentRegionAvail() );
            pWorkspace->InitializeDockingLayout( dockspaceID );
            ImGui::DockBuilderFinish( dockspaceID );
        }

        ImGuiDockNodeFlags const dockFlags = shouldDrawWindowContents ? ImGuiDockNodeFlags_None : ImGuiDockNodeFlags_KeepAliveOnly;
        ImGui::DockSpace( dockspaceID, ImGui::GetContentRegionAvail(), dockFlags, &workspaceWindowClass );

        //-------------------------------------------------------------------------
        // Draw workspace contents
        //-------------------------------------------------------------------------

        bool enableCameraUpdate = false;
        auto pWorld = pWorkspace->GetWorld();

        if ( shouldDrawWindowContents )
        {
            if ( pWorkspace != m_pMapEditor || m_pGamePreviewer == nullptr )
            {
                pWorld->ResumeUpdates();
            }

            if ( pWorkspace->HasViewportWindow() )
            {
                Workspace::ViewportInfo viewportInfo;
                viewportInfo.m_pViewportRenderTargetTexture = (void*) &m_pRenderingSystem->GetRenderTargetTextureForViewport( pWorld->GetViewport() );
                viewportInfo.m_retrievePickingID = [this, pWorld] ( Int2 const& pixelCoords ) { return m_pRenderingSystem->GetViewportPickingID( pWorld->GetViewport(), pixelCoords ); };
                enableCameraUpdate = pWorkspace->DrawViewport( context, viewportInfo, &workspaceWindowClass );
            }

            pWorkspace->CommonUpdate( context, &workspaceWindowClass, isFocused );
            pWorkspace->Update( context, &workspaceWindowClass, isFocused );
        }
        else // If the workspace window is hidden suspend world updates
        {
            pWorld->SuspendUpdates();
        }

        pWorkspace->SetCameraUpdateEnabled( enableCameraUpdate );

        //-------------------------------------------------------------------------

        // End the workspace window here so that it is still in the window stack so that popups can get the correct viewport
        ImGui::End();

        //-------------------------------------------------------------------------

        return isTabOpen;
    }

    void EditorUI::CreateGamePreviewWorkspace( UpdateContext const& context )
    {
        EE_ASSERT( m_pGamePreviewer == nullptr );

        auto pPreviewWorld = m_pWorldManager->CreateWorld( EntityWorldType::Game );
        m_pRenderingSystem->CreateCustomRenderTargetForViewport( pPreviewWorld->GetViewport() );
        m_pGamePreviewer = EE::New<GamePreviewer>( this, pPreviewWorld );
        m_pGamePreviewer->Initialize( context );
        m_pGamePreviewer->LoadMapToPreview( m_pMapEditor->GetLoadedMap() );
        m_workspaces.emplace_back( m_pGamePreviewer );

        m_pMapEditor->NotifyGamePreviewStarted();
    }

    void EditorUI::DestroyGamePreviewWorkspace( UpdateContext const& context )
    {
        EE_ASSERT( m_pGamePreviewer != nullptr );
        QueueDestroyWorkspace( m_pGamePreviewer );
    }

    //-------------------------------------------------------------------------
    // Misc
    //-------------------------------------------------------------------------

    void EditorUI::DrawUITestWindow()
    {
        if ( ImGui::Begin( "UI Test", &m_isUITestWindowOpen ) )
        {
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Tiny );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - Tiny" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::TinyBold );
                ImGui::Text( EE_ICON_ALERT"This is a test - Tiny Bold" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - Small" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::SmallBold );
                ImGui::Text( EE_ICON_ALERT"This is a test - Small Bold" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - Medium" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::MediumBold );
                ImGui::Text( EE_ICON_ALERT"This is a test - Medium Bold" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - Large" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                ImGui::Text( EE_ICON_CCTV_OFF"This is a test - Large Bold" );
            }

            //-------------------------------------------------------------------------

            ImGui::NewLine();

            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_PLUS"ADD" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::SmallBold );
                ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_PLUS"ADD" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
                ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_PLUS"ADD" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::MediumBold );
                ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_PLUS"ADD" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_PLUS"ADD" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_PLUS"ADD" );
            }

            //-------------------------------------------------------------------------

            ImGui::NewLine();

            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                ImGui::Button( EE_ICON_HAIR_DRYER );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_Z_WAVE );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_KANGAROO );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_YIN_YANG );
            }

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
                ImGui::Button( EE_ICON_HAIR_DRYER );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_Z_WAVE );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_KANGAROO );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_YIN_YANG );
            }

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                ImGui::Button( EE_ICON_HAIR_DRYER );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_Z_WAVE );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_KANGAROO );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_YIN_YANG );
            }

            //-------------------------------------------------------------------------

            ImGuiX::IconButton( EE_ICON_KANGAROO, "Test", ImGuiX::ImColors::PaleGreen, ImVec2( 100, 0 ) );

            ImGuiX::IconButton( EE_ICON_HOME, "Home", ImGuiX::ImColors::RoyalBlue, ImVec2( 100, 0 ) );

            ImGuiX::IconButton( EE_ICON_MOVIE_PLAY, "Play", ImGuiX::ImColors::LightPink, ImVec2( 100, 0 ) );

            ImGuiX::ColoredIconButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, ImGuiX::ImColors::Yellow, EE_ICON_KANGAROO, "Test", ImVec2( 100, 0 ) );

            ImGuiX::FlatIconButton( EE_ICON_HOME, "Home", ImGuiX::ImColors::RoyalBlue, ImVec2( 100, 0 ) );

            //-------------------------------------------------------------------------

            ImGui::AlignTextToFramePadding();
            ImGuiX::SameLineSeparator(20);

            ImGui::SameLine(0,0);
            ImGui::Text( "Test" );

            ImGui::SameLine( 0, 0 );
            ImGuiX::SameLineSeparator();

            ImGui::SameLine( 0, 0 );
            ImGui::Text( "Test" );

            ImGui::SameLine( 0, 0 );
            ImGuiX::SameLineSeparator( 40 );

            ImGui::SameLine( 0, 0 );
            ImGui::Text( "Test" );

            ImGui::SameLine( 0, 0 );
            ImGuiX::SameLineSeparator();
        }
        ImGui::End();
    }
}