#include "Plane.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool Plane::IntersectRay( Vector const& rayOrigin, Vector const& rayDirection, Vector& intersectionPoint ) const
    {
        auto P = ToVector();
        auto planeDotRayOrigin = Vector::Dot3( P, rayOrigin );
        auto planeDotRayDir = Vector::Dot3( P, rayDirection );

        if ( planeDotRayDir.IsZero4() )
        {
            return false;
        }

        auto T = ( P.GetSplatW() + planeDotRayOrigin ).GetNegated();
        T /= planeDotRayDir;

        intersectionPoint = Vector::MultiplyAdd( rayDirection, T, rayOrigin );
        return true;
    }

    bool Plane::IntersectPlane( Plane const& otherPlane, Vector& outLineStart, Vector& outLineEnd ) const
    {
        Vector vPlane = AsVector();
        Vector vOtherPlane = otherPlane.AsVector();

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
        outLineStart = Vector::Select( LinePoint1, Vector::QNaN, Control );
        outLineEnd = Vector::Select( LinePoint2, Vector::QNaN, Control );

        return !outLineStart.IsNaN4();
    }

    bool Plane::IntersectLine( Vector const& point0, Vector const& point1, Vector& intersectionPoint ) const
    {
        auto P = ToVector();
        auto rayDirection = point1 - point0;

        auto planeDotRayOrigin = Vector::Dot3( P, point0 );
        auto planeDotRayDir = Vector::Dot3( P, rayDirection );

        if ( planeDotRayDir.IsZero4() )
        {
            return false;
        }

        auto T = ( P.GetSplatW() + planeDotRayOrigin ).GetNegated();
        T /= planeDotRayDir;

        if ( T.IsGreaterThanEqual4( Vector::Zero ) && T.IsLessThanEqual3( Vector::One ) )
        {
            intersectionPoint = Vector::MultiplyAdd( rayDirection, T, point0 );
            return true;
        }

        return false;
    }

}