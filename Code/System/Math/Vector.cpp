#include "Vector.h"
#include "Quaternion.h"

//-------------------------------------------------------------------------

namespace EE
{
    Vector const Vector::UnitX = { 1, 0, 0, 0 };
    Vector const Vector::UnitY = { 0, 1, 0, 0 };
    Vector const Vector::UnitZ = { 0, 0, 1, 0 };
    Vector const Vector::UnitW = { 0, 0, 0, 1 };

    Vector const Vector::Origin = { 0, 0, 0, 1 };
    Vector const Vector::WorldForward = { 0, -1, 0, 0 };
    Vector const Vector::WorldBackward = { 0, 1, 0, 0 };
    Vector const Vector::WorldUp = { 0, 0, 1, 0 };
    Vector const Vector::WorldDown = { 0, 0, -1, 0 };
    Vector const Vector::WorldLeft = { 1, 0, 0, 0 };
    Vector const Vector::WorldRight = { -1, 0, 0, 0 };

    Vector const Vector::Infinity = { 0x7F800000, 0x7F800000, 0x7F800000, 0x7F800000 };
    Vector const Vector::QNaN = { 0x7FC00000, 0x7FC00000, 0x7FC00000, 0x7FC00000 };

    Vector const Vector::NegativeOne( -1.0f );
    Vector const Vector::Zero( 0.0f );
    Vector const Vector::Half( 0.5f );
    Vector const Vector::One( 1.0f );

    Vector const Vector::Epsilon( Math::Epsilon );
    Vector const Vector::LargeEpsilon( Math::LargeEpsilon );
    Vector const Vector::OneMinusEpsilon( 1.0f - Math::Epsilon );
    Vector const Vector::EpsilonMinusOne( Math::Epsilon - 1.0f );
    Vector const Vector::NormalizeCheckThreshold( 0.01f ); // Squared Error

    Vector const Vector::Pi( Math::Pi );
    Vector const Vector::PiDivTwo( Math::PiDivTwo );
    Vector const Vector::TwoPi( Math::TwoPi );
    Vector const Vector::OneDivTwoPi( Math::OneDivTwoPi );

    Vector const Vector::Select0000( 0, 0, 0, 0 );
    Vector const Vector::Select0001( 0, 0, 0, 1 );
    Vector const Vector::Select0010( 0, 0, 1, 0 );
    Vector const Vector::Select0011( 0, 0, 1, 1 );
    Vector const Vector::Select0100( 0, 1, 0, 0 );
    Vector const Vector::Select0101( 0, 1, 0, 1 );
    Vector const Vector::Select0110( 0, 1, 1, 0 );
    Vector const Vector::Select0111( 0, 1, 1, 1 );
    Vector const Vector::Select1000( 1, 0, 0, 0 );
    Vector const Vector::Select1001( 1, 0, 0, 1 );
    Vector const Vector::Select1010( 1, 0, 1, 0 );
    Vector const Vector::Select1011( 1, 0, 1, 1 );
    Vector const Vector::Select1100( 1, 1, 0, 0 );
    Vector const Vector::Select1101( 1, 1, 0, 1 );
    Vector const Vector::Select1110( 1, 1, 1, 0 );
    Vector const Vector::Select1111( 1, 1, 1, 1 );

    Vector const Vector::BoxCorners[8] =
    {
        { -1.0f, -1.0f,  1.0f, 0.0f },
        {  1.0f, -1.0f,  1.0f, 0.0f },
        {  1.0f,  1.0f,  1.0f, 0.0f },
        { -1.0f,  1.0f,  1.0f, 0.0f },
        { -1.0f, -1.0f, -1.0f, 0.0f },
        {  1.0f, -1.0f, -1.0f, 0.0f },
        {  1.0f,  1.0f, -1.0f, 0.0f },
        { -1.0f,  1.0f, -1.0f, 0.0f },
    };

    //-------------------------------------------------------------------------

    Vector Vector::SLerp( Vector const& from, Vector const& to, float t )
    {
        EE_ASSERT( t >= 0.0f && t <= 1.0f );
        if ( from.GetLengthSquared3() < Epsilon.m_x || from.GetLengthSquared3() < Epsilon.m_x )
        {
            return Lerp( from, to, t );
        }

        // Calculate the final length
        Vector const fromLength = from.Length3();
        Vector const toLength = to.Length3();
        Vector const finalLength = Lerp( fromLength, toLength, t );

        // Normalize vectors
        Vector const normalizedFrom = from / fromLength;
        Vector const normalizedTo = to / toLength;

        // Handle parallel vector
        Vector result;
        if ( normalizedFrom.IsParallelTo( normalizedTo ) )
        {
            result = normalizedFrom;
        }
        else // Interpolate the rotation between the vectors
        {
            Vector const dot = Dot3( normalizedFrom, normalizedTo );
            Vector const angle = ACos( dot );
            Vector const axis = Cross3( normalizedFrom, normalizedTo ).Normalize3();
            Vector const interpolatedAngle = Lerp( Zero, angle, t );

            Quaternion const rotation( axis, Radians( interpolatedAngle.ToFloat() ) );
            Vector const finalDirection = rotation.RotateVector( normalizedFrom );
            result = finalDirection.GetNormalized3() * finalLength;
        }

        return result;
    }
}