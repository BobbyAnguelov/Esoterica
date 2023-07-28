#include "DebugDrawing.h"
#include "Base/Math/Plane.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Drawing
{
    namespace
    {
        constexpr static uint32_t const g_numCircleVertices = 16;
        static_assert( ( g_numCircleVertices % 4 ) == 0 );

        static bool g_circleVerticesInitialized = false;
        static Float4 g_circleVerticesXUp[g_numCircleVertices];
        static Float4 g_circleVerticesYUp[g_numCircleVertices];
        static Float4 g_circleVerticesZUp[g_numCircleVertices];

        static void CreateCircleVertices( Float4* pVerts, Vector planeVector, Vector upAxis )
        {
            Radians const radiansPerEdge( Math::TwoPi / g_numCircleVertices );
            Quaternion const rotation( upAxis, radiansPerEdge );
            for ( auto i = 0; i < g_numCircleVertices; i++ )
            {
                planeVector = rotation.RotateVector( planeVector );
                pVerts[i] = planeVector.SetW1();
            }
        }

        static bool InitializeCircleVertices()
        {
            CreateCircleVertices( g_circleVerticesXUp, Vector::UnitY, Vector::UnitX );
            CreateCircleVertices( g_circleVerticesYUp, Vector::UnitX, Vector::UnitY );
            CreateCircleVertices( g_circleVerticesZUp, Vector::UnitX, Vector::UnitZ );
            return true;
        }

        //-------------------------------------------------------------------------

        // Actual length is 2 since we specify dimensions in a half-size and then scale
        static uint32_t const g_unitCubeNumVertices = 8;
        static Float4 const g_unitCubeVertices[8] =
        {
            Float4( -1.0f, -1.0f, -1.0f, 1.0f ),    // BFL
            Float4( -1.0f, 1.0f, -1.0f, 1.0f ),     // TFL
            Float4( 1.0f, 1.0f, -1.0f, 1.0f ),      // TFR
            Float4( 1.0f, -1.0f, -1.0f, 1.0f ),     // BFR
            Float4( -1.0f, -1.0f, 1.0f, 1.0f ),     // BBL
            Float4( -1.0f, 1.0f, 1.0f, 1.0f ),      // TBL
            Float4( 1.0f, 1.0f, 1.0f, 1.0f ),       // TBR
            Float4( 1.0f, -1.0f, 1.0f, 1.0f )       // BBR
        };

        // Box Primitives
        static uint32_t const g_unitCubeSolidNumIndices = 36;
        static const uint16_t g_unitCubeSolidIndices[36] =
        {
            // Triangle faces for a solid box
            0, 1, 3, 3, 1, 2,
            3, 2, 7, 7, 2, 6,
            7, 6, 4, 4, 6, 5,
            4, 5, 0, 0, 5, 1,
            1, 5, 2, 2, 5, 6,
            4, 0, 7, 7, 0, 3
        };

        static uint32_t const g_unitCubeWireNumIndices = 24;
        static const uint16_t g_unitCubeWireIndices[24] =
        {
            // Lines for a wire-frame box
            0, 1, 1, 2, 2, 3, 3, 0,
            4, 5, 5, 6, 6, 7, 7, 4,
            0, 4, 1, 5, 2, 6, 3, 7,
        };
    }

    //-------------------------------------------------------------------------
    // Drawing Context
    //-------------------------------------------------------------------------

    void DrawContext::DrawWireTriangle( Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color, float lineThickness, DepthTest depthTestState, Seconds TTL )
    {
        InternalDrawLine( m_commandBuffer, v0, v1, color, lineThickness, depthTestState, TTL );
        InternalDrawLine( m_commandBuffer, v1, v2, color, lineThickness, depthTestState, TTL );
        InternalDrawLine( m_commandBuffer, v0, v2, color, lineThickness, depthTestState, TTL );
    }

    //-------------------------------------------------------------------------
    // Boxes / Volumes / Planes
    //-------------------------------------------------------------------------

    void DrawContext::DrawPlane( Float4 const& planeEquation, Float4 const& color, DepthTest depthTestState, Seconds TTL )
    {
        auto const plane = Plane::FromPlaneEquation( planeEquation );
        auto const translation = plane.ProjectPoint( Vector::UnitW );
        auto const rotation = Quaternion::FromRotationBetweenNormalizedVectors( Vector::UnitZ, plane.GetNormal() );

        Transform transform;
        transform.SetTranslation( translation );
        transform.SetRotation( rotation );

        static Float4 const g_planeVertices[4] =
        {
            Float4( -200.0f, -200.0f, 0.0f, 1.0f ),   // BL
            Float4( -200.0f, 200.0f,  0.0f, 1.0f ),   // TL
            Float4( 200.0f,  -200.0f, 0.0f, 1.0f ),   // BR
            Float4( 200.0f,  200.0f,  0.0f, 1.0f ),   // TR
        };

        Vector verts[4] =
        {
            transform.TransformPoint( g_planeVertices[0] ),
            transform.TransformPoint( g_planeVertices[1] ),
            transform.TransformPoint( g_planeVertices[2] ),
            transform.TransformPoint( g_planeVertices[3] ),
        };

        InternalDrawTriangle( m_commandBuffer, verts[0], verts[1], verts[2], color, depthTestState, TTL );
        InternalDrawTriangle( m_commandBuffer, verts[2], verts[1], verts[3], color, depthTestState, TTL );
    }

    void DrawContext::DrawBox( Transform const& transform, Float3 const& halfsize, Float4 const& color, DepthTest depthTestState, Seconds TTL )
    {
        // Calculate transformed vertices
        Vector verts[8] =
        {
            transform.TransformPoint( Vector( g_unitCubeVertices[0] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[1] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[2] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[3] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[4] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[5] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[6] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[7] ) * halfsize )
        };

        // Register draw commands

        for ( auto i = 0; i < 36; i += 3 )
        {
            InternalDrawTriangle( m_commandBuffer, verts[g_unitCubeSolidIndices[i]], verts[g_unitCubeSolidIndices[i + 1]], verts[g_unitCubeSolidIndices[i + 2]], color, depthTestState, TTL );
        }
    }

    void DrawContext::DrawWireBox( Transform const& transform, Float3 const& halfsize, Float4 const& color, float lineThickness, DepthTest depthTestState, Seconds TTL )
    {
        // Calculate vertices
        Vector verts[8] =
        {
            transform.TransformPoint( Vector( g_unitCubeVertices[0] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[1] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[2] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[3] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[4] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[5] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[6] ) * halfsize ),
            transform.TransformPoint( Vector( g_unitCubeVertices[7] ) * halfsize )
        };

        // Register draw commands

        for ( auto i = 0; i < 24; i += 2 )
        {
            InternalDrawLine( m_commandBuffer, verts[g_unitCubeWireIndices[i]], verts[g_unitCubeWireIndices[i + 1]], color, lineThickness, depthTestState, TTL );
        }
    }

    //-------------------------------------------------------------------------
    // Sphere / Circle
    //-------------------------------------------------------------------------

    void DrawContext::DrawCircle( Transform const& transform, Axis upAxis, float radius, Float4 const& color, float lineThickness, DepthTest depthTestState, Seconds TTL )
    {
        if ( !g_circleVerticesInitialized )
        {
            InitializeCircleVertices();
        }

        Float4* pCircleVerts = nullptr;

        switch ( upAxis )
        {
            case Axis::X:
            case Axis::NegX:
            pCircleVerts = g_circleVerticesXUp;
            break;
            
            case Axis::Y:
            case Axis::NegY:
            pCircleVerts = g_circleVerticesYUp;
            break;
            
            case Axis::Z:
            case Axis::NegZ:
            pCircleVerts = g_circleVerticesZUp;
            break;
        }

        EE_ASSERT( pCircleVerts != nullptr );

        // Create and transform vertices
        auto verts = EE_STACK_ARRAY_ALLOC( Vector, g_numCircleVertices );
        for ( auto i = 0; i < g_numCircleVertices; i++ )
        {
            verts[i] = transform.TransformPoint( pCircleVerts[i] * radius );
        }

        // Register line commands
        for ( auto i = 1; i < g_numCircleVertices; i++ )
        {
            InternalDrawLine( m_commandBuffer, verts[i - 1], verts[i], color, lineThickness, depthTestState, TTL );
        }

        InternalDrawLine( m_commandBuffer, verts[g_numCircleVertices - 1], verts[0], color, lineThickness, depthTestState, TTL );
    }

    void DrawContext::DrawSphere( Transform const& transform, float radius, Float4 const& color, float lineThickness, DepthTest depthTestState, Seconds TTL )
    {
        DrawCircle( transform, Axis::X, radius, color, lineThickness, depthTestState, TTL );
        DrawCircle( transform, Axis::Y, radius, color, lineThickness, depthTestState, TTL );
        DrawCircle( transform, Axis::Z, radius, color, lineThickness, depthTestState, TTL );
    }

    void DrawContext::DrawDisc( Float3 const& worldPoint, float radius, Float4 const& color, DepthTest depthTestState, Seconds TTL )
    {
        if ( !g_circleVerticesInitialized )
        {
            InitializeCircleVertices();
        }

        int32_t const numVerts = g_numCircleVertices;

        // Create and transform vertices
        Vector const radiusVector( radius );
        Vector const worldPointVector( worldPoint );

        auto verts = EE_STACK_ARRAY_ALLOC( Vector, numVerts );
        for ( auto i = 0; i < numVerts; i++ )
        {
            verts[i] = Vector::MultiplyAdd( g_circleVerticesZUp[i], radiusVector, worldPointVector );
        }


        for ( auto i = 1; i < numVerts; i++ )
        {
            InternalDrawTriangle( m_commandBuffer, worldPoint, verts[i - 1], verts[i], color, depthTestState, TTL );
        }

        InternalDrawTriangle( m_commandBuffer, worldPoint, verts[numVerts - 1], verts[0], color, depthTestState, TTL );
    }


    void DrawContext::InternalDrawCylinderOrCapsule( bool isCapsule, Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, float thickness, DepthTest depthTestState, Seconds TTL )
    {
        Vector const axisX = worldTransform.GetAxisX();
        Vector const axisY = worldTransform.GetAxisY();
        Vector const axisZ = worldTransform.GetAxisZ();

        Vector const origin = worldTransform.GetTranslation();
        Vector const halfHeightOffset = ( axisZ * halfHeight );

        Vector const cylinderCenterTop = origin + halfHeightOffset;
        Vector const cylinderCenterBottom = origin - halfHeightOffset;

        // 8 lines
        //-------------------------------------------------------------------------

        Vector xOffset = ( axisX * radius );
        Vector lt0 = cylinderCenterTop + xOffset;
        Vector lt1 = cylinderCenterTop - xOffset;
        Vector lb0 = cylinderCenterBottom + xOffset;
        Vector lb1 = cylinderCenterBottom - xOffset;

        DrawLine( lt0, lb0, color, thickness, depthTestState, TTL );
        DrawLine( lt1, lb1, color, thickness, depthTestState, TTL );

        Vector yOffset = ( axisY * radius );
        Vector lt2 = cylinderCenterTop + yOffset;
        Vector lt3 = cylinderCenterTop - yOffset;
        Vector lb2 = cylinderCenterBottom + yOffset;
        Vector lb3 = cylinderCenterBottom - yOffset;

        DrawLine( lt2, lb2, color, thickness, depthTestState, TTL );
        DrawLine( lt3, lb3, color, thickness, depthTestState, TTL );

        Vector xzOffset0 = ( axisX + axisY ).GetNormalized3() * radius;
        Vector lt4 = cylinderCenterTop + xzOffset0;
        Vector lt5 = cylinderCenterTop - xzOffset0;
        Vector lb4 = cylinderCenterBottom + xzOffset0;
        Vector lb5 = cylinderCenterBottom - xzOffset0;

        DrawLine( lt4, lb4, color, thickness, depthTestState, TTL );
        DrawLine( lt5, lb5, color, thickness, depthTestState, TTL );

        Vector xzOffset1 = ( axisX - axisY ).GetNormalized3() * radius;
        Vector lt6 = cylinderCenterTop + xzOffset1;
        Vector lt7 = cylinderCenterTop - xzOffset1;
        Vector lb6 = cylinderCenterBottom + xzOffset1;
        Vector lb7 = cylinderCenterBottom - xzOffset1;

        DrawLine( lt6, lb6, color, thickness, depthTestState, TTL );
        DrawLine( lt7, lb7, color, thickness, depthTestState, TTL );

        // Caps
        //-------------------------------------------------------------------------

        DrawCircle( Transform( worldTransform.GetRotation(), cylinderCenterTop ), Axis::Z, radius, color, thickness, depthTestState, TTL );
        DrawCircle( Transform( worldTransform.GetRotation(), cylinderCenterBottom ), Axis::Z, radius, color, thickness, depthTestState, TTL );

        //-------------------------------------------------------------------------

        if ( isCapsule )
        {
            Radians const radiansPerEdge( Math::TwoPi / g_numCircleVertices );

            auto DrawSemiCircle = [&]( Vector const& startPoint, Vector const& capCenterPoint, Vector const& shapeOrigin )
            {
                Vector planeVector = startPoint - capCenterPoint;
                Vector const rotationAxis = ( startPoint - capCenterPoint ).Cross3( startPoint - shapeOrigin ).GetNormalized3();
                Quaternion const rotation( rotationAxis, radiansPerEdge );

                Vector prevPlaneVector = planeVector;
                for ( auto c = 0; c < g_numCircleVertices / 2; c++ )
                {
                    prevPlaneVector = planeVector;
                    planeVector = rotation.RotateVector( planeVector );
                    DrawLine( capCenterPoint + prevPlaneVector, capCenterPoint + planeVector, color, thickness, depthTestState, TTL );
                }
            };

            DrawSemiCircle( lt0, cylinderCenterTop, origin );
            DrawSemiCircle( lt2, cylinderCenterTop, origin );
            DrawSemiCircle( lt4, cylinderCenterTop, origin );
            DrawSemiCircle( lt6, cylinderCenterTop, origin );

            DrawSemiCircle( lb0, cylinderCenterBottom, origin );
            DrawSemiCircle( lb2, cylinderCenterBottom, origin );
            DrawSemiCircle( lb4, cylinderCenterBottom, origin );
            DrawSemiCircle( lb6, cylinderCenterBottom, origin );
        }
    }

    //-------------------------------------------------------------------------
    // Compound Shapes
    //-------------------------------------------------------------------------

    void DrawContext::DrawArrow( Float3 const& startPoint, Float3 const& endPoint, Float4 const& color, float thickness, DepthTest depthTestState, Seconds TTL )
    {
        constexpr static float const minArrowHeadThickness = 16.0f; // 16 pixels
        constexpr static float const maxArrowHeadLength = 0.1f; // 10cm

        Vector const vStart( startPoint );
        Vector const vEnd( endPoint );

        Vector arrowDirection;
        float arrowLength;
        ( vEnd - vStart ).ToDirectionAndLength3( arrowDirection, arrowLength );

        float const arrowHeadLength = Math::Max( arrowLength * 0.8f, arrowLength - maxArrowHeadLength );
        float const arrowHeadThickness = Math::Max( thickness * 2, minArrowHeadThickness );
        Vector const vArrowHeadStartPoint = vStart + ( arrowDirection * arrowHeadLength );
        Float3 const arrowHeadStartPoint = vArrowHeadStartPoint.ToFloat3();

        InternalDrawLine( m_commandBuffer, startPoint, arrowHeadStartPoint, color, thickness, depthTestState, TTL );
        InternalDrawLine( m_commandBuffer, arrowHeadStartPoint, endPoint, color, arrowHeadThickness, 2.0f, depthTestState, TTL );
    }

    void DrawContext::DrawCone( Transform const& transform, Radians coneAngle, float length, Float4 const& color, float thickness, DepthTest depthTestState, Seconds TTL )
    {
        Vector const capOffset = ( transform.GetForwardVector() * length );
        float const coneCapRadius = Math::Tan( coneAngle.ToFloat() ) * length;

        Transform capTransform = transform;
        capTransform.SetScale( coneCapRadius );
        capTransform.AddTranslation( capOffset );

        // Draw cone cap
        //-------------------------------------------------------------------------

        auto verts = EE_STACK_ARRAY_ALLOC( Vector, g_numCircleVertices );
        for ( auto i = 0; i < g_numCircleVertices; i++ )
        {
            verts[i] = capTransform.TransformPoint( g_circleVerticesYUp[i] );
        }

        // Register line commands
        for ( auto i = 1; i < g_numCircleVertices; i++ )
        {
            InternalDrawLine( m_commandBuffer, transform.GetTranslation(), verts[i], color, thickness, depthTestState, TTL );
            InternalDrawLine( m_commandBuffer, verts[i - 1], verts[i], color, thickness, depthTestState, TTL );
        }

        InternalDrawLine( m_commandBuffer, transform.GetTranslation(), verts[0], color, thickness, depthTestState, TTL );
        InternalDrawLine( m_commandBuffer, verts[0], verts[g_numCircleVertices-1], color, thickness, depthTestState, TTL );
    }
}
#endif