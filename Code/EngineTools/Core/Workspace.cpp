#include "Workspace.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/Resource/ResourceCompiler.h"
#include "EngineTools/ThirdParty/subprocess/subprocess.h"
#include "Engine/Render/DebugViews/DebugView_Render.h"
#include "Engine/Camera/DebugViews/DebugView_Camera.h"
#include "Engine/Camera/Components/Component_DebugCamera.h"
#include "Engine/Camera/Systems/EntitySystem_DebugCameraController.h"
#include "Engine/Camera/Systems/WorldSystem_CameraManager.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/UpdateContext.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Resource/ResourceSystem.h"
#include "System/Serialization/TypeSerialization.h"
#include "System/Resource/ResourceRequesterID.h"
#include "System/Resource/ResourceSettings.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE
{
    ResourceDescriptorUndoableAction::ResourceDescriptorUndoableAction( TypeSystem::TypeRegistry const* pTypeRegistry, Workspace* pWorkspace )
        : m_pTypeRegistry( pTypeRegistry )
        , m_pWorkspace( pWorkspace )
    {
        EE_ASSERT( m_pTypeRegistry != nullptr );
        EE_ASSERT( m_pWorkspace != nullptr );
        EE_ASSERT( m_pWorkspace->m_pDescriptor != nullptr );
    }

    void ResourceDescriptorUndoableAction::Undo()
    {
        EE_ASSERT( m_pTypeRegistry != nullptr && m_pWorkspace != nullptr );

        Serialization::JsonArchiveReader typeReader;

        typeReader.ReadFromString( m_valueBefore.c_str() );
        auto const& document = typeReader.GetDocument();
        Serialization::ReadNativeType( *m_pTypeRegistry, document, m_pWorkspace->m_pDescriptor );
        m_pWorkspace->ReadCustomDescriptorData( *m_pTypeRegistry, document );
        m_pWorkspace->m_isDirty = true;
    }

    void ResourceDescriptorUndoableAction::Redo()
    {
        EE_ASSERT( m_pTypeRegistry != nullptr && m_pWorkspace != nullptr );

        Serialization::JsonArchiveReader typeReader;

        typeReader.ReadFromString( m_valueAfter.c_str() );
        auto const& document = typeReader.GetDocument();
        Serialization::ReadNativeType( *m_pTypeRegistry, document, m_pWorkspace->m_pDescriptor );
        m_pWorkspace->ReadCustomDescriptorData( *m_pTypeRegistry, document );
        m_pWorkspace->m_isDirty = true;
    }

    void ResourceDescriptorUndoableAction::SerializeBeforeState()
    {
        EE_ASSERT( m_pTypeRegistry != nullptr && m_pWorkspace != nullptr );

        Serialization::JsonArchiveWriter writer;

        auto pWriter = writer.GetWriter();
        pWriter->StartObject();
        Serialization::WriteNativeTypeContents( *m_pTypeRegistry, m_pWorkspace->m_pDescriptor, *pWriter );
        m_pWorkspace->WriteCustomDescriptorData( *m_pTypeRegistry, *pWriter );
        pWriter->EndObject();

        m_valueBefore.resize( writer.GetStringBuffer().GetSize() );
        memcpy( m_valueBefore.data(), writer.GetStringBuffer().GetString(), writer.GetStringBuffer().GetSize() );
    }

    void ResourceDescriptorUndoableAction::SerializeAfterState()
    {
        EE_ASSERT( m_pTypeRegistry != nullptr && m_pWorkspace != nullptr );

        Serialization::JsonArchiveWriter writer;

        auto pWriter = writer.GetWriter();
        pWriter->StartObject();
        Serialization::WriteNativeTypeContents( *m_pTypeRegistry, m_pWorkspace->m_pDescriptor, *pWriter );
        m_pWorkspace->WriteCustomDescriptorData( *m_pTypeRegistry, *pWriter );
        pWriter->EndObject();

        m_valueAfter.resize( writer.GetStringBuffer().GetSize() );
        memcpy( m_valueAfter.data(), writer.GetStringBuffer().GetString(), writer.GetStringBuffer().GetSize() );

        m_pWorkspace->m_isDirty = true;
    }

    //-------------------------------------------------------------------------

    Workspace::Workspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
        : m_pWorld( pWorld )
        , m_pToolsContext( pToolsContext )
        , m_windowName( String::CtorSprintf(), "%s##%s", resourceID.GetResourcePath().GetFileName().c_str(), resourceID.GetResourcePath().c_str())
        , m_descriptorID( resourceID )
        , m_descriptorPath( GetFileSystemPath( resourceID ) )
    {
        EE_ASSERT( m_pWorld != nullptr );
        EE_ASSERT( m_pToolsContext != nullptr && m_pToolsContext->IsValid() );
        EE_ASSERT( resourceID.IsValid() );

        m_ID = m_descriptorID.GetPathID();

        CreateCamera();

        // Create descriptor property grid
        //-------------------------------------------------------------------------

        auto const PreDescEdit = [this] ( PropertyEditInfo const& info )
        {
            EE_ASSERT( m_pActiveUndoableAction == nullptr );
            EE_ASSERT( IsADescriptorWorkspace() && IsDescriptorLoaded() );
            BeginDescriptorModification();
        };

        auto const PostDescEdit = [this] ( PropertyEditInfo const& info )
        {
            EE_ASSERT( m_pActiveUndoableAction != nullptr );
            EE_ASSERT( IsADescriptorWorkspace() && IsDescriptorLoaded() );
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

    Workspace::Workspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, String const& displayName )
        : m_pWorld( pWorld )
        , m_pToolsContext( pToolsContext )
        , m_windowName( displayName )
    {
        EE_ASSERT( m_pWorld != nullptr );
        EE_ASSERT( m_pToolsContext != nullptr && m_pToolsContext->IsValid() );

        m_ID = Hash::GetHash32( displayName );

        CreateCamera();
    }

    Workspace::~Workspace()
    {
        EE_ASSERT( m_requestedResources.empty() );
        EE_ASSERT( m_reloadingResources.empty() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        EE_ASSERT( !m_isInitialized );
        EE_ASSERT( !m_isHotReloading );

        if ( m_pDescriptorPropertyGrid != nullptr )
        {
            m_pDescriptorPropertyGrid->OnPreEdit().Unbind( m_preEditEventBindingID );
            m_pDescriptorPropertyGrid->OnPostEdit().Unbind( m_postEditEventBindingID );
            EE::Delete( m_pDescriptorPropertyGrid );
        }

        EE::Delete( m_pDescriptor );
    }

    //-------------------------------------------------------------------------

    void Workspace::CreateCamera()
    {
        EE_ASSERT( m_pWorld != nullptr && m_pCamera == nullptr );
        auto pCameraManager = m_pWorld->GetWorldSystem<CameraManager>();
        m_pCamera = pCameraManager->TrySpawnDebugCamera();
        m_pCamera->SetDefaultMoveSpeed( 5.0f );
        m_pCamera->ResetMoveSpeed();

        pCameraManager->EnableDebugCamera();
    }

    //-------------------------------------------------------------------------

    void Workspace::Initialize( UpdateContext const& context )
    {
        // Load the editor map
        if ( ShouldLoadDefaultEditorMap() )
        {
            // TODO: make this is a setting
            m_pWorld->LoadMap( ResourcePath( "data://Editor/EditorMap.map" ) );
        }

        // We should never initialize a workspace for a corrupted descriptor
        if ( IsADescriptorWorkspace() )
        {
            EE_ASSERT( IsDescriptorLoaded() );

            Serialization::JsonArchiveReader archive;
            if ( !archive.ReadFromFile( m_descriptorPath ) )
            {
                EE_LOG_ERROR( "Tools", "Resource Workspace", "Failed to read resource descriptor file: %s", m_descriptorPath.c_str() );
                return;
            }

            ReadCustomDescriptorData( *m_pToolsContext->m_pTypeRegistry, archive.GetDocument() );
        }

        //-------------------------------------------------------------------------

        m_pResourceSystem = context.GetSystem<Resource::ResourceSystem>();

        SetDisplayName( m_windowName );

        // Create Tool Windows
        //-------------------------------------------------------------------------

        if ( IsADescriptorWorkspace() )
        {
            CreateToolWindow( "Descriptor", [this] ( UpdateContext const& context, bool isFocused ) { DrawDescriptorEditorWindow( context, isFocused ); } );
            m_toolWindows.back().m_type = ToolWindow::Type::Descriptor;
        }

        if ( HasViewportWindow() )
        {
            CreateToolWindow( "Viewport", [this] ( UpdateContext const& context, bool isFocused ) { DrawDescriptorEditorWindow( context, isFocused ); } );
            m_toolWindows.back().m_type = ToolWindow::Type::Viewport;
        }

        //-------------------------------------------------------------------------

        m_isInitialized = true;
    }

    void Workspace::Shutdown( UpdateContext const& context )
    {
        EE_ASSERT( m_isInitialized );
        m_pResourceSystem = nullptr;
        m_isInitialized = false;
    }

    void Workspace::SetDisplayName( String const& name )
    {
        m_windowName = name;
        m_pWorld->SetDebugName( m_windowName.c_str() );
    }

    void Workspace::CreateToolWindow( String const& name, TFunction<void( UpdateContext const&, bool )> const& drawFunction, ImVec2 const& windowPadding )
    {
        for ( auto const& toolWindow : m_toolWindows )
        {
            EE_ASSERT( toolWindow.m_name != name );
        }

        m_toolWindows.emplace_back( name, drawFunction, windowPadding );

        eastl::sort( m_toolWindows.begin(), m_toolWindows.end(), [] ( ToolWindow const& lhs, ToolWindow const& rhs ) { return lhs.m_name < rhs.m_name; } );
    }

    void Workspace::HideDescriptorWindow()
    {
        for ( auto& toolWindow : m_toolWindows )
        {
            if ( toolWindow.m_type == ToolWindow::Type::Descriptor )
            {
                toolWindow.m_isOpen = false;
            }
        }
    }

    Drawing::DrawContext Workspace::GetDrawingContext()
    {
        return m_pWorld->GetDebugDrawingSystem()->GetDrawingContext();
    }

    //-------------------------------------------------------------------------

    void Workspace::InitializeDockingLayout( ImGuiID const dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGui::DockBuilderRemoveNodeChildNodes( dockspaceID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Descriptor" ).c_str(), dockspaceID );
    }

    //-------------------------------------------------------------------------

    void Workspace::DrawMenu( UpdateContext const& context )
    {
        bool const isSavingAllowed = AlwaysAllowSaving() || IsDirty();
        ImGui::BeginDisabled( !isSavingAllowed );
        if ( ImGui::MenuItem( EE_ICON_CONTENT_SAVE"##Save" ) )
        {
            Save();
        }
        ImGuiX::ItemTooltip( "Save" );
        ImGui::EndDisabled();

        //-------------------------------------------------------------------------

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

        //-------------------------------------------------------------------------

        if ( IsADescriptorWorkspace() )
        {
            if ( ImGui::MenuItem( EE_ICON_FILE_OUTLINE" Copy Resource Path" ) )
            {
                ImGui::SetClipboardText( m_descriptorID.c_str() );
            }
            ImGuiX::ItemTooltip( "Copy Resource Path" );
        }

        //-------------------------------------------------------------------------

        if ( !m_toolWindows.empty() )
        {
            if ( ImGui::BeginMenu( EE_ICON_WINDOW_RESTORE" Window" ) )
            {
                for ( auto& toolWindow : m_toolWindows )
                {
                    ImGui::MenuItem( toolWindow.m_name.c_str(), nullptr, &toolWindow.m_isOpen );
                }

                ImGui::EndMenu();
            }
        }
    }

    //-------------------------------------------------------------------------

    bool Workspace::BeginViewportToolbarGroup( char const* pGroupID, ImVec2 groupSize, ImVec2 const& padding )
    {
        ImGui::PushStyleColor( ImGuiCol_ChildBg, ImGuiX::Style::s_colorGray5.Value );
        ImGui::PushStyleColor( ImGuiCol_Header, ImGuiX::Style::s_colorGray5.Value );
        ImGui::PushStyleColor( ImGuiCol_FrameBg, ImGuiX::Style::s_colorGray5.Value );
        ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImGuiX::Style::s_colorGray4.Value );
        ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImGuiX::Style::s_colorGray3.Value );

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, padding );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 4.0f );

        // Adjust "use available" height to default toolbar height
        if ( groupSize.y <= 0 )
        {
            groupSize.y = ImGui::GetFrameHeight();
        }

        return ImGui::BeginChild( pGroupID, groupSize, false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar );
    }

    void Workspace::EndViewportToolbarGroup()
    {
        ImGui::EndChild();
        ImGui::PopStyleVar( 2 );
        ImGui::PopStyleColor( 5 );
    }

    void Workspace::DrawViewportToolbarCombo( char const* pID, char const* pLabel, char const* pTooltip, TFunction<void()> const& function, float width )
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

    void Workspace::DrawViewportToolbarComboIcon( char const* pID, char const* pIcon, char const* pTooltip, TFunction<void()> const& function )
    {
        float const width = ImGui::CalcTextSize( pIcon ).x + ( ImGui::GetStyle().FramePadding.x * 2 ) + 28;
        DrawViewportToolbarCombo( pID, pIcon, pTooltip, function, width );
    }

    void Workspace::DrawViewportToolBar_Common()
    {
        auto RenderingOptions = [this]()
        {
            Render::RenderDebugView::DrawRenderVisualizationModesMenu( GetWorld() );
        };

        DrawViewportToolbarComboIcon( "##RenderingOptions", EE_ICON_EYE, "Rendering Modes", RenderingOptions );

        //-------------------------------------------------------------------------

        ImGui::SameLine();

        //-------------------------------------------------------------------------

        auto CameraOptions = [this]()
        {
            CameraDebugView::DrawDebugCameraOptions( GetWorld() );
        };

        DrawViewportToolbarComboIcon( "##CameraOptions", EE_ICON_CCTV, "Camera", CameraOptions );
    }

    void Workspace::DrawViewportToolBar_TimeControls()
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

            //ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 0, 3 ) );

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

            //ImGui::PopStyleVar();
        }
        EndViewportToolbarGroup();
    }

    void Workspace::DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        DrawViewportToolBar_Common();

        ImGui::SameLine();

        DrawViewportToolBar_TimeControls();
    }

    void Workspace::DrawViewport( UpdateContext const& context, ViewportInfo const& viewportInfo )
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

        auto pWorld = GetWorld();
        Math::Rectangle const viewportRect( Float2::Zero, viewportSize );
        Render::Viewport* pEngineViewport = pWorld->GetViewport();
        pEngineViewport->Resize( viewportRect );

        // Draw 3D scene
        //-------------------------------------------------------------------------

        ImVec2 const viewportImageCursorPos = ImGui::GetCursorPos();
        ImGui::Image( viewportInfo.m_pViewportRenderTargetTexture, viewportSize );

        if ( ImGui::BeginDragDropTarget() )
        {
            if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( "ResourceFile", ImGuiDragDropFlags_AcceptBeforeDelivery ) )
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

                        // Notify workspace of a resource drag and drop operation
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

    //-------------------------------------------------------------------------

    void Workspace::SetCameraUpdateEnabled( bool isEnabled )
    {
        m_pCamera->SetEnabled( isEnabled );
    }

    void Workspace::ResetCameraView()
    {
        EE_ASSERT( m_pCamera != nullptr );
        m_pCamera->ResetView();
    }

    void Workspace::FocusCameraView( Entity* pTarget )
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

    void Workspace::SetCameraSpeed( float cameraSpeed )
    {
        EE_ASSERT( m_pCamera != nullptr );
        m_pCamera->SetMoveSpeed( cameraSpeed );
    }

    void Workspace::SetCameraTransform( Transform const& cameraTransform )
    {
        EE_ASSERT( m_pCamera != nullptr );
        m_pCamera->SetWorldTransform( cameraTransform );
    }

    Transform Workspace::GetCameraTransform() const
    {
        EE_ASSERT( m_pCamera != nullptr );
        return m_pCamera->GetWorldTransform();
    }

    //-------------------------------------------------------------------------

    void Workspace::SetWorldPaused( bool newPausedState )
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

    void Workspace::SetWorldTimeScale( float newTimeScale )
    {
        m_worldTimeScale = Math::Clamp( newTimeScale, 0.1f, 3.5f );
        if ( !m_pWorld->IsPaused() )
        {
            m_pWorld->SetTimeScale( m_worldTimeScale );
        }
    }

    void Workspace::ResetWorldTimeScale()
    {
        m_worldTimeScale = 1.0f;
        if ( !m_pWorld->IsPaused() )
        {
            m_pWorld->SetTimeScale( 1.0f );
        }
    }

    //-------------------------------------------------------------------------

    void Workspace::LoadResource( Resource::ResourcePtr* pResourcePtr )
    {
        EE_ASSERT( m_pResourceSystem != nullptr );
        EE_ASSERT( pResourcePtr != nullptr && pResourcePtr->IsUnloaded() );
        EE_ASSERT( !VectorContains( m_requestedResources, pResourcePtr ) );
        m_requestedResources.emplace_back( pResourcePtr );
        m_pResourceSystem->LoadResource( *pResourcePtr, Resource::ResourceRequesterID( Resource::ResourceRequesterID::s_toolsRequestID ) );
    }

    void Workspace::UnloadResource( Resource::ResourcePtr* pResourcePtr )
    {
        EE_ASSERT( m_pResourceSystem != nullptr );
        EE_ASSERT( pResourcePtr->WasRequested() );
        EE_ASSERT( VectorContains( m_requestedResources, pResourcePtr ) );
        m_pResourceSystem->UnloadResource( *pResourcePtr, Resource::ResourceRequesterID( Resource::ResourceRequesterID::s_toolsRequestID ) );
        m_requestedResources.erase_first_unsorted( pResourcePtr );
    }

    bool Workspace::RequestImmediateResourceCompilation( ResourceID const& resourceID )
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

    //-------------------------------------------------------------------------

    void Workspace::AddEntityToWorld( Entity* pEntity )
    {
        EE_ASSERT( m_pWorld != nullptr );
        EE_ASSERT( pEntity != nullptr && !pEntity->IsAddedToMap() );
        EE_ASSERT( !VectorContains( m_addedEntities, pEntity ) );
        m_addedEntities.emplace_back( pEntity );
        m_pWorld->GetPersistentMap()->AddEntity( pEntity );
    }

    void Workspace::RemoveEntityFromWorld( Entity* pEntity )
    {
        EE_ASSERT( m_pWorld != nullptr );
        EE_ASSERT( pEntity != nullptr && pEntity->GetMapID() == m_pWorld->GetPersistentMap()->GetID() );
        EE_ASSERT( VectorContains( m_addedEntities, pEntity ) );
        m_pWorld->GetPersistentMap()->RemoveEntity( pEntity->GetID() );
        m_addedEntities.erase_first_unsorted( pEntity );
    }

    void Workspace::DestroyEntityInWorld( Entity*& pEntity )
    {
        EE_ASSERT( m_pWorld != nullptr );
        EE_ASSERT( pEntity != nullptr && pEntity->GetMapID() == m_pWorld->GetPersistentMap()->GetID() );
        EE_ASSERT( VectorContains( m_addedEntities, pEntity ) );
        m_pWorld->GetPersistentMap()->DestroyEntity( pEntity->GetID() );
        m_addedEntities.erase_first_unsorted( pEntity );
        pEntity = nullptr;
    }

    //-------------------------------------------------------------------------

    bool Workspace::Save()
    {
        // Save Descriptor
        if ( IsADescriptorWorkspace() )
        {
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
        }

        return true;
    }

    //-------------------------------------------------------------------------

    void Workspace::BeginDescriptorModification()
    {
        if ( m_beginModificationCallCount == 0 )
        {
            auto pUndoableAction = EE::New<ResourceDescriptorUndoableAction>( m_pToolsContext->m_pTypeRegistry, this );
            pUndoableAction->SerializeBeforeState();
            m_pActiveUndoableAction = pUndoableAction;
        }
        m_beginModificationCallCount++;
    }

    void Workspace::EndDescriptorModification()
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

    //-------------------------------------------------------------------------

    void Workspace::Undo()
    {
        PreUndoRedo( UndoStack::Operation::Undo );
        auto pAction = m_undoStack.Undo();
        PostUndoRedo( UndoStack::Operation::Undo, pAction );

        if ( m_pDescriptorPropertyGrid != nullptr )
        {
            m_pDescriptorPropertyGrid->MarkDirty();
        }

        MarkDirty();
    }

    void Workspace::Redo()
    {
        PreUndoRedo( UndoStack::Operation::Redo );
        auto pAction = m_undoStack.Redo();
        PostUndoRedo( UndoStack::Operation::Redo, pAction );

        if ( m_pDescriptorPropertyGrid != nullptr )
        {
            m_pDescriptorPropertyGrid->MarkDirty();
        }

        MarkDirty();
    }

    //-------------------------------------------------------------------------

    void Workspace::BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded )
    {
        EE_ASSERT( !m_isHotReloading );

        // Does the descriptor need reloading?
        bool reloadDescriptor = false;
        if ( IsADescriptorWorkspace() )
        {
            if ( VectorContains( resourcesToBeReloaded, m_descriptorID ) )
            {
                reloadDescriptor = true;
            }
        }

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
        if ( reloadDescriptor || !resourcesThatNeedReloading.empty() )
        {
            OnHotReloadStarted( reloadDescriptor, resourcesThatNeedReloading );

            //-------------------------------------------------------------------------

            // Unload descriptor
            if ( reloadDescriptor )
            {
                EE::Delete( m_pDescriptor );
            }

            // Unload resources
            for ( auto& pResource : resourcesThatNeedReloading )
            {
                // The 'OnHotReloadStarted' function might have unloaded some resources as part of its execution, so ignore those here
                if ( pResource->IsUnloaded() )
                {
                    continue;
                }

                m_pResourceSystem->UnloadResource( *pResource, Resource::ResourceRequesterID( Resource::ResourceRequesterID::s_toolsRequestID ) );
                m_reloadingResources.emplace_back( pResource );
            }

            m_isHotReloading = true;
        }
    }

    void Workspace::EndHotReload()
    {
        bool const hasResourcesToReload = !m_reloadingResources.empty();
        if ( hasResourcesToReload )
        {
            // Load all unloaded resources
            for ( auto& pReloadedResource : m_reloadingResources )
            {
                m_pResourceSystem->LoadResource( *pReloadedResource, Resource::ResourceRequesterID( Resource::ResourceRequesterID::s_toolsRequestID ) );
            }
            m_reloadingResources.clear();
        }

        //-------------------------------------------------------------------------

        bool const needsToReloadDescriptor = IsADescriptorWorkspace() && m_pDescriptor == nullptr;
        if ( needsToReloadDescriptor )
        {
            // Try to load the descriptor, if this fails to load, then we early out of the hot-reload
            LoadDescriptor();
            if ( !IsDescriptorLoaded() )
            {
                m_isHotReloading = false;
                return;
            }
        }

        //-------------------------------------------------------------------------

        // Notify workspaces that the reload is complete
        if ( m_isHotReloading )
        {
            OnHotReloadComplete();
            m_isHotReloading = false;
        }
    }

    //-------------------------------------------------------------------------

    void Workspace::LoadDescriptor()
    {
        EE_ASSERT( IsADescriptorWorkspace() );
        EE_ASSERT( m_pDescriptor == nullptr );
        EE_ASSERT( m_pDescriptorPropertyGrid != nullptr );

        Serialization::JsonArchiveReader archive;
        if ( !archive.ReadFromFile( m_descriptorPath ) )
        {
            EE_LOG_ERROR( "Tools", "Resource Workspace", "Failed to read resource descriptor file: %s", m_descriptorPath.c_str() );
            return;
        }

        auto const& document = archive.GetDocument();
        m_pDescriptor = Serialization::TryCreateAndReadNativeType<Resource::ResourceDescriptor>( *m_pToolsContext->m_pTypeRegistry, document );
        m_pDescriptorPropertyGrid->SetTypeToEdit( m_pDescriptor );
    }

    void Workspace::DrawDescriptorEditorWindow( UpdateContext const& context, bool isFocused )
    {
        EE_ASSERT( IsADescriptorWorkspace() );
        EE_ASSERT( m_pDescriptorPropertyGrid != nullptr );

        if ( m_pDescriptor == nullptr )
        {
            ImGui::Text( "Failed to load descriptor!" );
        }
        else
        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
            ImGui::Text( "Descriptor: %s", m_descriptorID.c_str() );

            ImGui::BeginDisabled( !m_pDescriptorPropertyGrid->IsDirty() );
            if ( ImGuiX::ColoredButton( ImGuiX::ImColors::ForestGreen, ImGuiX::ImColors::White, EE_ICON_CONTENT_SAVE" Save", ImVec2( -1, 0 ) ) )
            {
                Save();
            }
            ImGui::EndDisabled();

            m_pDescriptorPropertyGrid->DrawGrid();
        }

        m_isDescriptorWindowFocused = isFocused;
    }

    //-------------------------------------------------------------------------

    void Workspace::SharedUpdate( UpdateContext const& context, bool isFocused )
    {
        if ( isFocused )
        {
            auto& IO = ImGui::GetIO();
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

//-------------------------------------------------------------------------

namespace EE
{
    EE_GLOBAL_REGISTRY( ResourceWorkspaceFactory );

    //-------------------------------------------------------------------------

    bool ResourceWorkspaceFactory::CanCreateWorkspace( ToolsContext const* pToolsContext, ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );
        auto resourceTypeID = resourceID.GetResourceTypeID();
        EE_ASSERT( resourceTypeID.IsValid() );

        // Check if we have a custom workspace for this type
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

    Workspace* ResourceWorkspaceFactory::CreateWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );
        auto resourceTypeID = resourceID.GetResourceTypeID();
        EE_ASSERT( resourceTypeID.IsValid() );

        // Check if we have a custom workspace for this type
        //-------------------------------------------------------------------------

        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( resourceTypeID == pCurrentFactory->GetSupportedResourceTypeID() )
            {
                return pCurrentFactory->CreateWorkspaceInternal( pToolsContext, pWorld, resourceID );
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        // Create generic descriptor workspace
        //-------------------------------------------------------------------------

        auto const resourceDescriptorTypes = pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( Resource::ResourceDescriptor::GetStaticTypeID(), false, false );
        for ( auto pResourceDescriptorTypeInfo : resourceDescriptorTypes )
        {
            auto pDefaultInstance = Cast<Resource::ResourceDescriptor>( pResourceDescriptorTypeInfo->GetDefaultInstance() );
            if ( pDefaultInstance->GetCompiledResourceTypeID() == resourceID.GetResourceTypeID() )
            {
                return EE::New<GenericWorkspace>( pToolsContext, pWorld, resourceID );
            }
        }

        EE_UNREACHABLE_CODE();
        return nullptr;
    }
}