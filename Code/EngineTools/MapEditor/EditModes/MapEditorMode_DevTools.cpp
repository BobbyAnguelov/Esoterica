#include "MapEditorMode_DevTools.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "Engine/Render/DebugMesh/DebugMesh.h"
#include "Engine/Render/DebugMesh/DebugMeshRegistry.h"
#include "Base/Math/Intersection.h"
#include "box3d/collision.h"
#include "Engine/Physics/Physics.h"

//-------------------------------------------------------------------------

namespace EE
{
    DeveloperToolsMapEditorMode::~DeveloperToolsMapEditorMode()
    {}

    void DeveloperToolsMapEditorMode::Initialize( EntityModel::EditorContext* pEntityEditorContext )
    {
        EntityModel::MapEditorMode::Initialize( pEntityEditorContext );
    }

    void DeveloperToolsMapEditorMode::Shutdown()
    {
        EntityModel::MapEditorMode::Shutdown();
    }

    void DeveloperToolsMapEditorMode::UpdateAndDraw( UpdateContext const& context, bool isFocused )
    {
        auto pWorld = m_pEntityEditorContext->GetWorld();

        // Draw Mode Selector
        //-------------------------------------------------------------------------

        char const* const labels[] = { "None", "Debug Mesh Tests", "Intersection" };

        int32_t currentMode = (int32_t) m_mode;
        ImGui::SetNextItemWidth( -1 );
        if ( ImGui::Combo( "##ModeSelector", &currentMode, labels, sizeof( labels ) / sizeof( char const* ) ) )
        {
            m_mode = (Mode) currentMode;
        }

        // Draw Mode
        //-------------------------------------------------------------------------

        switch ( m_mode )
        {
            case Mode::DebugMeshTests:
            {
                UpdateAndDraw_DebugMeshTests( context, isFocused );
            }
            break;

            case Mode::IntersectionTests:
            {
                UpdateAndDraw_IntersectionTests( context, isFocused );
            }
            break;

            default:
            break;
        }
    }

    void DeveloperToolsMapEditorMode::DrawViewportOverlayElements( UpdateContext const& context, Viewport const* pViewport, bool isViewportHovered, bool isViewportFocused )
    {
        switch ( m_mode )
        {
            case Mode::DebugMeshTests:
            {
                DrawViewportOverlayElements_DebugMeshTests( context, pViewport );
            }
            break;

            default:
            break;
        }
    }

    bool DeveloperToolsMapEditorMode::AllowDefaultViewportInteractions() const
    {
        switch ( m_mode )
        {
            case Mode::DebugMeshTests:
            case Mode::IntersectionTests:
            default:
            break;
        }

        return EntityModel::MapEditorMode::AllowDefaultViewportInteractions();
    }

    bool DeveloperToolsMapEditorMode::AllowDefaultViewportOverlayElements() const
    {
        switch ( m_mode )
        {
            case Mode::DebugMeshTests:
            {
                return true;
            }
            break;

            case Mode::IntersectionTests:
            {
                return true;
            }
            break;


            default:
            break;
        }

        return EntityModel::MapEditorMode::AllowDefaultViewportOverlayElements();
    }

    // Debug Mesh
    //-------------------------------------------------------------------------

    void DeveloperToolsMapEditorMode::UpdateAndDraw_DebugMeshTests( UpdateContext const& context, bool isFocused )
    {
        // Nothing for now
    }

    void DeveloperToolsMapEditorMode::DrawViewportOverlayElements_DebugMeshTests( UpdateContext const& context, Viewport const* pViewport )
    {
        auto drawingCtx = GetDebugDrawContext();
        PickingID pickingID = GetPickingID();

        auto DrawPickable = [pickingID, &drawingCtx] ( uint64_t hitTestID, DebugMeshID meshID, Transform const& transform, bool wireframe = false )
        {
            Color color = Colors::Red.GetAlphaVersion( 0.75f );
            if ( pickingID.IsSet() && pickingID.m_primaryID == hitTestID && !pickingID.HasSecondaryID() )
            {
                color = Colors::Green;
            }

            drawingCtx.SetHitTestID( hitTestID );
            if ( wireframe )
            {
                drawingCtx.DrawWireMesh( meshID, transform, color );
            }
            else
            {
                drawingCtx.DrawMesh( meshID, transform, color );
            }
            drawingCtx.ClearHitTestID();
        };

        DrawPickable( 1, DebugMeshID::Box, Transform( Quaternion::Identity, Vector( 0, -2, 3 ) ) );
        DrawPickable( 2, DebugMeshID::Sphere, Transform( Quaternion::Identity, Vector( 3, -2, 3 ) ) );
        DrawPickable( 3, DebugMeshID::Hemisphere, Transform( Quaternion::Identity, Vector( 6, -2, 3 ) ) );
        DrawPickable( 4, DebugMeshID::Cylinder, Transform( Quaternion::Identity, Vector( 9, -2, 3 ) ) );
        DrawPickable( 5, DebugMeshID::OpenCylinder, Transform( Quaternion::Identity, Vector( 12, -2, 3 ) ) );
        
        DrawPickable( 6, DebugMeshID::Box, Transform( Quaternion::Identity, Vector( 0, -2, 0 ) ), true );
        DrawPickable( 7, DebugMeshID::Sphere, Transform( Quaternion::Identity, Vector( 3, -2, 0 ) ), true );
        DrawPickable( 8, DebugMeshID::Hemisphere, Transform( Quaternion::Identity, Vector( 6, -2, 0 ) ), true );
        DrawPickable( 9, DebugMeshID::Cylinder, Transform( Quaternion::Identity, Vector( 12, -2, 0 ) ), true );

        //-------------------------------------------------------------------------

        drawingCtx.DrawCapsule( Transform( Quaternion::Identity, Vector( 14, -2, 0 ) ), 0.5f, 1.0f, Colors::Blue );

        //-------------------------------------------------------------------------

        //auto pRegistry = context.GetSystem<Render::DebugMeshRegistry>();
        //Render::DebugMesh const* pDebugMeshHemisphere = pRegistry->GetDebugMesh( (uint64_t) DebugMeshID::Hemisphere );
        //Render::DebugMesh const* pDebugMeshCylinder = pRegistry->GetDebugMesh( (uint64_t) DebugMeshID::OpenCylinder );
        //Render::DebugMesh const* pDebugMeshSphere = pRegistry->GetDebugMesh( (uint64_t) DebugMeshID::Sphere );

        //float radius = 0.4f;
        //float halfHeight = 0.5f;
        //Matrix m = Matrix::Identity;
        //m.SetScale( Vector( radius, radius, halfHeight ) );

        /*for ( auto const& vert : pDebugMeshHemisphere->m_vertices )
        {
            drawingCtx.DrawLine( vert.m_pos, vert.m_pos + vert.m_normal, Colors::YellowGreen );
        }*/

        /*for ( auto const& vert : pDebugMeshHemisphere->m_vertices )
        {
            drawingCtx.DrawLine( vert.m_pos, vert.m_pos + vert.m_normal, Colors::YellowGreen );
        }*/


       /* drawingCtx.DrawBox( Transform( Quaternion::Identity, Vector( 0, -4, 3 ) ), Vector( 1, 0.2f, 0.5f ), Colors::Red.GetAlphaVersion( 0.5f ) );

        drawingCtx.DrawBox( Transform( Quaternion::Identity, Vector( 3, -4, 3 ) ), Vector( 1, 0.2f, 0.5f ), Colors::Red.GetAlphaVersion( 0.5f ) );

        drawingCtx.DrawCylinder( Transform( Quaternion::Identity, Vector( 6, -4, 3 ) ), 0.3f, 0.75f, Colors::Green.GetAlphaVersion( 0.5f ) );


        Vector capsuleOrigin = Vector( 9, -4, 3 );
        drawingCtx.DrawCapsule( Transform( Quaternion::Identity, capsuleOrigin ), 0.3f, 0.75f, Colors::Blue.GetAlphaVersion( 0.25f ) );
        drawingCtx.DrawLine( capsuleOrigin - Vector( 0, 0, 1.05f ), capsuleOrigin + Vector( 0, 0, 1.05f ), Colors::Gold, 1.0f );
        drawingCtx.DrawLine( capsuleOrigin - Vector( 0, 0, 0.75f ), capsuleOrigin + Vector( 0, 0, 0.75f ), Colors::Red, 1.0f );

        drawingCtx.DrawPoint( Vector( 11, -4, 3 ), Colors::Yellow );
        drawingCtx.DrawMesh( MeshIDs::Hemisphere, Transform( Quaternion::Identity, Vector( 11, -4, 3 ) ) );*/

        //-------------------------------------------------------------------------

        //Quaternion q = Quaternion::Identity;// ( EulerAngles( 34, 45, 0 ) );
        //Vector v = Vector::Zero;

        //Render::DebugMesh mesh;
        //Render::DebugMesh::CreateHemisphere( q, v, 1, mesh );

        //for ( int32_t i = 0; i < mesh.m_vertices.size(); i += 3 )
        //{
        //    Render::DebugMesh::Vertex v0 = mesh.m_vertices[i];
        //    Render::DebugMesh::Vertex v1 = mesh.m_vertices[i + 1];
        //    Render::DebugMesh::Vertex v2 = mesh.m_vertices[i + 2];

        //    drawingCtx.DrawLine( v0.m_pos, v1.m_pos, Colors::Red );
        //    drawingCtx.DrawLine( v1.m_pos, v2.m_pos, Colors::Green );
        //    drawingCtx.DrawLine( v2.m_pos, v0.m_pos, Colors::Blue );
        //}
    }

    void DeveloperToolsMapEditorMode::UpdateAndDraw_IntersectionTests( UpdateContext const& context, bool isFocused )
    {
        auto drawingCtx = GetDebugDrawContext();

        //-------------------------------------------------------------------------
        // AABB
        //-------------------------------------------------------------------------

        //Transform bT( Quaternion::Identity/*( EulerAngles( 10, 35, 0 ) )*/, Vector( 0, -2, 2 ) );

        //Vector p0 = bT.TransformPoint( Vector( -0.5f ) );
        //Vector p1 = bT.TransformPoint( Vector( 0.5f ) );

        //AABB box = AABB::FromMinMax( Vector::Min( p0, p1 ), Vector::Max( p0, p1 ) );

        ////-------------------------------------------------------------------------

        //Vector rStart = Vector( -1.2f, -5.f, 2.f );
        //Vector rEnd = Vector( 1.1f, 1.f, 2.f );
        //Ray r0( Ray::CtorStartEnd(), rStart, rEnd );
        //auto hitBox = Math::IntersectRayBox( r0, box );

        //drawingCtx.DrawLine( r0.GetStartPoint(), r0.GetStartPoint() + ( r0.GetDirection() * 10000 ), Colors::HotPink, 2.0f );
        //drawingCtx.DrawWireBox( box, hitBox ? Colors::Red : Colors::Blue );

        //if ( hitBox )
        //{
        //    drawingCtx.DrawPoint( r0.GetStartPoint() + ( r0.GetDirection() * hitBox.m_T ), Colors::Gold, 10 );
        //}

        ////-------------------------------------------------------------------------
        // Sphere
        //-------------------------------------------------------------------------

        //Vector raySphereStart( 2.29216003f, -6.37393999f, 0.899999976f );
        Vector raySphereDir( -0.289096773f, -0.953421950f, 0.0860790610f );
        Vector raySphereStart( 2.29216003f, -6.37393999f, 0.899999976f );
        
        raySphereStart = Vector::MultiplyAdd( raySphereDir, Vector( 8 ), raySphereStart );

        Vector sphere( -0.0497840717f, -14.0975161f, 1.59731781f );
        float radius = 0.161106005f;

        Ray rs( raySphereStart, raySphereDir );
        auto hitSphere = Math::IntersectRaySphere( rs, sphere, radius );

        drawingCtx.DrawLine( rs.GetStartPoint(), rs.GetStartPoint() + ( rs.GetDirection() * 10000 ), Colors::Cyan, 1.0f );
        drawingCtx.DrawWireSphere( sphere, radius, hitSphere ? Colors::Red : Colors::Blue );

        if ( hitSphere )
        {
            drawingCtx.DrawPoint( rs.GetStartPoint() + ( rs.GetDirection() * hitSphere.m_intersectionDistance0 ), Colors::Gold, 10 );

            if ( hitSphere.m_result == Math::ConvexIntersectResult::Double )
            {
                drawingCtx.DrawPoint( rs.GetStartPoint() + ( rs.GetDirection() * hitSphere.m_intersectionDistance1 ), Colors::Gold, 10 );
            }
        }

        ////-------------------------------------------------------------------------
        //// Capsule
        ////-------------------------------------------------------------------------

        //Vector c0( 1, 1, 2 );
        //Vector c1( 10, 10, 2 );
        //float radius = 0.4f;

        //Ray r1( Vector( 3.2f, 2.3f, 2.f ), Vector( -0.5f, 1.f, 0.f ).GetNormalized3() );
        //auto hitCapsule = Math::IntersectRayCapsule( r1, c0, c1, radius );

        //drawingCtx.DrawLine( r1.GetStartPoint(), r1.GetStartPoint() + ( r1.GetDirection() * 10000 ), Colors::Cyan, 1.0f );
        //drawingCtx.DrawWireCapsule( c0, c1, radius, hitCapsule ? Colors::Red : Colors::Blue );

        //if ( hitCapsule )
        //{
        //    drawingCtx.DrawPoint( r1.GetStartPoint() + ( r1.GetDirection() * hitCapsule.m_T ), Colors::Gold, 10 );
        //}

        //-------------------------------------------------------------------------
        // OBB
        //-------------------------------------------------------------------------

        /*OBB obb( Vector( 1, 1, 1 ), Vector( 0.5f, 0.25f, 0.65f ), Quaternion( EulerAngles( -67, -37.5f, 45.0f ) ) );

        Ray r2( Vector( 3.0f, -3.0f, 1.f ), Vector( -0.5f, 1.f, 0.f ).GetNormalized3() );
        auto hitOBB = Math::IntersectRayBox( r2, obb );

        drawingCtx.DrawLine( r2.GetStartPoint(), r2.GetStartPoint() + ( r2.GetDirection() * 10000 ), Colors::Cyan, 1.0f );
        drawingCtx.DrawLitBox( obb, hitOBB ? Colors::Red : Colors::Blue );

        if ( hitOBB )
        {
            drawingCtx.DrawPoint( r2.GetStartPoint() + ( r2.GetDirection() * hitOBB.m_T ), Colors::Gold, 10 );
        }*/
    }
}