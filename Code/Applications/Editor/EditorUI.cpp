#include "EditorUI.h"
#include "RenderingSystem.h"
#include "EngineTools/Entity/ResourceEditors/ResourceEditor_MapEditor.h"
#include "EngineTools/Entity/Tools/EditorTool_GamePreviewer.h"
#include "EngineTools/Resource/Tools/EditorTool_ResourceBrowser.h"
#include "EngineTools/Resource/Tools/EditorTool_ResourceSystem.h"
#include "EngineTools/Resource/Tools/EditorTool_ResourceImporter.h"
#include "EngineTools/Logging/Tools/EditorTool_SystemLog.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "EngineTools/Core/ToolsEmbeddedResources.inl"
#include "Engine/ToolsUI/EngineDebugUI.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Render/DebugViews/DebugView_Render.h"
#include "Base/Imgui/ImguiImageCache.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/Resource/ResourceSettings.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/ThirdParty/implot/implot.h"
#include "Base/Logging/LoggingSystem.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE
{
    EditorUI::~EditorUI()
    {
        EE_ASSERT( m_editorTools.empty() );
        EE_ASSERT( m_pMapEditor == nullptr );
        EE_ASSERT( m_pGamePreviewer == nullptr );

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
        m_pRenderingSystem = context.GetSystem<Render::RenderingSystem>();
        auto pTaskSystem = context.GetSystem<TaskSystem>();

        // Resources
        //-------------------------------------------------------------------------

        auto pResourceSystem = context.GetSystem<Resource::ResourceSystem>();
        m_resourceDB.Initialize( m_pTypeRegistry, pTaskSystem, pResourceSystem->GetSettings().m_rawResourcePath, pResourceSystem->GetSettings().m_compiledResourcePath );
        m_resourceDeletedEventID = m_resourceDB.OnResourceDeleted().Bind( [this] ( ResourceID const& resourceID ) { OnResourceDeleted( resourceID ); } );
        m_pResourceDatabase = &m_resourceDB;

        // Icons/Images
        //-------------------------------------------------------------------------

        EE_ASSERT( pImageCache != nullptr );
        m_pImageCache = pImageCache;
        m_editorIcon = m_pImageCache->LoadImageFromMemoryBase64( g_encodedDataGreen, 3328 );

        // Map Editor
        //-------------------------------------------------------------------------

        auto& request = m_editorToolCreationRequests.emplace_back();
        request.m_type = ToolCreationRequest::MapEditor;

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

        for ( auto& creationRequest : m_editorToolCreationRequests )
        {
            if ( creationRequest.m_type == ToolCreationRequest::UninitializedTool )
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

        m_pResourceDatabase = nullptr;
        m_resourceDB.OnResourceDeleted().Unbind( m_resourceDeletedEventID );
        m_resourceDB.Shutdown();

        // Systems
        //-------------------------------------------------------------------------

        m_pWorldManager = nullptr;
        m_pRenderingSystem = nullptr;
        m_pTypeRegistry = nullptr;
    }

    bool EditorUI::TryOpenResource( ResourceID const& resourceID ) const
    {
        if ( resourceID.IsValid() )
        {
            const_cast<EditorUI*>( this )->QueueCreateTool( resourceID );
            return true;
        }

        return false;
    }

    bool EditorUI::TryFindInResourceBrowser( ResourceID const& resourceID ) const
    {
        auto pResourceBrowser = GetTool<Resource::ResourceBrowserEditorTool>();
        if ( pResourceBrowser == nullptr )
        {
            pResourceBrowser = const_cast<EditorUI*>( this )->CreateTool<Resource::ResourceBrowserEditorTool>( this );
        }

        //-------------------------------------------------------------------------

        pResourceBrowser->TryFindAndSelectResource( resourceID );
        return true;
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
        ImGuiX::Image( m_editorIcon );

        //-------------------------------------------------------------------------

        ImGui::SameLine();

        if ( ImGui::BeginMenu( "Tools" ) )
        {
            ImGuiX::TextSeparator( "Resource" );

            bool isResourceBrowserOpen = GetTool<Resource::ResourceBrowserEditorTool>() != nullptr;
            if ( ImGui::MenuItem( "Resource Browser", nullptr, &isResourceBrowserOpen, !isResourceBrowserOpen ) )
            {
                CreateTool<Resource::ResourceBrowserEditorTool>( this );
            }

            bool isResourceSystemOpen = GetTool<Resource::ResourceSystemEditorTool>() != nullptr;
            if ( ImGui::MenuItem( "Resource System", nullptr, &isResourceSystemOpen, !isResourceSystemOpen ) )
            {
                CreateTool<Resource::ResourceSystemEditorTool>( this );
            }

            bool isResourceImporterOpen = GetTool<Resource::ResourceImporterEditorTool>() != nullptr;
            if ( ImGui::MenuItem( "Resource Importer", nullptr, &isResourceImporterOpen, !isResourceImporterOpen ) )
            {
                CreateTool<Resource::ResourceImporterEditorTool>( this );
            }

            if ( ImGui::MenuItem( "Rebuild Resource Database" ) )
            {
                m_resourceDB.RequestRebuild();
            }

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "System" );

            bool isLogOpen = GetTool<SystemLogEditorTool>() != nullptr;
            if ( ImGui::MenuItem( "System Log", nullptr, &isLogOpen, !isLogOpen ) )
            {
                CreateTool<SystemLogEditorTool>( this );
            }

            if ( ImGui::MenuItem( "Open Profiler" ) )
            {
                Profiling::OpenProfiler();
            }

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Editor" );

            ImGui::MenuItem( "UI Test Window", nullptr, &m_isUITestWindowOpen, !m_isUITestWindowOpen );
            ImGui::MenuItem( "ImGui Demo Window", nullptr, &m_isImguiDemoWindowOpen, !m_isImguiDemoWindowOpen );
            ImGui::MenuItem( "ImGui Plot Demo Window", nullptr, &m_isImguiPlotDemoWindowOpen, !m_isImguiPlotDemoWindowOpen );

            //-------------------------------------------------------------------------

            ImGui::EndMenu();
        }
    }

    void EditorUI::DrawTitleBarInfoStats( UpdateContext const& context )
    {
        auto pResourceSystem = m_pSystemRegistry->GetSystem<Resource::ResourceSystem>();
        if ( pResourceSystem->IsBusy() )
        {
            ImGui::SameLine();
            if ( ImGuiX::DrawSpinner( "##RS", Colors::LimeGreen ) )
            {
                CreateTool<Resource::ResourceSystemEditorTool>( this );
            }
            ImGuiX::TextTooltip( "Resource System Busy" );
        }
        else
        {
            if ( ImGuiX::FlatButton( EE_ICON_SLEEP ) )
            {
                CreateTool<Resource::ResourceSystemEditorTool>( this );
            }
            ImGuiX::TextTooltip( "Resource System Idle" );
        }

        ImGui::SameLine();
        SystemDebugView::DrawFrameLimiterCombo( const_cast<UpdateContext&>( context ) );
        ImGuiX::ItemTooltip( "Frame Limiter" );

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
        // Handle Warnings/Errors
        //-------------------------------------------------------------------------

        auto const unhandledWarningsAndErrors = Log::System::GetUnhandledWarningsAndErrors();
        if ( !unhandledWarningsAndErrors.empty() )
        {
            CreateTool<SystemLogEditorTool>( this );
        }

        //-------------------------------------------------------------------------
        // Editor Tool Management
        //-------------------------------------------------------------------------

        // Destroy all required editor tools
        // We needed to defer this to the start of the update since we may have references resources that we might unload (i.e. textures)
        for ( auto pEditorToolToDestroy : m_editorToolDestructionRequests )
        {
            if ( m_pLastActiveTool == pEditorToolToDestroy )
            {
                m_pLastActiveTool = nullptr;
            }

            DestroyTool( context, pEditorToolToDestroy );
        }
        m_editorToolDestructionRequests.clear();

        // Create all editor tools
        for ( ToolCreationRequest const& request : m_editorToolCreationRequests )
        {
            TryCreateTool( context, request );
        }
        m_editorToolCreationRequests.clear();

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

        m_titleBar.Draw( TitleBarLeftContents, 210, TitleBarRightContents, 250 );

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
            DrawUITestWindow();
        }

        //-------------------------------------------------------------------------
        // Draw open editor tools
        //-------------------------------------------------------------------------

        EditorTool* pEditorToolToClose = nullptr;

        // Clear the modal dialog flag - this is done to ensure we only get one modal dialog at a time
        m_hasOpenModalDialog = false;

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
            if ( VectorContains( m_editorToolDestructionRequests, pEditorTool ) )
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
        if ( m_pGamePreviewer != nullptr && !VectorContains( m_editorToolDestructionRequests, m_pGamePreviewer ) )
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
                pEditorTool->PreWorldUpdate( updateContext );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Hot Reload
    //-------------------------------------------------------------------------

    void EditorUI::HotReload_UnloadResources( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded )
    {
        for ( auto pEditorTool : m_editorTools )
        {
            pEditorTool->HotReload_UnloadResources( usersToBeReloaded, resourcesToBeReloaded );
        }

        if ( m_pGamePreviewer != nullptr )
        {
            m_pGamePreviewer->m_pDebugUI->HotReload_UnloadResources( usersToBeReloaded, resourcesToBeReloaded );
        }
    }

    void EditorUI::HotReload_ReloadResources()
    {
        for ( auto pEditorTool : m_editorTools )
        {
            pEditorTool->HotReload_ReloadResources();

            // Auto destroy any editor tools that had a problem loading their descriptor i.e. they were externally corrupted.
            if ( pEditorTool->IsResourceEditor() && !pEditorTool->IsDescriptorLoaded() )
            {
                InlineString const str( InlineString::CtorSprintf(), "There was an error reloading the descriptor for editor tool: %s! Please check the log for details.", pEditorTool->GetDisplayName() );
                pfd::message( "Error Loading Descriptor", str.c_str(), pfd::choice::ok, pfd::icon::error ).result();
                QueueDestroyTool( pEditorTool );
            }
        }

        if ( m_pGamePreviewer != nullptr )
        {
            m_pGamePreviewer->m_pDebugUI->HotReload_ReloadResources();
        }
    }

    //-------------------------------------------------------------------------
    // Resource Management
    //-------------------------------------------------------------------------

    void EditorUI::OnResourceDeleted( ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );

        for ( auto pEditorTool : m_editorTools )
        {
            if ( pEditorTool->HasDependencyOnResource( resourceID ) )
            {
                QueueDestroyTool( pEditorTool );
            }
        }
    }

    //-------------------------------------------------------------------------
    // EditorTool Management
    //-------------------------------------------------------------------------

    bool EditorUI::TryCreateTool( UpdateContext const& context, ToolCreationRequest const& request )
    {
        // Uninitialized EditorTool
        //-------------------------------------------------------------------------

        if ( request.m_type == ToolCreationRequest::UninitializedTool )
        {
            request.m_pEditorTool->Initialize( context );
            m_editorTools.emplace_back( request.m_pEditorTool );
            return true;
        }

        // Map editor
        //-------------------------------------------------------------------------

        else if( request.m_type == ToolCreationRequest::MapEditor )
        {
            // Destroy the default created game world
            m_pWorldManager->DestroyWorld( m_pWorldManager->GetWorlds()[0] );

            // Create a new editor world for the map editor editor tool
            auto pMapEditorWorld = m_pWorldManager->CreateWorld( EntityWorldType::Tools );
            m_pRenderingSystem->CreateCustomRenderTargetForViewport( pMapEditorWorld->GetViewport(), true );

            // Create the map editor editor tool
            m_pMapEditor = EE::New<EntityModel::EntityMapEditor>( this, pMapEditorWorld );
            m_pMapEditor->Initialize( context );
            m_editorTools.emplace_back( m_pMapEditor );

            m_gamePreviewStartRequestEventBindingID = m_pMapEditor->OnGamePreviewStartRequested().Bind( [this] ( UpdateContext const& context ) { CreateGamePreviewTool( context ); } );
            m_gamePreviewStopRequestEventBindingID = m_pMapEditor->OnGamePreviewStopRequested().Bind( [this] ( UpdateContext const& context ) { DestroyGamePreviewTool( context ); } );

            // Load startup map
            if ( m_startupMapResourceID.IsValid() )
            {
                EE_ASSERT( m_startupMapResourceID.GetResourceTypeID() == EntityModel::SerializedEntityMap::GetStaticResourceTypeID() );
                m_pMapEditor->LoadMap( m_startupMapResourceID );
            }

            return true;
        }

        // Game previewer
        //-------------------------------------------------------------------------

        else if ( request.m_type == ToolCreationRequest::GamePreview )
        {
            auto pPreviewWorld = m_pWorldManager->CreateWorld( EntityWorldType::Game );
            m_pRenderingSystem->CreateCustomRenderTargetForViewport( pPreviewWorld->GetViewport() );
            m_pGamePreviewer = EE::New<GamePreviewer>( this, pPreviewWorld );
            m_pGamePreviewer->Initialize( context );
            m_pGamePreviewer->LoadMapToPreview( m_pMapEditor->GetLoadedMap() );
            m_editorTools.emplace_back( m_pGamePreviewer );

            m_pMapEditor->NotifyGamePreviewStarted();

            return true;
        }

        // Resource
        //-------------------------------------------------------------------------

        else if ( request.m_type == ToolCreationRequest::ResourceEditor )
        {
            EE_ASSERT( request.m_resourceID.IsValid() );
            ResourceTypeID const resourceTypeID = request.m_resourceID.GetResourceTypeID();

            // Don't try to open invalid resource IDs
            if ( !m_resourceDB.DoesResourceExist( request.m_resourceID ) )
            {
                return false;
            }

            // Handle maps explicitly
            //-------------------------------------------------------------------------

            if ( resourceTypeID == EntityModel::SerializedEntityMap::GetStaticResourceTypeID() )
            {
                m_pMapEditor->LoadMap( request.m_resourceID );
                ImGuiX::MakeTabVisible( m_pMapEditor->m_windowName.c_str() );
                return true;
            }

            // Other resource types
            //-------------------------------------------------------------------------

            // Check if we already have a editor tool open for this resource, if so then switch focus to it
            for ( auto pEditorTool : m_editorTools )
            {
                if ( pEditorTool->IsEditingResource( request.m_resourceID ) )
                {
                    ImGuiX::MakeTabVisible( pEditorTool->m_windowName.c_str() );
                    return true;
                }
            }

            // Check if we can create a new editor tool
            if ( !ResourceEditorFactory::CanCreateEditor( this, request.m_resourceID ) )
            {
                return false;
            }

            // Create tools world
            auto pEditorToolWorld = m_pWorldManager->CreateWorld( EntityWorldType::Tools );
            m_pRenderingSystem->CreateCustomRenderTargetForViewport( pEditorToolWorld->GetViewport(), true );

            // Create editor tool
            auto pCreatedTool = ResourceEditorFactory::CreateEditor( this, pEditorToolWorld, request.m_resourceID );
            m_editorTools.emplace_back( pCreatedTool );

            // Initialize editor tool
            pCreatedTool->Initialize( context );

            // Check if the descriptor was correctly loaded, if not schedule this editor tool to be destroyed
            if ( !pCreatedTool->IsDescriptorLoaded() )
            {
                InlineString const str( InlineString::CtorSprintf(), "There was an error loading the descriptor for %s! Please check the log for details.", request.m_resourceID.c_str() );
                pfd::message( "Error Loading Descriptor", str.c_str(), pfd::choice::ok, pfd::icon::error ).result();
                DestroyTool( context, pCreatedTool );
                return false;
            }

            return true;
        }
        else
        {
            EE_UNREACHABLE_CODE();
        }

        //-------------------------------------------------------------------------

        return false;
    }

    void EditorUI::QueueCreateTool( ResourceID const& resourceID )
    {
        auto& request = m_editorToolCreationRequests.emplace_back();
        request.m_type = ToolCreationRequest::ResourceEditor;
        request.m_resourceID = resourceID;
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
            auto messageDialog = pfd::message( "Unsaved Changes", "You have unsaved changes!\nDo you wish to save these changes before closing?", isEditorShutdown ? pfd::choice::yes_no : pfd::choice::yes_no_cancel );
            switch ( messageDialog.result() )
            {
                case pfd::button::yes:
                {
                    if ( !pEditorTool->Save() )
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

        // Get the world before we destroy the editor tool
        auto pEditorToolWorld = pEditorTool->GetEntityWorld();

        // Destroy editor tool
        if ( pEditorTool->IsInitialized() )
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
            m_pRenderingSystem->DestroyCustomRenderTargetForViewport( pEditorToolWorld->GetViewport() );
            m_pWorldManager->DestroyWorld( pEditorToolWorld );
        }
    }

    void EditorUI::QueueDestroyTool( EditorTool* pEditorTool )
    {
        EE_ASSERT( pEditorTool != nullptr );
        EE_ASSERT( m_pMapEditor != pEditorTool );
        m_editorToolDestructionRequests.emplace_back( pEditorTool );
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
        for ( auto& toolWindow : pEditorTool->m_toolWindows )
        {
            InlineString const sourceToolWindowName = EditorTool::GetToolWindowName( toolWindow.m_name.c_str(), sourceToolID );
            InlineString const destinationToolWindowName = EditorTool::GetToolWindowName( toolWindow.m_name.c_str(), destinationToolID );
            namePairsBuilder.AddEntry( sourceToolWindowName.c_str(), sourceToolWindowName.length() + 1 );
            namePairsBuilder.AddEntry( destinationToolWindowName.c_str(), destinationToolWindowName.length() + 1 );
        }

        // Build the same array with char* pointers at it is the input of DockBuilderCopyDockspace() (may change its signature?)
        ImVector<const char*> windowRemapPairs;
        namePairsBuilder.BuildPointerArray( windowRemapPairs );

        // Perform the cloning
        ImGui::DockBuilderCopyDockSpace( sourceToolID, destinationToolID, &windowRemapPairs );
        ImGui::DockBuilderFinish( destinationToolID );
    }

    bool EditorUI::SubmitToolMainWindow( UpdateContext const& context, EditorTool* pEditorTool, ImGuiID editorDockspaceID )
    {
        EE_ASSERT( pEditorTool != nullptr && pEditorTool->IsInitialized() );
        IM_ASSERT( editorDockspaceID != 0 );

        bool isToolStillOpen = true;
        bool* pIsToolOpen = ( pEditorTool == m_pMapEditor ) ? nullptr : &isToolStillOpen; // Prevent closing the map-editor editor tool

        // Top level editors can only be docked with each others
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
        ImGui::PushStyleColor( ImGuiCol_Text, isVisible ? ImGuiX::Style::s_colorAccent0 : ImGuiX::Style::s_colorText );
        ImGui::SetNextWindowSizeConstraints( ImVec2( 128, 128 ), ImVec2( FLT_MAX, FLT_MAX ) );
        ImGui::SetNextWindowSize( ImVec2( 1024, 768 ), ImGuiCond_FirstUseEver );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1.0f );
        ImGui::Begin( pEditorTool->m_windowName.c_str(), pIsToolOpen, windowFlags );
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

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
        EE_ASSERT( pEditorTool != nullptr && pEditorTool->IsInitialized() );

        auto pWorld = pEditorTool->GetEntityWorld();

        //-------------------------------------------------------------------------

        // This is the second Begin(), as SubmitToolWindow() has already done one
        // (Therefore only the p_open and flags of the first call to Begin() applies)
        ImGui::Begin( pEditorTool->m_windowName.c_str() );
        int32_t const beginCount = ImGui::GetCurrentWindow()->BeginCount;
        IM_ASSERT( beginCount == 2 );

        ImGuiID const dockspaceID = pEditorTool->m_currentDockspaceID;

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
        else if ( ImGui::DockBuilderGetNode( pEditorTool->m_currentDockspaceID ) == nullptr )
        {
            ImVec2 dockspaceSize = ImGui::GetContentRegionAvail();
            dockspaceSize.x = Math::Max( dockspaceSize.x, 1.0f );
            dockspaceSize.y = Math::Max( dockspaceSize.y, 1.0f );

            ImGui::DockBuilderAddNode( pEditorTool->m_currentDockspaceID, ImGuiDockNodeFlags_DockSpace );
            ImGui::DockBuilderSetNodeSize( pEditorTool->m_currentDockspaceID, dockspaceSize );
            if ( !pEditorTool->IsSingleWindowTool() )
            {
                pEditorTool->InitializeDockingLayout( dockspaceID, dockspaceSize );
            }
            ImGui::DockBuilderFinish( dockspaceID );
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
        pEditorTool->SharedUpdate( context, isVisible, isLastFocusedTool );
        pEditorTool->Update( context, isVisible, isLastFocusedTool );
        pEditorTool->m_isViewportFocused = false;
        pEditorTool->m_isViewportHovered = false;

        // Check Visibility
        //-------------------------------------------------------------------------

        if ( !isVisible )
        {
            if ( !pEditorTool->IsSingleWindowTool() )
            {
                // Keep alive document dockspace so windows that are docked into it but which visibility are not linked to the dockspace visibility won't get undocked.
                ImGui::DockSpace( dockspaceID, ImVec2( 0, 0 ), ImGuiDockNodeFlags_KeepAliveOnly, &pEditorTool->m_toolWindowClass);
            }

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

        // Submit the dockspace node and end window
        //-------------------------------------------------------------------------

        if ( pEditorTool->IsSingleWindowTool() )
        {
            EE_ASSERT( pEditorTool->m_toolWindows.size() == 1 );
            pEditorTool->m_toolWindows[0].m_drawFunction( context, isLastFocusedTool );
        }
        else
        {
            ImGui::DockSpace( dockspaceID, ImVec2( 0, 0 ), ImGuiDockNodeFlags_None, &pEditorTool->m_toolWindowClass );
        }
        ImGui::End();

        // Manage World state
        //-------------------------------------------------------------------------

        if ( pEditorTool->HasEntityWorld() && ( pEditorTool != m_pMapEditor || m_pGamePreviewer == nullptr ) )
        {
            pWorld->ResumeUpdates();
        }

        // Draw editor tool tool windows
        //-------------------------------------------------------------------------

        if ( !pEditorTool->IsSingleWindowTool() )
        {
            for ( auto& toolWindow : pEditorTool->m_toolWindows )
            {
                if ( !toolWindow.m_isOpen )
                {
                    continue;
                }

                InlineString const toolWindowName = EditorTool::GetToolWindowName( toolWindow.m_name.c_str(), pEditorTool->m_currentDockspaceID );

                // When multiple documents are open, floating tools only appear for focused one
                if ( !isLastFocusedTool )
                {
                    if ( ImGuiWindow* pWindow = ImGui::FindWindowByName( toolWindowName.c_str() ) )
                    {
                        if ( pWindow->DockNode == nullptr || ImGui::DockNodeGetRootNode( pWindow->DockNode )->ID != dockspaceID )
                        {
                            continue;
                        }
                    }
                }

                //-------------------------------------------------------------------------

                if ( toolWindow.m_isViewport )
                {
                    EE_ASSERT( pEditorTool->SupportsViewport() );
                    EE_ASSERT( pEditorTool->HasEntityWorld() && pWorld != nullptr );

                    EditorTool::ViewportInfo viewportInfo;
                    viewportInfo.m_pViewportRenderTargetTexture = (void*) &m_pRenderingSystem->GetRenderTargetTextureForViewport( pWorld->GetViewport() );
                    viewportInfo.m_retrievePickingID = [this, pWorld] ( Int2 const& pixelCoords ) { return m_pRenderingSystem->GetViewportPickingID( pWorld->GetViewport(), pixelCoords ); };

                    ImGuiWindowFlags const viewportWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus;
                    ImGui::SetNextWindowClass( &pEditorTool->m_toolWindowClass );

                    ImGui::SetNextWindowSizeConstraints( ImVec2( 128, 128 ), ImVec2( FLT_MAX, FLT_MAX ) );
                    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
                    bool const drawViewportWindow = ImGui::Begin( toolWindowName.c_str(), nullptr, viewportWindowFlags );
                    ImGui::PopStyleVar();

                    if ( drawViewportWindow )
                    {
                        pEditorTool->m_isViewportFocused = ImGui::IsWindowFocused();
                        pEditorTool->m_isViewportHovered = ImGui::IsWindowHovered();
                        pEditorTool->DrawViewport( context, viewportInfo );
                    }

                    ImGui::End();
                }
                else // Draw the tool window
                {
                    ImGuiWindowFlags toolWindowFlags = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus;
                    if ( toolWindow.m_disableScrolling )
                    {
                        toolWindowFlags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
                    }

                    ImGui::SetNextWindowClass( &pEditorTool->m_toolWindowClass );

                    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, toolWindow.HasUserSpecifiedWindowPadding() ? toolWindow.m_windowPadding : ImGui::GetStyle().WindowPadding );
                    bool const drawToolWindow = ImGui::Begin( toolWindowName.c_str(), &toolWindow.m_isOpen, toolWindowFlags );
                    ImGui::PopStyleVar();

                    if ( drawToolWindow )
                    {
                        bool const isToolWindowFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_DockHierarchy );
                        toolWindow.m_drawFunction( context, isToolWindowFocused );
                    }

                    ImGui::End();
                }
            }
        }

        // Camera
        //-------------------------------------------------------------------------

        if ( pEditorTool->HasEntityWorld() )
        {
            pEditorTool->SetCameraUpdateEnabled( pEditorTool->m_isViewportFocused && pEditorTool->m_isViewportHovered );
        }

        // Draw any open dialogs
        //-------------------------------------------------------------------------

        // If we dont already have an open modal dialog, draw any dialogs required
        if ( !m_hasOpenModalDialog )
        {
            m_hasOpenModalDialog = pEditorTool->m_dialogManager.DrawDialog( context );
        }
    }

    void EditorUI::CreateGamePreviewTool( UpdateContext const& context )
    {
        EE_ASSERT( m_pGamePreviewer == nullptr );
        auto& request = m_editorToolCreationRequests.emplace_back();
        request.m_type = ToolCreationRequest::GamePreview;
    }

    void EditorUI::DestroyGamePreviewTool( UpdateContext const& context )
    {
        EE_ASSERT( m_pGamePreviewer != nullptr );
        QueueDestroyTool( m_pGamePreviewer );
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
                ImGuiX::ColoredButton( Colors::Green, Colors::White, EE_ICON_PLUS"ADD" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::SmallBold );
                ImGuiX::ColoredButton( Colors::Green, Colors::White, EE_ICON_PLUS"ADD" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
                ImGuiX::ColoredButton( Colors::Green, Colors::White, EE_ICON_PLUS"ADD" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::MediumBold );
                ImGuiX::ColoredButton( Colors::Green, Colors::White, EE_ICON_PLUS"ADD" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                ImGuiX::ColoredButton( Colors::Green, Colors::White, EE_ICON_PLUS"ADD" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                ImGuiX::ColoredButton( Colors::Green, Colors::White, EE_ICON_PLUS"ADD" );
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

            ImGuiX::IconButton( EE_ICON_KANGAROO, "Test", Colors::PaleGreen, ImVec2( 100, 0 ) );

            ImGuiX::IconButton( EE_ICON_HOME, "Home", Colors::RoyalBlue, ImVec2( 100, 0 ) );

            ImGuiX::IconButton( EE_ICON_MOVIE_PLAY, "Play", Colors::LightPink, ImVec2( 100, 0 ) );

            ImGuiX::ColoredIconButton( Colors::Green, Colors::White, Colors::Yellow, EE_ICON_KANGAROO, "Test", ImVec2( 100, 0 ) );

            ImGuiX::FlatIconButton( EE_ICON_HOME, "Home", Colors::RoyalBlue, ImVec2( 100, 0 ) );

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

            ImGui::NewLine();
            ImGui::Separator();

            ImGuiX::DrawSpinner( "S1");
            ImGui::SameLine();
            ImGui::Text( "Spinner 1" );

            ImGuiX::DrawSpinner( "S2", Colors::Yellow, ImVec2( 100, 100 ), 10 );
            ImGui::SameLine();
            ImGui::Text( "Spinner 2" );

            ImGuiX::DrawSpinner( "S3", Colors::Blue, ImVec2( -1, -1 ), 5 );
            ImGui::SameLine();
            ImGui::Text( "Spinner 3" );
        }
        ImGui::End();
    }
}