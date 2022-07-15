#pragma once
#include "System/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    class QuadraticBezier
    {
        // Simple static evaluation function
        template<typename T>
        static T Evaluate( T p0, T cp, T p1, float t )
        {
            float const oneMinusT = ( 1 - t );
            float const oneMinusTSquared = Math::Pow( oneMinusT, 2 );
            T const a = p0 * oneMinusTSquared;
            T const b = cp * 2 * t * oneMinusT;
            T const c = p1 * t * t;
            return a + b + c;
        }

        template<typename T>
        static T GetTangent( T p0, T cp, T p1, float t )
        {
            float const oneMinusT = ( 1 - t );
            auto const a = ( cp - p0 ) * ( 2 * oneMinusT );
            auto const b = ( p1 - cp ) * 2 * t;
            return a + b;
        }
    };

    //-------------------------------------------------------------------------

    class CubicBezier
    {
    public:

        // Simple static evaluation function
        template<typename T>
        static T Evaluate( T p0, T cp0, T cp1, T p1, float t )
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
    };

    //-------------------------------------------------------------------------

    class CubicHermite
    {
    public:

        // Cubic Hermite Spline Interpolation
        template<typename T>
        static T Evaluate( T const& point0, T const& tangent0, T const& point1, T const& tangent1, float t )
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
    };
}