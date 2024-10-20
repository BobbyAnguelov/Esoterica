#include "Rectangle.h"
#include "Line.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    template<typename T>
    static Float2 GetClosestPointOnRectBorder( T const& rect, Float2 const& point )
    {
        Vector const verts[4] = { rect.GetTL(), rect.GetTR(), rect.GetBL(), rect.GetBR() };

        Vector const closestPoints[4] =
        {
            LineSegment( verts[0], verts[1] ).GetClosestPointOnSegment( point ),
            LineSegment( verts[2], verts[3] ).GetClosestPointOnSegment( point ),
            LineSegment( verts[0], verts[2] ).GetClosestPointOnSegment( point ),
            LineSegment( verts[1], verts[3] ).GetClosestPointOnSegment( point )
        };

        Vector vPoint( point );

        float distancesSq[4] =
        {
            ( closestPoints[0] - vPoint ).GetLengthSquared2(),
            ( closestPoints[1] - vPoint ).GetLengthSquared2(),
            ( closestPoints[2] - vPoint ).GetLengthSquared2(),
            ( closestPoints[3] - vPoint ).GetLengthSquared2()
        };

        // Get closest point
        float lowestDistance = FLT_MAX;
        int32_t closestPointIdx = -1;
        for ( auto i = 0; i < 4; i++ )
        {
            if ( distancesSq[i] < lowestDistance )
            {
                closestPointIdx = i;
                lowestDistance = distancesSq[i];
            }
        }

        EE_ASSERT( closestPointIdx >= 0 && closestPointIdx < 4 );
        return closestPoints[closestPointIdx].ToFloat2();
    }

    //-------------------------------------------------------------------------

    Float2 Rectangle::GetClosestPointOnBorder( Float2 const& point ) const
    {
        return GetClosestPointOnRectBorder( *this, point );
    }

    //-------------------------------------------------------------------------

    Float2 ScreenSpaceRectangle::GetClosestPointOnBorder( Float2 const& point ) const
    {
        return GetClosestPointOnRectBorder( *this, point );
    }
}