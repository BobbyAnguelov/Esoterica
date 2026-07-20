#include "Intersection.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    RayPlaneResult IntersectRayPlane( Vector const& rayOrigin, Vector const& rayDirection, Plane const& plane )
    {
        RayPlaneResult result;

        Vector T;
        if ( plane.RayIntersection( rayOrigin, rayDirection, T, result.m_intersectionPoint ) )
        {
            result.m_intersectionDistance = T.GetX();
            result.m_intersects = true;
        }

        return result;
    }

    RayPlaneResult IntersectLinePlane( Vector const& point0, Vector const& point1, Plane const& plane )
    {
        RayPlaneResult result;

        Vector const rayDirection = point1 - point0;

        Vector T;
        if ( plane.RayIntersection( point0, rayDirection, T, result.m_intersectionPoint ) )
        {
            if ( T.IsGreaterThanEqual4( Vector::Zero ) && T.IsLessThanEqual4( Vector::One ) )
            {
                result.m_intersectionDistance = T.GetX();
                result.m_intersects = true;
            }
        }

        return result;
    }

    PlanePlaneResult IntersectPlanePlane( Plane const& planeA, Plane const& planeB )
    {
        Vector vPlane = planeA.AsVector();
        Vector vOtherPlane = planeB.AsVector();

        Vector V1 = Vector::Cross3( vOtherPlane, vPlane );
        Vector LengthSq = V1.LengthSquared3();

        Vector V2 = Vector::Cross3( vOtherPlane, V1 );

        Vector P1W = vPlane.GetSplatW();
        Vector Point = V2 * P1W;

        Vector V3 = Vector::Cross3( V1, vPlane );

        Vector P2W = vOtherPlane.GetSplatW();
        Point = Vector::MultiplyAdd( V3, P2W, Point );

        Vector LinePoint1 = Point / LengthSq;
        Vector LinePoint2 = LinePoint1 + V1;
        Vector Control = LengthSq.LessThanEqual( Vector::Epsilon );

        PlanePlaneResult result;
        result.m_intersectionLineStart = Vector::Select( LinePoint1, Vector::QNaN, Control );
        result.m_intersectionLineEnd = Vector::Select( LinePoint2, Vector::QNaN, Control );
        result.m_intersects = !result.m_intersectionLineStart.IsNaN4();

        return result;
    }

    RaySphereResult IntersectRaySphere( Vector const& rayOrigin, Vector const& rayDirection, Vector const& sphereOrigin, float sphereRadius )
    {
        EE_ASSERT( rayDirection.IsNormalized3() );
        EE_ASSERT( sphereRadius > 0.0f );

        RaySphereResult result;

        Vector const radiusSq( sphereRadius * sphereRadius );
        Vector const& rayOriginToSphereOrigin = ( sphereOrigin - rayOrigin );
        Vector t = -rayOriginToSphereOrigin.Dot3( rayDirection );
        Vector const c = rayOriginToSphereOrigin.Dot3( rayOriginToSphereOrigin ) - radiusSq;

        if ( c.IsGreaterThan4( Vector::Zero ) && t.IsGreaterThan4( Vector::Zero ) )
        {
            return result;
        }

        Vector h = ( t * t ) - c;
        if ( h.IsLessThan4( Vector::Zero ) )
        {
            return result;
        }

        h = h.GetSqrt();
        t.Negate();
        Vector t0 = t - h;
        Vector t1 = t + h;

        //-------------------------------------------------------------------------

        if ( t0.IsAnyGreaterThan( t1 ) )
        {
            Vector tmp = t0;
            t0 = t1;
            t1 = tmp;
        }

        if ( t0.IsLessThan4( Vector::Zero ) )
        {
            t0 = t1; // If t0 is negative, let's use t1 instead.

            if ( t0.IsLessThan4( Vector::Zero ) )
            {
                return result;
            }

            result.m_intersectionDistance0 = t0.GetX();
            result.m_intersectionPoint0 = Vector::MultiplyAdd( rayDirection, t0, rayOrigin );
            result.m_result = ConvexIntersectResult::Single;
        }
        else
        {
            result.m_intersectionDistance0 = t0.GetX();
            result.m_intersectionDistance1 = t1.GetX();
            result.m_intersectionPoint0 = Vector::MultiplyAdd( rayDirection, t0, rayOrigin );
            result.m_intersectionPoint1 = Vector::MultiplyAdd( rayDirection, t1, rayOrigin );
            result.m_result = ConvexIntersectResult::Double;
        }

        return result;
    }

    // Based on: https://tavianator.com/cgit/dimension.git/tree/libdimension/bvh/bvh.c#n196
    RayBoxResult IntersectRayBox( Vector const& startPoint, Vector const& dir, AABB const& box )
    {
        EE_ASSERT( dir.IsNormalized3() );

        Vector const min = box.GetMin();
        Vector const max = box.GetMax();
        Vector const recipDir = dir.GetReciprocal();

        Vector t1( ( min - startPoint ) * recipDir );
        Vector t2( ( max - startPoint ) * recipDir );

        Vector vTMin( Vector::Min( t1, t2 ) );
        Vector vTMax( Vector::Max( t1, t2 ) );

        float tmin = vTMin.GetMaxElement3();
        float tmax = vTMax.GetMinElement3();

        RayBoxResult result;
        result.m_T = tmin >= 0 ? tmin : tmax;
        result.m_intersects = tmax >= Math::Max( 0.0f, tmin );

        return result;
    }

    RayBoxResult IntersectRayBox( Vector const& startPoint, Vector const& dir, OBB const& box )
    {
        // Transform ray into box space i.e. convert the problem into a ray vs AABB test
        Vector startPointRelativeToBox = startPoint - box.m_center;
        Vector aabbRayOrigin = box.m_orientation.RotateVectorInverse( startPointRelativeToBox );
        Vector aabbRayDir = box.m_orientation.RotateVectorInverse( dir );
        return IntersectRayBox( aabbRayOrigin, aabbRayDir, AABB( Vector::Zero, box.m_extents ) );
    }

    // Based on box3D
    RayCapsuleResult IntersectRayCapsule( Vector const& rayOrigin, Vector const& rayDir, Vector const& capOrigin0, Vector const& capOrigin1, float radius )
    {
        EE_ASSERT( radius > 0.0f );
        EE_ASSERT( rayDir.IsNormalized3() );

        RayCapsuleResult result;

        //-------------------------------------------------------------------------

        float height = 0.0f;
        Vector cylinderSpineDir;
        Vector cylinderSpine = capOrigin1 - capOrigin0;
        cylinderSpine.ToDirectionAndLength3( cylinderSpineDir, height );

        // Compute height and handle degenerate capsules
        if ( height < 1000.0f * FLT_MIN )
        {
            RaySphereResult const rs = IntersectRaySphere( rayOrigin, rayDir, capOrigin0, radius );
            if ( rs )
            {
                result.m_intersects = true;
                result.m_T = rs.m_intersectionDistance0;
            }

            return result;
        }

        // Transform ray and capsule into local space capsule space
        Quaternion rotation = Quaternion::FromRotationBetweenUnitVectors( Vector::UnitY, cylinderSpineDir );

        // Capsule starts at the origin and is along the y-axis
        Vector a = Vector::Zero;
        Vector b = { 0.0f, height, 0.0f };
        Vector ab = b - a;

        // Ray expressed relative to capsule space (capsule along y-axis)
        Vector p = rotation.RotateVectorInverse( rayOrigin - capOrigin0 );
        Vector q = p + rotation.RotateVectorInverse( rayDir );
        Vector pq = q - p;
        float const pqx = pq.GetX();
        float const pqy = pq.GetY();
        float const pqz = pq.GetZ();

        // Ray 2D translation length squared
        float k1 = pq.GetLengthSquared2();

        // Ray start point 2D separation squared from circle
        float const px = p.GetX();
        float const py = p.GetY();
        float const pz = p.GetZ();
        float k3 = ( px * px ) + ( pz * pz ) - ( radius * radius );

        // Parallel case (2D ray translation is zero)
        if ( k1 < 1000.0f * FLT_MIN )
        {
            if ( k3 > 0.0f )
            {
                // Parallel and outside
                return result;
            }

            if ( 0.0f <= py && py <= height )
            {
                // Parallel and inside
                result.m_intersects = true;
                result.m_T = 0;
                return result;
            }

            // Below cylinder and casting upwards
            if ( py < 0.0f && pqy > 0.0f )
            {
                RaySphereResult const rs = IntersectRaySphere( p, ( q - p ).GetNormalized3(), capOrigin0, radius );
                if ( rs )
                {
                    result.m_intersects = true;
                    result.m_T = rs.m_intersectionDistance0;
                }

                return result;
            }

            // Above cylinder and casting downwards
            if ( py > height && pqy < 0.0f )
            {
                RaySphereResult const rs = IntersectRaySphere( p, ( q - p ).GetNormalized3(), b, radius );
                if ( rs )
                {
                    result.m_intersects = true;
                    result.m_T = rs.m_intersectionDistance0;
                }

                return result;
            }

            // Above or below and casting away from cylinder
            return result;
        }

        // Non-parallel case
        float k2 = pqx * px + pqz * pz;

        float discriminant = k2 * k2 - k1 * k3;
        if ( discriminant < 0.0f )
        {
            // No real roots - no intersection
            return result;
        }

        // Don't skip t < 0. This means that we start in the *infinite* cylinder and still might hit a cap
        float t = ( -k2 - Math::Sqrt( discriminant ) ) / k1;

        // This is the point on the ray that hits the infinite cylinder
        Vector c = Vector::MultiplyAdd( pq, Vector( t ), p );

        // This is the cylinder hit point relative to the capsule base
        Vector ac = c - a;

        // Fraction of the cylinder hit point along the capsule axis
        float s = ac.GetDot3( ab ) / ( height * height );

        if ( s < 0.0f )
        {
            // X projects outside A, run test through sphere at A
            RaySphereResult const rs = IntersectRaySphere( p, ( q - p ).GetNormalized3(), a, radius );
            if ( rs )
            {
                result.m_intersects = true;
                result.m_T = rs.m_intersectionDistance0;
            }

            return result;
        }

        if ( s > 1.0f )
        {
            // X projects outside B, run test through sphere at B
            RaySphereResult const rs = IntersectRaySphere( p, ( q - p ).GetNormalized3(), b, radius );
            if ( rs )
            {
                result.m_intersects = true;
                result.m_T = rs.m_intersectionDistance0;
            }

            return result;
        }

        if ( t < 0.0f )
        {
            // Ray starts inside
            result.m_intersects = true;
            result.m_T = 0;
            return result;
        }

        // Ray hits cylinder inside segment AB
        result.m_T = t;
        result.m_intersects = true;
        return result;
    }
}
