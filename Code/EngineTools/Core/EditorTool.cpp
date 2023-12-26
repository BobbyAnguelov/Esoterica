#include "EditorTool.h"
#include "Engine/UpdateContext.h"
#include "EngineTools/ThirdParty/subprocess/subprocess.h"
#include "EngineTools/Resource/ResourceCompiler.h"
#include "EngineTools/Resource/ResourceToolDefines.h"
#include "Engine/Render/DebugViews/DebugView_Render.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Camera/DebugViews/DebugView_Camera.h"
#include "Engine/Camera/Components/Component_DebugCamera.h"
#include "Engine/Camera/Systems/EntitySystem_DebugCameraController.h"
#include "Engine/Camera/Systems/WorldSystem_CameraManager.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/Resource/ResourceSettings.h"
#include "Base/Serialization/JsonSerialization.h"
#include "Base/Serialization/TypeSerialization.h"
#include "Base/Types/ScopedValue.h"
#include <EASTL/sort.h>

//-------------------------------------------------------------------------

namespace EE
{
    EditorTool::EditorTool( ToolsContext const* pToolsContext, String const& displayName )
        : m_pToolsContext( pToolsContext )
        , m_windowName( displayName ) // Temp storage for display name since we cant call virtuals from CTOR
        , m_ID( Hash::GetHash32( displayName ) )
    {
        EE_ASSERT( m_pToolsContext != nullptr && m_pToolsContext->IsValid() );
    }

    EditorTool::EditorTool( ToolsContext const* pToolsContext, String const& displayName, EntityWorld* pWorld )
        : EditorTool( pToolsContext, displayName )
    {
        EE_ASSERT( pWorld != nullptr );
        m_pWorld = pWorld;
    }

    EditorTool::EditorTool( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
        : m_pToolsContext( pToolsContext )
        , m_windowName( String( String::CtorSprintf(), "%s##%s", resourceID.GetResourcePath().GetFileName().c_str(), resourceID.GetResourcePath().c_str() ) ) // Temp storage for display name since we cant call virtuals from CTOR
        , m_pWorld( pWorld )
        , m_descriptorID( resourceID )
        , m_descriptorPath( GetFileSystemPath( resourceID ) )
    {
        EE_ASSERT( m_pToolsContext != nullptr && m_pToolsContext->IsValid() );
        EE_ASSERT( resourceID.IsValid() );
        EE_ASSERT( m_pWorld != nullptr );

        m_ID = m_descriptorID.GetPathID();
    }

    EditorTool::~EditorTool()
    {
        EE_ASSERT( m_requestedResources.empty() );
        EE_ASSERT( m_resourcesToBeReloaded.empty() );
        EE_ASSERT( m_loadingResources.empty() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        EE_ASSERT( !m_isInitialized );
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
                m_pWorld->LoadMap( GetDefaultEditorMapPath() );
            }

            //-------------------------------------------------------------------------

            CreateCamera();
        }

        // Descriptor
        //-------------------------------------------------------------------------

        if ( IsResourceEditor() )
        {
            // Create descriptor property grid
            //-------------------------------------------------------------------------

            auto const PreDescEdit = [this] ( PropertyEditInfo const& info )
            {
                EE_ASSERT( m_pActiveUndoableAction == nullptr );
                EE_ASSERT( IsDescriptorLoaded() );
                BeginDescriptorModification();
            };

            auto const PostDescEdit = [this] ( PropertyEditInfo const& info )
            {
                EE_ASSERT( m_pActiveUndoableAction != nullptr );
                EE_ASSERT( IsDescriptorLoaded() );
                EndDescriptorModification();
            };

            m_pDescriptorPropertyGrid = EE::New<PropertyGrid>( m_pToolsContext );
            m_pDescriptorPropertyGrid->SetControlBarVisible( true );
            m_preEditEventBindingID = m_pDescriptorPropertyGrid->OnPreEdit().Bind( PreDescEdit );
            m_postEditEventBindingID = m_pDescriptorPropertyGrid->OnPostEdit().Bind( PostDescEdit );

            // Load descriptor
            //-------------------------------------------------------------------------

            LoadDescriptor();
        }

        // Tool Windows
        //-------------------------------------------------------------------------

        if ( IsResourceEditor() )
        {
            CreateToolWindow( s_descriptorWindowName, [this] ( UpdateContext const& context, bool isFocused ) { if ( IsDescriptorManualEditingAllowed() ) { DrawDescriptorEditorWindow( context, isFocused ); } } );
        }

        if ( SupportsViewport() )
        {
            CreateToolWindow( s_viewportWindowName, [this] ( UpdateContext const& context, bool isFocused ) { DrawDescriptorEditorWindow( context, isFocused ); } );
            m_toolWindows.back().m_isViewport = true;
        }

        if ( !IsDescriptorManualEditingAllowed() )
        {
            HideDescriptorWindow();
        }

        //-------------------------------------------------------------------------

        m_isInitialized = true;
    }

    void EditorTool::Shutdown( UpdateContext const& context )
    {
        EE_ASSERT( m_isInitialized );

        if ( IsResourceEditor() )
        {
            UnloadDescriptor();

            if ( m_pDescriptorPropertyGrid != nullptr )
            {
                m_pDescriptorPropertyGrid->OnPreEdit().Unbind( m_preEditEventBindingID );
                m_pDescriptorPropertyGrid->OnPostEdit().Unbind( m_postEditEventBindingID );
                EE::Delete( m_pDescriptorPropertyGrid );
            }
        }

        m_pResourceSystem = nullptr;
        m_isInitialized = false;
    }

    void EditorTool::SharedUpdate( UpdateContext const& context, bool isVisible, bool isFocused )
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

    void EditorTool::CreateToolWindow( String const& name, TFunction<void( UpdateContext const&, bool )> const& drawFunction, ImVec2 const& windowPadding, bool disableScrolling )
    {
        for ( auto const& toolWindow : m_toolWindows )
        {
            EE_ASSERT( toolWindow.m_name != name );
        }

        m_toolWindows.emplace_back( name, drawFunction, windowPadding, disableScrolling );

        eastl::sort( m_toolWindows.begin(), m_toolWindows.end(), [] ( ToolWindow const& lhs, ToolWindow const& rhs ) { return lhs.m_name < rhs.m_name; } );
    }

    void EditorTool::InitializeDockingLayout( ImGuiID const dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGui::DockBuilderRemoveNodeChildNodes( dockspaceID );

        if ( IsResourceEditor() )
        {
            ImGui::DockBuilderDockWindow( GetToolWindowName( s_descriptorWindowName ).c_str(), dockspaceID );
        }
    }

    String EditorTool::GetFilenameForSave() const
    {
        EE_ASSERT( SupportsSaving() );

        if ( IsResourceEditor() )
        {
            return GetFileSystemPath( m_descriptorID ).c_str();
        }

        return "";
    }

    // Save
    //-------------------------------------------------------------------------

    bool EditorTool::Save()
    {
        EE_ASSERT( SupportsSaving() );

        TScopedGuardValue<bool> const scopedValue( m_isSavingAllowedValidation, true );

        String const filename = GetFilenameForSave();

        if ( SaveData() )
        {
            ImGuiX::NotifySuccess( "Saved: %s", filename.c_str() );
            return true;
        }
        else
        {
            ImGuiX::NotifyError( "Failed to save: %s", filename.c_str() );
            return false;
        }
    }

    bool EditorTool::SaveData()
    {
        // DO NOT CALL THIS FUNCTION DIRECTLY
        EE_ASSERT( m_isSavingAllowedValidation );

        // Save Descriptor
        EE_ASSERT( m_descriptorPath.IsFilePath() );
        EE_ASSERT( m_pDescriptor != nullptr );
        EE_ASSERT( m_pDescriptorPropertyGrid != nullptr );

        // Serialize descriptor
        //-------------------------------------------------------------------------

        Serialization::JsonArchiveWriter descriptorWriter;
        auto pWriter = descriptorWriter.GetWriter();

        pWriter->StartObject();
        Serialization::WriteNativeTypeContents( *m_pToolsContext->m_pTypeRegistry, m_pDescriptor, *pWriter );
        WriteCustomDescriptorData( *m_pToolsContext->m_pTypeRegistry, *pWriter );
        pWriter->EndObject();

        // Save to file
        //-------------------------------------------------------------------------

        if ( descriptorWriter.WriteToFile( m_descriptorPath ) )
        {
            m_pDescriptorPropertyGrid->ClearDirty();
            m_isDirty = false;
            return true;
        }
        else
        {
            return false;
        }

        return true;
    }

    // Undo/Redo
    //-------------------------------------------------------------------------

    void EditorTool::Undo()
    {
        EE_ASSERT( SupportsUndoRedo() );
        PreUndoRedo( UndoStack::Operation::Undo );
        auto pAction = m_undoStack.Undo();
        PostUndoRedo( UndoStack::Operation::Undo, pAction );
        MarkDirty();
    }

    void EditorTool::Redo()
    {
        EE_ASSERT( SupportsUndoRedo() );
        PreUndoRedo( UndoStack::Operation::Redo );
        auto pAction = m_undoStack.Redo();
        PostUndoRedo( UndoStack::Operation::Redo, pAction );
        MarkDirty();
    }

    void EditorTool::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        EE_ASSERT( SupportsUndoRedo() );
        m_isDirty = m_undoStack.CanUndo();

        //-------------------------------------------------------------------------

        if ( IsResourceEditor() )
        {
            if ( m_pDescriptorPropertyGrid != nullptr )
            {
                m_pDescriptorPropertyGrid->MarkDirty();
            }
        }
    }

    // Menu and Help
    //-------------------------------------------------------------------------

    void EditorTool::DrawMainMenu( UpdateContext const& context )
    {
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

        // Descriptor
        //-------------------------------------------------------------------------

        if ( IsResourceEditor() )
        {
            if ( ImGui::BeginMenu( EE_ICON_PACKAGE_VARIANT" Descriptor" ) )
            {
                if ( ImGui::MenuItem( EE_ICON_FILE_OUTLINE" Copy Resource Path" ) )
                {
                    ImGui::SetClipboardText( m_descriptorID.c_str() );
                }
                ImGuiX::ItemTooltip( "Copy Resource Path" );

                if ( ImGui::MenuItem( EE_ICON_FOLDER_OPEN_OUTLINE" Show in Resource Browser" ) )
                {
                    m_pToolsContext->TryFindInResourceBrowser( m_descriptorID.c_str() );
                }
                ImGuiX::ItemTooltip( "Copy Resource Path" );

                ImGui::EndMenu();
            }
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
                for ( auto& toolWindow : m_toolWindows )
                {
                    if ( toolWindow.m_name == s_descriptorWindowName && !IsDescriptorManualEditingAllowed() )
                    {
                        continue;
                    }

                    ImGui::MenuItem( toolWindow.m_name.c_str(), nullptr, &toolWindow.m_isOpen );
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

    Drawing::DrawContext EditorTool::GetDrawingContext()
    {
        return m_pWorld->GetDebugDrawingSystem()->GetDrawingContext();
    }

    void EditorTool::SetWorldPaused( bool newPausedState )
    {
        bool const currentPausedState = m_pWorld->IsPaused();

        if ( currentPausedState == newPausedState )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( m_pWorld->IsPaused() )
        {
            m_pWorld->SetTimeScale( m_worldTimeScale );
        }
        else // Pause
        {
            m_worldTimeScale = m_pWorld->GetTimeScale();
            m_pWorld->SetTimeScale( -1.0f );
        }
    }

    void EditorTool::SetWorldTimeScale( float newTimeScale )
    {
        m_worldTimeScale = Math::Clamp( newTimeScale, 0.1f, 3.5f );
        if ( !m_pWorld->IsPaused() )
        {
            m_pWorld->SetTimeScale( m_worldTimeScale );
        }
    }

    void EditorTool::ResetWorldTimeScale()
    {
        m_worldTimeScale = 1.0f;
        if ( !m_pWorld->IsPaused() )
        {
            m_pWorld->SetTimeScale( 1.0f );
        }
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

    void EditorTool::SetFloorVisibility( bool showFloorMesh )
    {
        StringID const floorID( "Floor" );
        StringID const floorMeshID( "FloorMesh" );

        // HACK
        auto pFloorEntity = m_pWorld->GetFirstNonPersistentMap()->FindEntityByName( floorID );
        if ( pFloorEntity != nullptr )
        {
            if ( auto pMeshComponent = TryCast<Render::StaticMeshComponent>( pFloorEntity->FindComponentByName( floorMeshID ) ) )
            {
                pMeshComponent->SetVisible( showFloorMesh );
            }
        }
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
        if ( ImGui::BeginCombo( pID, pLabel, ImGuiComboFlags_HeightLarge ) )
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
        float const width = ImGui::CalcTextSize( pIcon ).x + ( ImGui::GetStyle().FramePadding.x * 2 ) + 28;
        DrawViewportToolbarCombo( pID, pIcon, pTooltip, function, width );
    }

    void EditorTool::DrawViewportToolBarCommon()
    {
        auto RenderingOptions = [this] ()
        {
            Render::RenderDebugView::DrawRenderVisualizationModesMenu( GetEntityWorld() );
        };

        DrawViewportToolbarComboIcon( "##RenderingOptions", EE_ICON_EYE, "Rendering Modes", RenderingOptions );

        //-------------------------------------------------------------------------

        ImGui::SameLine();

        //-------------------------------------------------------------------------

        auto CameraOptions = [this] ()
        {
            ImGui::SeparatorText( "Speed" );

            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Speed:" );

            ImGui::SameLine();

            float cameraSpeed = m_pCamera->GetMoveSpeed();
            ImGui::SetNextItemWidth( 125 );
            if ( ImGui::SliderFloat( "##CameraSpeed", &cameraSpeed, DebugCameraComponent::s_minSpeed, DebugCameraComponent::s_maxSpeed ) )
            {
                m_pCamera->SetMoveSpeed( cameraSpeed );
            }

            if ( ImGui::MenuItem( EE_ICON_RUN_FAST" Reset Camera Speed" ) )
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
                    m_pCamera->SetWorldTransform( cameraWorldTransform );
                }
            }

            if ( ImGui::MenuItem( EE_ICON_EYE_REFRESH_OUTLINE" Reset View" ) )
            {
                m_pCamera->ResetView();
            }
        };

        DrawViewportToolbarComboIcon( "##CameraOptions", EE_ICON_CCTV, "Camera", CameraOptions );
    }

    void EditorTool::DrawViewportToolBar_TimeControls()
    {
        if ( !HasViewportToolbarTimeControls() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        constexpr float const buttonWidth = 28;
        constexpr float const totalWidth = 180;
        constexpr float const sliderWidth = totalWidth - ( 3 * buttonWidth );

        if ( BeginViewportToolbarGroup( "TimeControls", ImVec2( totalWidth, 0 ), ImVec2( 2, 1 ) ) )
        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Small );

            // Play/Pause
            if ( m_pWorld->IsPaused() )
            {
                if ( ImGui::Button( EE_ICON_PLAY"##ResumeWorld", ImVec2( buttonWidth, -1 ) ) )
                {
                    SetWorldPaused( false );
                }
                ImGuiX::ItemTooltip( "Resume" );
            }
            else
            {
                if ( ImGui::Button( EE_ICON_PAUSE"##PauseWorld", ImVec2( buttonWidth, -1 ) ) )
                {
                    SetWorldPaused( true );
                }
                ImGuiX::ItemTooltip( "Pause" );
            }

            // Step
            ImGui::SameLine( 0, 0 );
            ImGui::BeginDisabled( !m_pWorld->IsPaused() );
            if ( ImGui::Button( EE_ICON_ARROW_RIGHT_BOLD"##StepFrame", ImVec2( buttonWidth, -1 ) ) )
            {
                m_pWorld->RequestTimeStep();
            }
            ImGuiX::ItemTooltip( "Step Frame" );
            ImGui::EndDisabled();

            // Slider
            ImGui::SameLine( 0, 0 );
            ImGui::SetNextItemWidth( sliderWidth );
            float newTimeScale = m_worldTimeScale;
            if ( ImGui::SliderFloat( "##TimeScale", &newTimeScale, 0.1f, 3.5f, "%.2f" ) )
            {
                SetWorldTimeScale( Math::Clamp( newTimeScale, 0.1f, 3.5f ) );
            }
            ImGuiX::ItemTooltip( "Time Scale" );

            // Reset
            ImGui::SameLine( 0, 0 );
            if ( ImGui::Button( EE_ICON_UPDATE"##ResetTimeScale", ImVec2( buttonWidth, -1 ) ) )
            {
                ResetWorldTimeScale();
            }
            ImGuiX::ItemTooltip( "Reset TimeScale" );
        }
        EndViewportToolbarGroup();
    }

    void EditorTool::DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        DrawViewportToolBarCommon();

        ImGui::SameLine();

        DrawViewportToolBar_TimeControls();
    }

    void EditorTool::DrawViewport( UpdateContext const& context, ViewportInfo const& viewportInfo )
    {
        EE_ASSERT( viewportInfo.m_pViewportRenderTargetTexture != nullptr && viewportInfo.m_retrievePickingID != nullptr );

        ImVec2 const viewportSize( Math::Max( ImGui::GetContentRegionAvail().x, 64.0f ), Math::Max( ImGui::GetContentRegionAvail().y, 64.0f ) );
        ImVec2 const windowPos = ImGui::GetWindowPos();

        // Switch focus based on mouse input
        //-------------------------------------------------------------------------

        if ( m_isViewportHovered )
        {
            if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseClicked( ImGuiMouseButton_Right ) || ImGui::IsMouseClicked( ImGuiMouseButton_Middle ) )
            {
                ImGui::SetWindowFocus();
                m_isViewportFocused = true;
            }
        }

        // Update engine viewport dimensions
        //-------------------------------------------------------------------------

        auto pWorld = GetEntityWorld();
        Math::Rectangle const viewportRect( Float2::Zero, viewportSize );
        Render::Viewport* pEngineViewport = pWorld->GetViewport();
        pEngineViewport->Resize( viewportRect );

        // Draw 3D scene
        //-------------------------------------------------------------------------

        ImVec2 const viewportImageCursorPos = ImGui::GetCursorPos();
        ImGui::Image( viewportInfo.m_pViewportRenderTargetTexture, viewportSize );

        if ( ImGui::BeginDragDropTarget() )
        {
            if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( Resource::DragAndDrop::s_payloadID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
            {
                if ( payload->IsDelivery() )
                {
                    InlineString payloadStr = (char*) payload->Data;

                    ResourceID const resourceID( payloadStr.c_str() );
                    if ( resourceID.IsValid() && m_pToolsContext->m_pResourceDatabase->DoesResourceExist( resourceID ) )
                    {
                        // Unproject mouse into viewport
                        Vector const nearPointWS = pEngineViewport->ScreenSpaceToWorldSpaceNearPlane( ImGui::GetMousePos() - windowPos );
                        Vector const farPointWS = pEngineViewport->ScreenSpaceToWorldSpaceFarPlane( ImGui::GetMousePos() - windowPos );
                        Vector worldPosition = Vector::Zero;

                        // Raycast against the environmental collision
                        Physics::RayCastResults results;
                        Physics::QueryRules rules;
                        rules.SetCollidesWith( Physics::CollisionCategory::Environment );
                        Physics::PhysicsWorld* pPhysicsWorld = m_pWorld->GetWorldSystem<Physics::PhysicsWorldSystem>()->GetWorld();
                        pPhysicsWorld->AcquireReadLock();
                        if ( pPhysicsWorld->RayCast( nearPointWS, farPointWS, rules, results ) )
                        {
                            worldPosition = results.GetFirstHitPosition();
                        }
                        else // Arbitrary position
                        {
                            worldPosition = nearPointWS + ( ( farPointWS - nearPointWS ).GetNormalized3() * 10.0f );
                        }
                        pPhysicsWorld->ReleaseReadLock();

                        // Notify tool of a resource drag and drop operation
                        DropResourceInViewport( resourceID, worldPosition );
                    }
                }
            }
            else // Generic drag and drop notification
            {
                OnDragAndDropIntoViewport( pEngineViewport );
            }

            ImGui::EndDragDropTarget();
        }

        // Draw loading message
        //-------------------------------------------------------------------------

        ImGui::SetCursorPos( ImVec2( ( viewportSize.x - 100 ) / 2, viewportSize.y / 2 ) );
        if ( m_pWorld->IsBusyLoading() || m_pWorld->HasPendingMapChangeActions() )
        {
            ImGuiX::DrawSpinner( "Loading - Please Wait!", ImGuiX::Style::s_colorAccent0 );
        }

        // Draw overlay elements
        //-------------------------------------------------------------------------

        ImGuiStyle const& style = ImGui::GetStyle();
        ImGui::SetCursorPos( style.WindowPadding );
        DrawViewportOverlayElements( context, pEngineViewport );

        if ( HasViewportOrientationGuide() )
        {
            ImGuiX::OrientationGuide::Draw( windowPos + viewportSize - ImVec2( ImGuiX::OrientationGuide::GetWidth() + 4, ImGuiX::OrientationGuide::GetWidth() + 4 ), *pEngineViewport );
        }

        // Draw viewport toolbar
        //-------------------------------------------------------------------------

        ImGui::SetCursorPos( ImGui::GetWindowContentRegionMin() + style.ItemSpacing );
        DrawViewportToolbar( context, pEngineViewport );

        // Handle picking
        //-------------------------------------------------------------------------

        if ( m_isViewportHovered && !ImGui::IsAnyItemHovered() )
        {
            if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
            {
                ImVec2 const mousePos = ImGui::GetMousePos();
                if ( mousePos.x != FLT_MAX && mousePos.y != FLT_MAX )
                {
                    ImVec2 const mousePosWithinViewportImage = ( mousePos - windowPos ) - viewportImageCursorPos;
                    Int2 const pixelCoords = Int2( Math::RoundToInt( mousePosWithinViewportImage.x ), Math::RoundToInt( mousePosWithinViewportImage.y ) );
                    Render::PickingID const pickingID = viewportInfo.m_retrievePickingID( pixelCoords );
                    OnMousePick( pickingID );
                }
            }
        }

        // Handle being docked
        //-------------------------------------------------------------------------

        if ( auto pDockNode = ImGui::GetWindowDockNode() )
        {
            pDockNode->LocalFlags = 0;
            pDockNode->LocalFlags |= ImGuiDockNodeFlags_NoDockingOverMe;
            pDockNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
        }
    }

    // Camera
    //-------------------------------------------------------------------------

    void EditorTool::CreateCamera()
    {
        EE_ASSERT( m_pWorld != nullptr && m_pCamera == nullptr );
        auto pCameraManager = m_pWorld->GetWorldSystem<CameraManager>();
        m_pCamera = pCameraManager->TrySpawnDebugCamera();
        m_pCamera->SetDefaultMoveSpeed( 5.0f );
        m_pCamera->ResetMoveSpeed();

        pCameraManager->EnableDebugCamera();
    }

    void EditorTool::SetCameraUpdateEnabled( bool isEnabled )
    {
        m_pCamera->SetEnabled( isEnabled );
    }

    void EditorTool::ResetCameraView()
    {
        EE_ASSERT( m_pCamera != nullptr );
        m_pCamera->ResetView();
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

    void EditorTool::SetCameraSpeed( float cameraSpeed )
    {
        EE_ASSERT( m_pCamera != nullptr );
        m_pCamera->SetMoveSpeed( cameraSpeed );
    }

    void EditorTool::SetCameraTransform( Transform const& cameraTransform )
    {
        EE_ASSERT( m_pCamera != nullptr );
        m_pCamera->SetWorldTransform( cameraTransform );
    }

    Transform EditorTool::GetCameraTransform() const
    {
        EE_ASSERT( m_pCamera != nullptr );
        return m_pCamera->GetWorldTransform();
    }

    // Resources
    //-------------------------------------------------------------------------

    void EditorTool::LoadResource( Resource::ResourcePtr* pResourcePtr )
    {
        EE_ASSERT( m_pResourceSystem != nullptr );
        EE_ASSERT( pResourcePtr != nullptr && pResourcePtr->IsUnloaded() );
        EE_ASSERT( !VectorContains( m_requestedResources, pResourcePtr ) );
        m_requestedResources.emplace_back( pResourcePtr );
        m_pResourceSystem->LoadResource( *pResourcePtr, Resource::ResourceRequesterID( Resource::ResourceRequesterID::s_toolsRequestID ) );
        m_loadingResources.emplace_back( pResourcePtr );
    }

    void EditorTool::UnloadResource( Resource::ResourcePtr* pResourcePtr )
    {
        EE_ASSERT( m_pResourceSystem != nullptr );
        EE_ASSERT( pResourcePtr->WasRequested() );
        EE_ASSERT( VectorContains( m_requestedResources, pResourcePtr ) );
        OnResourceUnload( pResourcePtr );
        m_pResourceSystem->UnloadResource( *pResourcePtr, Resource::ResourceRequesterID( Resource::ResourceRequesterID::s_toolsRequestID ) );
        m_requestedResources.erase_first_unsorted( pResourcePtr );
        m_loadingResources.erase_first_unsorted( pResourcePtr );
    }

    bool EditorTool::RequestImmediateResourceCompilation( ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );

        // Trigger external recompile request and wait for it
        //-------------------------------------------------------------------------

        char const* processCommandLineArgs[5] = { m_pResourceSystem->GetSettings().m_resourceCompilerExecutablePath.c_str(), "-compile", resourceID.c_str(), nullptr, nullptr };

        subprocess_s subProcess;
        int32_t result = subprocess_create( processCommandLineArgs, subprocess_option_combined_stdout_stderr | subprocess_option_inherit_environment | subprocess_option_no_window, &subProcess );
        if ( result != 0 )
        {
            return false;
        }

        int32_t exitCode = -1;
        result = subprocess_join( &subProcess, &exitCode );
        if ( result != 0 )
        {
            subprocess_destroy( &subProcess );
            return false;
        }

        subprocess_destroy( &subProcess );

        // Notify resource server to reload the resource
        //-------------------------------------------------------------------------

        if ( exitCode >= (int32_t) Resource::CompilationResult::SuccessUpToDate )
        {
            m_pResourceSystem->RequestResourceHotReload( resourceID );
        }

        //-------------------------------------------------------------------------

        return true;
    }

    // Descriptor
    //-------------------------------------------------------------------------

    void EditorTool::BeginDescriptorModification()
    {
        if ( m_beginModificationCallCount == 0 )
        {
            auto pUndoableAction = EE::New<ResourceDescriptorUndoableAction>( m_pToolsContext->m_pTypeRegistry, this );
            pUndoableAction->SerializeBeforeState();
            m_pActiveUndoableAction = pUndoableAction;
        }
        m_beginModificationCallCount++;
    }

    void EditorTool::EndDescriptorModification()
    {
        EE_ASSERT( m_beginModificationCallCount > 0 );
        EE_ASSERT( m_pActiveUndoableAction != nullptr );

        m_beginModificationCallCount--;

        if ( m_beginModificationCallCount == 0 )
        {
            auto pUndoableAction = static_cast<ResourceDescriptorUndoableAction*>( m_pActiveUndoableAction );
            pUndoableAction->SerializeAfterState();
            m_undoStack.RegisterAction( pUndoableAction );
            m_pActiveUndoableAction = nullptr;
            m_isDirty = true;
        }
    }

    void EditorTool::LoadDescriptor()
    {
        EE_ASSERT( m_pDescriptor == nullptr );
        EE_ASSERT( m_pDescriptorPropertyGrid != nullptr );

        // Read descriptor from file
        //-------------------------------------------------------------------------

        Serialization::JsonArchiveReader archive;
        if ( !archive.ReadFromFile( m_descriptorPath ) )
        {
            EE_LOG_ERROR( "Tools", "Resource EditorTool", "Failed to read resource descriptor file: %s", m_descriptorPath.c_str() );
            return;
        }

        auto const& document = archive.GetDocument();
        m_pDescriptor = Serialization::TryCreateAndReadNativeType<Resource::ResourceDescriptor>( *m_pToolsContext->m_pTypeRegistry, document );
        m_pDescriptorPropertyGrid->SetTypeToEdit( m_pDescriptor );

        // Read any additional custom data from descriptor
        //-------------------------------------------------------------------------

        ReadCustomDescriptorData( *m_pToolsContext->m_pTypeRegistry, archive.GetDocument() );

        // Notify tool that load is completed
        //-------------------------------------------------------------------------

        OnDescriptorLoadCompleted();
    }

    void EditorTool::UnloadDescriptor()
    {
        if ( m_pDescriptor != nullptr )
        {
            OnDescriptorUnload();
            EE::Delete( m_pDescriptor );
        }
    }

    void EditorTool::ShowDescriptorWindow()
    {
        for ( auto& toolWindow : m_toolWindows )
        {
            if ( toolWindow.m_name == s_descriptorWindowName )
            {
                toolWindow.m_isOpen = true;
                return;
            }
        }
    }

    void EditorTool::HideDescriptorWindow()
    {
        for ( auto& toolWindow : m_toolWindows )
        {
            if ( toolWindow.m_name == s_descriptorWindowName )
            {
                toolWindow.m_isOpen = false;
                return;
            }
        }
    }

    void EditorTool::DrawDescriptorEditorWindow( UpdateContext const& context, bool isFocused )
    {
        EE_ASSERT( m_pDescriptorPropertyGrid != nullptr );
        EE_ASSERT( IsDescriptorManualEditingAllowed() );

        if ( m_pDescriptor == nullptr )
        {
            ImGui::Text( "Failed to load descriptor!" );
        }
        else
        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
            ImGui::Text( "Descriptor: %s", m_descriptorID.c_str() );

            ImGui::BeginDisabled( !m_pDescriptorPropertyGrid->IsDirty() );
            if ( ImGuiX::ColoredButton( Colors::ForestGreen, Colors::White, EE_ICON_CONTENT_SAVE" Save", ImVec2( -1, 0 ) ) )
            {
                Save();
            }
            ImGui::EndDisabled();

            m_pDescriptorPropertyGrid->DrawGrid();
        }
    }

    // Hot-Reload
    //-------------------------------------------------------------------------

    void EditorTool::HotReload_UnloadResources( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded )
    {
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
                m_pResourceSystem->UnloadResource( *pResourcePtr, Resource::ResourceRequesterID( Resource::ResourceRequesterID::s_toolsRequestID ) );
                m_resourcesToBeReloaded.emplace_back( pResourcePtr );
            }
        }

        //-------------------------------------------------------------------------

        // Does the descriptor need reloading?
        if ( IsResourceEditor() )
        {
            if ( VectorContains( resourcesToBeReloaded, m_descriptorID ) )
            {
                UnloadDescriptor();
            }
        }
    }

    void EditorTool::HotReload_ReloadResources()
    {
        bool const hasResourcesToReload = !m_resourcesToBeReloaded.empty();
        if ( hasResourcesToReload )
        {
            // Load all unloaded resources
            for ( auto& pReloadedResource : m_resourcesToBeReloaded )
            {
                m_pResourceSystem->LoadResource( *pReloadedResource, Resource::ResourceRequesterID( Resource::ResourceRequesterID::s_toolsRequestID ) );
                VectorEmplaceBackUnique( m_loadingResources, pReloadedResource );
            }
            m_resourcesToBeReloaded.clear();
        }

        if ( IsResourceEditor() && !IsDescriptorLoaded() )
        {
            LoadDescriptor();
        }
    }
}

//-------------------------------------------------------------------------

namespace EE
{
    EE_GLOBAL_REGISTRY( ResourceEditorFactory );

    //-------------------------------------------------------------------------

    class GenericResourceEditor : public EditorTool
    {
        EE_EDITOR_TOOL( GenericResourceEditor );

    public:

        using EditorTool::EditorTool;
    };

    //-------------------------------------------------------------------------

    bool ResourceEditorFactory::CanCreateEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );
        auto resourceTypeID = resourceID.GetResourceTypeID();
        EE_ASSERT( resourceTypeID.IsValid() );

        // Check if we have a custom editor for this type
        //-------------------------------------------------------------------------

        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( resourceTypeID == pCurrentFactory->GetSupportedResourceTypeID() )
            {
                return true;
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        // Check if a descriptor type
        //-------------------------------------------------------------------------

        auto const resourceDescriptorTypes = pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( Resource::ResourceDescriptor::GetStaticTypeID(), false, false );
        for ( auto pResourceDescriptorTypeInfo : resourceDescriptorTypes )
        {
            auto pDefaultInstance = Cast<Resource::ResourceDescriptor>( pResourceDescriptorTypeInfo->GetDefaultInstance() );
            if ( pDefaultInstance->GetCompiledResourceTypeID() == resourceID.GetResourceTypeID() )
            {
                return true;
            }
        }

        //-------------------------------------------------------------------------

        return false;
    }

    EditorTool* ResourceEditorFactory::CreateEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );
        auto resourceTypeID = resourceID.GetResourceTypeID();
        EE_ASSERT( resourceTypeID.IsValid() );

        // Check if we have a custom editor for this type
        //-------------------------------------------------------------------------

        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( resourceTypeID == pCurrentFactory->GetSupportedResourceTypeID() )
            {
                return pCurrentFactory->CreateEditorInternal( pToolsContext, pWorld, resourceID );
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        // Create generic descriptor editor
        //-------------------------------------------------------------------------

        auto const resourceDescriptorTypes = pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( Resource::ResourceDescriptor::GetStaticTypeID(), false, false );
        for ( auto pResourceDescriptorTypeInfo : resourceDescriptorTypes )
        {
            auto pDefaultInstance = Cast<Resource::ResourceDescriptor>( pResourceDescriptorTypeInfo->GetDefaultInstance() );
            if ( pDefaultInstance->GetCompiledResourceTypeID() == resourceID.GetResourceTypeID() )
            {
                return EE::New<GenericResourceEditor>( pToolsContext, pWorld, resourceID );
            }
        }

        EE_UNREACHABLE_CODE();
        return nullptr;
    }
}

//-------------------------------------------------------------------------

namespace EE
{
    ResourceDescriptorUndoableAction::ResourceDescriptorUndoableAction( TypeSystem::TypeRegistry const* pTypeRegistry, EditorTool* pEditorTool )
        : m_pTypeRegistry( pTypeRegistry )
        , m_pEditorTool( pEditorTool )
    {
        EE_ASSERT( m_pTypeRegistry != nullptr );
        EE_ASSERT( m_pEditorTool != nullptr );
        EE_ASSERT( m_pEditorTool->m_pDescriptor != nullptr );
    }

    void ResourceDescriptorUndoableAction::Undo()
    {
        EE_ASSERT( m_pTypeRegistry != nullptr && m_pEditorTool != nullptr );

        Serialization::JsonArchiveReader typeReader;

        typeReader.ReadFromString( m_valueBefore.c_str() );
        auto const& document = typeReader.GetDocument();
        Serialization::ReadNativeType( *m_pTypeRegistry, document, m_pEditorTool->m_pDescriptor );
        m_pEditorTool->ReadCustomDescriptorData( *m_pTypeRegistry, document );
        m_pEditorTool->MarkDirty();
    }

    void ResourceDescriptorUndoableAction::Redo()
    {
        EE_ASSERT( m_pTypeRegistry != nullptr && m_pEditorTool != nullptr );

        Serialization::JsonArchiveReader typeReader;

        typeReader.ReadFromString( m_valueAfter.c_str() );
        auto const& document = typeReader.GetDocument();
        Serialization::ReadNativeType( *m_pTypeRegistry, document, m_pEditorTool->m_pDescriptor );
        m_pEditorTool->ReadCustomDescriptorData( *m_pTypeRegistry, document );
        m_pEditorTool->MarkDirty();
    }

    void ResourceDescriptorUndoableAction::SerializeBeforeState()
    {
        EE_ASSERT( m_pTypeRegistry != nullptr && m_pEditorTool != nullptr );

        Serialization::JsonArchiveWriter writer;

        auto pWriter = writer.GetWriter();
        pWriter->StartObject();
        Serialization::WriteNativeTypeContents( *m_pTypeRegistry, m_pEditorTool->m_pDescriptor, *pWriter );
        m_pEditorTool->WriteCustomDescriptorData( *m_pTypeRegistry, *pWriter );
        pWriter->EndObject();

        m_valueBefore.resize( writer.GetStringBuffer().GetSize() );
        memcpy( m_valueBefore.data(), writer.GetStringBuffer().GetString(), writer.GetStringBuffer().GetSize() );
    }

    void ResourceDescriptorUndoableAction::SerializeAfterState()
    {
        EE_ASSERT( m_pTypeRegistry != nullptr && m_pEditorTool != nullptr );

        Serialization::JsonArchiveWriter writer;

        auto pWriter = writer.GetWriter();
        pWriter->StartObject();
        Serialization::WriteNativeTypeContents( *m_pTypeRegistry, m_pEditorTool->m_pDescriptor, *pWriter );
        m_pEditorTool->WriteCustomDescriptorData( *m_pTypeRegistry, *pWriter );
        pWriter->EndObject();

        m_valueAfter.resize( writer.GetStringBuffer().GetSize() );
        memcpy( m_valueAfter.data(), writer.GetStringBuffer().GetString(), writer.GetStringBuffer().GetSize() );

        m_pEditorTool->MarkDirty();
    }
}