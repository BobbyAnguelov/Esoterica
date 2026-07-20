#include "MapEditorMode_Physics.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/Settings/ViewportSettings_Physics.h"
#include "Base/Serialization/TypeSerialization.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    void PhysicsMapEditorMode::Initialize( EntityModel::EditorContext* pEntityEditorContext )
    {
        EntityModel::MapEditorMode::Initialize( pEntityEditorContext );

        m_pPropertyGrid = EE::New<PropertyGrid>( m_pEntityEditorContext->m_pToolsContext );
        m_pPropertyGrid->SetTypeToEdit( &m_settings );
        m_pPropertyGrid->SetControlBarVisible( false );

        m_startGizmo.SetOption( ImGuiX::Gizmo::Options::AllowScale, false );
        m_endGizmo.SetOption( ImGuiX::Gizmo::Options::AllowScale, false );

        //-------------------------------------------------------------------------

        auto pWorld = m_pEntityEditorContext->GetWorld();
        auto pViewport = pWorld->GetMainViewport();
        if ( pViewport != nullptr )
        {
            PhysicsViewportSettings* pViewportSettings = pViewport->GetViewportSettings<PhysicsViewportSettings>();
            pViewportSettings->m_drawDebug = true;
            pViewportSettings->m_drawShapes = true;
        }

        //-------------------------------------------------------------------------

        PasteTransformsFromString( "<Type TypeID=\"EE::Physics::PhysicsMapEditorMode::Settings\"><Property ID = \"m_sweepStart\" Value = \"0,-0,0,-8.20652,24.5309,2.71846,1\" /><Property ID = \"m_sweepEnd\" Value = \"0,-0,0,1.67452,0,2.64756,1\"/></Type>" );
    }

    void PhysicsMapEditorMode::Shutdown()
    {
        auto pWorld = m_pEntityEditorContext->GetWorld();
        auto pViewport = pWorld->GetMainViewport();
        if ( pViewport != nullptr )
        {
            PhysicsViewportSettings* pViewportSettings = pViewport->GetViewportSettings<PhysicsViewportSettings>();
            pViewportSettings->m_drawDebug = false;
        }

        EE::Delete( m_pPropertyGrid );

        EntityModel::MapEditorMode::Shutdown();
    }

    void PhysicsMapEditorMode::UpdateAndDraw( UpdateContext const& context, bool isFocused )
    {
        auto pWorld = m_pEntityEditorContext->GetWorld();

        // Draw Mode (controls)
        //-------------------------------------------------------------------------

        constexpr static char const* const queryTypeLabels[] = { "Overlap Test", "Shape Cast" };
        int32_t currentTest = m_settings.m_isCastQuery ? 1 : 0;
        ImGui::SetNextItemWidth( -1 );
        if ( ImGui::Combo( "##TestSelector", &currentTest, queryTypeLabels, 2 ) )
        {
            m_settings.m_isCastQuery = currentTest != 0;

            if ( !m_settings.m_isCastQuery && m_settings.m_shapeType == ShapeType::Ray )
            {
                m_settings.m_shapeType = ShapeType::Sphere;
            }
        }

        // Add copy paste buttons for the sweep coords
        //-------------------------------------------------------------------------

        float const buttonWidth = ( ImGui::GetContentRegionAvail().x / 2 ) - ImGui::GetStyle().ItemSpacing.x;
        if ( ImGui::Button( EE_ICON_CONTENT_COPY" Copy Settings", ImVec2( buttonWidth, 0 ) ) )
        {
            ImGui::SetClipboardText( CopyTransformsToString().c_str() );
        }

        ImGui::SameLine();

        if ( ImGui::Button( EE_ICON_CONTENT_PASTE" Paste Settings", ImVec2( -1, 0 ) ) )
        {
            String str = ImGui::GetClipboardText();
            PasteTransformsFromString( str );
        }

        // Draw shape controls
        //-------------------------------------------------------------------------

        constexpr static char const* const shapeTypeLabels[] = { "Ray", "Box", "Capsule", "Cylinder", "Sphere" };

        char const* const* pLabels = ( m_settings.m_isCastQuery ? shapeTypeLabels : &shapeTypeLabels[1] );
        int32_t const numLabels = sizeof( shapeTypeLabels ) / sizeof( char const* ) - ( m_settings.m_isCastQuery ? 0 : 1 );

        ImGui::SeparatorText( "Shape Settings" );

        int32_t currentType = (int32_t) m_settings.m_shapeType - ( m_settings.m_isCastQuery ? 0 : 1 );
        ImGui::SetNextItemWidth( -1 );
        if ( ImGui::Combo( "##TypeSelector", &currentType, pLabels, numLabels ) )
        {
            m_settings.m_shapeType = (ShapeType) currentType;
        }

        if ( m_settings.m_shapeType == ShapeType::Box )
        {
            DrawBoxSettings( context, isFocused );
        }
        else if ( m_settings.m_shapeType == ShapeType::Sphere )
        {
            DrawSphereSettings( context, isFocused );
        }
        else if ( m_settings.m_shapeType == ShapeType::Capsule || m_settings.m_shapeType == ShapeType::Cylinder )
        {
            DrawCapsuleAndCylinderSettings( context, isFocused );
        }

        // Collision Settings
        //-------------------------------------------------------------------------

        ImGui::SeparatorText( "Collision Settings" );

        m_pPropertyGrid->UpdateAndDraw();

        // UI
        //-------------------------------------------------------------------------

        if ( m_settings.m_isCastQuery )
        {
            UpdateAndDrawShapeCast( context, isFocused );
        }
        else
        {
            UpdateAndDrawOverlap( context, isFocused );
        }
    }

    void PhysicsMapEditorMode::DrawBoxSettings( UpdateContext const& context, bool isFocused )
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Extents: " );
        ImGui::SameLine();
        if ( ImGuiX::InputFloat3( "###F", m_settings.m_extents ) )
        {
            m_settings.m_extents.m_x = Math::Abs( m_settings.m_extents.m_x );
            m_settings.m_extents.m_y = Math::Abs( m_settings.m_extents.m_y );
            m_settings.m_extents.m_z = Math::Abs( m_settings.m_extents.m_z );
        }
    }

    void PhysicsMapEditorMode::DrawSphereSettings( UpdateContext const& context, bool isFocused )
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Radius: " );
        ImGui::SameLine();
        ImGui::SetNextItemWidth( -1 );
        if ( ImGui::InputFloat( "###F", &m_settings.m_extents.m_x ) )
        {
            m_settings.m_extents.m_x = Math::Abs( m_settings.m_extents.m_x );
            if ( Math::IsNearZero( m_settings.m_extents.m_x ) )
            {
                m_settings.m_extents.m_x = 0.01f;
            }
        }
    }

    void PhysicsMapEditorMode::DrawCapsuleAndCylinderSettings( UpdateContext const& context, bool isFocused )
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Radius: " );
        ImGui::SameLine();
        ImGui::SetNextItemWidth( -1 );
        if ( ImGui::InputFloat( "###F", &m_settings.m_extents.m_x ) )
        {
            m_settings.m_extents.m_x = Math::Abs( m_settings.m_extents.m_x );
            if ( Math::IsNearZero( m_settings.m_extents.m_x ) )
            {
                m_settings.m_extents.m_x = 0.01f;
            }
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Half Height: " );
        ImGui::SameLine();
        ImGui::SetNextItemWidth( -1 );
        if ( ImGui::InputFloat( "###H", &m_settings.m_extents.m_y ) )
        {
            m_settings.m_extents.m_y = Math::Abs( m_settings.m_extents.m_y );
            if ( Math::IsNearZero( m_settings.m_extents.m_y ) )
            {
                m_settings.m_extents.m_y = 0.01f;
            }
        }
    }

    void PhysicsMapEditorMode::UpdateAndDrawShapeCast( UpdateContext const& context, bool isFocused )
    {
        // Perform Cast
        //-------------------------------------------------------------------------

        auto pPhysicsWorldSystem = m_pEntityEditorContext->GetWorld()->GetWorldSystem<Physics::PhysicsWorldSystem>();
        Physics::PhysicsWorld* pPhysicsWorld = pPhysicsWorldSystem->GetPhysicsWorld();

        m_castQuery.Reset();
        m_castQuery.SetCategory( m_settings.m_collisionSettings.m_category );
        m_castQuery.SetCollisionMask( m_settings.m_collisionSettings.m_collisionMask );
        m_castQuery.SetAllowMultipleHits( true );

        switch ( m_settings.m_shapeType )
        {
            case ShapeType::Ray:
            {
                pPhysicsWorld->RayCast( m_settings.m_sweepStart.GetTranslation(), m_settings.m_sweepEnd.GetTranslation(), m_castQuery );
            }
            break;

            case ShapeType::Sphere:
            {
                pPhysicsWorld->SphereCast( m_settings.m_extents.m_x, m_settings.m_sweepStart.GetTranslation(), m_settings.m_sweepEnd.GetTranslation(), m_castQuery );
            }
            break;

            case ShapeType::Box:
            {
                pPhysicsWorld->BoxCast( m_settings.m_extents, m_settings.m_sweepStart.GetRotation(), m_settings.m_sweepStart.GetTranslation(), m_settings.m_sweepEnd.GetTranslation(), m_castQuery );
            }
            break;

            case ShapeType::Capsule:
            {
                pPhysicsWorld->CapsuleCast( m_settings.m_extents.m_x, m_settings.m_extents.m_y, m_settings.m_sweepStart.GetRotation(), m_settings.m_sweepStart.GetTranslation(), m_settings.m_sweepEnd.GetTranslation(), m_castQuery );
            }
            break;

            case ShapeType::Cylinder:
            {
                pPhysicsWorld->CylinderCast( m_settings.m_extents.m_x, m_settings.m_extents.m_y, m_settings.m_sweepStart.GetRotation(), m_settings.m_sweepStart.GetTranslation(), m_settings.m_sweepEnd.GetTranslation(), m_castQuery );
            }
            break;
        }

        // Draw 3D results
        //-------------------------------------------------------------------------

        auto drawingCtx = GetDebugDrawContext();
        drawingCtx.DrawLine( m_settings.m_sweepStart.GetTranslation(), m_settings.m_sweepEnd.GetTranslation(), m_castQuery.HasHits() ? Colors::Red : Colors::LimeGreen, 5.0f );

        switch ( m_settings.m_shapeType )
        {
            case ShapeType::Ray:
            {
                for ( auto const& hit : m_castQuery.m_hits )
                {
                    drawingCtx.DrawPoint( hit.m_position, Colors::Red, 10 );
                    drawingCtx.DrawLine( hit.m_contactPoint, hit.m_contactPoint + hit.m_contactNormal, Colors::Orange );
                }
            }
            break;

            case ShapeType::Sphere:
            {
                for ( auto const& hit : m_castQuery.m_hits )
                {
                    drawingCtx.DrawSphere( hit.m_position, m_settings.m_extents.m_x, Colors::Red );
                    drawingCtx.DrawPoint( hit.m_contactPoint, Colors::Orange, 10 );
                    drawingCtx.DrawLine( hit.m_contactPoint, hit.m_contactPoint + hit.m_contactNormal, Colors::Orange );
                }

                if ( m_castQuery.m_hits.empty() )
                {
                    drawingCtx.DrawSphere( m_castQuery.m_endPosition, 0.1f, Colors::LimeGreen );
                }
            }
            break;

            case ShapeType::Box:
            {
                for ( auto const& hit : m_castQuery.m_hits )
                {
                    drawingCtx.DrawWireBox( hit.m_position, m_settings.m_sweepStart.GetRotation(), m_settings.m_extents, Colors::Red );
                    drawingCtx.DrawPoint( hit.m_contactPoint, Colors::Orange, 10 );
                    drawingCtx.DrawLine( hit.m_contactPoint, hit.m_contactPoint + hit.m_contactNormal, Colors::Orange );
                }

                if ( m_castQuery.m_hits.empty() )
                {
                    drawingCtx.DrawWireBox( m_castQuery.m_endPosition, m_settings.m_sweepStart.GetRotation(), m_settings.m_extents, Colors::LimeGreen );
                }
            }
            break;

            case ShapeType::Capsule:
            {
                for ( auto const& hit : m_castQuery.m_hits )
                {
                    drawingCtx.DrawCapsule( hit.m_position, m_settings.m_sweepStart.GetRotation(), m_settings.m_extents.m_x, m_settings.m_extents.m_y, Colors::Red );
                    drawingCtx.DrawPoint( hit.m_contactPoint, Colors::Orange, 10 );
                    drawingCtx.DrawLine( hit.m_contactPoint, hit.m_contactPoint + hit.m_contactNormal, Colors::Orange );
                }

                if ( m_castQuery.m_hits.empty() )
                {
                    drawingCtx.DrawCapsule( m_castQuery.m_endPosition, m_settings.m_sweepStart.GetRotation(), m_settings.m_extents.m_x, m_settings.m_extents.m_y, Colors::LimeGreen );
                }
            }
            break;

            case ShapeType::Cylinder:
            {
                for ( auto const& hit : m_castQuery.m_hits )
                {
                    drawingCtx.DrawCylinder( hit.m_position, m_settings.m_sweepStart.GetRotation(), m_settings.m_extents.m_x, m_settings.m_extents.m_y, Colors::Red );
                    drawingCtx.DrawPoint( hit.m_contactPoint, Colors::Orange, 10 );
                    drawingCtx.DrawLine( hit.m_contactPoint, hit.m_contactPoint + hit.m_contactNormal, Colors::Orange );
                }

                if ( m_castQuery.m_hits.empty() )
                {
                    drawingCtx.DrawCylinder( m_castQuery.m_endPosition, m_settings.m_sweepStart.GetRotation(), m_settings.m_extents.m_x, m_settings.m_extents.m_y, Colors::LimeGreen );
                }
            }
            break;
        }

        // Draw Results UI
        //-------------------------------------------------------------------------

        ImGui::SeparatorText( "Shape Cast" );

        ImGui::Text( "Start Pos: " );
        ImGui::SameLine();
        ImGuiX::DrawFloat3( m_castQuery.m_startPosition );

        ImGui::Text( "End Pos: " );
        ImGui::SameLine();
        ImGuiX::DrawFloat3( m_castQuery.m_endPosition );

        ImGui::Text( "Dir: " );
        ImGui::SameLine();
        ImGuiX::DrawFloat3( m_castQuery.m_direction );

        ImGui::Text( "Desired Distance: %.2f", m_castQuery.m_desiredDistance );
        ImGui::Text( "Distance Traveled: %.2f", m_castQuery.m_actualDistance );
        ImGui::Text( "Remaining Distance: %.2f", m_castQuery.m_remainingDistance );

        if ( m_castQuery.m_hasInitialOverlap )
        {
            ImGui::TextColored( Colors::Red, "InitiallyOverlaps" );
        }

        ImGui::SeparatorText( "Hits" );

        if ( m_castQuery.HasHits() )
        {
            for ( auto const& hit : m_castQuery.m_hits )
            {
                ImGui::Text( "Pos: " );
                ImGui::SameLine();
                ImGuiX::DrawFloat3( hit.m_position );

                ImGui::Text( "Contact Pos: " );
                ImGui::SameLine();
                ImGuiX::DrawFloat3( hit.m_contactPoint );

                ImGui::Text( "Contact Normal: " );
                ImGui::SameLine();
                ImGuiX::DrawFloat3( hit.m_contactNormal );

                ImGui::Text( "Surface Normal: " );
                ImGui::SameLine();
                ImGuiX::DrawFloat3( hit.m_surfaceNormal );

                ImGui::Text( "Material: %llu", hit.m_materialID );
                ImGui::Text( "Distance: %.4f", hit.m_distance );

                if ( hit.m_isInitiallyOverlapping )
                {
                    ImGui::TextColored( Colors::Red, "InitiallyOverlaps" );
                }

                ImGui::Separator();
            }
        }
        else
        {
            ImGui::Text( "No Hits" );
        }
    }

    void PhysicsMapEditorMode::UpdateAndDrawOverlap( UpdateContext const& context, bool isFocused )
    {
        // Perform Overlap
        //-------------------------------------------------------------------------

        auto pPhysicsWorldSystem = m_pEntityEditorContext->GetWorld()->GetWorldSystem<Physics::PhysicsWorldSystem>();
        Physics::PhysicsWorld* pPhysicsWorld = pPhysicsWorldSystem->GetPhysicsWorld();

        m_overlapQuery.Reset();
        m_overlapQuery.SetAllowMultipleHits( true );
        m_overlapQuery.SetCollidesWithAll();

        switch ( m_settings.m_shapeType )
        {
            case ShapeType::Sphere:
            {
                pPhysicsWorld->SphereOverlap( m_settings.m_extents.m_x, m_settings.m_sweepStart.GetTranslation(), m_overlapQuery );
            }
            break;

            case ShapeType::Box:
            {
                pPhysicsWorld->BoxOverlap( m_settings.m_extents, m_settings.m_sweepStart.GetRotation(), m_settings.m_sweepStart.GetTranslation(), m_overlapQuery );
            }
            break;

            case ShapeType::Capsule:
            {
                pPhysicsWorld->CapsuleOverlap( m_settings.m_extents.m_x, m_settings.m_extents.m_y, m_settings.m_sweepStart.GetRotation(), m_settings.m_sweepStart.GetTranslation(), m_overlapQuery );
            }
            break;

            case ShapeType::Cylinder:
            {
                pPhysicsWorld->CylinderOverlap( m_settings.m_extents.m_x, m_settings.m_extents.m_y, m_settings.m_sweepStart.GetRotation(), m_settings.m_sweepStart.GetTranslation(), m_overlapQuery );
            }
            break;

            default:
            break;
        }

        // Draw 3D results
        //-------------------------------------------------------------------------

        auto drawingCtx = GetDebugDrawContext();
        Color const resultColor = m_overlapQuery.HasOverlaps() ? Colors::Red : Colors::LimeGreen;

        switch ( m_settings.m_shapeType )
        {
            case ShapeType::Sphere:
            {
                drawingCtx.DrawSphere( m_overlapQuery.m_transform, m_settings.m_extents.m_x, resultColor );
            }
            break;

            case ShapeType::Box:
            {
                drawingCtx.DrawWireBox( m_overlapQuery.m_transform, m_settings.m_extents, resultColor );
            }
            break;

            case ShapeType::Capsule:
            {
                drawingCtx.DrawCapsule( m_overlapQuery.m_transform, m_settings.m_extents.m_x, m_settings.m_extents.m_y, resultColor );
            }
            break;

            case ShapeType::Cylinder:
            {
                drawingCtx.DrawCylinder( m_overlapQuery.m_transform, m_settings.m_extents.m_x, m_settings.m_extents.m_y, resultColor );
            }
            break;

            default:
            break;
        }

        //-------------------------------------------------------------------------

        ImGui::SeparatorText( "Shape Overlap" );

        ImGui::Text( "Transform: " );
        ImGuiX::DrawTransform( m_overlapQuery.m_transform );

        ImGui::SeparatorText( "Overlaps" );

        if ( m_overlapQuery.HasOverlaps() )
        {
            for ( OverlapQuery::Overlap const& overlap : m_overlapQuery.m_overlaps )
            {
                char const* const pBodyName = b3Body_GetName( overlap.m_bodyID );
                char const* const pShapeName = reinterpret_cast<UserData const*>( b3Shape_GetUserData( overlap.m_shapeID ) )->m_name.c_str();

                ImGui::Text( "Body: %s", pBodyName );
                ImGui::Text( "Shape: %s", pShapeName );

                ImGui::Text( "Contact Normal: TODO" );
                ImGui::Text( "Surface Normal: TODO" );

                ImGui::Separator();
            }
        }
        else
        {
            ImGui::Text( "No Overlaps" );
        }
    }

    void PhysicsMapEditorMode::DrawViewportOverlayElements( UpdateContext const& context, Viewport const* pViewport, bool isViewportHovered, bool isViewportFocused )
    {
        m_isInteractingWithPhysicsGizmo = false;

        //-------------------------------------------------------------------------

        if ( ( isViewportHovered || isViewportFocused ) && !m_pEntityEditorContext->HasSpatialSelection() )
        {
            if ( ImGui::IsKeyPressed( ImGuiKey_Space ) )
            {
                m_startGizmo.SwitchToNextMode();
            }
        }

        //-------------------------------------------------------------------------

        ImGuiX::Gizmo::Result startResult = m_startGizmo.Draw( m_settings.m_sweepStart.GetTranslation(), m_settings.m_sweepStart.GetRotation(), *pViewport, m_settings.m_isCastQuery ? "Sweep Start" : "Overlap" );
        startResult.ApplyResult( m_settings.m_sweepStart );
        if ( startResult.m_state == ImGuiX::GizmoState::StartedManipulating )
        {
            m_pEntityEditorContext->ClearSelection();
        }

        if ( startResult.IsManipulating() )
        {
            m_isInteractingWithPhysicsGizmo = true;
        }

        //-------------------------------------------------------------------------

        if ( m_settings.m_isCastQuery )
        {
            ImGuiX::Gizmo::Result endResult = m_endGizmo.Draw( m_settings.m_sweepEnd.GetTranslation(), m_settings.m_sweepEnd.GetRotation(), *pViewport, "Sweep End" );

            if ( !startResult.IsManipulating() )
            {
                endResult.ApplyResult( m_settings.m_sweepEnd );

                if ( endResult.m_state == ImGuiX::GizmoState::StartedManipulating )
                {
                    m_pEntityEditorContext->ClearSelection();
                }
            }

            if ( endResult.IsManipulating() )
            {
                m_isInteractingWithPhysicsGizmo = true;
            }
        }
    }

    String PhysicsMapEditorMode::CopyTransformsToString() const
    {
        String serializedType;
        Serialization::WriteTypeToString( *m_pToolsContext->m_pTypeRegistry, &m_settings, serializedType );
        return serializedType;
    }

    void PhysicsMapEditorMode::PasteTransformsFromString( String const& str )
    {
        Settings pastedSettings;
        Log log;
        if ( Serialization::ReadTypeFromString( *m_pToolsContext->m_pTypeRegistry, log, str, &pastedSettings ) )
        {
            m_settings = pastedSettings;
        }
    }
}