#include "Workspace_EntityEditor.h"

#include "EngineTools/Entity/EntitySerializationTools.h"
#include "EngineTools/Entity/EntityEditor/EntityUndoableAction.h"
#include "Engine/Navmesh/Systems/WorldSystem_Navmesh.h"
#include "Engine/Navmesh/DebugViews/DebugView_Navmesh.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/PhysicsLayers.h"
#include "Engine/Physics/PhysicsMesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Math/Math.h"
#include "System/Math/MathStringHelpers.h"
#include "System/Log.h"

#include "Engine/AI/Components/Component_AISpawn.h"
#include "Engine/Physics/Components/Component_PhysicsMesh.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Render/Components/Component_Lights.h"
#include "Engine/Volumes/Components/Component_Volumes.h"
#include "Engine/Player/Components/Component_PlayerSpawn.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityEditorWorkspace::EntityEditorWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
        : Workspace( pToolsContext, pWorld, resourceID )
        , m_outliner( pToolsContext, &m_undoStack, m_pWorld )
        , m_entityStructureEditor( pToolsContext, &m_undoStack, m_pWorld )
        , m_propertyGrid( pToolsContext, &m_undoStack, m_pWorld )
    {
        m_gizmo.SetTargetTransform( &m_gizmoTransform );
    }

    EntityEditorWorkspace::EntityEditorWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, String const& displayName )
        : Workspace( pToolsContext, pWorld, displayName )
        , m_outliner( pToolsContext, &m_undoStack, m_pWorld )
        , m_entityStructureEditor( pToolsContext, &m_undoStack, m_pWorld )
        , m_propertyGrid( pToolsContext, &m_undoStack, m_pWorld )
    {
        m_gizmo.SetTargetTransform( &m_gizmoTransform );
    }

    //-------------------------------------------------------------------------

    void EntityEditorWorkspace::Initialize( UpdateContext const& context )
    {
        Workspace::Initialize( context );

        m_actionPerformEventBindingID = m_undoStack.OnActionPerformed().Bind( [this] () { OnActionPerformed(); } );

        //-------------------------------------------------------------------------

        m_entityStructureEditor.Initialize( context, GetID() );
        m_structureEditorSelectionUpdateEventBindingID = m_entityStructureEditor.OnSelectionManuallyChanged().Bind( [this] ( IRegisteredType* pTypeToEdit ) { OnStructureEditorSelectionChanged( pTypeToEdit ); } );

        //-------------------------------------------------------------------------

        m_outliner.Initialize( context, GetID() );
        m_outlinerSelectionUpdateEventBindingID = m_outliner.OnSelectedChanged().Bind( [this] ( TreeListView::ChangeReason reason ) { OnOutlinerSelectionChanged( reason ); } );

        // Property grid
        //-------------------------------------------------------------------------

        m_propertyGrid.Initialize( context, GetID() );

        // Visualizations
        //-------------------------------------------------------------------------

        m_volumeTypes = context.GetSystem<TypeSystem::TypeRegistry>()->GetAllDerivedTypes( VolumeComponent::GetStaticTypeID(), false, false, true );
    }

    void EntityEditorWorkspace::Shutdown( UpdateContext const& context )
    {
        m_outliner.OnSelectedChanged().Unbind( m_outlinerSelectionUpdateEventBindingID );
        m_outliner.Shutdown( context );

        //-------------------------------------------------------------------------

        m_entityStructureEditor.OnSelectionManuallyChanged().Unbind( m_structureEditorSelectionUpdateEventBindingID );
        m_entityStructureEditor.Shutdown( context );

        //-------------------------------------------------------------------------

        m_propertyGrid.Shutdown( context );

        //-------------------------------------------------------------------------

        m_undoStack.OnActionPerformed().Unbind( m_actionPerformEventBindingID );

        Workspace::Shutdown( context );
    }

    void EntityEditorWorkspace::InitializeDockingLayout( ImGuiID dockspaceID ) const
    {
        ImGuiID topDockID = 0, bottomLeftDockID = 0, bottomCenterDockID = 0, bottomRightDockID = 0;
        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.5f, &bottomCenterDockID, &topDockID );
        ImGui::DockBuilderSplitNode( bottomCenterDockID, ImGuiDir_Right, 0.66f, &bottomCenterDockID, &bottomLeftDockID );
        ImGui::DockBuilderSplitNode( bottomCenterDockID, ImGuiDir_Right, 0.5f, &bottomRightDockID, &bottomCenterDockID );

        // Viewport Flags
        ImGuiDockNode* pTopNode = ImGui::DockBuilderGetNode( topDockID );
        pTopNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoDockingSplitMe | ImGuiDockNodeFlags_NoDockingOverMe;

        // Dock windows
        ImGui::DockBuilderDockWindow( GetViewportWindowID(), topDockID );
        ImGui::DockBuilderDockWindow( m_outliner.GetWindowName(), bottomLeftDockID );
        ImGui::DockBuilderDockWindow( m_propertyGrid.GetWindowName(), bottomCenterDockID);
        ImGui::DockBuilderDockWindow( m_entityStructureEditor.GetWindowName(), bottomRightDockID );
    }

    //-------------------------------------------------------------------------

    void EntityEditorWorkspace::DropResourceInViewport( ResourceID const& resourceID, Vector const& worldPosition )
    {
        // Static Mesh Resource
        //-------------------------------------------------------------------------

        if ( resourceID.GetResourceTypeID() == Render::StaticMesh::GetStaticResourceTypeID() )
        {
            auto pMap = m_pWorld->GetFirstNonPersistentMap();
            if ( pMap != nullptr )
            {
                // Create entity
                StringID const entityName( resourceID.GetFileNameWithoutExtension().c_str() );
                auto pMeshEntity = EE::New<Entity>( entityName );

                // Create mesh component
                auto pMeshComponent = EE::New<Render::StaticMeshComponent>( StringID( "Mesh Component" ) );
                pMeshComponent->SetMesh( resourceID );
                pMeshComponent->SetWorldTransform( Transform( Quaternion::Identity, worldPosition ) );
                pMeshEntity->AddComponent( pMeshComponent );

                // Try create optional physics collision component
                ResourcePath physicsResourcePath = resourceID.GetResourcePath();
                physicsResourcePath.ReplaceExtension( Physics::PhysicsMesh::GetStaticResourceTypeID().ToString() );
                ResourceID const physicsResourceID( physicsResourcePath );
                if ( m_pToolsContext->m_pResourceDatabase->DoesResourceExist( physicsResourceID ) )
                {
                    auto pPhysicsMeshComponent = EE::New<Physics::PhysicsMeshComponent>( StringID( "Physics Component" ) );
                    pPhysicsMeshComponent->SetMesh( physicsResourceID );
                    pMeshEntity->AddComponent( pPhysicsMeshComponent );
                }

                // Add entity to map
                pMap->AddEntity( pMeshEntity );

                // Record undo action
                auto pActiveUndoableAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
                pActiveUndoableAction->RecordCreate( { pMeshEntity } );
                m_undoStack.RegisterAction( pActiveUndoableAction );

                // Switch selection
                m_outliner.SetSelection( pMeshEntity );
            }
        }
    }

    void EntityEditorWorkspace::OnMousePick( Render::PickingID pickingID )
    {
        if ( !pickingID.IsSet() )
        {
            m_outliner.ClearSelection();
            return;
        }

        //-------------------------------------------------------------------------

        EntityID const pickedEntityID( pickingID.m_0 );
        auto pEntity = m_pWorld->FindEntity( pickedEntityID );
        if ( pEntity == nullptr )
        {
            m_outliner.ClearSelection();
            return;
        }

        //-------------------------------------------------------------------------

        // If we have ctrl-held, select the individual component
        if ( ImGui::GetIO().KeyAlt )
        {
            m_outliner.SetSelection( pEntity );

            ComponentID const pickedComponentID( pickingID.m_1 );
            auto pComponent = pEntity->FindComponent( pickedComponentID );
            EE_ASSERT( pComponent != nullptr );
            m_entityStructureEditor.SetEntityToEdit( pEntity, pComponent );
        }
        // Else just select the entire entity
        else if ( ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl )
        {
            if ( m_outliner.IsEntitySelected( pEntity ) )
            {
                m_outliner.RemoveFromSelection( pEntity );
            }
            else
            {
                m_outliner.AddToSelection( pEntity );
            }
        }
        else // No modifier so just set selection
        {
            m_outliner.SetSelection( pEntity );
        }
    }

    void EntityEditorWorkspace::OnOutlinerSelectionChanged( TreeListView::ChangeReason reason )
    {
        // Notify structure editor
        //-------------------------------------------------------------------------

        if ( reason != TreeListView::ChangeReason::TreeRebuild )
        {
            auto const selectedEntities = m_outliner.GetSelectedEntities();
            m_entityStructureEditor.SetEntitiesToEdit( selectedEntities );
            m_propertyGrid.SetTypeInstanceToEdit( nullptr );
        }

        // Update spatial info
        //-------------------------------------------------------------------------

        UpdateSelectionSpatialInfo();
    }

    void EntityEditorWorkspace::OnStructureEditorSelectionChanged( IRegisteredType* pTypeToEdit )
    {
        m_propertyGrid.SetTypeInstanceToEdit( pTypeToEdit );
    }

    void EntityEditorWorkspace::OnActionPerformed()
    {
        MarkDirty();
    }

    void EntityEditorWorkspace::PreUndoRedo( UndoStack::Operation operation )
    {
    }

    void EntityEditorWorkspace::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        m_propertyGrid.RequestRefresh();
    }

    //-------------------------------------------------------------------------

    void EntityEditorWorkspace::UpdateSelectionSpatialInfo()
    {
        TVector<OBB> bounds;

        m_selectionTransform = Transform::Identity;
        m_selectionOffsetTransforms.clear();
        m_hasSpatialSelection = false;

        // Check if we have spatial selection
        //-------------------------------------------------------------------------
        // Record the the transforms of all found spatial elements in the offset array

        // Check all selected components
        auto const& selectedSpatialComponents = m_entityStructureEditor.GetSelectedSpatialComponents();
        if ( selectedSpatialComponents.size() > 0 )
        {
            for ( auto pComponent : selectedSpatialComponents )
            {
                m_selectionOffsetTransforms.emplace_back( pComponent->GetWorldTransform() );
                bounds.emplace_back( pComponent->GetWorldBounds() );
                m_hasSpatialSelection = true;
            }
        }
        else // Check all selected entities
        {
            for ( auto pSelectedEntity : m_outliner.GetSelectedEntities() )
            {
                if ( pSelectedEntity->IsSpatialEntity() )
                {
                    m_selectionOffsetTransforms.emplace_back( pSelectedEntity->GetWorldTransform() );
                    bounds.emplace_back( pSelectedEntity->GetCombinedWorldBounds() ); // TODO: calculate combined bounds
                    m_hasSpatialSelection = true;
                }
            }
        }

        // Update selection transform
        //-------------------------------------------------------------------------

        if ( m_hasSpatialSelection )
        {
            if ( m_selectionOffsetTransforms.size() == 1 )
            {
                m_selectionTransform = m_selectionOffsetTransforms[0];
                m_selectionOffsetTransforms[0] = Transform::Identity;
            }
            else
            {
                // Calculate the average position of all transforms found
                Vector averagePosition = Vector::Zero;
                for ( auto const& t : m_selectionOffsetTransforms )
                {
                    averagePosition += t.GetTranslation();
                }
                averagePosition /= (float) m_selectionOffsetTransforms.size();

                // Set the average transform
                m_selectionTransform = Transform( Quaternion::Identity, averagePosition );

                // Calculate the offsets
                for ( auto& t : m_selectionOffsetTransforms )
                {
                    t = Transform::Delta( m_selectionTransform, t );
                }
            }
        }

        // Update selection bounds
        //-------------------------------------------------------------------------

        if ( m_hasSpatialSelection )
        {
            if ( bounds.size() == 1 )
            {
                m_selectionBounds = bounds[0];
            }
            else
            {
                TVector<Vector> points;

                for ( auto itemBounds : bounds )
                {
                    Vector corners[8];
                    itemBounds.GetCorners( corners );
                    for ( auto i = 0; i < 8; i++ )
                    {
                        points.emplace_back( corners[i] );
                    }
                }

                m_selectionBounds = OBB( points.data(), (uint32_t) points.size() );
            }
        }
    }

    //-------------------------------------------------------------------------

    void EntityEditorWorkspace::DrawViewportToolbarItems( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        constexpr float const buttonWidth = 20;

        if ( BeginViewportToolbarGroup( "SpatialControls", ImVec2( 106, 0 ) ) )
        {
            TInlineString<10> const coordinateSpaceSwitcherLabel( TInlineString<10>::CtorSprintf(), "%s##CoordinateSpace", m_gizmo.IsInWorldSpace() ? EE_ICON_EARTH : EE_ICON_MAP_MARKER );
            if ( ImGui::Selectable( coordinateSpaceSwitcherLabel.c_str(), false, 0, ImVec2( buttonWidth, 0 ) ) )
            {
                m_gizmo.SetCoordinateSystemSpace( m_gizmo.IsInWorldSpace() ? CoordinateSpace::Local : CoordinateSpace::World );
            }
            ImGuiX::ItemTooltip( "Current Mode: %s", m_gizmo.IsInWorldSpace() ? "World Space" : "Local Space" );

            ImGuiX::VerticalSeparator( ImVec2( 11, -1 ) );

            //-------------------------------------------------------------------------

            bool t = m_gizmo.GetMode() == ImGuiX::Gizmo::GizmoMode::Translation;
            bool r = m_gizmo.GetMode() == ImGuiX::Gizmo::GizmoMode::Rotation;
            bool s = m_gizmo.GetMode() == ImGuiX::Gizmo::GizmoMode::Scale;

            if ( ImGui::Selectable( EE_ICON_AXIS_ARROW, t, 0, ImVec2( buttonWidth, 0 ) ) )
            {
                m_gizmo.SwitchMode( ImGuiX::Gizmo::GizmoMode::Translation );
            }
            ImGuiX::ItemTooltip( "Translate" );

            ImGui::SameLine();

            if ( ImGui::Selectable( EE_ICON_ROTATE_ORBIT, r, 0, ImVec2( buttonWidth, 0 ) ) )
            {
                m_gizmo.SwitchMode( ImGuiX::Gizmo::GizmoMode::Rotation );
            }
            ImGuiX::ItemTooltip( "Rotate" );

            ImGui::SameLine();

            if ( ImGui::Selectable( EE_ICON_ARROW_TOP_RIGHT_BOTTOM_LEFT, s, 0, ImVec2( buttonWidth, 0 ) ) )
            {
                m_gizmo.SwitchMode( ImGuiX::Gizmo::GizmoMode::Scale );
            }
            ImGuiX::ItemTooltip( "Scale" );
        }
        EndViewportToolbarGroup();

        //-------------------------------------------------------------------------

        ImGui::SetNextItemWidth( 48 );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4.0f, 4.0f ) );
        if ( ImGui::BeginCombo( "##MapEditorOptions", EE_ICON_MONITOR_EYE, ImGuiComboFlags_HeightLarge ) )
        {
            ImGuiX::TextSeparator( "Physics" );

            auto pPhysicsWorldSystem = m_pWorld->GetWorldSystem<Physics::PhysicsWorldSystem>();
            bool isDebugEnabled = pPhysicsWorldSystem->IsDebugDrawingEnabled();
            if ( ImGui::MenuItem( "Show Physics Collision", nullptr, &isDebugEnabled ) )
            {
                pPhysicsWorldSystem->SetDebugDrawingEnabled( isDebugEnabled );
            }

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Navmesh" );

            auto pNavmeshWorldSystem = m_pWorld->GetWorldSystem<Navmesh::NavmeshWorldSystem>();
            Navmesh::NavmeshDebugView::DrawNavmeshRuntimeSettings( pNavmeshWorldSystem );

            //-------------------------------------------------------------------------

            ImGui::EndCombo();
        }
        ImGuiX::ItemTooltip( "Map Visualizations" );
        ImGui::PopStyleVar();

        //-------------------------------------------------------------------------

        ImGui::SameLine();
        ImGui::SetNextItemWidth( 48 );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4.0f, 4.0f ) );
        if ( ImGui::BeginCombo( "##Volumes", EE_ICON_CUBE_OUTLINE, ImGuiComboFlags_HeightLarge ) )
        {
            for ( auto pVolumeTypeInfo : m_volumeTypes )
            {
                bool isVisualized = VectorContains( m_visualizedVolumeTypes, pVolumeTypeInfo );
                if ( ImGui::MenuItem( pVolumeTypeInfo->GetFriendlyTypeName(), nullptr, isVisualized ) )
                {
                    if ( isVisualized )
                    {
                        m_visualizedVolumeTypes.erase_first( pVolumeTypeInfo );
                    }
                    else
                    {
                        m_visualizedVolumeTypes.emplace_back( pVolumeTypeInfo );
                    }
                }
            }

            ImGui::EndCombo();
        }
        ImGuiX::ItemTooltip( "Volume Visualizations" );
        ImGui::PopStyleVar();

        //-------------------------------------------------------------------------

        ImGui::SameLine();
        ImGui::SetNextItemWidth( 48 );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4.0f, 4.0f ) );
        if ( ImGui::BeginCombo( "##Help", EE_ICON_HELP_CIRCLE_OUTLINE, ImGuiComboFlags_HeightLarge ) )
        {
            auto DrawHelpRow = [] ( char const* pLabel, char const* pHotkey )
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
                    ImGui::Text( pHotkey );
                }
            };

            //-------------------------------------------------------------------------

            if ( ImGui::BeginTable( "HelpTable", 2 ) )
            {
                DrawHelpRow( "Switch Gizmo Mode","Spacebar" );
                DrawHelpRow( "Multi Select", "Ctrl/Shift + Left Click" );
                DrawHelpRow( "Directly Select Component", "Alt + Left Click" );
                DrawHelpRow( "Duplicate Selected Entities","Alt + translate" );

                ImGui::EndTable();
            }

            ImGui::EndCombo();
        }
        ImGuiX::ItemTooltip( "Help" );
        ImGui::PopStyleVar();
    }

    void EntityEditorWorkspace::DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        auto drawingCtx = GetDrawingContext();

        // Overlay Widgets
        //-------------------------------------------------------------------------

        //auto DrawComponentWorldIcon = [this, pViewport] ( SpatialEntityComponent* pComponent, char icon[4], bool isSelected )
        //{
        //    if ( pViewport->IsWorldSpacePointVisible( pComponent->GetPosition() ) )
        //    {
        //        ImVec2 const positionScreenSpace = pViewport->WorldSpaceToScreenSpace( pComponent->GetPosition() );
        //        if ( ImGuiX::DrawOverlayIcon( positionScreenSpace, icon, pComponent, isSelected, ImColor( Colors::Yellow.ToFloat4() ) ) )
        //        {
        //            return true;
        //        }
        //    }

        //    return false;
        //};

        //auto const& registeredLights = m_pWorld->GetAllRegisteredComponentsOfType<Render::LightComponent>();
        //for ( auto pComponent : registeredLights )
        //{
        //    auto pLightComponent = const_cast<Render::LightComponent*>( pComponent );
        //    bool const isSelected = m_context.HasSingleSelectedComponent() && m_context.IsSelected( pComponent );
        //    if ( DrawComponentWorldIcon( pLightComponent, EE_ICON_LIGHTBULB, isSelected ) )
        //    {
        //        auto pEntity = pEditedMap->FindEntity( pLightComponent->GetEntityID() );
        //        m_context.SelectEntity( pEntity );
        //        m_context.SelectComponent( pLightComponent );
        //    }
        //}

        //auto const& registeredPlayerSpawns = m_pWorld->GetAllRegisteredComponentsOfType<Player::PlayerSpawnComponent>();
        //for ( auto pComponent : registeredPlayerSpawns )
        //{
        //    auto pSpawnComponent = const_cast<Player::PlayerSpawnComponent*>( pComponent );
        //    bool const isSelected = m_context.HasSingleSelectedComponent() && m_context.IsSelected( pComponent );
        //    if ( DrawComponentWorldIcon( pSpawnComponent, EE_ICON_GAMEPAD, isSelected ) )
        //    {
        //        auto pEntity = pEditedMap->FindEntity( pSpawnComponent->GetEntityID() );
        //        m_context.SelectEntity( pEntity );
        //        m_context.SelectComponent( pSpawnComponent );
        //    }

        //    auto const& spawnTransform = pComponent->GetWorldTransform();
        //    drawingCtx.DrawArrow( spawnTransform.GetTranslation(), spawnTransform.GetForwardVector(), 0.5f, Colors::Yellow, 3.0f );
        //}

        //auto const& registeredAISpawns = m_pWorld->GetAllRegisteredComponentsOfType<AI::AISpawnComponent>();
        //for ( auto pComponent : registeredAISpawns )
        //{
        //    auto pSpawnComponent = const_cast<AI::AISpawnComponent*>( pComponent );
        //    bool const isSelected = m_context.HasSingleSelectedComponent() && m_context.IsSelected( pComponent );
        //    if ( DrawComponentWorldIcon( pSpawnComponent, EE_ICON_ROBOT, isSelected ) )
        //    {
        //        auto pEntity = pEditedMap->FindEntity( pSpawnComponent->GetEntityID() );
        //        m_context.SelectEntity( pEntity );
        //        m_context.SelectComponent( pSpawnComponent );
        //    }

        //    auto const& spawnTransform = pComponent->GetWorldTransform();
        //    drawingCtx.DrawArrow( spawnTransform.GetTranslation(), spawnTransform.GetForwardVector(), 0.5f, Colors::Yellow, 3.0f );
        //}

        //// Draw light debug widgets
        ////-------------------------------------------------------------------------
        //// TODO: generalized component visualizers

        //if ( m_context.HasSingleSelectedComponent() )
        //{
        //    auto pSelectedComponent = m_context.GetSelectedComponent();
        //    if ( IsOfType<Render::DirectionalLightComponent>( pSelectedComponent ) )
        //    {
        //        auto pLightComponent = Cast<Render::DirectionalLightComponent>( pSelectedComponent );
        //        auto forwardDir = pLightComponent->GetForwardVector();
        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() + forwardDir, pLightComponent->GetLightColor(), 3.0f );
        //    }
        //    else if ( IsOfType<Render::SpotLightComponent>( pSelectedComponent ) )
        //    {
        //        auto pLightComponent = Cast<Render::SpotLightComponent>( pSelectedComponent );
        //        drawingCtx.DrawCone( pLightComponent->GetWorldTransform(), pLightComponent->GetLightInnerUmbraAngle(), 1.5f, pLightComponent->GetLightColor(), 3.0f );
        //        drawingCtx.DrawCone( pLightComponent->GetWorldTransform(), pLightComponent->GetLightOuterUmbraAngle(), 1.5f, pLightComponent->GetLightColor(), 3.0f );
        //    }
        //    else if ( IsOfType<Render::PointLightComponent>( pSelectedComponent ) )
        //    {
        //        auto pLightComponent = Cast<Render::PointLightComponent>( pSelectedComponent );
        //        auto forwardDir = pLightComponent->GetForwardVector();
        //        auto upDir = pLightComponent->GetUpVector();
        //        auto rightDir = pLightComponent->GetRightVector();

        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() + ( forwardDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() - ( forwardDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() + ( upDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() - ( upDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() + ( rightDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() - ( rightDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
        //    }
        //}

        //// Volume Debug
        ////-------------------------------------------------------------------------

        //for ( auto pVisualizedVolumeTypeInfo : m_visualizedVolumeTypes )
        //{
        //    auto const& volumeComponents = m_pWorld->GetAllRegisteredComponentsOfType( pVisualizedVolumeTypeInfo->m_ID );
        //    for ( auto pComponent : volumeComponents )
        //    {
        //        auto pVolumeComponent = static_cast<VolumeComponent const*>( pComponent );
        //        pVolumeComponent->Draw( drawingCtx );
        //    }
        //}

        // Selection and Manipulation
        //-------------------------------------------------------------------------

        if ( m_hasSpatialSelection )
        {
            // Draw selection bounds
            //-------------------------------------------------------------------------

            for ( auto pEntity : m_outliner.GetSelectedEntities() )
            {
                if ( pEntity->IsSpatialEntity() )
                {
                    /*InlineString debugStr = Math::ToString( pEntity->GetLocalTransform() );
                    drawingCtx.DrawText3D( pEntity->GetWorldTransform().GetTranslation(), debugStr.c_str(), Colors::Red );*/
                    drawingCtx.DrawWireBox( pEntity->GetCombinedWorldBounds(), Colors::Cyan, 3.0f, Drawing::EnableDepthTest );
                }
            }

            drawingCtx.DrawWireBox( m_selectionBounds, Colors::Yellow, 3.0f, Drawing::EnableDepthTest );

            // Update Gizmo
            //-------------------------------------------------------------------------

            if ( m_gizmo.GetMode() == ImGuiX::Gizmo::GizmoMode::None )
            {
                m_gizmo.SwitchMode( ImGuiX::Gizmo::GizmoMode::Translation );
            }

            m_gizmoTransform = m_selectionTransform;

            switch ( m_gizmo.Draw( *m_pWorld->GetViewport() ) )
            {
                case ImGuiX::Gizmo::Result::StartedManipulating:
                {
                    bool const hasSelectedComponent = !m_entityStructureEditor.GetSelectedSpatialComponents().empty(); // Disallow component duplication atm
                    bool const duplicateSelection = !hasSelectedComponent &&( m_gizmo.GetMode() == ImGuiX::Gizmo::GizmoMode::Translation ) && ImGui::GetIO().KeyAlt;
                    BeginTransformManipulation( m_gizmoTransform, duplicateSelection );
                }
                break;

                case ImGuiX::Gizmo::Result::Manipulating:
                {
                    ApplyTransformManipulation( m_gizmoTransform );
                }
                break;

                case ImGuiX::Gizmo::Result::StoppedManipulating:
                {
                    EndTransformManipulation( m_gizmoTransform );
                }
                break;
            }
        }
    }

    void EntityEditorWorkspace::Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        if ( m_isViewportFocused || m_isViewportHovered )
        {
            if ( ImGui::IsKeyPressed( ImGuiKey_Space ) )
            {
                m_gizmo.SwitchToNextMode();
            }

            if ( ImGui::IsKeyPressed( ImGuiKey_Delete ) )
            {
                m_outliner.DestroySelectedEntities();
            }
        }

        m_outliner.UpdateAndDraw( context, pWindowClass );
        m_entityStructureEditor.UpdateAndDraw( context, pWindowClass );
        m_propertyGrid.UpdateAndDraw( context, pWindowClass );
    }

    //-------------------------------------------------------------------------
    // Transform editing
    //-------------------------------------------------------------------------

    void EntityEditorWorkspace::BeginTransformManipulation( Transform const& newTransform, bool duplicateSelection )
    {
        EE_ASSERT( !m_outliner.GetSelectedEntities().empty() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );

        // Should we duplicate the selection?
        if ( duplicateSelection )
        {
            m_outliner.DuplicateSelectedEntities();
        }

        // Create undo action
        auto const selectedEntities = m_outliner.GetSelectedEntities();
        auto pEntityUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        pEntityUndoAction->RecordBeginEdit( selectedEntities, duplicateSelection );
        m_pActiveUndoableAction = pEntityUndoAction;

        // Apply transform modification
        ApplyTransformManipulation( newTransform );
    }

    void EntityEditorWorkspace::ApplyTransformManipulation( Transform const& newTransform )
    {
        EE_ASSERT( m_pActiveUndoableAction != nullptr );

        // Update all selected components
        auto const& selectedSpatialComponents = m_entityStructureEditor.GetSelectedSpatialComponents();
        if ( selectedSpatialComponents.size() > 0 )
        {
            int32_t offsetIdx = 0;
            for ( auto pComponent : selectedSpatialComponents )
            {
                if ( pComponent->HasSpatialParent() )
                {
                    auto Comparator = [] ( SpatialEntityComponent const* pComponent, ComponentID const& ID )
                    {
                        return pComponent->GetID() == ID;
                    };

                    if ( VectorContains( selectedSpatialComponents, pComponent->GetSpatialParentID(), Comparator ) )
                    {
                        offsetIdx++;
                        continue;
                    }
                }

                pComponent->SetWorldTransform( m_selectionOffsetTransforms[offsetIdx] * newTransform );
                offsetIdx++;
            }
        }
        else // Update all selected entities
        {
            int32_t offsetIdx = 0;
            auto const selectedEntities = m_outliner.GetSelectedEntities();
            for ( auto pSelectedEntity : selectedEntities )
            {
                if ( !pSelectedEntity->IsSpatialEntity() )
                {
                    continue;
                }

                if ( pSelectedEntity->HasSpatialParent() )
                {
                    if ( VectorContains( selectedEntities, pSelectedEntity->GetSpatialParent() ) )
                    {
                        offsetIdx++;
                        continue;
                    }
                }
                
                pSelectedEntity->SetWorldTransform( m_selectionOffsetTransforms[offsetIdx] * newTransform );
                offsetIdx++;
            }
        }

        m_selectionTransform = newTransform;
        UpdateSelectionSpatialInfo();
    }

    void EntityEditorWorkspace::EndTransformManipulation( Transform const& newTransform )
    {
        ApplyTransformManipulation( newTransform );

        auto pEntityUndoAction = Cast<EntityUndoableAction>( m_pActiveUndoableAction );
        pEntityUndoAction->RecordEndEdit();
        m_undoStack.RegisterAction( m_pActiveUndoableAction );
        m_pActiveUndoableAction = nullptr;
    }
}