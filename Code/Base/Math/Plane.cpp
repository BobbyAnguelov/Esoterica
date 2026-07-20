#include "Plane.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool Plane::RayIntersection( Vector const& rayOrigin, Vector const& rayDirection, Vector& intersectionDistance, Vector& intersectionPoint ) const
    {
        auto P = ToVector();
        auto planeDotRayOrigin = Vector::Dot3( P, rayOrigin );
        auto planeDotRayDir = Vector::Dot3( P, rayDirection );

        if ( planeDotRayDir.IsZero4() )
        {
            return false;
        }

        intersectionDistance = ( P.GetSplatW() + planeDotRayOrigin ).GetNegated();
        intersectionDistance /= planeDotRayDir;

        intersectionPoint = Vector::MultiplyAdd( rayDirection, intersectionDistance, rayOrigin );
        return true;
    }
}
