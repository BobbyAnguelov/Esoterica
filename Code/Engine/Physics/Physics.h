#pragma once
#include "Engine/_Module/API.h"
#include "PhysicsQuery.h"
#include "PhysicsTypes.h"
#include "Engine/ThirdParty/box3d/include/box3d/box3d.h"
#include "Base/Math/Plane.h"
#include "Base/Math/BoundingVolumes.h"
#include "Base/Types/Color.h"
#include "Base/Types/PointerID.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
    class DebugDrawContext;

    namespace Render
    {
        struct DebugMesh;
        class DebugMeshRegistry;
    }
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    //-------------------------------------------------------------------------
    // Type Conversion
    //-------------------------------------------------------------------------

    EE_FORCE_INLINE Float2 FromBox3D( b3Vec2 const& v )
    {
        return Float2( v.x, v.y );
    }

    EE_FORCE_INLINE Float3 FromBox3D( b3Vec3 const& v )
    {
        return Float3( v.x, v.y, v.z );
    }

    EE_FORCE_INLINE Quaternion FromBox3D( b3Quat const& q )
    {
        return Quaternion( q.v.x, q.v.y, q.v.z, q.s );
    }

    EE_FORCE_INLINE Transform FromBox3D( b3Transform const& v )
    {
        return Transform( FromBox3D( v.q ), FromBox3D( v.p ) );
    }

    EE_FORCE_INLINE Plane FromBox3D( b3Plane const& v )
    {
        return Plane::FromNormalAndDistance( FromBox3D( v.normal ), v.offset );
    }

    EE_FORCE_INLINE AABB FromBox3D( b3AABB const& v )
    {
        return AABB::FromMinMax( FromBox3D( v.lowerBound ), FromBox3D( v.upperBound ) );
    }

    EE_FORCE_INLINE Matrix FromBox3D( b3Matrix3& v )
    {
        Matrix const m
        (
            v.cx.x, v.cy.x, v.cz.x, 0.0f,
            v.cx.y, v.cy.y, v.cz.y, 0.0f,
            v.cx.z, v.cy.z, v.cz.z, 0.0f,
            0, 0, 0, 1.0f
        );

        return m;
    }

    EE_FORCE_INLINE Color FromBox3D( b3HexColor c, float alpha = 1.0f )
    {
        uint8_t const b = (uint8_t) c;
        uint8_t const g = (uint8_t) ( c >> 8 );
        uint8_t const r = (uint8_t) ( c >> 16 );
        return Color( r, g, b, uint8_t( alpha * 255 ) );
    }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE b3Vec2 ToBox3D( Float2 const& v )
    {
        return { v.m_x, v.m_y };
    }

    EE_FORCE_INLINE b3Vec3 ToBox3D( Float3 const& v )
    {
        return { v.m_x, v.m_y, v.m_z };
    }

    EE_FORCE_INLINE b3Vec3 ToBox3D( Vector const& v )
    {
        Float3 const f3 = v.ToFloat3();
        return { f3.m_x, f3.m_y, f3.m_z };
    }

    EE_FORCE_INLINE b3Quat ToBox3D( Quaternion v )
    {
        Float4 const f4 = v.GetNormalized().ToFloat4();
        return { { f4.m_x, f4.m_y, f4.m_z }, f4.m_w };
    }

    EE_FORCE_INLINE b3Transform ToBox3D( Transform const& v )
    {
        EE_ASSERT( v.IsRigidTransform() );
        return { ToBox3D( v.GetTranslation().ToFloat3() ), ToBox3D( v.GetRotation().GetNormalized() ) };
    }

    EE_FORCE_INLINE b3Plane ToBox3D( Plane const& v )
    {
        EE_ASSERT( v.IsNormalized() );
        Float4 const f4 = v.ToFloat4();
        return { { f4.m_x, f4.m_y, f4.m_z }, f4.m_w };
    }

    EE_FORCE_INLINE b3AABB ToBox3D( AABB const& v )
    {
        return { ToBox3D( v.GetMin() ), ToBox3D( v.GetMax() ) };
    }

    EE_FORCE_INLINE b3Matrix3 ToBox3D( Matrix const& v )
    {
        b3Matrix3 m;

        m.cx.x = v[0][0];
        m.cx.y = v[1][0];
        m.cx.z = v[2][0];

        m.cy.x = v[0][1];
        m.cy.y = v[1][1];
        m.cy.z = v[2][1];

        m.cz.x = v[0][2];
        m.cz.y = v[1][2];
        m.cz.z = v[2][2];

        return m;
    }

    EE_FORCE_INLINE b3Filter ToBox3D( CollisionSettings const& v )
    {
        return { uint64_t( 1 ) << (uint8_t) v.m_category, v.m_collisionMask };
    }

    EE_FORCE_INLINE b3QueryFilter ToBox3D( QueryRules const& rules )
    {
        return { rules.GetCategory(), rules.GetCollisionMask() };
    }

    //-------------------------------------------------------------------------
    // Helpers
    //-------------------------------------------------------------------------

    // Get the points for a unit cylinder hull we can then scale as needed for cast and overlap queries
    EE_ENGINE_API TVector<Float4> const& GetUnitCylinderHullPoints();

    EE_FORCE_INLINE void GetTransformedCylinderHullPoints( Transform const& transform, float radius, float halfHeight, TInlineVector<b3Vec3, 32>& outHullPoints )
    {
        auto const& unitHullVerts = GetUnitCylinderHullPoints();
        Vector const scale( radius, radius, halfHeight );

        for ( int32_t i = 0; i < 32; i++ )
        {
            Vector const point = transform.TransformPoint( unitHullVerts[i] * scale );
            outHullPoints.emplace_back( ToBox3D( point ) );
        }
    }

    EE_FORCE_INLINE void GetTransformedCylinderHullPoints( Vector const& position, Quaternion const& orientation, float radius, float halfHeight, TInlineVector<b3Vec3, 32>& outHullPoints )
    {
        GetTransformedCylinderHullPoints( Transform( orientation, position ), radius, halfHeight, outHullPoints );
    }

    EE_FORCE_INLINE void SetAngularMotorTarget( b3JointId jointId, Quaternion const& targetRelativeRotation, float springHertz )
    {
        float const clampedSpringHertz = Math::Clamp( springHertz, 0.0f, 60.0f );
        b3Quat const relativeRotation = ToBox3D( targetRelativeRotation );

        b3JointType const jointType = b3Joint_GetType( jointId );

        if ( jointType == b3_revoluteJoint )
        {
            b3RevoluteJoint_SetTargetAngle( jointId, b3GetTwistAngle( relativeRotation ) );
            b3RevoluteJoint_SetSpringHertz( jointId, clampedSpringHertz );
        }
        else if ( jointType == b3_sphericalJoint )
        {
            b3SphericalJoint_SetTargetRotation( jointId, relativeRotation );
            b3SphericalJoint_SetSpringHertz( jointId, clampedSpringHertz );
        }
    }

    //-------------------------------------------------------------------------
    // Debug
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    EE_ENGINE_API void CreateDebugMeshFromTriangleMesh( b3MeshData const* pMesh, Render::DebugMesh& outMesh );
    EE_ENGINE_API void CreateDebugMeshFromConvexHull( b3HullData const* pHull, Render::DebugMesh& outMesh );
    #endif

    //-------------------------------------------------------------------------
    // Core
    //-------------------------------------------------------------------------

    namespace Core
    {
        void Initialize();
        void Shutdown();

        #if EE_DEVELOPMENT_TOOLS
        PointerID RegisterDebugShapeInstance( Render::DebugMeshRegistry* pDebugMeshRegistry, b3DebugShape const* pDebugShape );
        void UnregisterDebugShapeInstance( Render::DebugMeshRegistry* pDebugMeshRegistry, PointerID ID );
        void DrawDebugShapeInstance( DebugDrawContext* pDrawContext, PointerID instanceID, Transform const& transform, Color const& color );
        #endif
    };
}