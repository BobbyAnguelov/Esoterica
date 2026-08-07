#include "Curves.h"
#include "Vector.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    Polyline::Segment::Segment( Vector const& start, Vector const& end )
    {
        m_start = start;
        m_end = end;
        ( m_end - m_start ).ToDirectionAndLength3( m_direction, m_length );
    }

    #if EE_DEVELOPMENT_TOOLS
    void Polyline::Draw( DebugDrawContext& ctx, Color color, float thickness, float zOffset ) const
    {
        Vector const offset( 0, 0, zOffset );
        int32_t const numSegments = int32_t( m_segments.size() );
        for ( int32_t i = 0; i < numSegments; i++ )
        {
            ctx.DrawLine( m_segments[i].m_start + offset, m_segments[i].m_end + offset, color, thickness );
        }
    }
    #endif

    //-------------------------------------------------------------------------

    void QuadraticBezier::CreatePolyline( Vector const& p0, Vector const& cp, Vector const& p1, int32_t numDiscretizations, Polyline& outLine )
    {
        outLine.Clear();

        float distance = 0;
        float step = 1.0f / numDiscretizations;
        float currentT = step;
        Vector previousPoint = p0;

        while ( true )
        {
            Vector const point = GetPoint( p0, cp, p1, currentT );
            Polyline::Segment& segment = outLine.m_segments.emplace_back( previousPoint, point );
            segment.m_distanceAlongCurve = distance;
            segment.m_previousSegmentDirection = ( outLine.m_segments.size() > 1 ) ? ( outLine.m_segments.end() - 2 )->m_direction : segment.m_direction;
            distance += segment.m_length;
            previousPoint = point;

            if ( currentT == 1.0f )
            {
                break;
            }

            currentT = Math::Min( currentT + step, 1.0f );
        }

        outLine.m_length = distance;
    }

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

    void CubicBezier::CreatePolyline( Vector const& p0, Vector const& cp0, Vector const& cp1, Vector const& p1, int32_t numDiscretizations, Polyline& outLine )
    {
        outLine.Clear();

        float distance = 0;
        float step = 1.0f / numDiscretizations;
        float currentT = step;
        Vector previousPoint = p0;

        while ( true )
        {
            Vector const point = GetPoint( p0, cp0, cp1, p1, currentT );
            Polyline::Segment& segment = outLine.m_segments.emplace_back( previousPoint, point );
            segment.m_distanceAlongCurve = distance;
            segment.m_previousSegmentDirection = ( outLine.m_segments.size() > 1 ) ? ( outLine.m_segments.end() - 2 )->m_direction : segment.m_direction;
            distance += segment.m_length;
            previousPoint = point;

            if ( currentT == 1.0f )
            {
                break;
            }

            currentT = Math::Min( currentT + step, 1.0f );
        }

        outLine.m_length = distance;
    }

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

    struct GaussLengendreCoefficient
    {
        float m_abscissa; // xi
        float m_weight;   // wi
    };

    constexpr static GaussLengendreCoefficient const g_gaussLengendreCoefficients[] =
    {
           { 0.0f, 0.5688889f },
           { -0.5384693f, 0.47862867f },
           { 0.5384693f, 0.47862867f },
           { -0.90617985f, 0.23692688f },
           { 0.90617985f, 0.23692688f }
    };

    void CubicHermite::CreatePolyline( Vector const& point0, Vector const& tangent0, Vector const& point1, Vector const& tangent1, int32_t numDiscretizations, Polyline& outLine )
    {
        outLine.Clear();

        float distance = 0;
        float step = 1.0f / numDiscretizations;
        float currentT = step;
        Vector previousPoint = point0;

        while ( true )
        {
            Vector const point = GetPoint( point0, tangent0, point1, tangent1, currentT );
            Polyline::Segment& segment = outLine.m_segments.emplace_back( previousPoint, point );
            segment.m_distanceAlongCurve = distance;
            segment.m_previousSegmentDirection = ( outLine.m_segments.size() > 1 ) ? ( outLine.m_segments.end() - 2 )->m_direction : segment.m_direction;
            distance += segment.m_length;
            previousPoint = point;

            if ( currentT == 1.0f )
            {
                break;
            }

            currentT = Math::Min( currentT + step, 1.0f );
        }

        outLine.m_length = distance;
    }

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
        for ( auto const& coefficient : g_gaussLengendreCoefficients )
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
        for ( auto const& coefficient : g_gaussLengendreCoefficients )
        {
            float const t = 0.5f * ( 1.f + coefficient.m_abscissa ); // This and the final (0.5 *) below are needed for a change of interval to [0, 1] from [-1, 1]
            length += EvaluateDerivative( t ) * coefficient.m_weight;
        }

        return 0.5f * length;
    }
}