#pragma once
#include "Base/_Module/API.h"
#include "Base/Types/Arrays.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Math.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Vector;
}

//-------------------------------------------------------------------------

namespace EE::Math
{
    struct PointAndTangent
    {
        Vector m_point;
        Vector m_tangent; // NOTE: Tangents are NOT guaranteed to be normalized
    };

    //-------------------------------------------------------------------------

    struct EE_BASE_API Polyline
    {
        struct Segment
        {
            Segment() = default;
            Segment( Vector const& start, Vector const& end );

        public:

            Vector  m_start;
            Vector  m_end;
            Vector  m_direction;
            Vector  m_previousSegmentDirection;
            float   m_length = 0.0f;
            float   m_distanceAlongCurve = 0.0f; // The distance along the curve that the start point is at
        };

    public:

        inline bool IsValid() const { return m_segments.empty(); }

        inline int32_t GetNumSegments() const { return int32_t( m_segments.size() ); }

        void Clear()
        {
            m_segments.clear();
            m_length = 0.0f;
        }

        #if EE_DEVELOPMENT_TOOLS
        void Draw( DebugDrawContext& ctx, Color color, float thickness = 1, float zOffset = 0.0f ) const;
        #endif

    public:

        TVector<Segment>            m_segments;
        float                       m_length = 0;
    };

    //-------------------------------------------------------------------------

    class EE_BASE_API QuadraticBezier
    {
        // Simple static evaluation function
        static inline Vector GetPoint( Vector const& p0, Vector const& cp, Vector const& p1, float t )
        {
            float const oneMinusT = ( 1 - t );
            float const oneMinusTSquared = Math::Pow( oneMinusT, 2 );
            float const twoT = 2 * t;

            Vector const a = p0 * oneMinusTSquared;
            Vector const b = cp * twoT * oneMinusT;
            Vector const c = p1 * t * t;

            return a + b + c;
        }

        static inline Vector GetTangent( Vector const& p0, Vector const& cp, Vector const& p1, float t )
        {
            float const oneMinusT = ( 1 - t );
            float const twoT = 2 * t;

            Vector const a = ( cp - p0 ) * ( 2 * oneMinusT );
            Vector const b = ( p1 - cp ) * twoT;

            return a + b;
        }

        static inline PointAndTangent GetPointAndTangent( Vector const& p0, Vector const& cp, Vector const& p1, float t )
        {
            PointAndTangent result;

            float const oneMinusT = ( 1 - t );
            float const oneMinusTSquared = Math::Pow( oneMinusT, 2 );
            float const twoT = 2 * t;

            //-------------------------------------------------------------------------

            Vector const a = p0 * oneMinusTSquared;
            Vector const b = cp * twoT * oneMinusT;
            Vector const c = p1 * t * t;

            result.m_point = a + b + c;

            //-------------------------------------------------------------------------

            Vector const d = ( cp - p0 ) * ( 2 * oneMinusT );
            Vector const e = ( p1 - cp ) * twoT;

            result.m_tangent = d + e;

            //-------------------------------------------------------------------------

            return result;
        };

        // Create a polyline to represent the curve defined by the supplied parameters
        static void CreatePolyline( Vector const& p0, Vector const& cp, Vector const& p1, int32_t numDiscretizations, Polyline& outLine );

        // Returns the estimated length of the curve by discretizing it into N steps and calculating the distance between those points
        static float GetEstimatedLength( Vector const& p0, Vector const& cp, Vector const& p1, uint32_t numDiscretizations = 10 );
    };

    //-------------------------------------------------------------------------

    class EE_BASE_API CubicBezier
    {
    public:

        // Simple static evaluation function
        static inline Vector GetPoint( Vector const& p0, Vector const& cp0, Vector const& cp1, Vector const& p1, float t )
        {
            float const TSquared = Math::Pow( t, 2 );
            float const TCubed = TSquared * t;
            float const oneMinusT = ( 1 - t );
            float const oneMinusTSquared = Math::Pow( oneMinusT, 2 );
            float const oneMinusTCubed = Math::Pow( oneMinusT, 3 );

            Vector const a = p0 * oneMinusTCubed;
            Vector const b = cp0 * 3 * t * oneMinusTSquared;
            Vector const c = cp1 * 3 * TSquared * oneMinusT;
            Vector const d = p1 * TCubed;

            return a + b + c + d;
        }

        static inline Vector GetTangent( Vector const& p0, Vector const& cp0, Vector const& cp1, Vector const& p1, float t )
        {
            float const TSquared = Math::Pow( t, 2 );
            float const twoT = 2 * t;
            float const threeTSquared = ( 3 * TSquared );

            Vector const a = p0 * ( twoT - 1 - TSquared );
            Vector const b = cp0 * ( 1 - ( 4 * t ) + threeTSquared );
            Vector const c = cp1 * ( twoT - threeTSquared );
            Vector const d = p1 * TSquared;

            return a + b + c + d;
        }

        static inline PointAndTangent GetPointAndTangent( Vector const& p0, Vector const& cp0, Vector const& cp1, Vector const& p1, float t )
        {
            PointAndTangent result;

            float const TSquared = Math::Pow( t, 2 );
            float const TCubed = TSquared * t;
            float const oneMinusT = ( 1 - t );
            float const oneMinusTSquared = Math::Pow( oneMinusT, 2 );
            float const oneMinusTCubed = Math::Pow( oneMinusT, 3 );
            float const twoT = 2 * t;

            //-------------------------------------------------------------------------

            Vector const a = p0 * oneMinusTCubed;
            Vector const b = cp0 * 3 * t * oneMinusTSquared;
            Vector const c = cp1 * 3 * TSquared * oneMinusT;
            Vector const d = p1 * TCubed;

            result.m_point = a + b + c + d;

            //-------------------------------------------------------------------------

            float const threeTSquared = ( 3 * TSquared );

            Vector const e = p0 * ( twoT - 1 - TSquared );
            Vector const f = cp0 * ( 1 - ( 4 * t ) + threeTSquared );
            Vector const g = cp1 * ( twoT - threeTSquared );
            Vector const h = p1 * TSquared;

            result.m_tangent = e + f + g + h;

            //-------------------------------------------------------------------------

            return result;
        };

        // Create a polyline to represent the curve defined by the supplied parameters
        static void CreatePolyline( Vector const& p0, Vector const& cp0, Vector const& cp1, Vector const& p1, int32_t numDiscretizations, Polyline& outLine );

        // Returns the estimated length of the curve by discretizing it into N steps and calculating the distance between those points
        static float GetEstimatedLength( Vector const& p0, Vector const& cp0, Vector const& cp1, Vector const& p1, uint32_t numDiscretizations = 10 );
    };

    //-------------------------------------------------------------------------

    class EE_BASE_API CubicHermite
    {
    public:

        // Cubic Hermite Spline Interpolation
        static inline Vector GetPoint( Vector const& point0, Vector const& tangent0, Vector const& point1, Vector const& tangent1, float t )
        {
            float const TSquared = t * t;
            float const TCubed = TSquared * t;
            float const ThreeTSquared = 3 * TSquared;
            float const TwoTCubed = TCubed * 2;

            Vector const a = point0 * ( TwoTCubed - ThreeTSquared + 1 );
            Vector const b = tangent0 * ( TCubed - ( 2 * TSquared ) + t );
            Vector const c = tangent1 * ( TCubed - TSquared );
            Vector const d = point1 * ( ThreeTSquared - TwoTCubed );
            return a + b + c + d;
        }

        // Cubic Hermite Spline Interpolation
        static inline float GetPoint( float const& point0, float const& tangent0, float const& point1, float const& tangent1, float t )
        {
            float const TSquared = t * t;
            float const TCubed = TSquared * t;
            float const ThreeTSquared = 3 * TSquared;
            float const TwoTCubed = TCubed * 2;

            float const a = point0 * ( TwoTCubed - ThreeTSquared + 1 );
            float const b = tangent0 * ( TCubed - ( 2 * TSquared ) + t );
            float const c = tangent1 * ( TCubed - TSquared );
            float const d = point1 * ( ThreeTSquared - TwoTCubed );
            return a + b + c + d;
        }

        static inline Vector GetTangent( Vector const& point0, Vector const& tangent0, Vector const& point1, Vector const& tangent1, float t )
        {
            float const SixT = 6 * t;
            float const TSquared = t * t;
            float const TCubed = TSquared * t;
            float const ThreeTSquared = 3 * TSquared;
            float const SixTSquared = 6 * TSquared;

            Vector const a = point0 * ( SixTSquared - SixT );
            Vector const b = tangent0 * ( ThreeTSquared - ( 4 * t ) + 1 );
            Vector const c = tangent1 * ( ThreeTSquared - ( 2 * t ) );
            Vector const d = point1 * ( SixT - SixTSquared );
            return a + b + c + d;
        }

        static inline PointAndTangent GetPointAndTangent( Vector const& point0, Vector const& tangent0, Vector const& point1, Vector const& tangent1, float t )
        {
            PointAndTangent result;

            float const SixT = 6 * t;
            float const TSquared = t * t;
            float const TCubed = TSquared * t;
            float const ThreeTSquared = 3 * TSquared;
            float const TwoTCubed = TCubed * 2;
            float const SixTSquared = 6 * TSquared;

            //-------------------------------------------------------------------------

            Vector const a = point0 * ( TwoTCubed - ThreeTSquared + 1 );
            Vector const b = tangent0 * ( TCubed - ( 2 * TSquared ) + t );
            Vector const c = tangent1 * ( TCubed - TSquared );
            Vector const d = point1 * ( ThreeTSquared - TwoTCubed );

            result.m_point = a + b + c + d;

            //-------------------------------------------------------------------------

            Vector const e = point0 * ( SixTSquared - SixT );
            Vector const f = tangent0 * ( ThreeTSquared - ( 4 * t ) + 1 );
            Vector const g = tangent1 * ( ThreeTSquared - ( 2 * t ) );
            Vector const h = point1 * ( SixT - SixTSquared );

            result.m_tangent = e + f + g + h;

            //-------------------------------------------------------------------------

            return result;
        }

        // Create a polyline to represent the curve defined by the supplied parameters
        static void CreatePolyline( Vector const& point0, Vector const& tangent0, Vector const& point1, Vector const& tangent1, int32_t numDiscretizations, Polyline& outLine );

        // Compute a length of a spline segment by using 5-point Legendre-Gauss quadrature
        // https://en.wikipedia.org/wiki/Gaussian_quadrature
        static float GetSplineLength( Vector const& point0, Vector const& tangent0, Vector const& point1, Vector const& tangent1 );

        // Compute a length of a spline segment by using 5-point Legendre-Gauss quadrature
        // https://en.wikipedia.org/wiki/Gaussian_quadrature
        static float GetSplineLength( float const& point0, float const& tangent0, float const& point1, float const& tangent1 );
    };
}