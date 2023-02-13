#pragma once
#include "System/_Module/API.h"
#include "System/Types/Arrays.h"
#include "Math.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Vector;
}

//-------------------------------------------------------------------------

namespace EE::Math
{
    template<typename T>
    struct PointAndTangent
    {
        T m_point;
        T m_tangent; // NOTE: Tangents are NOT guaranteed to be normalized
    };

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API QuadraticBezier
    {
        // Simple static evaluation function
        template<typename T>
        static T GetPoint( T p0, T cp, T p1, float t )
        {
            float const oneMinusT = ( 1 - t );
            float const oneMinusTSquared = Math::Pow( oneMinusT, 2 );
            float const twoT = 2 * t;

            T const a = p0 * oneMinusTSquared;
            T const b = cp * twoT * oneMinusT;
            T const c = p1 * t * t;

            return a + b + c;
        }

        template<typename T>
        static T GetTangent( T p0, T cp, T p1, float t )
        {
            float const oneMinusT = ( 1 - t );
            float const twoT = 2 * t;

            T const a = ( cp - p0 ) * ( 2 * oneMinusT );
            T const b = ( p1 - cp ) * twoT;

            return a + b;
        }

        template<typename T>
        static PointAndTangent<T> GetPointAndTangent( T p0, T cp, T p1, float t )
        {
            PointAndTangent<T> result;

            float const oneMinusT = ( 1 - t );
            float const oneMinusTSquared = Math::Pow( oneMinusT, 2 );
            float const twoT = 2 * t;

            //-------------------------------------------------------------------------

            T const a = p0 * oneMinusTSquared;
            T const b = cp * twoT * oneMinusT;
            T const c = p1 * t * t;

            result.m_point = a + b + c;

            //-------------------------------------------------------------------------

            T const d = ( cp - p0 ) * ( 2 * oneMinusT );
            T const e = ( p1 - cp ) * twoT;

            result.m_tangent = d + e;

            //-------------------------------------------------------------------------

            return result;
        };

        // Returns the estimated length of the curve by discretizing it into N steps and calculating the distance between those points
        static float GetEstimatedLength( Vector const& p0, Vector const& cp, Vector const& p1, uint32_t numDiscretizations = 10 );
    };

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API CubicBezier
    {
    public:

        // Simple static evaluation function
        template<typename T>
        static T GetPoint( T p0, T cp0, T cp1, T p1, float t )
        {
            float const TSquared = Math::Pow( t, 2 );
            float const TCubed = TSquared * t;
            float const oneMinusT = ( 1 - t );
            float const oneMinusTSquared = Math::Pow( oneMinusT, 2 );
            float const oneMinusTCubed = Math::Pow( oneMinusT, 3 );

            T const a = p0 * oneMinusTCubed;
            T const b = cp0 * 3 * t * oneMinusTSquared;
            T const c = cp1 * 3 * TSquared * oneMinusT;
            T const d = p1 * TCubed;

            return a + b + c + d;
        }

        template<typename T>
        static T GetTangent( T p0, T cp0, T cp1, T p1, float t )
        {
            float const TSquared = Math::Pow( t, 2 );
            float const twoT = 2 * t;
            float const threeTSquared = ( 3 * TSquared );

            T const a = p0 * ( twoT - 1 - TSquared );
            T const b = cp0 * ( 1 - ( 4 * t ) + threeTSquared );
            T const c = cp1 * ( twoT - threeTSquared );
            T const d = p1 * TSquared;

            return a + b + c + d;
        }

        template<typename T>
        static PointAndTangent<T> GetPointAndTangent( T p0, T cp0, T cp1, T p1, float t )
        {
            PointAndTangent<T> result;

            float const TSquared = Math::Pow( t, 2 );
            float const TCubed = TSquared * t;
            float const oneMinusT = ( 1 - t );
            float const oneMinusTSquared = Math::Pow( oneMinusT, 2 );
            float const oneMinusTCubed = Math::Pow( oneMinusT, 3 );
            float const twoT = 2 * t;

            //-------------------------------------------------------------------------

            T const a = p0 * oneMinusTCubed;
            T const b = cp0 * 3 * t * oneMinusTSquared;
            T const c = cp1 * 3 * TSquared * oneMinusT;
            T const d = p1 * TCubed;

            result.m_point = a + b + c + d;

            //-------------------------------------------------------------------------

            float const threeTSquared = ( 3 * TSquared );

            T const e = p0 * ( twoT - 1 - TSquared );
            T const f = cp0 * ( 1 - ( 4 * t ) + threeTSquared );
            T const g = cp1 * ( twoT - threeTSquared );
            T const h = p1 * TSquared;

            result.m_tangent = e + f + g + h;

            //-------------------------------------------------------------------------

            return result;
        };

        // Returns the estimated length of the curve by discretizing it into N steps and calculating the distance between those points
        static float GetEstimatedLength( Vector const& p0, Vector const& cp0, Vector const& cp1, Vector const& p1, uint32_t numDiscretizations = 10 );
    };

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API CubicHermite
    {
    public:

        // Cubic Hermite Spline Interpolation
        template<typename T>
        static T GetPoint( T const& point0, T const& tangent0, T const& point1, T const& tangent1, float t )
        {
            float const TSquared = t * t;
            float const TCubed = TSquared * t;
            float const ThreeTSquared = 3 * TSquared;
            float const TwoTCubed = TCubed * 2;

            T const a = point0 * ( TwoTCubed - ThreeTSquared + 1 );
            T const b = tangent0 * ( TCubed - ( 2 * TSquared ) + t );
            T const c = tangent1 * ( TCubed - TSquared );
            T const d = point1 * ( ThreeTSquared - TwoTCubed );
            return a + b + c + d;
        }

        template<typename T>
        static T GetTangent( T const& point0, T const& tangent0, T const& point1, T const& tangent1, float t )
        {
            float const SixT = 6 * t;
            float const TSquared = t * t;
            float const TCubed = TSquared * t;
            float const ThreeTSquared = 3 * TSquared;
            float const SixTSquared = 6 * TSquared;

            T const a = point0 * ( SixTSquared - SixT );
            T const b = tangent0 * ( ThreeTSquared - ( 4 * t ) + 1 );
            T const c = tangent1 * ( ThreeTSquared - ( 2 * t ) );
            T const d = point1 * ( SixT - SixTSquared );
            return a + b + c + d;
        }

        template<typename T>
        static PointAndTangent<T> GetPointAndTangent( T const& point0, T const& tangent0, T const& point1, T const& tangent1, float t )
        {
            PointAndTangent<T> result;

            float const SixT = 6 * t;
            float const TSquared = t * t;
            float const TCubed = TSquared * t;
            float const ThreeTSquared = 3 * TSquared;
            float const TwoTCubed = TCubed * 2;
            float const SixTSquared = 6 * TSquared;

            //-------------------------------------------------------------------------

            T const a = point0 * ( TwoTCubed - ThreeTSquared + 1 );
            T const b = tangent0 * ( TCubed - ( 2 * TSquared ) + t );
            T const c = tangent1 * ( TCubed - TSquared );
            T const d = point1 * ( ThreeTSquared - TwoTCubed );

            result.m_point = a + b + c + d;

            //-------------------------------------------------------------------------

            T const e = point0 * ( SixTSquared - SixT );
            T const f = tangent0 * ( ThreeTSquared - ( 4 * t ) + 1 );
            T const g = tangent1 * ( ThreeTSquared - ( 2 * t ) );
            T const h = point1 * ( SixT - SixTSquared );

            result.m_tangent = e + f + g + h;

            //-------------------------------------------------------------------------

            return result;
        }

        // Compute a length of a spline segment by using 5-point Legendre-Gauss quadrature
        // https://en.wikipedia.org/wiki/Gaussian_quadrature
        static float GetSplineLength( Vector const& point0, Vector const& tangent0, Vector const& point1, Vector const& tangent1 );

        // Compute a length of a spline segment by using 5-point Legendre-Gauss quadrature
        // https://en.wikipedia.org/wiki/Gaussian_quadrature
        static float GetSplineLength( float const& point0, float const& tangent0, float const& point1, float const& tangent1 );
    };
}