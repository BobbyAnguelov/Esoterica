#pragma once
#include "Quaternion.h"
#include "Plane.h"
#include "System/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Transform;
}

//-------------------------------------------------------------------------

namespace EE::Math
{
    // Velocity
    //-------------------------------------------------------------------------

    inline Vector CalculateAngularVelocity( Quaternion const& from, Quaternion const& to, float deltaTime )
    {
        EE_ASSERT( deltaTime > 0 );

        Quaternion const deltaQuat( to.ToVector() - from.ToVector() );
        Quaternion orientationDelta = deltaQuat * from.GetConjugate();

        // Make sure we move along the shortest arc.
        Vector const dotToFrom = Quaternion::Dot( to, from );
        if ( dotToFrom.IsLessThan4( Vector::Zero ) )
        {
            orientationDelta.Negate();
        }

        Vector angularVelocity = Vector::Zero;
        angularVelocity = orientationDelta.ToVector() * Vector( deltaTime / 2 );
        angularVelocity.SetW0();
        return angularVelocity;
    }

    inline Vector CalculateLinearVelocity( Vector const& from, Vector const& to, float deltaTime )
    {
        EE_ASSERT( deltaTime > 0 );
        return ( to - from ) / Vector( deltaTime );
    }

    // Direction
    //-------------------------------------------------------------------------

    inline bool IsVectorToTheLeft( Vector const& reference, Vector const& v, Vector const& upAxis = Vector::UnitZ )
    {
        Vector const nr = reference.GetNormalized3();
        Vector const nv = v.GetNormalized3();
        Vector const cross = Vector::Cross3( nr, nv );
        Vector const dot = Vector::Dot3( cross, upAxis );
        return dot.ToFloat() < 0.0f;
    }

    inline bool IsVectorToTheRight( Vector const& reference, Vector const& v, Vector const& upAxis = Vector::UnitZ )
    {
        Vector const nr = reference.GetNormalized3();
        Vector const nv = v.GetNormalized3();
        Vector const cross = Vector::Cross3( nr, nv );
        Vector const dot = Vector::Dot3( cross, upAxis );
        return dot.ToFloat() > 0.0f;
    }

    inline bool IsVectorToTheLeft2D( Vector const& reference, Vector const& v )
    {
        Vector const nr = reference.GetNormalized2();
        Vector const nv = v.GetNormalized2();
        Vector const cross = Vector::Cross3( nr, nv );
        Vector const dot = Vector::Dot3( cross, Vector::UnitZ );
        return dot.ToFloat() < 0.0f;
    }

    inline bool IsVectorToTheRight2D( Vector const& reference, Vector const& v )
    {
        Vector const nr = reference.GetNormalized2();
        Vector const nv = v.GetNormalized2();
        Vector const cross = Vector::Cross3( nr, nv );
        Vector const dot = Vector::Dot3( cross, Vector::UnitZ );
        return dot.ToFloat() > 0.0f;
    }

    inline bool IsVectorInTheSameHemisphere2D( Vector const& reference, Vector const& v )
    {
        Vector const na = reference.GetNormalized2();
        Vector const nb = v.GetNormalized2();
        Vector const dot = Vector::Dot3( na, nb );
        return dot.ToFloat() > 0.0f;
    }

    inline bool IsVectorInTheOppositeHemisphere2D( Vector const& reference, Vector const& v )
    {
        Vector const nr = reference.GetNormalized2();
        Vector const nv = v.GetNormalized2();
        Vector const dot = Vector::Dot3( nr, nv );
        return dot.ToFloat() < 0.0f;
    }

    inline bool AreVectorsOrthogonal2D( Vector const& reference, Vector const& v )
    {
        Vector const nr = reference.GetNormalized2();
        Vector const nv = v.GetNormalized2();
        Vector const dot = Vector::Dot3( nr, nv );
        return dot.IsNearZero4( 0.0f );
    }

    // Returns the shortest angle between two vectors (always positive)
    inline Radians GetAngleBetweenNormalizedVectors( Vector const& a, Vector const& b )
    {
        EE_ASSERT( !a.IsNearZero3() && !b.IsNearZero3() );

        if ( a.IsNearEqual3( b ) )
        {
            return Radians( 0.0f );
        }

        Vector const dot = Vector::Dot3( a, b );
        Vector const reciprocal = a.InverseLength3() * b.InverseLength3();
        Vector const CosAngle = Vector::Clamp( dot * reciprocal, Vector::NegativeOne, Vector::One );
        return Radians( Vector::ACos( CosAngle ).ToFloat() );
    }

    // Returns the shortest angle between two vectors (always positive)
    EE_FORCE_INLINE Radians GetAngleBetweenVectors( Vector const& a, Vector const& b )
    {
        return GetAngleBetweenNormalizedVectors( a.GetNormalized3(), b.GetNormalized3() );
    }

    // Returns the yaw angle (rotation around Z) between two vectors, relative to the reference vector
    inline Radians GetYawAngleBetweenVectors( Vector const& reference, Vector const& v )
    {
        Vector const nr = reference.GetNormalized2();
        Vector const nv = v.GetNormalized2();
        Radians horizontalAngle = GetAngleBetweenVectors( nr, nv );

        Vector const cross = Vector::Cross3( nr, nv );
        Vector const dot = Vector::Dot3( cross, Vector::UnitZ );
        if ( dot.ToFloat() < 0.0f )
        {
            horizontalAngle = -horizontalAngle;
        }

        return horizontalAngle;
    }

    // Returns the pitch angle (rotation around X) between two vectors, relative to the reference vector
    inline Radians GetPitchAngleBetweenNormalizedVectors( Vector const& reference, Vector const& v )
    {
        float const clampedVZ = Math::Clamp( v.GetZ(), -1.0f, 1.0f );
        float const vElevationAngle = Math::ASin( clampedVZ );

        float const clampedReferenceZ = Math::Clamp( reference.GetZ(), -1.0f, 1.0f );
        float const referenceElevationAngle = Math::ASin( clampedReferenceZ );

        return Radians( vElevationAngle - referenceElevationAngle );
    }

    // Returns the angle between two vectors (rotation around X), relative to the reference vector
    inline Radians GetPitchAngleBetweenVectors( Vector const& reference, Vector const& v )
    {
        Vector const nr = reference.GetNormalized3();
        Vector const nv = v.GetNormalized3();
        return GetPitchAngleBetweenNormalizedVectors( nr, nv );
    }

    // Misc Helpers
    //-------------------------------------------------------------------------

    inline float CalculateCapsuleVolume( float radius, float height )
    {
        constexpr float const fourThirds = 4.0f / 3.0f;
        float const radiusSq = radius * radius;
        float const v =  Math::Pi * radiusSq * ( ( fourThirds * radius ) + height );
        return v;
    }

    // Calculate the barycentric coordinates for a given point vs a triangle - returns true if the point is inside the triangle
    [[nodiscard]] EE_FORCE_INLINE bool CalculateBarycentricCoordinates( Vector const& point, Vector const& triangleVert0, Vector const& triangleVert1, Vector const& triangleVert2, Float3& outCoords )
    {
        Float3 const v0 = ( triangleVert1 - triangleVert0 ).ToFloat3();
        Float3 const v1 = ( triangleVert2 - triangleVert0 ).ToFloat3();
        Float3 const v2 = ( point - triangleVert0 ).ToFloat3();

        float const denominator = 1.0f / ( ( v0.m_x * v1.m_y ) - ( v1.m_x * v0.m_y ) );
        outCoords.m_y = ( ( v2.m_x * v1.m_y ) - ( v1.m_x * v2.m_y ) ) * denominator;
        outCoords.m_z = ( ( v0.m_x * v2.m_y ) - ( v2.m_x * v0.m_y ) ) * denominator;
        outCoords.m_x = 1.0f - outCoords.m_y - outCoords.m_z;

        Vector vCoords( outCoords );
        return vCoords.IsGreaterThanEqual3( Vector::Zero ) && vCoords.IsLessThanEqual3( Vector::One );
    }

    // String Conversion
    //-------------------------------------------------------------------------

    EE_SYSTEM_API char const* ToString( Axis axis );
    EE_SYSTEM_API InlineString ToString( Vector const& vector );
    EE_SYSTEM_API InlineString ToString( Quaternion const& q );
    EE_SYSTEM_API InlineString ToString( Transform const& t );
}