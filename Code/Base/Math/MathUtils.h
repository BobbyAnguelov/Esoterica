#pragma once
#include "Quaternion.h"
#include "Plane.h"
#include "Base/Types/Containers_ForwardDecl.h"

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
    inline Radians CalculateAngleBetweenUnitVectors( Vector const& a, Vector const& b )
    {
        EE_ASSERT( a.IsNormalized3() );
        EE_ASSERT( b.IsNormalized3() );

        if ( a.IsNearEqual3( b ) )
        {
            return Radians( 0.0f );
        }

        Vector const dot = Vector::Dot3( a, b );
        Vector const reciprocal = a.InverseLength3() * b.InverseLength3();
        Vector const cosAngle = Vector::Clamp( dot * reciprocal, Vector::NegativeOne, Vector::One );
        return Radians( Vector::ACos( cosAngle ).ToFloat() );
    }

    // Returns the shortest angle between two vectors (always positive)
    EE_FORCE_INLINE Radians CalculateAngleBetweenVectors( Vector const& a, Vector const& b )
    {
        return CalculateAngleBetweenUnitVectors( a.GetNormalized3(), b.GetNormalized3() );
    }

    // Returns the yaw angle (rotation around Z) between two vectors, relative to the reference vector
    inline Radians CalculateYawAngleBetweenVectors( Vector const& reference, Vector const& v )
    {
        Vector const nr = reference.GetNormalized2();
        Vector const nv = v.GetNormalized2();

        if ( nr.IsNearZero2() || nv.IsNearZero2() )
        {
            return 0.0f;
        }

        Radians horizontalAngle = CalculateAngleBetweenUnitVectors( nr, nv );

        Vector const cross = Vector::Cross3( reference, v );
        Vector const dot = Vector::Dot3( cross, Vector::UnitZ );
        if ( dot.ToFloat() < 0.0f )
        {
            horizontalAngle = -horizontalAngle;
        }

        return horizontalAngle;
    }

    // Returns the pitch angle (rotation around -X) between two vectors, relative to the reference vector
    inline Radians CalculatePitchAngleBetweenUnitVectors( Vector const& reference, Vector const& v )
    {
        EE_ASSERT( reference.IsNormalized3() );
        EE_ASSERT( v.IsNormalized3() );

        float const vZ = v.GetZ();
        float const vElevationAngle = Math::ASin( vZ );

        float const referenceZ = reference.GetZ();
        float const referenceElevationAngle = Math::ASin( referenceZ );

        return Radians( vElevationAngle - referenceElevationAngle );
    }

    // Returns the angle between two vectors (rotation around -X), relative to the reference vector
    inline Radians CalculatePitchAngleBetweenVectors( Vector const& reference, Vector const& v )
    {
        Vector const nr = reference.GetNormalized3();
        Vector const nv = v.GetNormalized3();
        return CalculatePitchAngleBetweenUnitVectors( nr, nv );
    }

    inline Radians CalculateAngleBetweenVectorsAroundAnAxis( Vector const& sourceVector, Vector const& targetVector, Vector const& rotationAxis )
    {
        EE_ASSERT( !sourceVector.IsNearZero3() && !targetVector.IsNearZero3() && !rotationAxis.IsNearZero3() );

        Vector vAltSrc = sourceVector.Cross3( rotationAxis ).GetNormalized3();
        Vector vAltDst = targetVector.Cross3( rotationAxis ).GetNormalized3();

        float dp = vAltSrc.GetDot3( vAltDst );
        float angle = Math::ACos( Math::Clamp( dp, -1.f, 1.f ) );

        // If the cross product between the vectors is facing away from the rotation axis then the angle is negative
        Vector vCp = vAltSrc.Cross3( vAltDst );
        if ( vCp.Dot3( rotationAxis ).IsLessThan3( Vector::Zero ) )
        {
            angle *= -1.f;
        }

        return Radians( angle );
    }

    // Lines
    //-------------------------------------------------------------------------
    // Lines are infinite

    // Get the distance along the line from the start point for the projected closest point
    inline float CalculateDistanceAlongLineToClosestPoint( Vector const& lineStart, Vector const& lineDirection, Vector const& point )
    {
        EE_ASSERT( lineDirection.IsNormalized3() );
        Vector const lineToPoint( point - lineStart );
        float const T = ( Vector::Dot3( lineToPoint, lineDirection ) ).ToFloat();
        return T;
    }

    // Get closest point on the line
    inline Vector CalculateClosestPointOnLine( Vector const& lineStart, Vector const& lineDirection, Vector const& point, float *pDistanceAlongLine = nullptr )
    {
        float const T = CalculateDistanceAlongLineToClosestPoint( lineStart, lineDirection, point );

        if ( pDistanceAlongLine )
        {
            *pDistanceAlongLine = T;
        }

        Vector const closestPoint = Vector::MultiplyAdd( lineDirection, Vector( T ), lineStart );
        return closestPoint;
    }

    // Get distance between a line and a given point
    inline float CalculateDistanceBetweenPointAndLine( Vector const& lineStart, Vector const& lineDirection, Vector const& point, float *pDistanceAlongLine = nullptr )
    {
        Vector const closestPoint = CalculateClosestPointOnLine( lineStart, lineDirection, point, pDistanceAlongLine );
        return point.Distance3( closestPoint ).ToFloat();
    }

    // Line Segments
    //-------------------------------------------------------------------------

    // Get the closest T value for the distance along the line segment, T will be clamped between 0 and 1
    inline float CalculatePercentageAlongLineSegmentToClosestPoint( Vector const& lineStart, Vector const& lineEnd, Vector const& point, Vector* pUnnormalizedSegmentDir = nullptr )
    {
        Vector const lineDir( lineEnd - lineStart );
        Vector const lineLengthSqr = Vector::Dot3( lineDir, lineDir );

        EE_ASSERT( !lineLengthSqr.IsNearZero3() );
        if ( pUnnormalizedSegmentDir != nullptr ) { *pUnnormalizedSegmentDir = lineDir; }

        Vector const lineToPoint( point - lineStart );
        float T = ( Vector::Dot3( lineToPoint, lineDir ) / lineLengthSqr ).ToFloat();
        T = Math::Clamp( T, 0.0f, 1.0f );
        return T;
    }

    inline Vector CalculateClosestPointOnLineSegment( Vector const& lineStart, Vector const& lineEnd, Vector const& point, float *pT = nullptr )
    {
        Vector dir;
        float T = CalculatePercentageAlongLineSegmentToClosestPoint( lineStart, lineEnd, point, &dir );

        if ( pT )
        {
            *pT = T;
        }

        Vector const closestPoint = Vector::MultiplyAdd( dir, Vector( T ), lineStart );
        return closestPoint;
    }

    inline float CalculateDistanceBetweenPointAndLineSegment( Vector const& lineStart, Vector const& lineEnd, Vector const& point, float *pT )
    {
        Vector const closestPoint = CalculateClosestPointOnLineSegment( lineStart, lineEnd, point );
        return point.Distance3( closestPoint ).ToFloat();
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

    inline void CalculateCapsuleProperties( Vector const& c0, Vector const& c1, Vector& outCenter, float& outHalfHeight, Quaternion& outOrientation )
    {
        Vector upDir;
        float dist = 0.0f;
        Vector( c1 - c0 ).ToDirectionAndLength3( upDir, dist );
        outHalfHeight = dist / 2.0f;

        Vector const rightDir = Vector::WorldForward.Cross3( upDir );
        Vector const forwardDir = upDir.Cross3( rightDir ).GetNormalized3();

        outOrientation = Quaternion::LookAt( forwardDir, upDir );
        outCenter = c0 + ( upDir * outHalfHeight );
    }

    // Create vertices for a unit circle in the desired plane
    inline void CreateCircleVertices( Vector planeVector, Vector upAxis, int32_t numVertices, Float4* pOutVertices )
    {
        EE_ASSERT( pOutVertices != nullptr );
        EE_ASSERT( numVertices >= 4 );
        EE_ASSERT( ( numVertices % 4 ) == 0 );

        Radians const radiansPerEdge( Math::TwoPi / numVertices );
        Quaternion const rotation( upAxis, radiansPerEdge );
        for ( int32_t i = 0; i < numVertices; i++ )
        {
            planeVector = rotation.RotateVector( planeVector );
            pOutVertices[i] = planeVector.SetW1();
        }
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

    EE_BASE_API char const* ToString( Axis axis );
    EE_BASE_API InlineString ToString( Vector const& vector );
    EE_BASE_API InlineString ToString( Quaternion const& q );
    EE_BASE_API InlineString ToString( Transform const& t );
}