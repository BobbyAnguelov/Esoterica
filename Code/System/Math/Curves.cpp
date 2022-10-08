#include "Curves.h"
#include "Vector.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    struct GaussLengendreCoefficient
    {
        float m_abscissa; // xi
        float m_weight;   // wi
    };

    static constexpr GaussLengendreCoefficient gauss_lengendre_coefficients[] =
    {
           { 0.0f, 0.5688889f },
           { -0.5384693f, 0.47862867f },
           { 0.5384693f, 0.47862867f },
           { -0.90617985f, 0.23692688f },
           { 0.90617985f, 0.23692688f }
    };

    //-------------------------------------------------------------------------

    float QuadraticBezier::GetEstimatedLength( Vector const& p0, Vector const& cp, Vector const& p1, uint32_t numDiscretizations )
    {
        float distance = 0;
        float step = 1.0f / numDiscretizations;
        float currentT = step;
        Vector previousPoint = p0;

        while ( true )
        {
            auto currentPoint = GetPoint( p0, cp, p1, currentT );
            distance += previousPoint.GetDistance3( currentPoint );
            previousPoint = currentPoint;

            if ( currentT == 1.0f )
            {
                break;
            }

            currentT = Math::Min( currentT + step, 1.0f );
        }

        return distance;
    }

    //-------------------------------------------------------------------------

    float CubicBezier::GetEstimatedLength( Vector const& p0, Vector const& cp0, Vector const& cp1, Vector const& p1, uint32_t numDiscretizations )
    {
        float distance = 0;
        float step = 1.0f / numDiscretizations;
        float currentT = step;
        Vector previousPoint = p0;

        while ( true )
        {
            auto currentPoint = GetPoint( p0, cp0, cp1, p1, currentT );
            distance += previousPoint.GetDistance3( currentPoint );
            previousPoint = currentPoint;

            if ( currentT == 1.0f )
            {
                break;
            }

            currentT = Math::Min( currentT + step, 1.0f );
        }

        return distance;
    }

    //-------------------------------------------------------------------------

    float CubicHermite::GetSplineLength( Vector const& point0, Vector const& tangent0, Vector const& point1, Vector const& tangent1 )
    {
        Vector const c0 = tangent0;
        Vector const c1 = ( ( point1 - point0 ) * 6.0f ) - ( tangent0 * 4.0f ) - ( tangent1 * 2.0f );
        Vector const c2 = ( ( point0 - point1 ) * 6.0f ) + ( tangent0 + tangent1 ) * 3.0f;

        auto const EvaluateDerivative = [c0, c1, c2] ( float t )
        {
            return c0 + ( ( c1 + ( c2 * t ) ) * t );
        };

        float length = 0.f;
        for ( auto const& coefficient : gauss_lengendre_coefficients )
        {
            float const t = 0.5f * ( 1.f + coefficient.m_abscissa ); // This and the final (0.5 *) below are needed for a change of interval to [0, 1] from [-1, 1]
            length += EvaluateDerivative( t ).GetLength3() * coefficient.m_weight;
        }

        return 0.5f * length;
    }

    float CubicHermite::GetSplineLength( float const& point0, float const& tangent0, float const& point1, float const& tangent1 )
    {
        float const c0 = tangent0;
        float const c1 = ( ( point1 - point0 ) * 6.0f ) - ( tangent0 * 4.0f ) - ( tangent1 * 2.0f );
        float const c2 = ( ( point0 - point1 ) * 6.0f ) + ( tangent0 + tangent1 ) * 3.0f;

        auto const EvaluateDerivative = [c0, c1, c2] ( float t )
        {
            return c0 + ( ( c1 + ( c2 * t ) ) * t );
        };

        float length = 0.f;
        for ( auto const& coefficient : gauss_lengendre_coefficients )
        {
            float const t = 0.5f * ( 1.f + coefficient.m_abscissa ); // This and the final (0.5 *) below are needed for a change of interval to [0, 1] from [-1, 1]
            length += EvaluateDerivative( t ) * coefficient.m_weight;
        }

        return 0.5f * length;
    }
}