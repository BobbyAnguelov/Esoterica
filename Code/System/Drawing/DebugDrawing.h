#pragma once

#include "../_Module/API.h"
#include "DebugDrawingCommands.h"
#include "System/Math/Transform.h"
#include "System/Types/Color.h"
#include "System/Math/BoundingVolumes.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Drawing
{
    //-------------------------------------------------------------------------
    // Debug Drawing Context
    //-------------------------------------------------------------------------

    // The primary API for debug drawing in EE
    // Drawing contexts are bound to threads, so each worker thread will have it's own drawing context

    // We have helper functions to allow you to draw custom types in the same manner as the basic primitive:
    // i.e. drawingContext.Draw( MyType const& type )
    // To support this, you need to implement one of the following functions in your type:
    // 1) 'void DrawDebug( Drawing::DrawContext& ctx ) const'
    // 2) 'void DrawDebug( Drawing::DrawContext& ctx, Transform const& worldTransform ) const'

    // Note: We use float4 for colors here since EE::Color will implicitly convert to a float4. 
    // We use float4s for color in the rendering code, so this allows you to save on constantly paying the conversion cost when drawing lots of primitives

    class EE_SYSTEM_API DrawContext
    {
        friend class DrawingSystem;

        constexpr static float const s_defaultLineThickness = 1.0f;
        constexpr static float const s_defaultPointThickness = 5.0f;

    public:

        inline DrawContext( DrawContext const& RHS ) : m_commandBuffer( RHS.m_commandBuffer ) {}

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
        // Basic Primitives
        //-------------------------------------------------------------------------

        inline void DrawPoint( Float3 const& position, Float4 const& color, float thickness = s_defaultPointThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            InternalDrawPoint( m_commandBuffer, position, color, thickness, depthTestState, TTL );
        }

        inline void DrawLine( Float3 const& startPosition, Float3 const& endPosition, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            InternalDrawLine( m_commandBuffer, startPosition, endPosition, color, lineThickness, depthTestState, TTL );
        }

        inline void DrawLine( Float3 const& startPosition, Float3 const& endPosition, Float4 const& startColor, Float4 const& endColor, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            InternalDrawLine( m_commandBuffer, startPosition, endPosition, startColor, endColor, lineThickness, depthTestState, TTL );
        }

        inline void DrawTriangle( Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            InternalDrawTriangle( m_commandBuffer, v0, v1, v2, color, color, color, depthTestState, TTL );
        }

        inline void DrawTriangle( Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color0, Float4 const& color1, Float4 const& color2, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            InternalDrawTriangle( m_commandBuffer, v0, v1, v2, color0, color1, color2, depthTestState, TTL );
        }

        void DrawWireTriangle( Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        inline void DrawText2D( Float2 const& screenPosition, char const* pText, Float4 const& color, FontSize size = FontSize::FontNormal, TextAlignment alignment = TextAlignment::AlignMiddleLeft, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            InternalDrawText2D( m_commandBuffer, screenPosition, pText, color, size, alignment, depthTestState, TTL );
        }

        inline void DrawText3D( Float3 const& worldPosition, char const* pText, Float4 const& color, FontSize size = FontSize::FontNormal, TextAlignment alignment = TextAlignment::AlignMiddleLeft, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            InternalDrawText3D( m_commandBuffer, worldPosition, pText, color, size, alignment, depthTestState, TTL );
        }

        inline void DrawTextBox2D( Float2 const& screenPos, char const* pText, Float4 const& color, FontSize size = FontSize::FontNormal, TextAlignment alignment = TextAlignment::AlignMiddleLeft, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            InternalDrawTextBox2D( m_commandBuffer, screenPos, pText, color, size, alignment, depthTestState, TTL );
        }

        inline void DrawTextBox3D( Float3 const& worldPos, char const* pText, Float4 const& color, FontSize size = FontSize::FontNormal, TextAlignment alignment = TextAlignment::AlignMiddleLeft, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            InternalDrawTextBox3D( m_commandBuffer, worldPos, pText, color, size, alignment, depthTestState, TTL );
        }

        //-------------------------------------------------------------------------
        // Boxes / Volumes / Planes
        //-------------------------------------------------------------------------

        void DrawPlane( Float4 const& planeEquation, Float4 const& color, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        void DrawBox( Transform const& worldTransform, Float3 const& halfsize, Float4 const& color, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        void DrawWireBox( Transform const& worldTransform, Float3 const& halfsize, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        inline void DrawBox( Float3 const& position, Quaternion const& rotation, Float3 const& halfsize, Float4 const& color, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            DrawBox( Transform( rotation, position), halfsize, color, depthTestState, TTL);
        }

        inline void DrawBox( OBB const& box, Float4 const& color, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            DrawBox( Transform( box.m_orientation, box.m_center ),box.m_extents, color, depthTestState, TTL );
        }

        inline void DrawBox( AABB const& box, Float4 const& color, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            DrawBox( Transform( Quaternion::Identity, box.m_center ), box.m_extents, color, depthTestState, TTL );
        }

        inline void DrawWireBox( Float3 const& position, Quaternion const& rotation, Float3 const& halfsize, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            DrawWireBox( Transform( rotation, position ), halfsize, color, lineThickness, depthTestState, TTL );
        }

        inline void DrawWireBox( OBB const& box, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            DrawWireBox( Transform( box.m_orientation, box.m_center ), box.m_extents, color, lineThickness, depthTestState, TTL );
        }

        inline void DrawWireBox( AABB const& box, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            DrawWireBox( Transform( Quaternion::Identity, box.m_center ), box.m_extents, color, lineThickness, depthTestState, TTL );
        }

        //-------------------------------------------------------------------------
        // Sphere / Circle / Cylinder / Capsule
        //-------------------------------------------------------------------------

        void DrawCircle( Transform const& transform, Axis upAxis, float radius, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        inline void DrawCircle( Vector const& worldPosition , Axis upAxis, float radius, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            DrawCircle( Transform( Quaternion::Identity, worldPosition ), upAxis, radius, color, lineThickness, depthTestState, TTL );
        }

        //-------------------------------------------------------------------------

        void DrawSphere( Transform const& transform, float radius, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        inline void DrawSphere( Float3 const& position, float radius, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            DrawSphere( Transform( Quaternion::Identity, position ), radius, color, lineThickness, depthTestState, TTL );
        }

        inline void DrawSphere( Sphere const& sphere, float radius, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            DrawSphere( Transform( Quaternion::Identity, sphere.GetCenter() ), sphere.GetRadius(), color, lineThickness, depthTestState, TTL );
        }

        //-------------------------------------------------------------------------

        // A half sphere from the transform point sliced along the XY plane ( Z is up )
        void DrawHalfSphere( Transform const& transform, float radius, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        // A half sphere from the transform point sliced along the YZ plane ( X is up )
        void DrawHalfSphereYZ( Transform const& transform, float radius, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        // A half sphere from the transform point, with the radius along the +Z axis
        inline void DrawHalfSphere( Float3 const& position, float radius, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            DrawHalfSphere( Transform( Quaternion::Identity, position ), radius, color, lineThickness, depthTestState, TTL );
        }

        //-------------------------------------------------------------------------

        // Disc align to the XY plane
        void DrawDisc( Float3 const& worldPoint, float radius, Float4 const& color, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        // Cylinder with radius on the XY plane and half-height along Z
        void DrawCylinder( Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, float thickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        // Capsule with radius on the XY plane and half-height along Z, total capsule height = 2 * ( halfHeight + radius )
        void DrawCapsule( Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, float thickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        // Capsule with radius on the YZ plane and half-height along X, total capsule height = 2 * ( halfHeight + radius )
        void DrawCapsuleHeightX( Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, float thickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        //-------------------------------------------------------------------------
        // Complex Shapes
        //-------------------------------------------------------------------------

        inline void DrawAxis( Transform const& worldTransform, float axisLength = 0.05f, float axisThickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            Vector const xAxis = worldTransform.GetAxisX().GetNormalized3() * axisLength;
            Vector const yAxis = worldTransform.GetAxisY().GetNormalized3() * axisLength;
            Vector const zAxis = worldTransform.GetAxisZ().GetNormalized3() * axisLength;

            DrawLine( worldTransform.GetTranslation(), worldTransform.GetTranslation() + xAxis, Colors::Red, axisThickness, depthTestState, TTL );
            DrawLine( worldTransform.GetTranslation(), worldTransform.GetTranslation() + yAxis, Colors::LimeGreen, axisThickness, depthTestState, TTL );
            DrawLine( worldTransform.GetTranslation(), worldTransform.GetTranslation() + zAxis, Colors::Blue, axisThickness, depthTestState, TTL );
        }

        void DrawArrow( Float3 const& startPoint, Float3 const& endPoint, Float4 const& color, float thickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        inline void DrawArrow( Float3 const& startPoint, Float3 const& direction, float arrowLength, Float4 const& color, float thickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            Vector const endPoint = Vector::MultiplyAdd( Vector( direction ).GetNormalized3(), Vector( arrowLength ), Vector( startPoint ) );
            DrawArrow( startPoint, endPoint.ToFloat3(), color, thickness, depthTestState, TTL );
        }

        // Draw a cone, originating at the transform, aligned to the -Y axis of the transform
        void DrawCone( Transform const& transform, Radians coneAngle, float length, Float4 const& color, float thickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 );

        // Draw a cone originating at the start point, aligned to the specified
        inline void DrawCone( Float3 const& startPoint, Float3 const& direction, Radians coneAngle, float length, Float4 const& color, float thickness = s_defaultLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            Quaternion const orientation = Quaternion::FromRotationBetweenNormalizedVectors( Vector::WorldForward, Vector( direction ).GetNormalized3() );
            Transform const coneTransform( orientation, startPoint );
            DrawCone( coneTransform, coneAngle, length, color, thickness, depthTestState, TTL );
        }

    private:

        EE_FORCE_INLINE void InternalDrawPoint( ThreadCommandBuffer& cmdList, Float3 const& position, Float4 const& color, float thickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            cmdList.AddCommand( PointCommand( position, color, thickness, TTL ), depthTestState );
        }

        EE_FORCE_INLINE void InternalDrawLine( ThreadCommandBuffer& cmdList, Float3 const& startPosition, Float3 const& endPosition, Float4 const& color, float lineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            cmdList.AddCommand( LineCommand( startPosition, endPosition, color, lineThickness, TTL ), depthTestState );
        }

        EE_FORCE_INLINE void InternalDrawLine( ThreadCommandBuffer& cmdList, Float3 const& startPosition, Float3 const& endPosition, Float4 const& startColor, Float4 const& endColor, float lineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            cmdList.AddCommand( LineCommand( startPosition, endPosition, startColor, endColor, lineThickness, TTL ), depthTestState );
        }

        EE_FORCE_INLINE void InternalDrawLine( ThreadCommandBuffer& cmdList, Float3 const& startPosition, Float3 const& endPosition, Float4 const& color, float startLineThickness, float endLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            cmdList.AddCommand( LineCommand( startPosition, endPosition, color, startLineThickness, endLineThickness, TTL ), depthTestState );
        }

        EE_FORCE_INLINE void InternalDrawLine( ThreadCommandBuffer& cmdList, Float3 const& startPosition, Float3 const& endPosition, Float4 const& startColor, Float4 const& endColor, float startLineThickness, float endLineThickness, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            cmdList.AddCommand( LineCommand( startPosition, endPosition, startColor, endColor, startLineThickness, endLineThickness, TTL ), depthTestState );
        }

        EE_FORCE_INLINE void InternalDrawTriangle( ThreadCommandBuffer& cmdList, Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            cmdList.AddCommand( TriangleCommand( v0, v1, v2, color, TTL ), depthTestState );
        }

        EE_FORCE_INLINE void InternalDrawTriangle( ThreadCommandBuffer& cmdList, Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color0, Float4 const& color1, Float4 const& color2, DepthTestState depthTestState = DepthTestState::DisableDepthTest, Seconds TTL = -1 )
        {
            cmdList.AddCommand( TriangleCommand( v0, v1, v2, color0, color1, color2, TTL ), depthTestState );
        }

        EE_FORCE_INLINE void InternalDrawText2D( ThreadCommandBuffer& cmdList, Float2 const& position, char const* pText, Float4 const& color, FontSize size, TextAlignment alignment, DepthTestState depthTestState, Seconds TTL = -1 )
        {
            cmdList.AddCommand( TextCommand( position, pText, color, size, alignment, false, TTL ), depthTestState );
        }

        EE_FORCE_INLINE void InternalDrawText3D( ThreadCommandBuffer& cmdList, Float3 const& position, char const* pText, Float4 const& color, FontSize size, TextAlignment alignment, DepthTestState depthTestState, Seconds TTL = -1 )
        {
            cmdList.AddCommand( TextCommand( position, pText, color, size, alignment, false, TTL ), depthTestState );
        }

        EE_FORCE_INLINE void InternalDrawTextBox2D( ThreadCommandBuffer& cmdList, Float2 const& position, char const* pText, Float4 const& color, FontSize size, TextAlignment alignment, DepthTestState depthTestState, Seconds TTL = -1 )
        {
            cmdList.AddCommand( TextCommand( position, pText, color, size, alignment, true, TTL ), depthTestState );
        }

        EE_FORCE_INLINE void InternalDrawTextBox3D( ThreadCommandBuffer& cmdList, Float3 const& position, char const* pText, Float4 const& color, FontSize size, TextAlignment alignment, DepthTestState depthTestState, Seconds TTL = -1 )
        {
            cmdList.AddCommand( TextCommand( position, pText, color, size, alignment, true, TTL ), depthTestState);
        }

        // Try to prevent users from copying these contexts around - TODO: we should record the thread ID and assert everywhere that we are on the correct thread
        inline DrawContext( ThreadCommandBuffer& buffer ) : m_commandBuffer( buffer ) {}
        inline DrawContext operator=( DrawContext const& RHS ) { return DrawContext( RHS.m_commandBuffer ); }

    private:

        ThreadCommandBuffer& m_commandBuffer;
    };
}
#endif