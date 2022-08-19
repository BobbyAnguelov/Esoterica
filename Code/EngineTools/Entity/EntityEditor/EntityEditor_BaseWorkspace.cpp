#include "EntityEditor_BaseWorkspace.h"

#include "EngineTools/Entity/EntitySerializationTools.h"
#include "Engine/Navmesh/Systems/WorldSystem_Navmesh.h"
#include "Engine/Navmesh/DebugViews/DebugView_Navmesh.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/PhysicsLayers.h"
#include "Engine/Physics/PhysicsMesh.h"
#include "Engine/Entity/EntitySystem.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Math/Math.h"
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
    EntityEditorBaseWorkspace::EntityEditorBaseWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
        : Workspace( pToolsContext, pWorld, resourceID )
        , m_context( pToolsContext, pWorld, m_undoStack )
        , m_entityOutliner( m_context )
        , m_entityInspector( m_context )
        , m_propertyGrid( m_context )
    {
        m_gizmo.SetTargetTransform( &m_gizmoTransform );
    }

    EntityEditorBaseWorkspace::EntityEditorBaseWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, String const& displayName )
        : Workspace( pToolsContext, pWorld, displayName )
        , m_context( pToolsContext, pWorld, m_undoStack )
        , m_entityOutliner( m_context )
        , m_entityInspector( m_context )
        , m_propertyGrid( m_context )
    {
        m_gizmo.SetTargetTransform( &m_gizmoTransform );
    }

    //-------------------------------------------------------------------------

    void EntityEditorBaseWorkspace::Initialize( UpdateContext const& context )
    {
        Workspace::Initialize( context );

        m_volumeTypes = context.GetSystem<TypeSystem::TypeRegistry>()->GetAllDerivedTypes( VolumeComponent::GetStaticTypeID(), false, false, true );

        m_outlinerWindowName.sprintf( "Map Outliner##%u", GetID() );
        m_entityViewWindowName.sprintf( "Entity Inspector##%u", GetID() );
        m_propertyGridWindowName.sprintf( "Properties##%u", GetID() );
    }

    void EntityEditorBaseWorkspace::Shutdown( UpdateContext const& context )
    {
        Workspace::Shutdown( context );
    }

    void EntityEditorBaseWorkspace::InitializeDockingLayout( ImGuiID dockspaceID ) const
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
        ImGui::DockBuilderDockWindow( m_outlinerWindowName.c_str(), bottomLeftDockID );
        ImGui::DockBuilderDockWindow( m_entityViewWindowName.c_str(), bottomCenterDockID );
        ImGui::DockBuilderDockWindow( m_propertyGridWindowName.c_str(), bottomRightDockID );
    }

    //-------------------------------------------------------------------------

    void EntityEditorBaseWorkspace::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        m_context.OnUndoRedo( operation, pAction );
    }

    void EntityEditorBaseWorkspace::OnDragAndDrop( Render::Viewport* pViewport )
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( "ResourceFile", ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            InlineString payloadStr = (char*) payload->Data;

            ResourceID const resourceID( payloadStr.c_str() );
            if ( !resourceID.IsValid() || !m_pToolsContext->m_pResourceDatabase->DoesResourceExist( resourceID ) )
            {
                return;
            }

            if ( !payload->IsDelivery() )
            {
                return;
            }

            // Calculate world position for new entity
            //-------------------------------------------------------------------------

            Physics::Scene* pPhysicsScene = m_pWorld->GetWorldSystem<Physics::PhysicsWorldSystem>()->GetScene();

            Vector const WSNear = pViewport->ScreenSpaceToWorldSpaceNearPlane( ImGui::GetMousePos() - ImGui::GetWindowPos() );
            Vector const WSFar = pViewport->ScreenSpaceToWorldSpaceFarPlane( ImGui::GetMousePos() - ImGui::GetWindowPos() );
            Vector worldPosition = Vector::Zero;

            Physics::RayCastResults results;
            Physics::QueryFilter filter( Physics::CreateLayerMask( Physics::Layers::Environment ) );
            pPhysicsScene->AcquireReadLock();
            if ( pPhysicsScene->RayCast( WSNear, WSFar, filter, results ) )
            {
                worldPosition = results.GetHitPosition();
            }
            else // Arbitrary position
            {
                worldPosition = WSNear + ( ( WSFar - WSNear ).GetNormalized3() * 10.0f );
            }
            pPhysicsScene->ReleaseReadLock();

            // Try add resource to map
            //-------------------------------------------------------------------------

            DropResourceInViewport( resourceID, worldPosition );
        }
    }

    void EntityEditorBaseWorkspace::DropResourceInViewport( ResourceID const& resourceID, Vector const& worldPosition )
    {
        // Static Mesh Resource
        //-------------------------------------------------------------------------

        if ( resourceID.GetResourceTypeID() == Render::StaticMesh::GetStaticResourceTypeID() )
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
            m_context.AddEntity( pMeshEntity );
        }
    }

    void EntityEditorBaseWorkspace::OnMousePick( Render::PickingID pickingID )
    {
        EntityID const pickedEntityID( pickingID.m_0 );
        auto pEntity = m_pWorld->FindEntity( pickedEntityID );
        if ( pEntity == nullptr )
        {
            return;
        }

        // If we have ctrl-held, select the individual component
        if ( ImGui::GetIO().KeyAlt )
        {
            ComponentID const pickedComponentID( pickingID.m_1 );
            auto pComponent = pEntity->FindComponent( pickedComponentID );
            EE_ASSERT( pComponent != nullptr );
            m_context.SelectEntity( pEntity );
            m_context.SelectComponent( pComponent );
        }
        else if ( ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl )
        {
            if ( m_context.IsSelected( pEntity ) )
            {
                m_context.RemoveFromSelectedEntities( pEntity );
            }
            else
            {
                m_context.AddToSelectedEntities( pEntity );
            }
        }
        else
        {
            m_context.SelectEntity( pEntity );
        }
    }

    //-------------------------------------------------------------------------

    void EntityEditorBaseWorkspace::DrawViewportToolbarItems( UpdateContext const& context, Render::Viewport const* pViewport )
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
    }

    void EntityEditorBaseWorkspace::DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        auto drawingCtx = GetDrawingContext();
        auto pEditedMap = m_context.GetMap();

        // Overlay Widgets
        //-------------------------------------------------------------------------

        auto DrawComponentWorldIcon = [this, pViewport] ( SpatialEntityComponent* pComponent, char icon[4], bool isSelected )
        {
            if ( pViewport->IsWorldSpacePointVisible( pComponent->GetPosition() ) )
            {
                ImVec2 const positionScreenSpace = pViewport->WorldSpaceToScreenSpace( pComponent->GetPosition() );
                if ( ImGuiX::DrawOverlayIcon( positionScreenSpace, icon, pComponent, isSelected, ImColor( Colors::Yellow.ToFloat4() ) ) )
                {
                    return true;
                }
            }

            return false;
        };

        auto const& registeredLights = m_pWorld->GetAllRegisteredComponentsOfType<Render::LightComponent>();
        for ( auto pComponent : registeredLights )
        {
            auto pLightComponent = const_cast<Render::LightComponent*>( pComponent );
            bool const isSelected = m_context.HasSingleSelectedComponent() && m_context.IsSelected( pComponent );
            if ( DrawComponentWorldIcon( pLightComponent, EE_ICON_LIGHTBULB, isSelected ) )
            {
                auto pEntity = pEditedMap->FindEntity( pLightComponent->GetEntityID() );
                m_context.SelectEntity( pEntity );
                m_context.SelectComponent( pLightComponent );
            }
        }

        auto const& registeredPlayerSpawns = m_pWorld->GetAllRegisteredComponentsOfType<Player::PlayerSpawnComponent>();
        for ( auto pComponent : registeredPlayerSpawns )
        {
            auto pSpawnComponent = const_cast<Player::PlayerSpawnComponent*>( pComponent );
            bool const isSelected = m_context.HasSingleSelectedComponent() && m_context.IsSelected( pComponent );
            if ( DrawComponentWorldIcon( pSpawnComponent, EE_ICON_GAMEPAD, isSelected ) )
            {
                auto pEntity = pEditedMap->FindEntity( pSpawnComponent->GetEntityID() );
                m_context.SelectEntity( pEntity );
                m_context.SelectComponent( pSpawnComponent );
            }

            auto const& spawnTransform = pComponent->GetWorldTransform();
            drawingCtx.DrawArrow( spawnTransform.GetTranslation(), spawnTransform.GetForwardVector(), 0.5f, Colors::Yellow, 3.0f );
        }

        auto const& registeredAISpawns = m_pWorld->GetAllRegisteredComponentsOfType<AI::AISpawnComponent>();
        for ( auto pComponent : registeredAISpawns )
        {
            auto pSpawnComponent = const_cast<AI::AISpawnComponent*>( pComponent );
            bool const isSelected = m_context.HasSingleSelectedComponent() && m_context.IsSelected( pComponent );
            if ( DrawComponentWorldIcon( pSpawnComponent, EE_ICON_ROBOT, isSelected ) )
            {
                auto pEntity = pEditedMap->FindEntity( pSpawnComponent->GetEntityID() );
                m_context.SelectEntity( pEntity );
                m_context.SelectComponent( pSpawnComponent );
            }

            auto const& spawnTransform = pComponent->GetWorldTransform();
            drawingCtx.DrawArrow( spawnTransform.GetTranslation(), spawnTransform.GetForwardVector(), 0.5f, Colors::Yellow, 3.0f );
        }

        // Draw light debug widgets
        //-------------------------------------------------------------------------
        // TODO: generalized component visualizers

        if ( m_context.HasSingleSelectedComponent() )
        {
            auto pSelectedComponent = m_context.GetSelectedComponent();
            if ( IsOfType<Render::DirectionalLightComponent>( pSelectedComponent ) )
            {
                auto pLightComponent = Cast<Render::DirectionalLightComponent>( pSelectedComponent );
                auto forwardDir = pLightComponent->GetForwardVector();
                drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() + forwardDir, pLightComponent->GetLightColor(), 3.0f );
            }
            else if ( IsOfType<Render::SpotLightComponent>( pSelectedComponent ) )
            {
                auto pLightComponent = Cast<Render::SpotLightComponent>( pSelectedComponent );
                drawingCtx.DrawCone( pLightComponent->GetWorldTransform(), pLightComponent->GetLightInnerUmbraAngle(), 1.5f, pLightComponent->GetLightColor(), 3.0f );
                drawingCtx.DrawCone( pLightComponent->GetWorldTransform(), pLightComponent->GetLightOuterUmbraAngle(), 1.5f, pLightComponent->GetLightColor(), 3.0f );
            }
            else if ( IsOfType<Render::PointLightComponent>( pSelectedComponent ) )
            {
                auto pLightComponent = Cast<Render::PointLightComponent>( pSelectedComponent );
                auto forwardDir = pLightComponent->GetForwardVector();
                auto upDir = pLightComponent->GetUpVector();
                auto rightDir = pLightComponent->GetRightVector();

                drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() + ( forwardDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
                drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() - ( forwardDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
                drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() + ( upDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
                drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() - ( upDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
                drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() + ( rightDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
                drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() - ( rightDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
            }
        }

        // Volume Debug
        //-------------------------------------------------------------------------

        for ( auto pVisualizedVolumeTypeInfo : m_visualizedVolumeTypes )
        {
            auto const& volumeComponents = m_pWorld->GetAllRegisteredComponentsOfType( pVisualizedVolumeTypeInfo->m_ID );
            for ( auto pComponent : volumeComponents )
            {
                auto pVolumeComponent = static_cast<VolumeComponent const*>( pComponent );
                pVolumeComponent->Draw( drawingCtx );
            }
        }

        // Selection and Manipulation
        //-------------------------------------------------------------------------

        if ( m_context.HasSpatialSelection() )
        {
            // Draw selection bounds
            //-------------------------------------------------------------------------

            for ( auto pEntity : m_context.GetSelectedEntities() )
            {
                drawingCtx.DrawWireBox( pEntity->GetCombinedWorldBounds(), Colors::Yellow, 3.0f, Drawing::EnableDepthTest );
            }

            drawingCtx.DrawWireBox( m_context.GetSelectionBounds(), Colors::Yellow, 3.0f, Drawing::EnableDepthTest );

            // Update Gizmo
            //-------------------------------------------------------------------------

            if ( m_gizmo.GetMode() == ImGuiX::Gizmo::GizmoMode::None )
            {
                m_gizmo.SwitchMode( ImGuiX::Gizmo::GizmoMode::Translation );
            }

            m_gizmoTransform = m_context.GetSelectionTransform();

            switch ( m_gizmo.Draw( *m_pWorld->GetViewport() ) )
            {
                case ImGuiX::Gizmo::Result::StartedManipulating:
                {
                    bool const duplicateSelection = m_gizmo.GetMode() == ImGuiX::Gizmo::GizmoMode::Translation && ImGui::GetIO().KeyAlt;
                    m_context.BeginTransformManipulation( m_gizmoTransform, duplicateSelection );
                }
                break;

                case ImGuiX::Gizmo::Result::Manipulating:
                {
                    m_context.ApplyTransformManipulation( m_gizmoTransform );
                }
                break;

                case ImGuiX::Gizmo::Result::StoppedManipulating:
                {
                    m_context.EndTransformManipulation( m_gizmoTransform );
                }
                break;
            }
        }
    }

    void EntityEditorBaseWorkspace::Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        m_context.Update( context );

        //-------------------------------------------------------------------------

        if ( m_isViewportFocused || m_isViewportHovered )
        {
            if ( ImGui::IsKeyPressed( ImGuiKey_Space ) )
            {
                m_gizmo.SwitchToNextMode();
            }

            if ( ImGui::IsKeyPressed( ImGuiKey_Delete ) )
            {
                m_context.DestroySelectedEntities();
            }
        }

        //-------------------------------------------------------------------------

        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_outlinerWindowName.c_str() ) )
        {
            m_entityOutliner.Draw( context );
        }
        ImGui::End();

        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_entityViewWindowName.c_str() ) )
        {
            m_entityInspector.Draw( context );
        }
        ImGui::End();

        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_propertyGridWindowName.c_str() ) )
        {
            m_propertyGrid.Draw( context );
        }
        ImGui::End();
    }
}