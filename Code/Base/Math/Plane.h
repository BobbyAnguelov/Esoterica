#pragma once

#include "Base/_Module/API.h"
#include "Base/Math/Matrix.h"
#include "Line.h"

//-------------------------------------------------------------------------

namespace EE
{
    // Plane equation: a + b + c + d = 0
    //-------------------------------------------------------------------------

    class EE_BASE_API alignas( 16 ) Plane
    {
        EE_SERIALIZE( a, b, c, d );

    public:

        // Explicit constructors since the default ones can be easily misused
        static EE_FORCE_INLINE Plane FromNormal( Vector const& normal ) { return Plane( normal ); }
        static EE_FORCE_INLINE Plane FromNormalAndPoint( Vector const& normal, Vector const& point ) { return Plane( normal, point ); }
        static EE_FORCE_INLINE Plane FromNormalAndDistance( Vector const& normal, float distance ) { return Plane( normal, distance ); }
        static EE_FORCE_INLINE Plane FromPoints( Vector const point0, Vector const point1, Vector const point2 ) { return Plane( point0, point1, point2 ); }
        static EE_FORCE_INLINE Plane FromPlaneEquation( Vector const& planeEquation ) { Plane p; p.m_data = planeEquation; return p; }

    public:

        Plane() {}

        explicit Plane( float a, float b, float c, float d )
        {
            m_data = _mm_set_ps( d, c, b, a );
            Normalize();
        }

    public:

        inline Vector ToVector() const { return AsVector(); }
        inline Float4 ToFloat4() const { return AsVector().ToFloat4(); }
        inline Vector GetNormal() const { return Vector( a, b, c, 0.0f ); }

        inline bool IsNormalized() const { return AsVector().IsNormalized3(); }

        Plane& Normalize()
        {
            auto vLengthSq = _mm_mul_ps( *this, *this );
            auto vTemp = _mm_shuffle_ps( vLengthSq, vLengthSq, _MM_SHUFFLE( 2, 1, 2, 1 ) );
            vLengthSq = _mm_add_ss( vLengthSq, vTemp );
            vTemp = _mm_shuffle_ps( vTemp, vTemp, _MM_SHUFFLE( 1, 1, 1, 1 ) );
            vLengthSq = _mm_add_ss( vLengthSq, vTemp );
            vLengthSq = _mm_shuffle_ps( vLengthSq, vLengthSq, _MM_SHUFFLE( 0, 0, 0, 0 ) );

            // Prepare for the division
            auto vResult = _mm_sqrt_ps( vLengthSq );
            // Failsafe on zero (Or epsilon) length planes
            // If the length is infinity, set the elements to zero
            vLengthSq = _mm_cmpneq_ps( vLengthSq, Vector::Infinity );
            // Reciprocal mul to perform the normalization
            vResult = _mm_div_ps( *this, vResult );
            // Any that are infinity, set to zero
            *this = _mm_and_ps( vResult, vLengthSq );
            return *this;
        }

        inline Plane GetNormalized() const
        {
            Plane p = *this;
            p.Normalize();
            return p;
        }

        inline Plane& operator*=( Matrix const& transform )
        {
            auto W = AsVector().GetSplatW();
            auto Z = AsVector().GetSplatZ();
            auto Y = AsVector().GetSplatY();
            auto X = AsVector().GetSplatX();

            Vector result = W * transform[3];
            Vector::MultiplyAdd( Z, transform[2], result );
            Vector::MultiplyAdd( Y, transform[1], result );
            Vector::MultiplyAdd( X, transform[0], result );
            AsVector() = result;
            return *this;
        }

        inline Plane operator*( Matrix const& transform ) const
        {
            Plane p = *this;
            p *= transform;
            return p;
        }

        //-------------------------------------------------------------------------

        // Projects a point to the nearest point on the plane
        inline Vector ProjectPoint( Vector const& point ) const
        {
            EE_ASSERT( IsNormalized() );
            Vector const pointW1 = point.GetWithW1();
            Vector const planeVector = ToVector();
            Vector const planeNormal = GetNormal();
            Vector const distanceToPlane = Vector::Dot4( pointW1, planeVector );
            Vector const projectedPoint = Vector::MultiplyAdd( planeNormal.GetNegated(), distanceToPlane, pointW1 );
            return projectedPoint;
        }

        // Align a vector onto the plane
        inline Vector ProjectVector( Vector const& v )const
        {
            EE_ASSERT( IsNormalized() );
            Vector const planeVector = ToVector();
            Vector const dot = v.Dot3( planeVector );
            Vector const projectedVector = Vector::MultiplyAdd( planeVector.GetNegated(), dot, v );
            return projectedVector;
        }

        // Project a point in 2D onto a plane. Basically intersects a vertical ray originating from the point with the plane
        inline Vector ProjectPointVertically( Vector const& v )
        {
            EE_ASSERT( !Math::IsNearZero( GetNormal().GetZ() ) );
            Vector Intersection;
            IntersectRay( v, Vector::UnitZ, Intersection );
            return Intersection.GetWithW1();
        }

        // Project a vector vertically onto a plane. Basically intersects a vertical ray originating from the point with the plane
        // Note: this operation will change the length of the vector!
        inline Vector ProjectVectorVertically( Vector const& v )
        {
            EE_ASSERT( !Math::IsNearZero( GetNormal().GetZ() ) );

            // Get a copy of the plane at a distance of 0, since this operation is done in the plane referential, and not in world space.
            Plane plane = *this;
            plane.d = 0; 

            Vector Intersection;
            plane.IntersectRay( v, Vector::UnitZ, Intersection );
            return Intersection;
        }

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE Vector SignedDistanceToPoint( Vector const point ) const
        {
            EE_ASSERT( IsNormalized() );
            return Vector::Dot4( point.GetWithW1(), ToVector() );
        }

        EE_FORCE_INLINE float GetSignedDistanceToPoint( Vector const point ) const 
        { 
            return SignedDistanceToPoint( point ).ToFloat();
        }

        EE_FORCE_INLINE Vector AbsoluteDistanceToPoint( Vector const point ) const
        { 
            return SignedDistanceToPoint( point ).Abs();
        }

        EE_FORCE_INLINE float GetAbsoluteDistanceToPoint( Vector const point ) const
        {
            return AbsoluteDistanceToPoint( point ).ToFloat();
        }

        EE_FORCE_INLINE bool ArePointsOnSameSide( Vector const point0, Vector const point1 ) const
        { 
            Vector const distanceProduct = ( SignedDistanceToPoint( point0 ) * SignedDistanceToPoint( point1 ) );
            return distanceProduct.IsGreaterThanEqual4( Vector::Zero );
        }

        EE_FORCE_INLINE bool IsPointInFront( Vector const point ) const
        {
            return SignedDistanceToPoint( point ).IsGreaterThanEqual4( Vector::Zero );
        }

        EE_FORCE_INLINE bool IsPointBehind( Vector const point ) const
        { 
            return SignedDistanceToPoint( point ).IsLessThan4( Vector::Zero );
        }

        //-------------------------------------------------------------------------

        bool IntersectLine( Vector const& point0, Vector const& point1, Vector& intersectionPoint ) const;
        bool IntersectLine( LineSegment const& line, Vector& intersectionPoint ) const { return IntersectLine( line.GetStartPoint(), line.GetEndPoint(), intersectionPoint ); }
        bool IntersectRay( Vector const& rayOrigin, Vector const& rayDirection, Vector& intersectionPoint ) const;
        bool IntersectPlane( Plane const& otherPlane, Vector& outLineStart, Vector& outLineEnd ) const;

    private:

        explicit Plane( Vector const normal )
            : m_data( Vector::Select( normal.m_data, Vector::Zero, Vector::Select0001 ) )
        {
            EE_ASSERT( normal.IsNormalized3() );
        }

        explicit Plane( Vector const normal, Vector const point )
        {
            EE_ASSERT( normal.IsNormalized3() );
            Vector const D = Vector::Dot3( point, normal ).GetNegated();
            m_data = Vector::Select( D, normal, Vector( 1, 1, 1, 0 ) );
        }

        explicit Plane( Vector const normal, float const& distance )
            : m_data( Vector::Select( normal.m_data, Vector( distance ), Vector::Select0001 ) )
        {
            EE_ASSERT( normal.IsNormalized3() );
        }

        explicit Plane( Vector const point0, Vector const point1, Vector const point2 )
        {
            Vector const V10 = point0 - point1;
            Vector const V20 = point0 - point2;
            Vector const normal = Vector::Cross3( V10, V20 ).GetNormalized3();
            Vector const D = Vector::Dot3( point0, normal ).GetNegated();
            AsVector() = Vector::Select( D, normal, Vector( 1, 1, 1, 0 ) );
        }

        inline Vector& AsVector() { return reinterpret_cast<Vector&>( *this ); }
        inline Vector const& AsVector() const { return reinterpret_cast<Vector const&>( *this ); }

        EE_FORCE_INLINE operator __m128&() { return m_data; }
        EE_FORCE_INLINE operator __m128 const&() const { return m_data; }
        inline Plane& operator=( __m128 v ) { m_data = v; return *this; }

    public:

        union
        {
            struct { float a,b,c,d; };
            __m128 m_data;
        };
    };

    //-------------------------------------------------------------------------

    static_assert( sizeof( Vector ) == 16, "Plane size must be 16 bytes!" );
}