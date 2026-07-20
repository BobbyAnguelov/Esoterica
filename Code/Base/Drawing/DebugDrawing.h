#pragma once

#include "../_Module/API.h"
#include "DebugDrawingCommands.h"
#include "Base/Math/Transform.h"
#include "Base/Types/Color.h"
#include "Base/Math/BoundingVolumes.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    //-------------------------------------------------------------------------
    // Debug Drawing Context
    //-------------------------------------------------------------------------

    // The primary API for debug drawing in EE
    // Drawing contexts are bound to threads, so each worker thread will have it's own drawing context

    // We have helper functions to allow you to draw custom types in the same manner as the basic primitive:
    // i.e. drawingContext.Draw( MyType const& type )
    // To support this, you need to implement one of the following functions in your type:
    // 1) 'void DrawDebug( DebugDrawContext& ctx ) const'
    // 2) 'void DrawDebug( DebugDrawContext& ctx, Transform const& worldTransform ) const'

    // Note: We use float4 for colors here since EE::Color will implicitly convert to a float4. 
    // We use float4s for color in the rendering code, so this allows you to save on constantly paying the conversion cost when drawing lots of primitives

    class EE_BASE_API DebugDrawContext
    {
        friend class DebugDrawSystem;

        constexpr static float const s_defaultLineThickness = 1.0f;
        constexpr static float const s_defaultPointThickness = 5.0f;

    public:

        inline DebugDrawContext( DebugDrawContext const& RHS ) : m_commandBuffer( RHS.m_commandBuffer ) {}

        //-------------------------------------------------------------------------

        template<typename T>
        inline void Draw( T const& type )
        {
            type.DrawDebug( *this );
        }

        template<typename T>
        inline void Draw( T const& type, Transform const& worldTransform )
        {
            type.DrawDebug( *this, worldTransform );
        }

        //-------------------------------------------------------------------------
        // Hit Testing
        //-------------------------------------------------------------------------

        // Set a hit test ID so that we can use picking to detect when we hover or click on debug draw shapes
        void SetHitTestID( uint64_t ID ) { m_commandBuffer.SetHitTestID( ID ); }

        // Get the currently set hit test ID, return 0 if not ID set
        uint64_t GetActiveHitTestID() const { return m_commandBuffer.GetActiveHitTestID(); }

        // Clear the currently set hit-test ID
        void ClearHitTestID() { m_commandBuffer.ClearHitTestID(); }

        //-------------------------------------------------------------------------
        // Basic Primitives
        //-------------------------------------------------------------------------

        inline void DrawPoint( Float3 const& position, Float4 const& color, float thickness = s_defaultPointThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawPoint( m_commandBuffer, position, color, thickness, layer, TTL );
        }

        inline void DrawLine( Float3 const& startPosition, Float3 const& endPosition, Float4 const& color, float lineThickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawLine( m_commandBuffer, startPosition, endPosition, color, lineThickness, layer, TTL );
        }

        inline void DrawLine( Float3 const& startPosition, Float3 const& endPosition, Float4 const& startColor, Float4 const& endColor, float lineThickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawLine( m_commandBuffer, startPosition, endPosition, startColor, endColor, lineThickness, layer, TTL );
        }

        inline void DrawTriangle( Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawTriangle( m_commandBuffer, v0, v1, v2, color, color, color, layer, TTL );
        }

        inline void DrawTriangle( Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color0, Float4 const& color1, Float4 const& color2, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawTriangle( m_commandBuffer, v0, v1, v2, color0, color1, color2, layer, TTL );
        }

        void DrawWireTriangle( Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color, float lineThickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 );

        //-------------------------------------------------------------------------
        // Text
        //-------------------------------------------------------------------------

        inline void DrawText2D( Float2 const& screenPosition, char const* pText, Float4 const& color, DebugFont font = DebugFont::Normal, DebugTextAlign alignment = DebugTextAlign::MiddleLeft, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawText2D( m_commandBuffer, screenPosition, pText, color, font, alignment, layer, TTL );
        }

        inline void DrawText3D( Float3 const& worldPosition, char const* pText, Float4 const& color, DebugFont font = DebugFont::Normal, DebugTextAlign alignment = DebugTextAlign::MiddleLeft, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawText3D( m_commandBuffer, worldPosition, pText, color, font, alignment, layer, TTL );
        }

        inline void DrawTextBox2D( Float2 const& screenPos, char const* pText, Float4 const& color, DebugFont font = DebugFont::Normal, DebugTextAlign alignment = DebugTextAlign::MiddleLeft, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawTextBox2D( m_commandBuffer, screenPos, pText, color, font, alignment, layer, TTL );
        }

        inline void DrawTextBox3D( Float3 const& worldPos, char const* pText, Float4 const& color, DebugFont font = DebugFont::Normal, DebugTextAlign alignment = DebugTextAlign::MiddleLeft, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawTextBox3D( m_commandBuffer, worldPos, pText, color, font, alignment, layer, TTL );
        }

        //-------------------------------------------------------------------------
        // Debug Meshes
        //-------------------------------------------------------------------------

        inline void DrawMesh( uint64_t debugMeshID, Matrix const& transform, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { InternalDrawMesh( m_commandBuffer, debugMeshID, transform, tintColor, DebugMeshStyle::Default, layer, TTL ); }
        inline void DrawMesh( uint64_t debugMeshID, Transform const& transform, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawMesh( debugMeshID, transform.ToMatrix(), tintColor,  layer, TTL ); }
        inline void DrawMesh( uint64_t debugMeshID, Vector const& position, Quaternion const& orientation, float scale = 1.0f, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawMesh( debugMeshID, Matrix( orientation, position, scale ), tintColor, layer, TTL ); }
        inline void DrawMesh( uint64_t debugMeshID, Vector const& position, Quaternion const& orientation, Vector const& scale = Vector::One, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawMesh( debugMeshID, Matrix( orientation, position, scale ), tintColor, layer, TTL ); }

        inline void DrawMesh( DebugMeshID debugMeshID, Matrix const& transform, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { InternalDrawMesh( m_commandBuffer, (uint64_t) debugMeshID, transform, tintColor, DebugMeshStyle::Default, layer, TTL ); }
        inline void DrawMesh( DebugMeshID debugMeshID, Transform const& transform, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawMesh( debugMeshID, transform.ToMatrix(), tintColor, layer, TTL ); }
        inline void DrawMesh( DebugMeshID debugMeshID, Vector const& position, Quaternion const& orientation, float scale = 1.0f, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawMesh( debugMeshID, Matrix( orientation, position, scale ), tintColor, layer, TTL ); }
        inline void DrawMesh( DebugMeshID debugMeshID, Vector const& position, Quaternion const& orientation, Vector const& scale = Vector::One, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawMesh( debugMeshID, Matrix( orientation, position, scale ), tintColor, layer, TTL ); }

        inline void DrawLitMesh( uint64_t debugMeshID, Matrix const& transform, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { InternalDrawMesh( m_commandBuffer, debugMeshID, transform, tintColor, DebugMeshStyle::Lit, layer, TTL ); }
        inline void DrawLitMesh( uint64_t debugMeshID, Transform const& transform, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawLitMesh( debugMeshID, transform.ToMatrix(), tintColor, layer, TTL ); }
        inline void DrawLitMesh( uint64_t debugMeshID, Vector const& position, Quaternion const& orientation, float scale = 1.0f, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawLitMesh( debugMeshID, Matrix( orientation, position, scale ), tintColor, layer, TTL ); }
        inline void DrawLitMesh( uint64_t debugMeshID, Vector const& position, Quaternion const& orientation, Vector const& scale = Vector::One, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawLitMesh( debugMeshID, Matrix( orientation, position, scale ), tintColor, layer, TTL ); }

        inline void DrawLitMesh( DebugMeshID debugMeshID, Matrix const& transform, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { InternalDrawMesh( m_commandBuffer, (uint64_t) debugMeshID, transform, tintColor, DebugMeshStyle::Lit, layer, TTL ); }
        inline void DrawLitMesh( DebugMeshID debugMeshID, Transform const& transform, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawLitMesh( debugMeshID, transform.ToMatrix(), tintColor, layer, TTL ); }
        inline void DrawLitMesh( DebugMeshID debugMeshID, Vector const& position, Quaternion const& orientation, float scale = 1.0f, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawLitMesh( debugMeshID, Matrix( orientation, position, scale ), tintColor, layer, TTL ); }
        inline void DrawLitMesh( DebugMeshID debugMeshID, Vector const& position, Quaternion const& orientation, Vector const& scale = Vector::One, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawLitMesh( debugMeshID, Matrix( orientation, position, scale ), tintColor, layer, TTL ); }

        inline void DrawWireMesh( uint64_t debugMeshID, Matrix const& transform, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { InternalDrawMesh( m_commandBuffer, debugMeshID, transform, tintColor, DebugMeshStyle::Wireframe, layer, TTL ); }
        inline void DrawWireMesh( uint64_t debugMeshID, Transform const& transform, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawWireMesh( debugMeshID, transform.ToMatrix(), tintColor,  layer, TTL ); }
        inline void DrawWireMesh( uint64_t debugMeshID, Vector const& position, Quaternion const& orientation, float scale = 1.0f, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawWireMesh( debugMeshID, Matrix( orientation, position, scale ), tintColor, layer, TTL ); }
        inline void DrawWireMesh( uint64_t debugMeshID, Vector const& position, Quaternion const& orientation, Vector const& scale = Vector::One, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawWireMesh( debugMeshID, Matrix( orientation, position, scale ), tintColor, layer, TTL ); }

        inline void DrawWireMesh( DebugMeshID debugMeshID, Matrix const& transform, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { InternalDrawMesh( m_commandBuffer, (uint64_t) debugMeshID, transform, tintColor, DebugMeshStyle::Wireframe, layer, TTL ); }
        inline void DrawWireMesh( DebugMeshID debugMeshID, Transform const& transform, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawWireMesh( debugMeshID, transform.ToMatrix(), tintColor, layer, TTL ); }
        inline void DrawWireMesh( DebugMeshID debugMeshID, Vector const& position, Quaternion const& orientation, float scale = 1.0f, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawWireMesh( debugMeshID, Matrix( orientation, position, scale ), tintColor, layer, TTL ); }
        inline void DrawWireMesh( DebugMeshID debugMeshID, Vector const& position, Quaternion const& orientation, Vector const& scale = Vector::One, Color tintColor = Colors::White, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 ) { DrawWireMesh( debugMeshID, Matrix( orientation, position, scale ), tintColor, layer, TTL ); }

        //-------------------------------------------------------------------------
        // Plane
        //-------------------------------------------------------------------------

        void DrawPlane( Float4 const& planeEquation, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 );

        //-------------------------------------------------------------------------
        // Box
        //-------------------------------------------------------------------------

        inline void DrawBox( Transform const& transform, Float3 const& halfsize, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            Matrix m = transform.ToMatrix();
            m.SetScale( halfsize );
            InternalDrawMesh( m_commandBuffer, (uint64_t) DebugMeshID::Box, m, color, DebugMeshStyle::Default, layer, TTL );
        }

        inline void DrawBox( Float3 const& position, Quaternion const& rotation, Float3 const& halfsize, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawBox( Transform( rotation, position ), halfsize, color, layer, TTL );
        }

        inline void DrawBox( OBB const& box, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawBox( Transform( box.m_orientation, box.m_center ), box.m_extents, color, layer, TTL );
        }

        inline void DrawBox( AABB const& box, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawBox( Transform( Quaternion::Identity, box.m_center ), box.m_halfExtents, color, layer, TTL );
        }

        //-------------------------------------------------------------------------

        inline void DrawLitBox( Transform const& transform, Float3 const& halfsize, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            Matrix m = transform.ToMatrix();
            m.SetScale( halfsize );
            InternalDrawMesh( m_commandBuffer, (uint64_t) DebugMeshID::Box, m, color, DebugMeshStyle::Lit, layer, TTL );
        }

        inline void DrawLitBox( Float3 const& position, Quaternion const& rotation, Float3 const& halfsize, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawLitBox( Transform( rotation, position ), halfsize, color, layer, TTL );
        }

        inline void DrawLitBox( OBB const& box, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawLitBox( Transform( box.m_orientation, box.m_center ), box.m_extents, color, layer, TTL );
        }

        inline void DrawLitBox( AABB const& box, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawLitBox( Transform( Quaternion::Identity, box.m_center ), box.m_halfExtents, color, layer, TTL );
        }

        //-------------------------------------------------------------------------

        void DrawWireBox( Transform const& transform, Float3 const& halfsize, Float4 const& color, float lineThickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 );

        inline void DrawWireBox( Float3 const& position, Quaternion const& rotation, Float3 const& halfsize, Float4 const& color, float lineThickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawWireBox( Transform( rotation, position ), halfsize, color, lineThickness, layer, TTL );
        }

        inline void DrawWireBox( OBB const& box, Float4 const& color, float lineThickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawWireBox( Transform( box.m_orientation, box.m_center ), box.m_extents, color, lineThickness, layer, TTL );
        }

        inline void DrawWireBox( AABB const& box, Float4 const& color, float lineThickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawWireBox( Transform( Quaternion::Identity, box.m_center ), box.m_halfExtents, color, lineThickness, layer, TTL );
        }

        //-------------------------------------------------------------------------
        //  Circle
        //-------------------------------------------------------------------------

        void DrawCircle( Transform const& transform, Axis upAxis, float radius, Float4 const& color, float lineThickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 );

        inline void DrawCircle( Vector const& worldPosition, Axis upAxis, float radius, Float4 const& color, float lineThickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawCircle( Transform( Quaternion::Identity, worldPosition ), upAxis, radius, color, lineThickness, layer, TTL );
        }

        // Disc align to the XY plane
        void DrawDisc( Float3 const& worldPoint, float radius, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 );

        //-------------------------------------------------------------------------
        // Sphere
        //-------------------------------------------------------------------------

        inline void DrawSphere( Transform const& transform, float radius, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            Matrix m = transform.ToMatrix();
            m.SetScale( radius );
            InternalDrawMesh( m_commandBuffer, (uint64_t) DebugMeshID::Sphere, m, color, DebugMeshStyle::Default, layer, TTL );
        }

        inline void DrawSphere( Float3 const& position, float radius, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawSphere( Transform( Quaternion::Identity, position ), radius, color, layer, TTL );
        }

        inline void DrawLitSphere( Transform const& transform, float radius, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            Matrix m = transform.ToMatrix();
            m.SetScale( radius );
            InternalDrawMesh( m_commandBuffer, (uint64_t) DebugMeshID::Sphere, m, color, DebugMeshStyle::Lit, layer, TTL );
        }

        inline void DrawLitSphere( Float3 const& position, float radius, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawLitSphere( Transform( Quaternion::Identity, position ), radius, color, layer, TTL );
        }

        void DrawWireSphere( Transform const& transform, float radius, Float4 const& color, float lineThickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawCircle( transform, Axis::X, radius, color, lineThickness, layer, TTL );
            DrawCircle( transform, Axis::Y, radius, color, lineThickness, layer, TTL );
            DrawCircle( transform, Axis::Z, radius, color, lineThickness, layer, TTL );
        };

        inline void DrawWireSphere( Float3 const& position, float radius, Float4 const& color, float lineThickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawWireSphere( Transform( Quaternion::Identity, position ), radius, color, lineThickness, layer, TTL );
        }

        //-------------------------------------------------------------------------
        // Cylinder
        //-------------------------------------------------------------------------

        // Cylinder with radius on the XY plane and half-height along Z
        inline void DrawCylinder( Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            Matrix m = worldTransform.ToMatrix();
            m.SetScale( Vector( radius, radius, halfHeight ) );
            InternalDrawMesh( m_commandBuffer, (uint64_t) DebugMeshID::Cylinder, m, color, DebugMeshStyle::Default, layer, TTL );
        }

        // Cylinder with radius on the XY plane and half-height along Z
        inline void DrawCylinder( Vector const& position, Quaternion const& orientation, float radius, float halfHeight, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawCylinder( Transform( orientation, position ), radius, halfHeight, color, layer, TTL );
        }

        // Cylinder with radius on the XY plane and half-height along Z
        inline void DrawLitCylinder( Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            Matrix m = worldTransform.ToMatrix();
            m.SetScale( Vector( radius, radius, halfHeight ) );
            InternalDrawMesh( m_commandBuffer, (uint64_t) DebugMeshID::Cylinder, m, color, DebugMeshStyle::Lit, layer, TTL );
        }

        // Cylinder with radius on the XY plane and half-height along Z
        inline void DrawLitCylinder( Vector const& position, Quaternion const& orientation, float radius, float halfHeight, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawLitCylinder( Transform( orientation, position ), radius, halfHeight, color, layer, TTL );
        }

        // Cylinder with radius on the XY plane and half-height along Z
        inline void DrawWireCylinder( Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, float thickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawWireCylinderOrCapsule( false, worldTransform, radius, halfHeight, color, thickness, layer, TTL );
        }

        // Cylinder with radius on the XY plane and half-height along Z
        inline void DrawWireCylinder( Vector const& position, Quaternion const& orientation, float radius, float halfHeight, Float4 const& color, float thickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawWireCylinderOrCapsule( false, Transform( orientation, position ), radius, halfHeight, color, thickness, layer, TTL );
        }

        //-------------------------------------------------------------------------
        // Capsule
        //-------------------------------------------------------------------------

        // Capsule with radius on the XY plane and half-height along Z, total capsule height = 2 * ( halfHeight + radius )
        void DrawCapsule( Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawCapsule( worldTransform, radius, halfHeight, color, DebugMeshStyle::Default, layer, TTL );
        }

        // Capsule with radius on the XY plane and half-height along Z, total capsule height = 2 * ( halfHeight + radius )
        void DrawCapsule( Vector const& position, Quaternion const& orientation, float radius, float halfHeight, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawCapsule( Transform( orientation, position ), radius, halfHeight, color, DebugMeshStyle::Default, layer, TTL );
        }

        // Capsule from two points and a radius
        void DrawCapsule( Vector const& c0, Vector const& c1, float radius, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawCapsule( c0, c1, radius, color, DebugMeshStyle::Default, layer, TTL );
        }

        // Capsule with radius on the XY plane and half-height along Z, total capsule height = 2 * ( halfHeight + radius )
        void DrawLitCapsule( Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawCapsule( worldTransform, radius, halfHeight, color, DebugMeshStyle::Lit, layer, TTL );
        }

        // Capsule with radius on the XY plane and half-height along Z, total capsule height = 2 * ( halfHeight + radius )
        void DrawLitCapsule( Vector const& position, Quaternion const& orientation, float radius, float halfHeight, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawCapsule( Transform( orientation, position ), radius, halfHeight, color, DebugMeshStyle::Lit, layer, TTL );
        }

        // Capsule from two points and a radius
        void DrawLitCapsule( Vector const& c0, Vector const& c1, float radius, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawCapsule( c0, c1, radius, color, DebugMeshStyle::Lit, layer, TTL );
        }

        // Capsule with radius on the XY plane and half-height along Z, total capsule height = 2 * ( halfHeight + radius )
        void DrawWireCapsule( Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, float thickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawWireCylinderOrCapsule( true, worldTransform, radius, halfHeight, color, thickness, layer, TTL );
        }

        // Capsule with radius on the XY plane and half-height along Z, total capsule height = 2 * ( halfHeight + radius )
        void DrawWireCapsule( Vector const& position, Quaternion const& orientation, float radius, float halfHeight, Float4 const& color, float thickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawWireCylinderOrCapsule( true, Transform( orientation, position ), radius, halfHeight, color, thickness, layer, TTL );
        }

        // Capsule from two points and a radius
        void DrawWireCapsule( Vector const& c0, Vector const& c1, float radius, Float4 const& color, float thickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            InternalDrawWireCylinderOrCapsule( true, c0, c1, radius, color, thickness, layer, TTL );
        }

        //-------------------------------------------------------------------------
        // Complex Shapes
        //-------------------------------------------------------------------------

        inline void DrawAxis( Transform const& worldTransform, Color axisColor1, Color axisColor2, Color axisColor3, float axisLength = 0.05f, float axisThickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            Vector const xAxis = worldTransform.GetAxisX().GetNormalized3() * axisLength;
            Vector const yAxis = worldTransform.GetAxisY().GetNormalized3() * axisLength;
            Vector const zAxis = worldTransform.GetAxisZ().GetNormalized3() * axisLength;

            DrawLine( worldTransform.GetTranslation(), worldTransform.GetTranslation() + xAxis, axisColor1.ToFloat4(), axisThickness, layer, TTL );
            DrawLine( worldTransform.GetTranslation(), worldTransform.GetTranslation() + yAxis, axisColor2.ToFloat4(), axisThickness, layer, TTL );
            DrawLine( worldTransform.GetTranslation(), worldTransform.GetTranslation() + zAxis, axisColor3.ToFloat4(), axisThickness, layer, TTL );
        }

        EE_FORCE_INLINE void DrawAxis( Transform const& worldTransform, float axisLength = 0.05f, float axisThickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            DrawAxis( worldTransform, Colors::Red, Colors::Lime, Colors::Blue, axisLength, axisThickness, layer, TTL );
        }

        void DrawArrow( Float3 const& startPoint, Float3 const& endPoint, Float4 const& color, float thickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 );

        inline void DrawArrow( Float3 const& startPoint, Float3 const& direction, float arrowLength, Float4 const& color, float thickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            Vector const endPoint = Vector::MultiplyAdd( Vector( direction ).GetNormalized3(), Vector( arrowLength ), Vector( startPoint ) );
            DrawArrow( startPoint, endPoint.ToFloat3(), color, thickness, layer, TTL );
        }

        // Draw a cone, originating at the transform, aligned to the -Y axis of the transform
        void DrawCone( Transform const& transform, Radians coneAngle, float length, Float4 const& color, float thickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 );

        // Draw a cone originating at the start point, aligned to the specified
        inline void DrawCone( Float3 const& startPoint, Float3 const& direction, Radians coneAngle, float length, Float4 const& color, float thickness = s_defaultLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            Quaternion const orientation = Quaternion::FromRotationBetweenUnitVectors( Vector::WorldForward, Vector( direction ).GetNormalized3() );
            Transform const coneTransform( orientation, startPoint );
            DrawCone( coneTransform, coneAngle, length, color, thickness, layer, TTL );
        }

    private:

        EE_FORCE_INLINE void InternalDrawPoint( DebugDrawInternal::ThreadCommandBuffer& cmdList, Float3 const& position, Float4 const& color, float thickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            cmdList.AddCommand( DebugDrawInternal::PointCommand( position, color, thickness, TTL ), layer );
        }

        EE_FORCE_INLINE void InternalDrawLine( DebugDrawInternal::ThreadCommandBuffer& cmdList, Float3 const& startPosition, Float3 const& endPosition, Float4 const& color, float lineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            cmdList.AddCommand( DebugDrawInternal::LineCommand( startPosition, endPosition, color, lineThickness, TTL ), layer );
        }

        EE_FORCE_INLINE void InternalDrawLine( DebugDrawInternal::ThreadCommandBuffer& cmdList, Float3 const& startPosition, Float3 const& endPosition, Float4 const& startColor, Float4 const& endColor, float lineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            cmdList.AddCommand( DebugDrawInternal::LineCommand( startPosition, endPosition, startColor, endColor, lineThickness, TTL ), layer );
        }

        EE_FORCE_INLINE void InternalDrawLine( DebugDrawInternal::ThreadCommandBuffer& cmdList, Float3 const& startPosition, Float3 const& endPosition, Float4 const& color, float startLineThickness, float endLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            cmdList.AddCommand( DebugDrawInternal::LineCommand( startPosition, endPosition, color, startLineThickness, endLineThickness, TTL ), layer );
        }

        EE_FORCE_INLINE void InternalDrawLine( DebugDrawInternal::ThreadCommandBuffer& cmdList, Float3 const& startPosition, Float3 const& endPosition, Float4 const& startColor, Float4 const& endColor, float startLineThickness, float endLineThickness, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            cmdList.AddCommand( DebugDrawInternal::LineCommand( startPosition, endPosition, startColor, endColor, startLineThickness, endLineThickness, TTL ), layer );
        }

        EE_FORCE_INLINE void InternalDrawTriangle( DebugDrawInternal::ThreadCommandBuffer& cmdList, Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            cmdList.AddCommand( DebugDrawInternal::TriangleCommand( v0, v1, v2, color, TTL ), layer );
        }

        EE_FORCE_INLINE void InternalDrawTriangle( DebugDrawInternal::ThreadCommandBuffer& cmdList, Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color0, Float4 const& color1, Float4 const& color2, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            cmdList.AddCommand( DebugDrawInternal::TriangleCommand( v0, v1, v2, color0, color1, color2, TTL ), layer );
        }

        EE_FORCE_INLINE void InternalDrawText2D( DebugDrawInternal::ThreadCommandBuffer& cmdList, Float2 const& position, char const* pText, Float4 const& color, DebugFont font, DebugTextAlign alignment, DebugDrawLayer layer, Seconds TTL = -1 )
        {
            cmdList.AddCommand( DebugDrawInternal::TextCommand( position, pText, color, font, alignment, false, TTL ), layer );
        }

        EE_FORCE_INLINE void InternalDrawText3D( DebugDrawInternal::ThreadCommandBuffer& cmdList, Float3 const& position, char const* pText, Float4 const& color, DebugFont font, DebugTextAlign alignment, DebugDrawLayer layer, Seconds TTL = -1 )
        {
            cmdList.AddCommand( DebugDrawInternal::TextCommand( position, pText, color, font, alignment, false, TTL ), layer );
        }

        EE_FORCE_INLINE void InternalDrawTextBox2D( DebugDrawInternal::ThreadCommandBuffer& cmdList, Float2 const& position, char const* pText, Float4 const& color, DebugFont font, DebugTextAlign alignment, DebugDrawLayer layer, Seconds TTL = -1 )
        {
            cmdList.AddCommand( DebugDrawInternal::TextCommand( position, pText, color, font, alignment, true, TTL ), layer );
        }

        EE_FORCE_INLINE void InternalDrawTextBox3D( DebugDrawInternal::ThreadCommandBuffer& cmdList, Float3 const& position, char const* pText, Float4 const& color, DebugFont font, DebugTextAlign alignment, DebugDrawLayer layer, Seconds TTL = -1 )
        {
            cmdList.AddCommand( DebugDrawInternal::TextCommand( position, pText, color, font, alignment, true, TTL ), layer );
        }

        EE_FORCE_INLINE void InternalDrawMesh( DebugDrawInternal::ThreadCommandBuffer& cmdList, uint64_t debugMeshID, Matrix transform, Float4 const& tintColor, DebugMeshStyle style = DebugMeshStyle::Default, DebugDrawLayer layer = DebugDrawLayer::Screen, Seconds TTL = -1 )
        {
            cmdList.AddCommand( DebugDrawInternal::MeshCommand( debugMeshID, transform, tintColor, style, TTL ), layer );
        }

        void InternalDrawCapsule( Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, DebugMeshStyle style, DebugDrawLayer layer, Seconds TTL );
        void InternalDrawWireCylinderOrCapsule( bool isCapsule, Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, float thickness, DebugDrawLayer layer, Seconds TTL );

        void InternalDrawCapsule( Vector const& c0, Vector const& c1, float radius, Float4 const& color, DebugMeshStyle style, DebugDrawLayer layer, Seconds TTL );
        void InternalDrawWireCylinderOrCapsule( bool isCapsule, Vector const& c0, Vector const& c1, float radius, Float4 const& color, float thickness, DebugDrawLayer layer, Seconds TTL );

        // Try to prevent users from copying these contexts around - TODO: we should record the thread ID and assert everywhere that we are on the correct thread
        inline DebugDrawContext( DebugDrawInternal::ThreadCommandBuffer& buffer ) : m_commandBuffer( buffer ) {}
        inline DebugDrawContext operator=( DebugDrawContext const& RHS ) { return DebugDrawContext( RHS.m_commandBuffer ); }

    private:

        DebugDrawInternal::ThreadCommandBuffer& m_commandBuffer;
    };
}
#endif