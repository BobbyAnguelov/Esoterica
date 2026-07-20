#include "Line.h"

//-------------------------------------------------------------------------

namespace EE
{
    void LineSegment::GetClosestPointsBetweenSegments( LineSegment const& p, LineSegment const& q, float& tP, Vector& closestP, float& tQ, Vector& closestQ, float* pSquaredDistanceBetweenPoints )
    {
        Vector dirP = p.GetEndPoint() - p.GetStartPoint();
        Vector dirQ = q.GetEndPoint() - q.GetStartPoint();
        Vector r = p.GetStartPoint() - q.GetStartPoint();

        float a = dirP.GetLengthSquared3();
        float e = dirQ.GetLengthSquared3();
        float f = dirQ.GetDot3( r );

        // Both segments degenerate into points
        bool const isPoint0 = a <= Math::Epsilon;
        bool const isPoint1 = e <= Math::Epsilon;
        if ( isPoint0 && isPoint1 )
        {
            tP = tQ = 0.0f;
            closestP = p.GetStartPoint();
            closestQ = q.GetStartPoint();
        }
        else
        {
            // First segment degenerates into a point
            if ( isPoint0 )
            {
                tP = 0.0f;
                tQ = Math::Clamp( f / e, 0.f, 1.f );
            }
            else 
            {
                float c = dirP.GetDot3( r );

                // Second segment degenerates into a point
                if ( isPoint1 )
                {
                    tQ = 0.0f;
                    tP = Math::Clamp( -c / a, 0.0f, 1.0f );
                }
                else // General non-degenerate case
                {
                    float b = dirP.GetDot3( dirQ );
                    float denom = ( a * e ) - ( b * b );  // Always >= 0

                    // If segments not parallel, compute closest point on infinite lines and clamp to segments. Otherwise pick arbitrary tP (here 0).
                    if ( denom != 0.0f )
                    {
                        tP = Math::Clamp( ( ( b * f ) - ( c * e ) ) / denom, 0.0f, 1.0f );
                    }
                    else
                    {
                        tP = 0.0;
                    }

                    tQ = ( ( b * tP ) + f ) / e;

                    // If tQ is outside [0,1], recompute tP for the clamped tQ
                    if ( tQ < 0.0f )
                    {
                        tQ = 0.0f;
                        tP = Math::Clamp( -c / a, 0.0f, 1.0f );
                    }
                    else if ( tQ > 1.0f )
                    {
                        tQ = 1.0f;
                        tP = Math::Clamp( ( b - c ) / a, 0.0f, 1.0f );
                    }
                }
            }

            closestP = p.GetStartPoint() + dirP * tP;
            closestQ = q.GetStartPoint() + dirQ * tQ;
        }

        //-------------------------------------------------------------------------

        if ( pSquaredDistanceBetweenPoints != nullptr )
        {
            *pSquaredDistanceBetweenPoints = ( closestP - closestQ ).GetLengthSquared3();
        }
    }
}