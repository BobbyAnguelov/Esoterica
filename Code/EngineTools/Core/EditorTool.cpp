#include "EditorTool.h"
#include "DialogManager.h"
#include "EngineTools/Core/CommonToolTypes.h"
#include "EngineTools/Resource/Dialogs/EditorDialog_OpenResource.h"
#include "Engine/UpdateContext.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Render/Imgui/ImguiRenderer.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Entity/Debug/DebugView_EntityWorld.h"
#include "Engine/Render/RenderViewport.h"
#include "Engine/Render/Components/Component_Lights.h"
#include "Engine/Camera/Systems/WorldSystem_Camera.h"
#include "Engine/Camera/Components/Component_ToolsCamera.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Imgui/ImguiOrientationGuide.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/Serialization/TypeSerialization.h"
#include <EASTL/sort.h>

//-------------------------------------------------------------------------
// Editor Tool
//-------------------------------------------------------------------------

namespace EE
{
    static Render::Window* GetRenderWindow()
    {
        ImGuiViewportP* pImguiViewport = ImGui::GetIO().Ctx->CurrentViewport;
        Render::ImGui_RendererUserData* pRendererUserData = static_cast<Render::ImGui_RendererUserData*>( pImguiViewport->RendererUserData );

        // Is it a main viewport?
        Render::Window* pRenderWindow = nullptr;
        if ( pRendererUserData == nullptr )
        {
            Render::ImGui_BackendUserData* pBackendUserData = static_cast<Render::ImGui_BackendUserData*>( ImGui::GetIO().BackendRendererUserData );
            EE_ASSERT( pBackendUserData != nullptr );
            pRenderWindow = pBackendUserData->m_pRenderWindow;
        }
        else
        {
            pRenderWindow = &pRendererUserData->m_renderWindow;
        }

        return pRenderWindow;
    }

    EditorTool::EditorTool( ToolsContext const* pToolsContext, String const& displayName, EntityWorld* pWorld )
        : m_pToolsContext( pToolsContext )
        , m_windowName( displayName ) // Temp storage for display name since we cant call virtuals from CTOR
        , m_ID( Hash::FNV1a::GetHash32( displayName.c_str() ) )
        , m_pWorld( pWorld )
    {
        EE_ASSERT( m_pToolsContext != nullptr && m_pToolsContext->IsValid() );

        if ( HasEntityWorld() )
        {
            CreateCamera();
        }
    }

    EditorTool::~EditorTool()
    {
        for ( auto pToolWindow : m_toolWindows )
        {
            Render::ImGui_BackendUserData* pBackendUserData = static_cast<Render::ImGui_BackendUserData*>( ImGui::GetIO().BackendRendererUserData );
            EE_ASSERT( pBackendUserData );

            if ( pToolWindow->m_pViewport != nullptr )
            {
                m_pWorld->DestroyViewport( pToolWindow->m_pViewport );
                pToolWindow->m_pViewport = nullptr;
            }

            EE::Delete( pToolWindow );
        }
        m_toolWindows.clear();

        //-------------------------------------------------------------------------

        EE_ASSERT( m_requestedResources.empty() );
        EE_ASSERT( m_resourcesToBeReloaded.empty() );
        EE_ASSERT( m_loadingResources.empty() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        EE_ASSERT( !m_wasInitializedCalled );
    }

    //-------------------------------------------------------------------------

    void EditorTool::Initialize( UpdateContext const& context )
    {
        m_pResourceSystem = context.GetSystem<Resource::ResourceSystem>();
        SetDisplayName( m_windowName );

        // Entity World
        //-------------------------------------------------------------------------

        if ( HasEntityWorld() )
        {
            // Load the editor map
            if ( ShouldLoadDefaultEditorMap() )
            {
                SpawnEditorMapEntities();
            }
        }

        // Tool Windows
        //-------------------------------------------------------------------------

        if ( SupportsViewport() )
        {
            Render::Window* pRenderWindow = GetRenderWindow();
            EE_ASSERT( pRenderWindow != nullptr );

            Viewport* pViewport = m_pWorld->CreateViewport( pRenderWindow );
            static_cast<Render::RenderViewport*>( pViewport )->m_isStandalone = false;

            CreateViewportToolWindow( s_viewportWindowName, pViewport, [] ( UpdateContext const& context, bool isFocused ) {} );
        }

        // Initial Camera Position
        //-------------------------------------------------------------------------

        if ( SupportsViewport() )
        {
            static Transform const cameraDefaultTransform = Transform( Quaternion( EulerAngles( 10.0f, 0.0f, -180.0f ) ), Vector( 0, -5, 1.5f, 1 ) );
            SetCameraTransform( cameraDefaultTransform );
        }

        //-------------------------------------------------------------------------

        m_wasInitializedCalled = true;
    }

    void EditorTool::Shutdown( UpdateContext const& context )
    {
        EE_ASSERT( m_wasInitializedCalled );

        m_pResourceSystem = nullptr;
        m_wasInitializedCalled = false;
    }

    void EditorTool::PreDrawUpdate( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        for ( int32_t i = (int32_t) m_loadingResources.size() - 1; i >= 0; i-- )
        {
            if ( m_loadingResources[i]->IsLoaded() || m_loadingResources[i]->HasLoadingFailed() )
            {
                OnResourceLoadCompleted( m_loadingResources[i] );
                m_loadingResources.erase_unsorted( m_loadingResources.begin() + i );
            }
        }

        //-------------------------------------------------------------------------

        if ( isVisible && isFocused )
        {
            auto& IO = ImGui::GetIO();

            if ( SupportsUndoRedo() )
            {
                if ( IO.KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_Z ) )
                {
                    if ( CanUndo() )
                    {
                        Undo();
                    }
                }

                if ( IO.KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_Y ) )
                {
                    if ( CanRedo() )
                    {
                        Redo();
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( SupportsSaving() )
            {
                if ( IO.KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_S ) )
                {
                    if ( IsDirty() || AlwaysAllowSaving() )
                    {
                        Save();
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( IO.KeyAlt && IO.KeyShift && ImGui::IsKeyPressed( ImGuiKey_O ) )
            {
                if ( !m_pToolsContext->m_pDialogManager->HasActiveModalDialog() )
                {
                    m_pToolsContext->m_pDialogManager->StartModalDialog<Resource::OpenResourceDialog>( m_pToolsContext );
                }
            }

            //-------------------------------------------------------------------------

            if ( ImGui::IsKeyPressed( ImGuiKey_F ) )
            {
                FocusOnSelected();
            }
        }

        //-------------------------------------------------------------------------

        switch ( m_previewState )
        {
            case PreviewState::Requested:
            {
                if ( m_previewDelayTimer.Update() )
                {
                    for ( auto& resourcePtr : m_previewResources )
                    {
                        LoadResource( &resourcePtr );
                    }

                    m_previewState = PreviewState::WaitForResourceLoad;
                }
            }
            break;

            case PreviewState::WaitForResourceLoad:
            {
                bool loadingFullyComplete = true;
                for ( auto& resourcePtr : m_previewResources )
                {
                    if ( resourcePtr.HasLoadingFailed() )
                    {
                        StopPreview();
                        return;
                    }
                    else if ( !resourcePtr.IsLoaded() )
                    {
                        loadingFullyComplete = false;
                        break;
                    }
                }

                if ( loadingFullyComplete )
                {
                    m_previewState = PreviewState::SettingUp;
                    OnPreviewStarted();
                    m_previewState = PreviewState::Previewing;
                }
            }
            break;

            case PreviewState::None:
            case PreviewState::Previewing:
            {
                // Do nothing
            }
            break;


            case PreviewState::SettingUp:
            case PreviewState::TearingDown:
            default:
            {
                EE_UNREACHABLE_CODE();
            }
            break;
        }
    }

    void EditorTool::PostDrawUpdate( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        // Camera
        //-------------------------------------------------------------------------

        if ( HasEntityWorld() )
        {
            EE_ASSERT( m_pCamera != nullptr );

            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

            if ( m_pCamera->IsManipulatingView() )
            {
                m_pCamera->SetUpdateEnabled( m_isViewportFocused );
                ImGui::SetMouseCursor( ImGuiMouseCursor_None );
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
            }
            else
            {
                m_pCamera->SetUpdateEnabled( m_isViewportFocused && m_isViewportHovered );
            }
        }
    }

    void EditorTool::SetDisplayName( String name )
    {
        EE_ASSERT( !name.empty() );

        // Add in title bar icon
        //-------------------------------------------------------------------------

        if ( HasTitlebarIcon() )
        {
            m_windowName.sprintf( "%s %s", GetTitlebarIcon(), name.c_str() );
        }
        else
        {
            m_windowName = name;
        }

        //-------------------------------------------------------------------------

        if ( m_pWorld != nullptr )
        {
            m_pWorld->SetDebugName( name.c_str() );
        }
    }

    // Docking and Tool Windows
    //-------------------------------------------------------------------------

    EditorTool::ToolWindow* EditorTool::CreateToolWindow( String const& name, TFunction<void( UpdateContext const&, bool )> const& drawFunction, ImVec2 const& windowPadding, bool disableScrolling )
    {
        for ( auto const& pToolWindow : m_toolWindows )
        {
            EE_ASSERT( pToolWindow->m_name != name );
        }

        //-------------------------------------------------------------------------

        auto WindowSortPredicate = [] ( ToolWindow const* pLHS, ToolWindow const* pRHS )
        {
            // Viewports must be first!
            if ( pLHS->m_pViewport != nullptr && pRHS->m_pViewport == nullptr )
            {
                return true;
            }
            else if ( pRHS->m_pViewport != nullptr && pLHS->m_pViewport == nullptr )
            {
                return false;
            }

            return pLHS->m_name < pRHS->m_name;
        };

        auto pToolWindow = m_toolWindows.emplace_back( EE::New<ToolWindow>( name, drawFunction, windowPadding, disableScrolling ) );
        eastl::sort( m_toolWindows.begin(), m_toolWindows.end(), WindowSortPredicate );
        return pToolWindow;
    }

    EditorTool::ToolWindow* EditorTool::CreateViewportToolWindow( String const& name, Viewport* pViewport, TFunction<void( UpdateContext const&, bool )> const& drawFunction, ImVec2 const& windowPadding /*= ImVec2( -1, -1 )*/, bool disableScrolling /*= false */ )
    {
        EE_ASSERT( pViewport != nullptr );

        for ( auto const& pToolWindow : m_toolWindows )
        {
            EE_ASSERT( pToolWindow->m_name != name );
            EE_ASSERT( pToolWindow->m_pViewport == nullptr ); // Only one viewport for now
        }

        //-------------------------------------------------------------------------

        auto WindowSortPredicate = [] ( ToolWindow const* pLHS, ToolWindow const* pRHS )
        {
            // Viewports must be first!
            if ( pLHS->m_pViewport != nullptr && pRHS->m_pViewport == nullptr )
            {
                return true;
            }
            else if ( pRHS->m_pViewport != nullptr && pLHS->m_pViewport == nullptr )
            {
                return false;
            }

            return pLHS->m_name < pRHS->m_name;
        };

        auto pToolWindow = m_toolWindows.emplace_back( EE::New<ToolWindow>( name, drawFunction, windowPadding, disableScrolling ) );
        pToolWindow->m_pViewport = pViewport;
        eastl::sort( m_toolWindows.begin(), m_toolWindows.end(), WindowSortPredicate );
        return pToolWindow;
    }

    // Save
    //-------------------------------------------------------------------------

    bool EditorTool::Save()
    {
        EE_ASSERT( SupportsSaving() );

        if ( IsPreviewingOrWaitingForPreview() )
        {
            StopPreview();
        }

        String const filename = GetFilenameForSave();
        if ( filename.empty() )
        {
            return false;
        }

        if ( SaveData() )
        {
            ClearDirty();
            ImGuiX::NotifySuccess( "Saved: %s", filename.c_str() );
            return true;
        }
        else
        {
            ImGuiX::NotifyError( "Failed to save: %s", filename.c_str() );
            return false;
        }
    }

    // Undo/Redo
    //-------------------------------------------------------------------------

    void EditorTool::Undo()
    {
        EE_ASSERT( SupportsUndoRedo() );
        auto pActionToUndo = m_undoStack.GetActionForOperation( UndoStack::Operation::Undo );
        PreUndoRedo( UndoStack::Operation::Undo, pActionToUndo );
        auto pUndoneAction = m_undoStack.Undo();
        PostUndoRedo( UndoStack::Operation::Undo, pUndoneAction );
        MarkDirty();
    }

    void EditorTool::Redo()
    {
        EE_ASSERT( SupportsUndoRedo() );
        auto pActionToRedo = m_undoStack.GetActionForOperation( UndoStack::Operation::Redo );
        PreUndoRedo( UndoStack::Operation::Redo, pActionToRedo );
        auto pRedoneAction = m_undoStack.Redo();
        PostUndoRedo( UndoStack::Operation::Redo, pRedoneAction );
        MarkDirty();
    }

    void EditorTool::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        EE_ASSERT( SupportsUndoRedo() );
        m_isDirty = m_undoStack.CanUndo();
    }

    // Menu and Help
    //-------------------------------------------------------------------------

    void EditorTool::DrawMainMenu( UpdateContext const& context )
    {
        // New
        //-------------------------------------------------------------------------

        if ( SupportsNewFileCreation() )
        {
            if ( ImGui::MenuItem( EE_ICON_FILE_PLUS_OUTLINE"##New" ) )
            {
                CreateNewFile();
            }
            ImGuiX::ItemTooltip( "Create New" );
        }

        // Save
        //-------------------------------------------------------------------------

        if ( SupportsSaving() )
        {
            bool const canSave = AlwaysAllowSaving() || IsDirty();
            ImGui::BeginDisabled( !canSave );
            if ( ImGui::MenuItem( EE_ICON_CONTENT_SAVE"##Save" ) )
            {
                Save();
            }
            ImGuiX::ItemTooltip( "Save" );
            ImGui::EndDisabled();
        }

        // Undo/Redo
        //-------------------------------------------------------------------------

        if ( SupportsUndoRedo() )
        {
            ImGui::BeginDisabled( !CanUndo() );
            if ( ImGui::MenuItem( EE_ICON_UNDO_VARIANT"##Undo" ) )
            {
                Undo();
            }
            ImGuiX::ItemTooltip( "Undo" );
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( !CanRedo() );
            if ( ImGui::MenuItem( EE_ICON_REDO_VARIANT"##Redo" ) )
            {
                Redo();
            }
            ImGuiX::ItemTooltip( "Redo" );
            ImGui::EndDisabled();
        }

        // Draw custom Menu
        //-------------------------------------------------------------------------

        DrawMenu( context );

        // Window and Help
        //-------------------------------------------------------------------------

        if ( !IsSingleWindowTool() && !m_toolWindows.empty() )
        {
            if ( ImGui::BeginMenu( EE_ICON_WINDOW_RESTORE" Window" ) )
            {
                if ( ImGui::MenuItem( "Reset Layout" ) )
                {
                    m_requiresLayoutReset = true;
                }

                ImGui::Separator();

                //-------------------------------------------------------------------------

                for ( auto& pToolWindow : m_toolWindows )
                {
                    if ( pToolWindow->m_name == s_viewportWindowName )
                    {
                        continue;
                    }

                    if ( !pToolWindow->IsHidden() )
                    {
                        ImGui::MenuItem( pToolWindow->m_name.c_str(), nullptr, &pToolWindow->m_isOpen );
                    }
                }

                ImGui::EndMenu();
            }
        }

        if ( ImGui::BeginMenu( EE_ICON_HELP_CIRCLE_OUTLINE" Help" ) )
        {
            if ( ImGui::BeginTable( "HelpTable", 2 ) )
            {
                DrawHelpMenu();
                ImGui::EndTable();
            }

            ImGui::EndMenu();
        }
    }

    void EditorTool::DrawHelpTextRow( char const* pLabel, char const* pText ) const
    {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
            ImGui::Text( pLabel );
        }

        ImGui::TableNextColumn();
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );
            ImGui::Text( pText );
        }
    }

    void EditorTool::DrawHelpTextRowCustom( char const* pLabel, TFunction<void()> const& function ) const
    {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
            ImGui::Text( pLabel );
        }

        ImGui::TableNextColumn();
        {
            function();
        }
    }

    // Editor World
    //-------------------------------------------------------------------------

    DebugDrawContext EditorTool::GetDebugDrawContext()
    {
        return m_pWorld->GetDebugDrawSystem()->GetDebugDrawContext();
    }

    void EditorTool::SetWorldPaused( bool newPausedState )
    {
        if ( newPausedState )
        {
            m_pWorld->Pause();
        }
        else
        {
            m_pWorld->Unpause();
        }
    }

    void EditorTool::SetWorldTimeScale( float newTimeScale )
    {
        float worldTimeScale = Math::Clamp( newTimeScale, 0.1f, 3.5f );
        m_pWorld->SetTimeScale( worldTimeScale );
    }

    void EditorTool::ResetWorldTimeScale()
    {
        m_pWorld->SetTimeScale( 1.0f );
    }

    void EditorTool::AddEntityToWorld( Entity* pEntity )
    {
        EE_ASSERT( m_pWorld != nullptr );
        EE_ASSERT( pEntity != nullptr && !pEntity->IsAddedToMap() );
        m_pWorld->GetPersistentMap()->AddEntity( pEntity );
    }

    void EditorTool::RemoveEntityFromWorld( Entity* pEntity )
    {
        EE_ASSERT( m_pWorld != nullptr );
        EE_ASSERT( pEntity != nullptr && pEntity->GetMapID() == m_pWorld->GetPersistentMap()->GetID() );
        m_pWorld->GetPersistentMap()->RemoveEntity( pEntity->GetID() );
    }

    void EditorTool::DestroyEntityInWorld( Entity*& pEntity )
    {
        EE_ASSERT( m_pWorld != nullptr );
        EE_ASSERT( pEntity != nullptr && pEntity->GetMapID() == m_pWorld->GetPersistentMap()->GetID() );
        m_pWorld->GetPersistentMap()->DestroyEntity( pEntity->GetID() );
        pEntity = nullptr;
    }

    // Default Editor Map
    //-------------------------------------------------------------------------

    void EditorTool::SpawnEditorMapEntities()
    {
        EE_ASSERT( m_pEditorMapEntity == nullptr );

        // Floor
        //-------------------------------------------------------------------------

        m_pFloorMeshComponent = EE::New<Render::StaticMeshComponent>( StringID( "Floor" ) );
        m_pFloorMeshComponent->SetMesh( ResourceID( "data://editor/floor/floor.mesh" ) );

        m_pFloorCollisionComponent = EE::New<Physics::CollisionMeshComponent>( StringID( "Floor" ) );
        m_pFloorCollisionComponent->SetCollisionMesh( ResourceID( "data://editor/floor/floor.physmesh" ) );

        // Sky
        //-------------------------------------------------------------------------

        TBitFlags<Render::ViewLayer> viewLayers;
        viewLayers.SetFlag( Render::ViewLayer::ForwardShading );
        viewLayers.SetFlag( Render::ViewLayer::GlobalEnvironmentMap );

        m_pSkyBoxMeshComponent = EE::New<Render::StaticMeshComponent>( StringID( "SkyBox" ) );
        m_pSkyBoxMeshComponent->SetMesh( ResourceID( "data://editor/skydome/skydome.mesh" ) );
        m_pSkyBoxMeshComponent->SetViewLayers( viewLayers );
        m_pSkyBoxMeshComponent->SetLocalTransform( Transform( Quaternion::Identity, Vector( 0, 0, 0 ), 200 ) );

        // Light
        //-------------------------------------------------------------------------

        m_pLightComponent = EE::New<Render::DirectionalLightComponent>();
        m_pLightComponent->SetLocalTransform( Transform( Quaternion( EulerAngles( 135, 0, 60 ) ), Vector( 0, 0, 0 ) ) );

        // Entity
        //-------------------------------------------------------------------------

        m_pEditorMapEntity = EE::New<Entity>( StringID( "EditorMapEntity" ) );
        m_pEditorMapEntity->AddComponent( m_pFloorMeshComponent );
        m_pEditorMapEntity->AddComponent( m_pFloorCollisionComponent );
        m_pEditorMapEntity->AddComponent( m_pSkyBoxMeshComponent );
        m_pEditorMapEntity->AddComponent( m_pLightComponent );
        AddEntityToWorld( m_pEditorMapEntity );
    }

    void EditorTool::DespawnEditorMapEntities()
    {
        EE_ASSERT( m_pEditorMapEntity != nullptr );

        DestroyEntityInWorld( m_pEditorMapEntity );

        m_pEditorMapEntity = nullptr;
        m_pFloorMeshComponent = nullptr;
        m_pFloorCollisionComponent = nullptr;
        m_pSkyBoxMeshComponent = nullptr;
        m_pLightComponent = nullptr;
    }

    void EditorTool::DrawGrid()
    {
        constexpr float const lineLength = 20;
        constexpr float const halfLineLength = lineLength / 2.0f;

        auto drawingCtx = GetDebugDrawContext();
        for ( float i = -halfLineLength; i <= halfLineLength; i++ )
        {
            if ( i == 0 )
            {
                drawingCtx.DrawLine( Vector( -halfLineLength, i, 0.0f ), Vector( halfLineLength, i, 0.0f ), ImGuiX::Style::s_axisColorX, 1.0f, DebugDrawLayer::World );
                drawingCtx.DrawLine( Vector( i, -halfLineLength, 0.0f ), Vector( i, halfLineLength, 0.0f ), ImGuiX::Style::s_axisColorY, 1.0f, DebugDrawLayer::World );
            }
            else
            {
                drawingCtx.DrawLine( Vector( -halfLineLength, i, 0.0f ), Vector( halfLineLength, i, 0.0f ), Colors::LightGray, 1.0f, DebugDrawLayer::World );
                drawingCtx.DrawLine( Vector( i, -halfLineLength, 0.0f ), Vector( i, halfLineLength, 0.0f ), Colors::LightGray, 1.0f, DebugDrawLayer::World );
            }
        }
    }

    void EditorTool::SetFloorVisibility( bool showFloorMesh )
    {
        if ( m_pFloorMeshComponent != nullptr )
        {
            m_pFloorMeshComponent->SetVisible( showFloorMesh );
        }
    }

    bool EditorTool::GetFloorVisibility() const
    {
        if ( m_pFloorMeshComponent != nullptr )
        {
            return m_pFloorMeshComponent->IsVisible();
        }

        return false;
    }

    // Viewport
    //-------------------------------------------------------------------------

    bool EditorTool::BeginViewportToolbarGroup( char const* pGroupID, ImVec2 groupSize, ImVec2 const& padding )
    {
        ImGui::PushStyleColor( ImGuiCol_ChildBg, ImGuiX::Style::s_colorGray5 );
        ImGui::PushStyleColor( ImGuiCol_Header, ImGuiX::Style::s_colorGray5 );
        ImGui::PushStyleColor( ImGuiCol_FrameBg, ImGuiX::Style::s_colorGray5 );
        ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImGuiX::Style::s_colorGray4 );
        ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImGuiX::Style::s_colorGray3 );

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, padding );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 4.0f );

        // Adjust "use available" height to default toolbar height
        if ( groupSize.y <= 0 )
        {
            groupSize.y = ImGui::GetFrameHeight();
        }

        return ImGui::BeginChild( pGroupID, groupSize, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar );
    }

    void EditorTool::EndViewportToolbarGroup()
    {
        ImGui::EndChild();
        ImGui::PopStyleVar( 2 );
        ImGui::PopStyleColor( 5 );
    }

    void EditorTool::DrawViewportToolbarCombo( char const* pID, char const* pLabel, char const* pTooltip, TFunction<void()> const& function, float width )
    {
        ImGui::SetNextItemWidth( width );
        if ( ImGui::BeginCombo( pID, pLabel, ImGuiComboFlags_HeightLargest ) )
        {
            function();

            ImGui::EndCombo();
        }

        if ( pTooltip != nullptr )
        {
            ImGuiX::ItemTooltip( pTooltip );
        }
    }

    void EditorTool::DrawViewportToolbarComboIcon( char const* pID, char const* pIcon, char const* pTooltip, TFunction<void()> const& function )
    {
        ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
        float const width = ImGui::CalcTextSize( pIcon ).x + ( ImGui::GetStyle().FramePadding.x * 2 ) + 32;
        DrawViewportToolbarCombo( pID, pIcon, pTooltip, function, width );
    }

    void EditorTool::DrawViewportToolBar_ViewportVisualizationControls( UpdateContext const& context, Viewport* pViewport )
    {
        auto DrawMenu = [this, pViewport] ()
        {
            for ( auto pVisSettings : pViewport->GetAllViewportSettings() )
            {
                if ( ImGui::BeginMenu( pVisSettings->GetFriendlyName() ) )
                {
                    pVisSettings->DrawMenu( m_pWorld );
                    ImGui::EndMenu();
                }
            }
        };

        DrawViewportToolbarComboIcon( "##ViewportVisControls", EE_ICON_MONITOR_EYE, "Viewport Visualization Options", DrawMenu );
    }

    void EditorTool::DrawViewportToolBar_CameraControls( UpdateContext const& context, Viewport* pViewport )
    {
        auto DrawMenu = [this, context, pViewport] ()
        {
            ImGui::MenuItem( "Orbit (when available)", "alt" );
            ImGui::MenuItem( "Speed Control", "mouse 5 + wheel" );

            ImGui::SeparatorText( "Speed" );

            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Speed:" );

            ImGui::SameLine();

            float cameraSpeed = m_pCamera->GetMoveSpeed();
            ImGui::SetNextItemWidth( -1 );
            if ( ImGui::SliderFloat( "##CameraSpeed", &cameraSpeed, ToolsCameraComponent::s_minSpeed, ToolsCameraComponent::s_maxSpeed ) )
            {
                m_pCamera->SetMoveSpeed( cameraSpeed );
            }

            if ( ImGui::MenuItem( EE_ICON_RUN_FAST" Reset Camera Speed", "mouse 5 + middle mouse" ) )
            {
                m_pCamera->ResetMoveSpeed();
            }

            ImGui::SeparatorText( "Transform" );

            if ( ImGui::MenuItem( EE_ICON_CONTENT_COPY" Copy Camera Transform" ) )
            {
                Transform const& cameraWorldTransform = m_pCamera->GetWorldTransform();

                String cameraTransformStr;
                if ( TypeSystem::Conversion::ConvertNativeTypeToString( *m_pToolsContext->m_pTypeRegistry, TypeSystem::GetCoreTypeID( TypeSystem::CoreTypeID::Transform ), TypeSystem::TypeID(), &cameraWorldTransform, cameraTransformStr ) )
                {
                    ImGui::SetClipboardText( cameraTransformStr.c_str() );
                }
            }

            if ( ImGui::MenuItem( EE_ICON_CONTENT_PASTE" Paste Camera Transform" ) )
            {
                String const cameraTransformStr = ImGui::GetClipboardText();

                Transform cameraWorldTransform;
                if ( TypeSystem::Conversion::ConvertStringToNativeType( *m_pToolsContext->m_pTypeRegistry, TypeSystem::GetCoreTypeID( TypeSystem::CoreTypeID::Transform ), TypeSystem::TypeID(), cameraTransformStr, &cameraWorldTransform ) )
                {
                    m_pCamera->SetCameraWorldTransform( cameraWorldTransform );
                }
            }

            if ( ImGui::MenuItem( EE_ICON_EYE_REFRESH_OUTLINE" Reset View" ) )
            {
                ResetCameraView();
            }

            ExtendViewportToolBar_CameraControls( context, pViewport );
        };

        DrawViewportToolbarComboIcon( "##CameraOptions", EE_ICON_VIDEO, "Camera", DrawMenu );
    }

    void EditorTool::DrawViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport )
    {
        auto DrawMenu = [this, context, pViewport] ()
        {
            if ( !ExtendViewportToolBar_VisualizationControls( context, pViewport ) )
            {
                ImGui::Text( "No Options" );
            }
        };

        DrawViewportToolbarComboIcon( "##ToolVisControls", EE_ICON_EYE_OUTLINE, "Tool Visualization Options", DrawMenu );
    }

    void EditorTool::DrawViewportToolBar_TimeControls( UpdateContext const& context, Viewport* pViewport )
    {
        if ( SupportsWorldTimeControls() )
        {
            EntityWorldTimeControlsWidget timeControlsWidget( m_pWorld );
            if ( BeginViewportToolbarGroup( "TimeControls", ImVec2( 180, 0 ), ImVec2( 2, 1 ) ) )
            {
                timeControlsWidget.Draw( 180 );
            }
            EndViewportToolbarGroup();
        }
    }

    void EditorTool::DrawViewportToolbar( UpdateContext const& context, Viewport* pViewport )
    {
        DrawViewportToolBar_ViewportVisualizationControls( context, pViewport );

        ImGui::SameLine();

        DrawViewportToolBar_CameraControls( context, pViewport );

        ImGui::SameLine();

        DrawViewportToolBar_VisualizationControls( context, pViewport );

        ImGui::SameLine();

        DrawViewportToolBar_TimeControls( context, pViewport );

        ImGui::SameLine();

        ExtendViewportToolBar( context, pViewport );
    }

    void EditorTool::DrawViewportWindow( UpdateContext const& context, Viewport* pViewport )
    {
        EE_ASSERT( pViewport != nullptr );

        ImVec2 const contentRegionAvailable = ImGui::GetContentRegionAvail();
        if ( contentRegionAvailable.x < 1 || contentRegionAvailable.y < 1 )
        {
            return;
        }

        //-------------------------------------------------------------------------

        ImVec2 const viewportSize( contentRegionAvailable );
        ImVec2 const windowPos = ImGui::GetWindowPos();
        ImVec2 const windowStartCursorPos = ImGui::GetCursorPos();

        // Update viewport
        //-------------------------------------------------------------------------

        Render::Window* pRenderWindow = GetRenderWindow();
        EE_ASSERT( pRenderWindow != nullptr );
        Render::RenderViewport* pRenderViewport = static_cast<Render::RenderViewport*>( pViewport );
        EE_ASSERT( !pRenderViewport->m_isStandalone );
        pRenderViewport->UpdateRenderWindow( pRenderWindow );
        pRenderViewport->SetPickingEnabled( true );

        // Draw 3D scene
        //-------------------------------------------------------------------------

        Math::Rectangle const viewportRect( windowPos + windowStartCursorPos, viewportSize );
        pRenderViewport->Resize( viewportRect );

        if ( pRenderViewport->m_finalTexture )
        {
            Render::ImGui_BackendUserData* pBackendUserData = static_cast<Render::ImGui_BackendUserData*>( ImGui::GetIO().BackendRendererUserData );
            EE_ASSERT( pBackendUserData != nullptr );

            ImTextureID toolsRenderTarget = ImTextureID_Pack( Render::RHI::GetSamplerStateHandle( pBackendUserData->m_pRenderSystem->GetPointClampSampler() ), Render::RHI::GetTextureHandle( pRenderViewport->m_finalTexture, Render::RHI::DescriptorTypeFlags::Texture, 0 ) );
            ImGui::Image( toolsRenderTarget, viewportSize );
        }
        else
        {
            ImGui::Dummy( viewportSize );
        }

        // Update picking ID
        //-------------------------------------------------------------------------

        m_pickingData.clear();

        if ( m_isViewportHovered )
        {
            ImVec2 const mousePos = ImGui::GetMousePos();
            if ( mousePos.x != FLT_MAX && mousePos.y != FLT_MAX )
            {
                pRenderViewport->m_lastKnownPickingMousePosition = mousePos;
                pRenderViewport->m_lastKnownPickingPixelRadius = 50;
                m_pickingData = pRenderViewport->GetPickingData();
            }
        }

        // Drag and Drop
        //-------------------------------------------------------------------------

        if ( ImGui::BeginDragDropTarget() )
        {
            if ( ImGuiPayload const* pPayload = ImGui::AcceptDragDropPayload( DragAndDrop::s_filePayloadID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
            {
                if ( pPayload->IsDelivery() )
                {
                    InlineString payloadStr = (char*) pPayload->Data;

                    ResourceID const resourceID( payloadStr.c_str() );
                    if ( resourceID.IsValid() && m_pToolsContext->m_pFileRegistry->DoesFileExist( resourceID ) )
                    {
                        // Unproject mouse into viewport
                        Float2 const mouseViewportPos = ImGui::GetMousePos();
                        Vector const nearPointWS = pRenderViewport->ScreenSpaceToWorldSpaceNearPlane( mouseViewportPos );
                        Vector const farPointWS = pRenderViewport->ScreenSpaceToWorldSpaceFarPlane( mouseViewportPos );
                        Vector worldPosition = Vector::Zero;

                        // Raycast against the environmental collision
                        Physics::CastQuery query( Physics::ObjectCategory::Environment );
                        query.SetCollidesWith( Physics::ObjectCategory::Environment );
                        Physics::PhysicsWorld* pPhysicsWorld = m_pWorld->GetWorldSystem<Physics::PhysicsWorldSystem>()->GetPhysicsWorld();
                        pPhysicsWorld->LockRead();
                        if ( pPhysicsWorld->RayCast( nearPointWS, farPointWS, query ) )
                        {
                            worldPosition = query.GetFirstHitPosition();
                        }
                        else // Arbitrary position
                        {
                            worldPosition = nearPointWS + ( ( farPointWS - nearPointWS ).GetNormalized3() * 10.0f );
                        }
                        pPhysicsWorld->UnlockRead();

                        // Notify tool of a resource drag and drop operation
                        DropResourceInViewport( resourceID, worldPosition );
                    }
                }
            }
            else // Generic drag and drop notification
            {
                OnDragAndDropIntoViewport( pRenderViewport );
            }

            ImGui::EndDragDropTarget();
        }

        // UI
        //-------------------------------------------------------------------------

        bool const disableUIElements = contentRegionAvailable.x < 64 || contentRegionAvailable.y < 64;
        if ( !disableUIElements )
        {
            // Draw overlay elements
            //-------------------------------------------------------------------------

            ImGuiStyle const& style = ImGui::GetStyle();
            ImGui::SetCursorPos( windowStartCursorPos );
            DrawViewportUI( context, pRenderViewport, m_isViewportFocused );

            if ( HasViewportOrientationGuide() )
            {
                ImGuiX::OrientationGuide::Draw( windowPos + windowStartCursorPos + viewportSize - ImVec2( ImGuiX::OrientationGuide::GetWidth() + 4, ImGuiX::OrientationGuide::GetWidth() + 4 ), *pRenderViewport );
            }

            // Draw viewport toolbar
            //-------------------------------------------------------------------------

            ImGui::SetCursorPos( windowStartCursorPos + ImVec2( 0, style.WindowPadding.y ) );
            ImGui::Indent( style.WindowPadding.x );
            DrawViewportToolbar( context, pRenderViewport );
            ImGui::Unindent( style.WindowPadding.x );

            // Draw loading message
            //-------------------------------------------------------------------------

            if ( m_pWorld->IsBusyLoading() || m_pWorld->HasPendingMapChangeActions() )
            {
                ViewportNotificationWindows::DrawLoadingWindow();
            }
        }

        // Handle viewport inputs
        //-------------------------------------------------------------------------

        if ( m_isViewportHovered && !ImGui::IsAnyItemHovered() )
        {
            if ( ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseClicked( ImGuiMouseButton_Right ) || ImGui::IsMouseClicked( ImGuiMouseButton_Middle ) ) )
            {
                ImGui::SetWindowFocus();
                m_isViewportFocused = true;
            }

            HandleViewportInteractions();
        }

        // Handle being docked
        //-------------------------------------------------------------------------

        if ( auto pDockNode = ImGui::GetWindowDockNode() )
        {
            pDockNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
            pDockNode->LocalFlags |= ImGuiDockNodeFlags_NoDockingOverMe;
            pDockNode->LocalFlags |= ImGuiDockNodeFlags_NoDockingOverOther;
        }
    }

    // Camera
    //-------------------------------------------------------------------------

    void EditorTool::CreateCamera()
    {
        EE_ASSERT( m_pWorld != nullptr && m_pCamera == nullptr );
        auto pCameraSystem = m_pWorld->GetWorldSystem<CameraSystem>();

        m_pCamera = pCameraSystem->TrySpawnToolsCamera();
        EE_ASSERT( m_pCamera != nullptr );

        m_pCamera->SetDefaultMoveSpeed( 5.0f );
        m_pCamera->ResetMoveSpeed();

        pCameraSystem->EnableToolsCamera();
    }

    void EditorTool::ResetCameraView()
    {
        EE_ASSERT( m_pCamera != nullptr );
        m_pCamera->SetPositionAndLookAtTarget( Vector( 0, -5, 5 ), Vector( 0, 0, 0 ) );
    }

    void EditorTool::FocusCameraView( Entity* pTarget )
    {
        if ( !pTarget->IsSpatialEntity() )
        {
            ResetCameraView();
            return;
        }

        EE_ASSERT( m_pCamera != nullptr );
        AABB const worldBounds = pTarget->GetCombinedWorldBounds();
        m_pCamera->FocusOn( worldBounds );
    }

    void EditorTool::FocusCameraView( AABB const& worldBounds )
    {
        m_pCamera->FocusOn( worldBounds );
    }

    void EditorTool::FocusCameraView( Vector const& worldPoint, float viewDistance )
    {
        m_pCamera->FocusOn( worldPoint, viewDistance );
    }

    void EditorTool::SetCameraSpeed( float cameraSpeed )
    {
        EE_ASSERT( m_pCamera != nullptr );
        m_pCamera->SetMoveSpeed( cameraSpeed );
    }

    void EditorTool::SetCameraTransform( Transform const& cameraTransform )
    {
        EE_ASSERT( m_pCamera != nullptr );
        m_pCamera->SetCameraWorldTransform( cameraTransform );
    }

    Transform EditorTool::GetCameraTransform() const
    {
        EE_ASSERT( m_pCamera != nullptr );
        return m_pCamera->GetWorldTransform();
    }

    // Preview
    //-------------------------------------------------------------------------

    void EditorTool::RequestPreview()
    {
        if ( IsDirty() )
        {
            Save();
            m_previewDelayTimer.Start( 0.5f );
        }
        else
        {
            m_previewDelayTimer.Start( 0.1f );
        }

        EE_ASSERT( m_previewResources.empty() );

        // Create the resource ptrs for all required preview resources
        auto const previewResourceIDs = GetPreviewResourceIDs();
        for ( auto const& resourceID : previewResourceIDs )
        {
            EE_ASSERT( resourceID.IsValid() );
            m_previewResources.emplace_back( resourceID );
        }

        m_previewState = PreviewState::Requested;
    }

    void EditorTool::StopPreview()
    {
        EE_ASSERT( m_previewState != PreviewState::None );
        EE_ASSERT( m_previewState != PreviewState::SettingUp );
        EE_ASSERT( m_previewState != PreviewState::TearingDown );

        if ( m_previewState == PreviewState::Previewing )
        {
            m_previewState = PreviewState::TearingDown;
            OnPreviewStopped();
        }

        if ( m_previewState != PreviewState::Requested )
        {
            for ( auto& resourcePtr : m_previewResources )
            {
                UnloadResource( &resourcePtr );
                EE_ASSERT( resourcePtr.IsUnloaded() );
            }
        }

        m_previewResources.clear();
        m_previewState = PreviewState::None;
    }

    // Resources
    //-------------------------------------------------------------------------

    void EditorTool::LoadResource( Resource::ResourcePtr* pResourcePtr )
    {
        EE_ASSERT( m_pResourceSystem != nullptr );
        EE_ASSERT( pResourcePtr != nullptr && pResourcePtr->IsUnloaded() );
        EE_ASSERT( !VectorContains( m_requestedResources, pResourcePtr ) );
        m_requestedResources.emplace_back( pResourcePtr );
        m_pResourceSystem->LoadResource( *pResourcePtr, Resource::ResourceRequesterID::ToolRequest( m_ID ) );
        m_loadingResources.emplace_back( pResourcePtr );
    }

    void EditorTool::UnloadResource( Resource::ResourcePtr* pResourcePtr )
    {
        EE_ASSERT( m_pResourceSystem != nullptr );
        EE_ASSERT( pResourcePtr->WasRequested() );
        EE_ASSERT( VectorContains( m_requestedResources, pResourcePtr ) );
        OnResourceUnload( pResourcePtr );
        m_pResourceSystem->UnloadResource( *pResourcePtr, Resource::ResourceRequesterID::ToolRequest( m_ID ) );
        m_requestedResources.erase_first_unsorted( pResourcePtr );
        m_loadingResources.erase_first_unsorted( pResourcePtr );
    }

    // Hot-Reload
    //-------------------------------------------------------------------------

    void EditorTool::HotReload_UnloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded )
    {
        if ( IsPreviewingOrWaitingForPreview() )
        {
            for ( auto& resourcePtr : m_previewResources )
            {
                EE_ASSERT( resourcePtr.IsSet() );
                if ( resourcePtr.WasRequested() )
                {
                    if ( VectorContains( resourcesToBeReloaded, resourcePtr.GetResourceID() ) )
                    {
                        StopPreview();
                    }
                }
            }
        }

        //-------------------------------------------------------------------------

        // Do we have any resources that need reloading
        TInlineVector<Resource::ResourcePtr*, 10> resourcesThatNeedReloading;
        for ( auto& pLoadedResource : m_requestedResources )
        {
            if ( pLoadedResource->IsUnloaded() )
            {
                continue;
            }

            // Check resource and install dependencies to see if we need to unload it
            bool shouldUnload = VectorContains( resourcesToBeReloaded, pLoadedResource->GetResourceID() );
            if ( !shouldUnload )
            {
                for ( ResourceID const& installDependency : pLoadedResource->GetInstallDependencies() )
                {
                    if ( VectorContains( resourcesToBeReloaded, installDependency ) )
                    {
                        shouldUnload = true;
                        break;
                    }
                }
            }

            // Request unload and track the resource we need to reload
            if ( shouldUnload )
            {
                resourcesThatNeedReloading.emplace_back( pLoadedResource );
            }
        }

        //-------------------------------------------------------------------------

        // Do we need to hot-reload anything?
        if ( !resourcesThatNeedReloading.empty() )
        {
            // Unload resources
            for ( auto& pResourcePtr : resourcesThatNeedReloading )
            {
                // The 'OnHotReloadStarted' function might have unloaded some resources as part of its execution, so ignore those here
                if ( pResourcePtr->IsUnloaded() )
                {
                    continue;
                }

                OnResourceUnload( pResourcePtr );

                // Note: On resource unload might cause a resource that was requested for reload to be unloaded! So we need to validate that this resource ptr is still valid
                if ( VectorContains( m_requestedResources, pResourcePtr ) )
                {
                    m_pResourceSystem->UnloadResource( *pResourcePtr, Resource::ResourceRequesterID::ToolRequest( m_ID ) );
                    m_resourcesToBeReloaded.emplace_back( pResourcePtr );
                }
            }
        }
    }

    bool EditorTool::HotReload_ReloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded )
    {
        bool const hasResourcesToReload = !m_resourcesToBeReloaded.empty();
        if ( hasResourcesToReload )
        {
            // Load all unloaded resources
            for ( auto& pReloadedResource : m_resourcesToBeReloaded )
            {
                m_pResourceSystem->LoadResource( *pReloadedResource, Resource::ResourceRequesterID::ToolRequest( m_ID ) );
                VectorEmplaceBackUnique( m_loadingResources, pReloadedResource );
            }
            m_resourcesToBeReloaded.clear();
        }

        return true;
    }
}

//-------------------------------------------------------------------------
// Viewport Notification Windows
//-------------------------------------------------------------------------

namespace EE::ViewportNotificationWindows
{
    void DrawLoadingWindow()
    {
        auto const cursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos( ImVec2( 0, 0 ) );
        ImVec2 const contentRegion = ImGui::GetContentRegionAvail();

        //-------------------------------------------------------------------------

        constexpr static char const* pLoadingMessage = "Loading - Please Wait!";
        ImVec2 const loadingMessageSize = ImGui::CalcTextSize( pLoadingMessage );

        float const width = Math::Min( contentRegion.x / 2, loadingMessageSize.x ) + 64;
        float const height = 50;


        ImVec2 const windowPos = ImVec2( ( contentRegion.x - width ) / 2, ( contentRegion.y - height ) / 2 );
        ImVec2 const windowSize( width, height );

        //-------------------------------------------------------------------------

        ImGui::SetCursorPos( windowPos );
        ImGui::SetNextWindowBgAlpha( 1.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 2 );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 4 );
        ImGui::PushStyleColor( ImGuiCol_Border, Colors::Green );
        if ( ImGui::BeginChild( "Loading", windowSize, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing ) )
        {
            ImGuiX::DrawSpinner( pLoadingMessage, ImGuiX::Style::s_colorAccent0 );
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text( pLoadingMessage );
        }
        ImGui::EndChild();
        ImGui::PopStyleVar( 2 );
        ImGui::PopStyleColor();

        ImGui::SetCursorPos( cursorPos );
    }

    void DrawResourceCompilationFailedWindow( String const& compilationLog )
    {
        TInlineVector<InlineString, 10> lines;
        StringUtils::Split( InlineString( compilationLog.c_str() ), lines, "\n" );

        for ( int32_t i = int32_t( lines.size() ) - 1; i >= 0; i-- )
        {
            if ( lines[i].find( "Error" ) == String::npos )
            {
                lines.erase( lines.begin() + i );
            }
            else
            {
                size_t idx = lines[i].find_last_of( ']' );
                lines[i] = lines[i].substr( idx + 1, lines[i].length() - idx );
            }
        }

        //-------------------------------------------------------------------------

        auto const cursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos( ImVec2( 0, 0 ) );
        ImVec2 const contentRegion = ImGui::GetContentRegionAvail();

        float const height = ImGui::GetTextLineHeightWithSpacing() * lines.size() + 50;

        ImVec2 const styleWindowPadding = ImGui::GetStyle().WindowPadding;
        float const windowPadding = Math::Max( styleWindowPadding.x, styleWindowPadding.y );
        ImVec2 const windowPos = ImVec2( windowPadding * 2, contentRegion.y - height - windowPadding );
        ImVec2 const windowSize( 0, height );

        //-------------------------------------------------------------------------

        ImGui::SetCursorPos( windowPos );
        ImGui::SetNextWindowBgAlpha( 1.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 2 );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 4 );
        ImGui::PushStyleColor( ImGuiCol_Border, Colors::Red );
        if ( ImGui::BeginChild( "ResErrorWin", windowSize, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing ) )
        {
            ImGui::BeginGroup();
            {
                ImGuiX::ScopedFont sf( ImGuiX::FontType::Regular, 50, Colors::Red );
                ImGui::AlignTextToFramePadding();
                ImGui::Text( EE_ICON_ALERT );
            }
            ImGui::EndGroup();

            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::Red );
                ImGui::Text( "Compilation Failed!" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                for ( auto const& line : lines )
                {
                    ImGui::BulletText( line.c_str() );
                }
            }
            ImGui::EndGroup();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar( 2 );
        ImGui::PopStyleColor();

        ImGui::SetCursorPos( cursorPos );
    }
}

//-------------------------------------------------------------------------
// Data File Editor
//-------------------------------------------------------------------------

namespace EE
{
    DataFileEditor::DataFileEditor( ToolsContext const* pToolsContext, DataPath const& dataPath, EntityWorld* pWorld )
        : EditorTool( pToolsContext, String( String::CtorSprintf(), "%s##%s", dataPath.GetFilename().c_str(), dataPath.c_str() ), pWorld )
        , m_dataFilePath( dataPath )
    {
        EE_ASSERT( m_dataFilePath.IsValid() );
        m_ID = ImGui::GetID( m_dataFilePath.c_str() );
    }

    DataFileEditor::~DataFileEditor()
    {
        EE_ASSERT( !IsPreviewingOrWaitingForPreview() );
    }

    void DataFileEditor::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );

        // Create data file property grid
        //-------------------------------------------------------------------------

        auto const PreDescEdit = [this] ( PropertyEditInfo const& info )
        {
            EE_ASSERT( m_pActiveUndoableAction == nullptr );
            EE_ASSERT( IsDataFileLoaded() );
            BeginDataFileModification();
        };

        auto const PostDescEdit = [this] ( PropertyEditInfo const& info )
        {
            EE_ASSERT( m_pActiveUndoableAction != nullptr );
            EE_ASSERT( IsDataFileLoaded() );
            EndDataFileModification();
        };

        m_pDataFilePropertyGrid = EE::New<PropertyGrid>( m_pToolsContext );
        m_pDataFilePropertyGrid->SetControlBarVisible( true );
        m_preDataFileEditEventBindingID = m_pDataFilePropertyGrid->OnPreEdit().Bind( PreDescEdit );
        m_postDataFileEditEventBindingID = m_pDataFilePropertyGrid->OnPostEdit().Bind( PostDescEdit );

        // Load data file
        //-------------------------------------------------------------------------

        LoadDataFile();

        // Windows
        //-------------------------------------------------------------------------

        auto DrawDataFileWindow = [this] ( UpdateContext const& context, bool isFocused )
        {
            EE_ASSERT( IsDataFileManualEditingAllowed() );
            DrawDataFileEditorWindow( context, isFocused );
        };

        CreateToolWindow( s_dataFileWindowName, DrawDataFileWindow )->SetIsHiddenFunction( [this] () { return !IsDataFileManualEditingAllowed(); } );

        if ( !IsDataFileManualEditingAllowed() )
        {
            HideDataFileWindow();
        }
    }

    void DataFileEditor::Shutdown( UpdateContext const& context )
    {
        if ( IsPreviewingOrWaitingForPreview() )
        {
            StopPreview();
        }

        UnloadDataFile();

        if ( m_pDataFilePropertyGrid != nullptr )
        {
            m_pDataFilePropertyGrid->OnPreEdit().Unbind( m_preDataFileEditEventBindingID );
            m_pDataFilePropertyGrid->OnPostEdit().Unbind( m_postDataFileEditEventBindingID );
            EE::Delete( m_pDataFilePropertyGrid );
        }

        EditorTool::Shutdown( context );
    }

    void DataFileEditor::DrawMenu( UpdateContext const& context )
    {
        if ( ImGui::BeginMenu( EE_ICON_PACKAGE_VARIANT" Data File" ) )
        {
            if ( ImGui::MenuItem( EE_ICON_FILE_OUTLINE" Copy Resource Path" ) )
            {
                ImGui::SetClipboardText( m_dataFilePath.c_str() );
            }
            ImGuiX::ItemTooltip( "Copy Resource Path" );

            if ( ImGui::MenuItem( EE_ICON_FOLDER_OPEN_OUTLINE" Show in Resource Browser" ) )
            {
                m_pToolsContext->TryFindInResourceBrowser( m_dataFilePath.c_str() );
            }
            ImGuiX::ItemTooltip( "Copy Resource Path" );

            ImGui::EndMenu();
        }
    }

    void DataFileEditor::MarkDirty()
    {
        EditorTool::MarkDirty();
        m_pDataFilePropertyGrid->MarkDirty();
    }

    void DataFileEditor::ClearDirty()
    {
        m_pDataFilePropertyGrid->ClearDirty();
        EditorTool::ClearDirty();
    }

    void DataFileEditor::PreUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        EditorTool::PreUndoRedo( operation, pAction );

        if ( m_pDataFilePropertyGrid != nullptr )
        {
            m_pDataFilePropertyGrid->GetCurrentVisualState( m_dataFilePropertyGridVisualState );
        }
    }

    void DataFileEditor::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        EditorTool::PostUndoRedo( operation, pAction );

        if ( m_pDataFilePropertyGrid != nullptr )
        {
            m_pDataFilePropertyGrid->SetTypeToEdit( m_pDataFile, &m_dataFilePropertyGridVisualState );
            m_pDataFilePropertyGrid->MarkDirty();
        }
    }

    void DataFileEditor::HotReload_UnloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded )
    {
        EditorTool::HotReload_UnloadResources( usersToReload, resourcesToBeReloaded );

        if ( VectorContains( resourcesToBeReloaded, m_dataFilePath ) )
        {
            UnloadDataFile();
        }
    }

    bool DataFileEditor::HotReload_ReloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded )
    {
        EditorTool::HotReload_ReloadResources( usersToReload, resourcesToBeReloaded );

        if ( !IsDataFileLoaded() )
        {
            LoadDataFile();
            return IsDataFileLoaded();
        }

        return true;
    }

    String DataFileEditor::GetFilenameForSave() const
    {
        EE_ASSERT( SupportsSaving() );
        return GetFileSystemPath( m_dataFilePath ).c_str();
    }

    bool DataFileEditor::SaveData()
    {
        // Save data file
        EE_ASSERT( m_pDataFile != nullptr );
        EE_ASSERT( m_pDataFilePropertyGrid != nullptr );

        FileSystem::Path const filePath = GetFileSystemPath( m_dataFilePath );
        Log log;
        return IDataFile::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, log, filePath, m_pDataFile );
    }

    bool DataFileEditor::IsEditingResourceDescriptor() const
    {
        return m_pDataFile->GetTypeInfo()->IsDerivedFrom( Resource::ResourceDescriptor::GetStaticTypeID() );
    }

    void DataFileEditor::BeginDataFileModification()
    {
        if ( m_beginDataFileModificationCallCount == 0 )
        {
            auto pUndoableAction = EE::New<DataFileUndoableAction>( m_pToolsContext->m_pTypeRegistry, this );
            pUndoableAction->SerializeBeforeState();
            m_pActiveUndoableAction = pUndoableAction;
        }
        m_beginDataFileModificationCallCount++;
    }

    void DataFileEditor::EndDataFileModification()
    {
        EE_ASSERT( m_beginDataFileModificationCallCount > 0 );
        EE_ASSERT( m_pActiveUndoableAction != nullptr );

        m_beginDataFileModificationCallCount--;

        if ( m_beginDataFileModificationCallCount == 0 )
        {
            auto pUndoableAction = static_cast<DataFileUndoableAction*>( m_pActiveUndoableAction );
            pUndoableAction->SerializeAfterState();
            m_undoStack.RegisterAction( pUndoableAction );
            m_pActiveUndoableAction = nullptr;
            MarkDirty();
        }
    }

    void DataFileEditor::LoadDataFile()
    {
        EE_ASSERT( m_pDataFile == nullptr );
        EE_ASSERT( m_pDataFilePropertyGrid != nullptr );

        // Read data file from file
        //-------------------------------------------------------------------------

        Log log( LogCategory::Tools, "DataFileEditor" );

        FileSystem::Path const filePath = GetFileSystemPath( m_dataFilePath );
        IDataFile::ReadResult const result = IDataFile::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, log, filePath );

        m_pDataFile = result.m_pDataFile;

        if ( m_pDataFile != nullptr && IsDataFileManualEditingAllowed() )
        {
            PropertyGrid::VisualState const* pVisualState = ( m_dataFilePropertyGridVisualState.m_editedTypeID == m_pDataFile->GetTypeID() ) ? &m_dataFilePropertyGridVisualState : nullptr;
            m_pDataFilePropertyGrid->SetUserContext( this );
            m_pDataFilePropertyGrid->SetTypeToEdit( m_pDataFile, pVisualState );
        }

        if ( result.m_wasUpgraded )
        {
            MarkDirty();
        }

        // Notify tool that load is completed
        //-------------------------------------------------------------------------

        OnDataFileLoadCompleted();
    }

    void DataFileEditor::UnloadDataFile()
    {
        if ( m_pDataFile != nullptr )
        {
            m_pDataFilePropertyGrid->GetCurrentVisualState( m_dataFilePropertyGridVisualState );
            m_pDataFilePropertyGrid->SetTypeToEdit( nullptr );
            OnDataFileUnload();
            EE::Delete( m_pDataFile );
        }
    }

    void DataFileEditor::ShowDataFileWindow()
    {
        for ( auto& pToolWindow : GetToolWindows() )
        {
            if ( pToolWindow->GetName() == s_dataFileWindowName )
            {
                pToolWindow->SetOpen( true );
                return;
            }
        }
    }

    void DataFileEditor::HideDataFileWindow()
    {
        for ( auto& pToolWindow : GetToolWindows() )
        {
            if ( pToolWindow->GetName() == s_dataFileWindowName )
            {
                pToolWindow->SetOpen( false );
                return;
            }
        }
    }

    void DataFileEditor::DrawDataFileEditorWindow( UpdateContext const& context, bool isFocused )
    {
        EE_ASSERT( m_pDataFilePropertyGrid != nullptr );
        EE_ASSERT( IsDataFileManualEditingAllowed() );

        if ( m_pDataFile == nullptr )
        {
            ImGui::Text( "Failed to load data file!" );
        }
        else
        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
            ImGui::Text( "Data File: %s", m_dataFilePath.c_str() );

            ImGui::BeginDisabled( !m_pDataFilePropertyGrid->IsDirty() );
            if ( ImGuiX::ButtonColored( EE_ICON_CONTENT_SAVE" Save", Colors::ForestGreen, Colors::White, ImVec2( -1, 0 ) ) )
            {
                Save();
            }
            ImGui::EndDisabled();

            m_pDataFilePropertyGrid->UpdateAndDraw();
        }
    }

    //-------------------------------------------------------------------------

    DataFileUndoableAction::DataFileUndoableAction( TypeSystem::TypeRegistry const* pTypeRegistry, DataFileEditor* pEditor )
        : m_pTypeRegistry( pTypeRegistry )
        , m_pEditor( pEditor )
    {
        EE_ASSERT( m_pTypeRegistry != nullptr );
        EE_ASSERT( m_pEditor != nullptr );
        EE_ASSERT( m_pEditor->m_pDataFile != nullptr );
    }

    void DataFileUndoableAction::Undo()
    {
        EE_ASSERT( m_pTypeRegistry != nullptr && m_pEditor != nullptr );
        Serialization::ReadTypeFromString( *m_pTypeRegistry, m_log, m_valueBefore.c_str(), m_pEditor->m_pDataFile );
        m_log.ReflectToSystemLogAndClear( LogCategory::Resource, "DataFile Undo/Redo" );
        m_pEditor->MarkDirty();
    }

    void DataFileUndoableAction::Redo()
    {
        EE_ASSERT( m_pTypeRegistry != nullptr && m_pEditor != nullptr );
        Serialization::ReadTypeFromString( *m_pTypeRegistry, m_log, m_valueAfter.c_str(), m_pEditor->m_pDataFile );
        m_log.ReflectToSystemLogAndClear( LogCategory::Resource, "DataFile Undo/Redo" );
        m_pEditor->MarkDirty();
    }

    void DataFileUndoableAction::SerializeBeforeState()
    {
        EE_ASSERT( m_pTypeRegistry != nullptr && m_pEditor != nullptr );
        Serialization::WriteTypeToString( *m_pTypeRegistry, m_pEditor->m_pDataFile, m_valueBefore );
    }

    void DataFileUndoableAction::SerializeAfterState()
    {
        EE_ASSERT( m_pTypeRegistry != nullptr && m_pEditor != nullptr );
        Serialization::WriteTypeToString( *m_pTypeRegistry, m_pEditor->m_pDataFile, m_valueAfter );
        m_pEditor->MarkDirty();
    }
}

//-------------------------------------------------------------------------
// Editor Tool Factory
//-------------------------------------------------------------------------

namespace EE
{
    class GenericDataFileEditor final : public DataFileEditor
    {
        EE_EDITOR_TOOL( GenericDataFileEditor );

    public:

        using DataFileEditor::DataFileEditor;

        virtual bool IsSingleWindowTool() const override { return true; }
    };

    //-------------------------------------------------------------------------

    EditorTool* EditorToolFactory::TryCreateEditor( ToolsContext const* pToolsContext, DataPath const& path, TFunction< EntityWorld*( )> worldCreationFunction )
    {
        EE_ASSERT( path.IsValid() && path.IsFilePath() );

        // Check for a valid extension
        FileSystem::Extension ext = path.GetExtension();
        ext.make_lower();

        if ( !ResourceTypeID::IsValidResourceTypeIdentifierString( ext ) )
        {
            return nullptr;
        }

        // Don't try to open files that dont exist
        if ( !pToolsContext->m_pFileRegistry->DoesFileExist( path ) )
        {
            return nullptr;
        }

        //-------------------------------------------------------------------------

        ResourceTypeID const resourceTypeID( ext.c_str() );

        // Resources
        if ( pToolsContext->m_pTypeRegistry->IsRegisteredResourceType( resourceTypeID ) )
        {
            // Check if we have an explicit factory registered for this resource type
            //-------------------------------------------------------------------------

            EditorToolFactory* pCurrentFactory = s_pHead;
            while ( pCurrentFactory != nullptr )
            {
                if ( pCurrentFactory->IsResourceEditorFactory() && resourceTypeID == pCurrentFactory->GetSupportedResourceTypeID() )
                {
                    EntityWorld* pWorld = nullptr;
                    if ( pCurrentFactory->RequiresEntityWorld() )
                    {
                        pWorld = worldCreationFunction();
                    }

                    return pCurrentFactory->CreateEditorInternal( pToolsContext, path, pWorld );
                }

                pCurrentFactory = pCurrentFactory->GetNextItem();
            }

            // If we couldn't find an explicit factory then check if there is a descriptor for this type, if there is, then we can create a generic editor
            auto const resourceDescriptorTypes = pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( Resource::ResourceDescriptor::GetStaticTypeID(), false, false );
            for ( auto pResourceDescriptorTypeInfo : resourceDescriptorTypes )
            {
                auto pDefaultInstance = pResourceDescriptorTypeInfo->GetDefaultInstance<Resource::ResourceDescriptor>();
                if ( pDefaultInstance->GetCompiledResourceTypeID() == resourceTypeID )
                {
                    return EE::New<GenericDataFileEditor>( pToolsContext, path );
                }
            }
        }
        // Data Files
        else if ( pToolsContext->m_pTypeRegistry->IsRegisteredDataFileType( resourceTypeID.m_ID ) )
        {
            // Check if we have an explicit factory registered for this resource type
            //-------------------------------------------------------------------------

            EditorToolFactory* pCurrentFactory = s_pHead;
            while ( pCurrentFactory != nullptr )
            {
                if ( !pCurrentFactory->IsResourceEditorFactory() && resourceTypeID.m_ID == pCurrentFactory->GetSupportedDataFileExtension() )
                {
                    EntityWorld* pWorld = nullptr;
                    if ( pCurrentFactory->RequiresEntityWorld() )
                    {
                        pWorld = worldCreationFunction();
                    }

                    return pCurrentFactory->CreateEditorInternal( pToolsContext, path, pWorld );
                }

                pCurrentFactory = pCurrentFactory->GetNextItem();
            }

            return EE::New<GenericDataFileEditor>( pToolsContext, path );
        }

        //-------------------------------------------------------------------------

        return nullptr;
    }
}