#pragma once

#include "Plane.h"
#include "Line.h"
#include "BoundingVolumes.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    enum class ConvexIntersectResult
    {
        None = 0,
        Single,
        Double,
    };

    //-------------------------------------------------------------------------

    struct LineLineResult
    {
        EE_FORCE_INLINE operator bool() const { return m_intersects; }

    public:

        Vector                      m_intersectionPoint;
        bool                        m_intersects = false;
    };

    inline LineLineResult IntersectLineLine( LineSegment const& l0, LineSegment const& l1 )
    {
        Vector closestP, closestQ;
        float tP = 0, tQ = 0;
        float squaredDist = FLT_MAX;
        LineSegment::GetClosestPointsBetweenSegments( l0, l1, tP, closestP, tQ, closestQ, &squaredDist );

        LineLineResult result;
        result.m_intersects = squaredDist < Math::Epsilon;
        result.m_intersectionPoint = closestP;
        return result;
    }

    // Plane Intersections
    //-------------------------------------------------------------------------

    struct RayPlaneResult
    {
        EE_FORCE_INLINE operator bool() const { return m_intersects; }

    public:

        Vector                      m_intersectionPoint;
        float                       m_intersectionDistance;
        bool                        m_intersects = false;
    };

    EE_BASE_API RayPlaneResult IntersectRayPlane( Vector const& rayOrigin, Vector const& rayDirection, Plane const& plane );

    inline RayPlaneResult IntersectRayPlane( Ray const& ray, Plane const& plane )
    {
        return IntersectRayPlane( ray.GetStartPoint(), ray.GetDirection(), plane );
    }

    //-------------------------------------------------------------------------

    EE_BASE_API RayPlaneResult IntersectLinePlane( Vector const& linePoint0, Vector const& linePoint1, Plane const& plane );

    inline RayPlaneResult IntersectLinePlane( LineSegment const& line, Plane const& plane )
    {
        return IntersectLinePlane( line.GetStartPoint(), line.GetEndPoint(), plane );
    }

    //-------------------------------------------------------------------------

    struct PlanePlaneResult
    {
        EE_FORCE_INLINE operator bool() const { return m_intersects; }

    public:

        Vector                      m_intersectionLineStart;
        Vector                      m_intersectionLineEnd;
        bool                        m_intersects = false;
    };

    EE_BASE_API PlanePlaneResult IntersectPlanePlane( Plane const& planeA, Plane const& planeB );

    // Sphere Intersections
    //-------------------------------------------------------------------------

    struct RaySphereResult
    {
        EE_FORCE_INLINE operator bool() const { return m_result != ConvexIntersectResult::None; }

    public:

        Vector                      m_intersectionPoint0;
        Vector                      m_intersectionPoint1;
        float                       m_intersectionDistance0;
        float                       m_intersectionDistance1;
        ConvexIntersectResult       m_result = ConvexIntersectResult::None;
    };

    EE_BASE_API RaySphereResult IntersectRaySphere( Vector const& rayOrigin, Vector const& rayDirection, Vector const& sphereOrigin, float sphereRadius );

    inline RaySphereResult IntersectRaySphere( Ray const& ray, Vector const& sphereOrigin, float sphereRadius )
    {
        return IntersectRaySphere( ray.GetStartPoint(), ray.GetDirection(), sphereOrigin, sphereRadius );
    }

    // Box Intersections
    //-------------------------------------------------------------------------

    struct RayBoxResult
    {
        EE_FORCE_INLINE operator bool() const { return m_intersects; }

    public:

        float                       m_T;
        bool                        m_intersects = false;
    };

    // Intersect a ray and a box, returns true if an intersection exists, also optionally returns the distance along the ray
    EE_BASE_API RayBoxResult IntersectRayBox( Vector const& startPoint, Vector const& dir, AABB const& box );

    // Intersect a ray and a box, returns true if an intersection exists, also optionally returns the distance along the ray
    inline RayBoxResult IntersectRayBox( Ray const& ray, AABB const& box )
    {
        return IntersectRayBox( ray.GetStartPoint(), ray.GetDirection(), box );
    }

    // Intersect a line and a box, returns true if an intersection exists, also optionally returns the distance along the line
    inline RayBoxResult LineBox( LineSegment const& line, AABB const& box )
    {
        RayBoxResult result = IntersectRayBox( line.GetStartPoint(), line.GetDirection(), box );
        if ( result.m_T < 0.0f || result.m_T > line.GetLength() )
        {
            result.m_intersects = false;
        }

        return result;
    }

    // Intersect a ray and a box, returns true if an intersection exists, also optionally returns the distance along the ray
    EE_BASE_API RayBoxResult IntersectRayBox( Vector const& startPoint, Vector const& dir, OBB const& box );

    // Intersect a ray and a box, returns true if an intersection exists, also optionally returns the distance along the ray
    inline RayBoxResult IntersectRayBox( Ray const& ray, OBB const& box )
    {
        return IntersectRayBox( ray.GetStartPoint(), ray.GetDirection(), box );
    }

    // Intersect a line and a box, returns true if an intersection exists, also optionally returns the distance along the line
    inline RayBoxResult LineBox( LineSegment const& line, OBB const& box )
    {
        RayBoxResult result = IntersectRayBox( line.GetStartPoint(), line.GetDirection(), box );
        if ( result.m_T < 0.0f || result.m_T > line.GetLength() )
        {
            result.m_intersects = false;
        }

        return result;
    }

    // Capsule
    //-------------------------------------------------------------------------

    struct RayCapsuleResult
    {
        EE_FORCE_INLINE operator bool() const { return m_intersects; }

    public:

        float                       m_T;
        bool                        m_intersects = false;
    };

    EE_BASE_API RayCapsuleResult IntersectRayCapsule( Vector const& rayOrigin, Vector const& rayDir, Vector const& capOrigin0, Vector const& capOrigin1, float radius );

    // Intersect a ray and a box, returns true if an intersection exists, also optionally returns the distance along the ray
    inline RayCapsuleResult IntersectRayCapsule( Ray const& ray, Vector const& capOrigin0, Vector const& capOrigin1, float radius )
    {
        return IntersectRayCapsule( ray.GetStartPoint(), ray.GetDirection(), capOrigin0, capOrigin1, radius );
    }
}