#pragma once

#include "System/_Module/API.h"
#include "System/Math/MathConstants.h"
#include "SIMD.h"

//-------------------------------------------------------------------------
// This math library is heavily based on the DirectX math library 
//-------------------------------------------------------------------------
// https://github.com/Microsoft/DirectXMath
//-------------------------------------------------------------------------

namespace EE
{
    class EE_SYSTEM_API alignas( 16 ) Vector
    {
        EE_SERIALIZE( m_x, m_y, m_z, m_w );

    public:

        static Vector const UnitX;
        static Vector const UnitY;
        static Vector const UnitZ;
        static Vector const UnitW;

        static Vector const Origin;
        static Vector const WorldForward;
        static Vector const WorldBackward;
        static Vector const WorldUp;
        static Vector const WorldDown;
        static Vector const WorldLeft;
        static Vector const WorldRight;

        static Vector const NegativeOne;
        static Vector const Zero;
        static Vector const Half;
        static Vector const One;
        static Vector const Epsilon;
        static Vector const LargeEpsilon;
        static Vector const OneMinusEpsilon;
        static Vector const EpsilonMinusOne;
        static Vector const NormalizeCheckThreshold;
        static Vector const Pi;
        static Vector const PiDivTwo;
        static Vector const TwoPi;
        static Vector const OneDivTwoPi;

        static Vector const Select0000;
        static Vector const Select0001;
        static Vector const Select0010;
        static Vector const Select0011;
        static Vector const Select0100;
        static Vector const Select0101;
        static Vector const Select0110;
        static Vector const Select0111;
        static Vector const Select1000;
        static Vector const Select1001;
        static Vector const Select1010;
        static Vector const Select1011;
        static Vector const Select1100;
        static Vector const Select1101;
        static Vector const Select1110;
        static Vector const Select1111;

        static Vector const Infinity;
        static Vector const QNaN;

        static Vector const BoxCorners[8];

        EE_FORCE_INLINE static Vector Cross2( Vector const& v0, Vector const& v1 ) { return v0.Cross2( v1 ); }
        EE_FORCE_INLINE static Vector Cross3( Vector const& v0, Vector const& v1 ) { return v0.Cross3( v1 ); }
        EE_FORCE_INLINE static Vector Dot2( Vector const& v0, Vector const& v1 ) { return v0.Dot2( v1 ); }
        EE_FORCE_INLINE static Vector Dot3( Vector const& v0, Vector const& v1 ) { return v0.Dot3( v1 ); }
        EE_FORCE_INLINE static Vector Dot4( Vector const& v0, Vector const& v1 ) { return v0.Dot4( v1 ); }
        EE_FORCE_INLINE static Vector Average2( Vector const& v0, Vector const& v1 );
        EE_FORCE_INLINE static Vector Average3( Vector const& v0, Vector const& v1 );
        EE_FORCE_INLINE static Vector Average4( Vector const& v0, Vector const& v1 );
        EE_FORCE_INLINE static Vector Min( Vector const& v0, Vector const& v1 );
        EE_FORCE_INLINE static Vector Max( Vector const& v0, Vector const& v1 );
        EE_FORCE_INLINE static Vector Clamp( Vector const& v, Vector const& min, Vector const& max );
        EE_FORCE_INLINE static Vector MultiplyAdd( Vector const& vec, Vector const& multiplier, Vector const& add );
        EE_FORCE_INLINE static Vector NegativeMultiplySubtract( Vector const& vec, Vector const& multiplier, Vector const& subtrahend );
        EE_FORCE_INLINE static Vector Xor( Vector const& vec0, Vector const& vec1 );
        EE_FORCE_INLINE static Vector LinearCombination( Vector const& v0, Vector const& v1, float scale0, float scale1 ) { return ( v0 * scale0 ) + ( v1 * scale1 ); }

        EE_FORCE_INLINE static Vector Sin( Vector const& vec );
        EE_FORCE_INLINE static Vector Cos( Vector const& vec );
        EE_FORCE_INLINE static Vector Tan( Vector const& vec );
        EE_FORCE_INLINE static Vector ASin( Vector const& vec );
        EE_FORCE_INLINE static Vector ACos( Vector const& vec );
        EE_FORCE_INLINE static Vector ATan( Vector const& vec );
        EE_FORCE_INLINE static Vector ATan2( Vector const& vec0, Vector const& vec1 );

        EE_FORCE_INLINE static Vector SinEst( Vector const& vec );
        EE_FORCE_INLINE static Vector CosEst( Vector const& vec );
        EE_FORCE_INLINE static Vector TanEst( Vector const& vec );
        EE_FORCE_INLINE static Vector ASinEst( Vector const& vec );
        EE_FORCE_INLINE static Vector ACosEst( Vector const& vec );
        EE_FORCE_INLINE static Vector ATanEst( Vector const& vec );
        EE_FORCE_INLINE static Vector ATan2Est( Vector const& vec0, Vector const& vec1 );

        EE_FORCE_INLINE static void SinCos( Vector& sin, Vector& cos, float angle ) { return SinCos( sin, cos, Vector( angle ) ); }
        EE_FORCE_INLINE static void SinCos( Vector& sin, Vector& cos, Vector const& angle );

        EE_FORCE_INLINE static Vector AngleMod2Pi( Vector const& angles );

        EE_FORCE_INLINE static Vector Lerp( Vector const& from, Vector const& to, float t ); // Linear interpolation
        EE_FORCE_INLINE static Vector NLerp( Vector const& from, Vector const& to, float t ); // Normalized linear interpolation of a vector
        static Vector SLerp( Vector const& from, Vector const& to, float t ); // Spherical interpolation of a vector

        //-------------------------------------------------------------------------

        // Combine the two vectors based on the control: 0 means select from v0, 1 means select from v1. E.G. To select XY from v0 and ZW from v1, control = Vector( 0, 0, 1, 1 )
        EE_FORCE_INLINE static Vector Select( Vector const& v0, Vector const& v1, Vector const& control );

        // Get a permutation of two vectors, each template argument represents the element index to select ( v0: 0-3, v1: 4-7 );
        template<uint32_t PermuteX, uint32_t PermuteY, uint32_t PermuteZ, uint32_t PermuteW>
        EE_FORCE_INLINE static Vector Permute( Vector const& v0, Vector const& v1 );

    public:

        EE_FORCE_INLINE operator __m128&() { return m_data; }
        EE_FORCE_INLINE operator __m128 const&() const { return m_data; }

        EE_FORCE_INLINE Vector() {}
        EE_FORCE_INLINE explicit Vector( Axis axis );
        EE_FORCE_INLINE explicit Vector( ZeroInit_t ) { memset( this, 0, sizeof( Vector ) ); }
        EE_FORCE_INLINE explicit Vector( float v ) { m_data = _mm_shuffle_ps( _mm_load_ss( &v ), _mm_load_ss( &v ), _MM_SHUFFLE( 0, 0, 0, 0 ) ); }
        EE_FORCE_INLINE Vector( __m128 v ) : m_data( v ) {}
        EE_FORCE_INLINE Vector( float ix, float iy, float iz, float iw = 1.0f ) { m_data = _mm_set_ps( iw, iz, iy, ix ); }

        EE_FORCE_INLINE Vector( Float2 const& v, float iz = 0.0f, float iw = 0.0f ) { m_data = _mm_set_ps( iw, iz, v.m_y, v.m_x ); }
        EE_FORCE_INLINE Vector( Float3 const& v, float iw = 1.0f ) { m_data = _mm_set_ps( iw, v.m_z, v.m_y, v.m_x ); } // Default behavior: create points (m_w=1)
        EE_FORCE_INLINE Vector( Float4 const& v ) { m_data = _mm_loadu_ps( &v.m_x ); }

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE bool IsValid() const { return !IsNaN4() && !IsInfinite4(); }

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE void StoreFloat( float& value ) const;
        EE_FORCE_INLINE void StoreFloat2( Float2& value ) const;
        EE_FORCE_INLINE void StoreFloat3( Float3& value ) const;
        EE_FORCE_INLINE void StoreFloat4( Float4& value ) const;

        EE_FORCE_INLINE float ToFloat() const;
        EE_FORCE_INLINE Float2 ToFloat2() const;
        EE_FORCE_INLINE Float3 ToFloat3() const;
        EE_FORCE_INLINE Float4 ToFloat4() const;

        EE_FORCE_INLINE operator Float2() const { return ToFloat2(); }
        EE_FORCE_INLINE operator Float3() const { return ToFloat3(); }
        EE_FORCE_INLINE operator Float4() const { return ToFloat4(); }

        //-------------------------------------------------------------------------

        float& operator[]( uint32_t i ) { EE_ASSERT( i < 4 ); return ( ( float* ) this )[i]; }
        float const& operator[]( uint32_t i ) const { EE_ASSERT( i < 4 ); return ( ( float* ) this )[i]; }

        // W component operations - needed primarily for homogeneous coordinate operations
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE bool IsW1() const { return m_w == 1.0f; }
        EE_FORCE_INLINE bool IsW0() const { return m_w == 0.0f; }
        EE_FORCE_INLINE Vector& SetW0() { m_w = 0.0f; return *this; }
        EE_FORCE_INLINE Vector& SetW1() { m_w = 1.0f; return *this; }
        EE_FORCE_INLINE Vector GetWithW0() const { Vector v = *this; v.SetW0(); return v; }
        EE_FORCE_INLINE Vector GetWithW1() const { Vector v = *this; v.SetW1(); return v; }

        // Element access (while you can still access the individual elements via the union, that is not performant, and it is preferable to call these functions to go between scalar and vector)
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE float GetX() const { return _mm_cvtss_f32( m_data ); }
        EE_FORCE_INLINE float GetY() const { auto vTemp = GetSplatY(); return _mm_cvtss_f32( vTemp ); }
        EE_FORCE_INLINE float GetZ() const { auto vTemp = GetSplatZ(); return _mm_cvtss_f32( vTemp ); }
        EE_FORCE_INLINE float GetW() const { auto vTemp = GetSplatW(); return _mm_cvtss_f32( vTemp ); }

        // Dimensional Getters
        //-------------------------------------------------------------------------

        // Returns only the first two components, z=w=0
        EE_FORCE_INLINE Vector Get2D() const { return Vector::Select( *this, Vector::Zero, Vector::Select0011 ); }
        
        // Returns only the first three components, w = 0
        EE_FORCE_INLINE Vector Get3D() const { return Vector::Select( *this, Vector::Zero, Vector::Select0001 ); }

        // Algebraic operators
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE Vector operator+( Vector const& v ) const { return _mm_add_ps( m_data, v ); }
        EE_FORCE_INLINE Vector& operator+=( Vector const& v ) { m_data = _mm_add_ps( m_data, v ); return *this; }
        EE_FORCE_INLINE Vector operator-( Vector const& v ) const { return _mm_sub_ps( m_data, v ); }
        EE_FORCE_INLINE Vector& operator-=( Vector const& v ) { m_data = _mm_sub_ps( m_data, v ); return *this; }
        EE_FORCE_INLINE Vector operator*( Vector const& v ) const { return _mm_mul_ps( m_data, v ); }
        EE_FORCE_INLINE Vector& operator*=( Vector const& v ) { m_data = _mm_mul_ps( m_data, v ); return *this; }
        EE_FORCE_INLINE Vector operator/( Vector const& v ) const { return _mm_div_ps( m_data, v ); }
        EE_FORCE_INLINE Vector& operator/=( Vector const& v ) { m_data = _mm_div_ps( m_data, v ); return *this; }

        EE_FORCE_INLINE Vector operator*( float const f ) const { return operator*( Vector( f ) ); }
        EE_FORCE_INLINE Vector& operator*=( float const f ) { return operator*=( Vector( f ) ); }
        EE_FORCE_INLINE Vector operator/( float const f ) const { return operator/( Vector( f ) ); }
        EE_FORCE_INLINE Vector& operator/=( float const f ) { return operator/=( Vector( f ) ); }

        EE_FORCE_INLINE Vector operator-() const { return GetNegated(); }
        EE_FORCE_INLINE Vector& operator-() { Negate(); return *this; }

        EE_FORCE_INLINE Vector Orthogonal2D() const;
        EE_FORCE_INLINE Vector Cross2( Vector const& other ) const;
        EE_FORCE_INLINE Vector Cross3( Vector const& other ) const;
        EE_FORCE_INLINE Vector Dot2( Vector const& other ) const;
        EE_FORCE_INLINE Vector Dot3( Vector const& other ) const;
        EE_FORCE_INLINE Vector Dot4( Vector const& other ) const;
        EE_FORCE_INLINE float GetDot2( Vector const& other ) const { return Dot2( other ).ToFloat(); }
        EE_FORCE_INLINE float GetDot3( Vector const& other ) const { return Dot3( other ).ToFloat(); }
        EE_FORCE_INLINE float GetDot4( Vector const& other ) const { return Dot4( other ).ToFloat(); }

        EE_FORCE_INLINE Vector ScalarProjection( Vector const& other ) const;
        EE_FORCE_INLINE float GetScalarProjection( Vector const& other ) const { return ScalarProjection( other ).ToFloat(); }
        EE_FORCE_INLINE Vector VectorProjection( Vector const& other ) const;

        // Transformations
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE Vector& Invert() { m_data = _mm_div_ps( Vector::One, m_data ); return *this; }
        EE_FORCE_INLINE Vector GetInverse() const { return _mm_div_ps( Vector::One, m_data ); }
        EE_FORCE_INLINE Vector GetReciprocal() const { return GetInverse(); }

        EE_FORCE_INLINE Vector& InvertEst() { m_data = _mm_rcp_ps( m_data ); return *this; }
        EE_FORCE_INLINE Vector GetInverseEst() const { return _mm_rcp_ps( m_data ); }

        EE_FORCE_INLINE Vector& Negate() { m_data = _mm_sub_ps( Vector::Zero, m_data ); return *this; }
        EE_FORCE_INLINE Vector GetNegated() const { return _mm_sub_ps( Vector::Zero, m_data ); }

        EE_FORCE_INLINE Vector& Abs() { m_data = _mm_max_ps( _mm_sub_ps( Vector::Zero, m_data ), m_data ); return *this; }
        EE_FORCE_INLINE Vector GetAbs() const { return _mm_max_ps( _mm_sub_ps( Vector::Zero, m_data ), m_data ); }

        EE_FORCE_INLINE Vector& Normalize2();
        EE_FORCE_INLINE Vector& Normalize3();
        EE_FORCE_INLINE Vector& Normalize4();

        EE_FORCE_INLINE Vector GetNormalized2() const;
        EE_FORCE_INLINE Vector GetNormalized3() const;
        EE_FORCE_INLINE Vector GetNormalized4() const;

        EE_FORCE_INLINE Vector& Floor();
        EE_FORCE_INLINE Vector GetFloor() const;
        EE_FORCE_INLINE Vector& Ceil();
        EE_FORCE_INLINE Vector GetCeil() const;
        EE_FORCE_INLINE Vector& Round();
        EE_FORCE_INLINE Vector GetRound() const;

        EE_FORCE_INLINE Vector GetSign() const;

        // Permutations
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE Vector GetSplatX() const { return _mm_shuffle_ps( m_data, m_data, _MM_SHUFFLE( 0, 0, 0, 0 ) ); }
        EE_FORCE_INLINE Vector GetSplatY() const { return _mm_shuffle_ps( m_data, m_data, _MM_SHUFFLE( 1, 1, 1, 1 ) ); }
        EE_FORCE_INLINE Vector GetSplatZ() const { return _mm_shuffle_ps( m_data, m_data, _MM_SHUFFLE( 2, 2, 2, 2 ) ); }
        EE_FORCE_INLINE Vector GetSplatW() const { return _mm_shuffle_ps( m_data, m_data, _MM_SHUFFLE( 3, 3, 3, 3 ) ); }

        // Get a shuffled version of the vector, each template argument represents the element index in the original vector
        template<uint32_t ElementX, uint32_t ElementY, uint32_t ElementZ, uint32_t ElementW>
        EE_FORCE_INLINE Vector Swizzle() const
        {
            static_assert( ElementX <= 3, "Element index parameter out of range" );
            static_assert( ElementY <= 3, "Element index parameter out of range" );
            static_assert( ElementZ <= 3, "Element index parameter out of range" );
            static_assert( ElementW <= 3, "Element index parameter out of range" );
            return _mm_shuffle_ps( m_data, m_data, _MM_SHUFFLE( ElementW, ElementZ, ElementY, ElementX ) );
        }

        EE_FORCE_INLINE Vector Shuffle( uint32_t xIdx, uint32_t yIdx, uint32_t zIdx, uint32_t wIdx ) const
        {
            EE_ASSERT( xIdx < 4 && yIdx < 4 && zIdx < 4 && wIdx < 4 );
            Vector result( (*this)[xIdx], ( *this )[yIdx], ( *this )[zIdx], ( *this )[wIdx] );
            return result;
        }

        // Queries
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE Vector Length2() const;
        EE_FORCE_INLINE Vector Length3() const;
        EE_FORCE_INLINE Vector Length4() const;

        EE_FORCE_INLINE float GetLength2() const { return Length2().GetX(); }
        EE_FORCE_INLINE float GetLength3() const { return Length3().GetX(); }
        EE_FORCE_INLINE float GetLength4() const { return Length4().GetX(); }

        EE_FORCE_INLINE Vector InverseLength2() const;
        EE_FORCE_INLINE Vector InverseLength3() const;
        EE_FORCE_INLINE Vector InverseLength4() const;

        EE_FORCE_INLINE float GetInverseLength2() const { return InverseLength2().GetX(); }
        EE_FORCE_INLINE float GetInverseLength3() const { return InverseLength3().GetX(); }
        EE_FORCE_INLINE float GetInverseLength4() const { return InverseLength4().GetX(); }

        EE_FORCE_INLINE Vector LengthSquared2() const { return Vector::Dot2( m_data, m_data ); }
        EE_FORCE_INLINE Vector LengthSquared3() const { return Vector::Dot3( m_data, m_data ); }
        EE_FORCE_INLINE Vector LengthSquared4() const { return Vector::Dot4( m_data, m_data ); }

        EE_FORCE_INLINE float GetLengthSquared2() const { return LengthSquared2().GetX(); }
        EE_FORCE_INLINE float GetLengthSquared3() const { return LengthSquared3().GetX(); }
        EE_FORCE_INLINE float GetLengthSquared4() const { return LengthSquared4().GetX(); }

        EE_FORCE_INLINE Vector Distance2( Vector const& to ) const { return ( to - *this ).Length2(); }
        EE_FORCE_INLINE Vector Distance3( Vector const& to ) const { return ( to - *this ).Length3(); }
        EE_FORCE_INLINE Vector Distance4( Vector const& to ) const { return ( to - *this ).Length4(); }

        EE_FORCE_INLINE float GetDistance2( Vector const& to ) const { return ( to - *this ).Length2().GetX(); }
        EE_FORCE_INLINE float GetDistance3( Vector const& to ) const { return ( to - *this ).Length3().GetX(); }
        EE_FORCE_INLINE float GetDistance4( Vector const& to ) const { return ( to - *this ).Length4().GetX(); }

        EE_FORCE_INLINE Vector DistanceSquared2( Vector const& to ) const { return ( to - *this ).LengthSquared2(); }
        EE_FORCE_INLINE Vector DistanceSquared3( Vector const& to ) const { return ( to - *this ).LengthSquared3(); }
        EE_FORCE_INLINE Vector DistanceSquared4( Vector const& to ) const { return ( to - *this ).LengthSquared4(); }

        EE_FORCE_INLINE float GetDistanceSquared2( Vector const& to ) const { return ( to - *this ).GetLengthSquared2(); }
        EE_FORCE_INLINE float GetDistanceSquared3( Vector const& to ) const { return ( to - *this ).GetLengthSquared3(); }
        EE_FORCE_INLINE float GetDistanceSquared4( Vector const& to ) const { return ( to - *this ).GetLengthSquared4(); }

        EE_FORCE_INLINE bool IsNormalized2() const { return ( LengthSquared2() - Vector::One ).Abs().IsLessThanEqual4( Vector::NormalizeCheckThreshold ); }
        EE_FORCE_INLINE bool IsNormalized3() const { return ( LengthSquared3() - Vector::One ).Abs().IsLessThanEqual4( Vector::NormalizeCheckThreshold ); }
        EE_FORCE_INLINE bool IsNormalized4() const { return ( LengthSquared4() - Vector::One ).Abs().IsLessThanEqual4( Vector::NormalizeCheckThreshold ); }

        EE_FORCE_INLINE Vector InBounds( Vector const& bounds ) const; // Is this vector within the range [-bounds, bounds]

        EE_FORCE_INLINE bool IsInBounds2( Vector const& bounds ) const { return ( _mm_movemask_ps( InBounds( bounds ) ) & 0x3 ) == 0x3 != 0; }
        EE_FORCE_INLINE bool IsInBounds3( Vector const& bounds ) const { return ( _mm_movemask_ps( InBounds( bounds ) ) & 0x7 ) == 0x7 != 0; }
        EE_FORCE_INLINE bool IsInBounds4( Vector const& bounds ) const { return ( _mm_movemask_ps( InBounds( bounds ) ) == 0x0f ) != 0; }

        EE_FORCE_INLINE Vector Equal( Vector const& v ) const { return _mm_cmpeq_ps( *this, v ); }

        EE_FORCE_INLINE bool IsEqual2( Vector const& v ) const { return ( ( ( _mm_movemask_ps( Equal( v ) ) & 3 ) == 3 ) != 0 ); }
        EE_FORCE_INLINE bool IsEqual3( Vector const& v ) const { return ( ( ( _mm_movemask_ps( Equal( v ) ) & 7 ) == 7 ) != 0 ); }
        EE_FORCE_INLINE bool IsEqual4( Vector const& v ) const { return ( ( _mm_movemask_ps( Equal( v ) ) == 0x0f ) != 0 ); }

        EE_FORCE_INLINE Vector NearEqual( Vector const& v, Vector const& epsilon ) const;

        EE_FORCE_INLINE bool IsNearEqual2( Vector const& v, float epsilon ) const { return IsNearEqual2( v, Vector( epsilon ) ); }
        EE_FORCE_INLINE bool IsNearEqual3( Vector const& v, float epsilon ) const { return IsNearEqual3( v, Vector( epsilon ) ); }
        EE_FORCE_INLINE bool IsNearEqual4( Vector const& v, float epsilon ) const { return IsNearEqual4( v, Vector( epsilon ) ); }

        EE_FORCE_INLINE bool IsNearEqual2( Vector const& v, Vector const& epsilon = Vector::Epsilon ) const { return ( ( ( _mm_movemask_ps( NearEqual( v, epsilon ) ) & 3 ) == 0x3 ) != 0 ); }
        EE_FORCE_INLINE bool IsNearEqual3( Vector const& v, Vector const& epsilon = Vector::Epsilon ) const { return ( ( ( _mm_movemask_ps( NearEqual( v, epsilon ) ) & 7 ) == 0x7 ) != 0 ); }
        EE_FORCE_INLINE bool IsNearEqual4( Vector const& v, Vector const& epsilon = Vector::Epsilon ) const { return ( ( _mm_movemask_ps( NearEqual( v, epsilon ) ) == 0xf ) != 0 ); }

        EE_FORCE_INLINE Vector GreaterThan( Vector const& v ) const { return _mm_cmpgt_ps( m_data, v ); }
        EE_FORCE_INLINE bool IsAnyGreaterThan( Vector const& v ) const { return !GreaterThan( v ).IsZero4(); }

        EE_FORCE_INLINE bool IsGreaterThan2( Vector const& v ) const { return ( ( ( _mm_movemask_ps( GreaterThan( v ) ) & 3 ) == 3 ) != 0 ); }
        EE_FORCE_INLINE bool IsGreaterThan3( Vector const& v ) const { return ( ( ( _mm_movemask_ps( GreaterThan( v ) ) & 7 ) == 7 ) != 0 ); }
        EE_FORCE_INLINE bool IsGreaterThan4( Vector const& v ) const { return ( ( _mm_movemask_ps( GreaterThan( v ) ) == 0x0f ) != 0 ); }

        EE_FORCE_INLINE Vector GreaterThanEqual( Vector const& v ) const { return _mm_cmpge_ps( m_data, v ); }
        EE_FORCE_INLINE bool IsAnyGreaterThanEqual( Vector const& v ) const { return !GreaterThanEqual( v ).IsZero4(); }

        EE_FORCE_INLINE bool IsGreaterThanEqual2( Vector const& v ) const { return ( ( _mm_movemask_ps( GreaterThanEqual( v ) ) & 3 ) == 3 ) != 0; }
        EE_FORCE_INLINE bool IsGreaterThanEqual3( Vector const& v ) const { return ( ( _mm_movemask_ps( GreaterThanEqual( v ) ) & 7 ) == 7 ) != 0; }
        EE_FORCE_INLINE bool IsGreaterThanEqual4( Vector const& v ) const { return ( _mm_movemask_ps( GreaterThanEqual( v ) ) == 0x0f ) != 0; }

        EE_FORCE_INLINE Vector LessThan( Vector const& v ) const { return _mm_cmplt_ps( m_data, v ); }
        EE_FORCE_INLINE bool IsAnyLessThan( Vector const& v ) const { return !LessThan( v ).IsZero4(); }

        EE_FORCE_INLINE bool IsLessThan2( Vector const& v ) const { return ( ( ( _mm_movemask_ps( LessThan( v ) ) & 3 ) == 3 ) != 0 ); }
        EE_FORCE_INLINE bool IsLessThan3( Vector const& v ) const { return ( ( ( _mm_movemask_ps( LessThan( v ) ) & 7 ) == 7 ) != 0 ); }
        EE_FORCE_INLINE bool IsLessThan4( Vector const& v ) const { return ( ( _mm_movemask_ps( LessThan( v ) ) == 0x0f ) != 0 ); }

        EE_FORCE_INLINE Vector LessThanEqual( Vector const& v ) const { return _mm_cmple_ps( m_data, v ); }
        EE_FORCE_INLINE bool IsAnyLessThanEqual( Vector const& v ) const { return !LessThanEqual( v ).IsZero4(); }

        EE_FORCE_INLINE bool IsLessThanEqual2( Vector const& v ) const { return ( ( ( _mm_movemask_ps( LessThanEqual( v ) ) & 3 ) == 3 ) != 0 ); }
        EE_FORCE_INLINE bool IsLessThanEqual3( Vector const& v ) const { return ( ( ( _mm_movemask_ps( LessThanEqual( v ) ) & 7 ) == 7 ) != 0 ); }
        EE_FORCE_INLINE bool IsLessThanEqual4( Vector const& v ) const { return ( ( _mm_movemask_ps( LessThanEqual( v ) ) == 0x0f ) != 0 ); }

        EE_FORCE_INLINE Vector EqualsZero() const { return Equal( Vector::Zero ); }
        EE_FORCE_INLINE bool IsAnyEqualsZero() const { return !EqualsZero().IsZero4(); }

        EE_FORCE_INLINE bool IsZero2() const { return IsEqual2( Vector::Zero ); }
        EE_FORCE_INLINE bool IsZero3() const { return IsEqual3( Vector::Zero ); }
        EE_FORCE_INLINE bool IsZero4() const { return IsEqual4( Vector::Zero ); }

        EE_FORCE_INLINE Vector NearEqualsZero( float epsilon = Math::Epsilon ) const { return NearEqual( Vector::Zero, Vector( epsilon ) ); }

        EE_FORCE_INLINE bool IsNearZero2( float epsilon = Math::Epsilon ) const { return IsNearEqual2( Vector::Zero, Vector( epsilon ) ); }
        EE_FORCE_INLINE bool IsNearZero3( float epsilon = Math::Epsilon ) const { return IsNearEqual3( Vector::Zero, Vector( epsilon ) ); }
        EE_FORCE_INLINE bool IsNearZero4( float epsilon = Math::Epsilon ) const { return IsNearEqual4( Vector::Zero, Vector( epsilon ) ); }

        EE_FORCE_INLINE Vector EqualsInfinity() const { __m128 vTemp = _mm_and_ps( m_data, SIMD::g_absMask ); return _mm_cmpeq_ps( vTemp, Vector::Infinity ); }

        EE_FORCE_INLINE bool IsInfinite2() const { return ( _mm_movemask_ps( EqualsInfinity() ) & 3 ) != 0; }
        EE_FORCE_INLINE bool IsInfinite3() const { return ( _mm_movemask_ps( EqualsInfinity() ) & 7 ) != 0; }
        EE_FORCE_INLINE bool IsInfinite4() const { return ( _mm_movemask_ps( EqualsInfinity() ) != 0 ); }

        EE_FORCE_INLINE Vector EqualsNaN() const { return _mm_cmpneq_ps( m_data, m_data ); }

        EE_FORCE_INLINE bool IsNaN2() const { return ( _mm_movemask_ps( EqualsNaN() ) & 3 ) != 0; }
        EE_FORCE_INLINE bool IsNaN3() const { return ( _mm_movemask_ps( EqualsNaN() ) & 7 ) != 0; }
        EE_FORCE_INLINE bool IsNaN4() const { return ( _mm_movemask_ps( EqualsNaN() ) != 0 ); }

        EE_FORCE_INLINE bool IsParallelTo( Vector const& v ) const;

        EE_FORCE_INLINE void ToDirectionAndLength2( Vector& direction, float& length ) const;
        EE_FORCE_INLINE void ToDirectionAndLength3( Vector& direction, float& length ) const;

        bool operator==( Vector const& rhs ) const { return IsEqual4( rhs ); }
        bool operator!=( Vector const& rhs ) const { return !IsEqual4( rhs ); }

    public:

        union
        {
            struct { float m_x, m_y, m_z, m_w; };
            __m128 m_data;
        };
    };

    //-------------------------------------------------------------------------

    static_assert( sizeof( Vector ) == 16, "Vector size must be 16 bytes!" );

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE void Vector::StoreFloat( float& value ) const
    {
        _mm_store_ss( &value, m_data );
    }

    EE_FORCE_INLINE void Vector::StoreFloat2( Float2& value ) const
    {
        auto yVec = _mm_shuffle_ps( m_data, m_data, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        _mm_store_ss( &value.m_x, m_data );
        _mm_store_ss( &value.m_y, yVec );
    }

    EE_FORCE_INLINE void Vector::StoreFloat3( Float3& value ) const
    {
        auto yVec = _mm_shuffle_ps( m_data, m_data, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        auto zVec = _mm_shuffle_ps( m_data, m_data, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        _mm_store_ss( &value.m_x, m_data );
        _mm_store_ss( &value.m_y, yVec );
        _mm_store_ss( &value.m_z, zVec );
    }

    EE_FORCE_INLINE void Vector::StoreFloat4( Float4& value ) const
    {
        _mm_storeu_ps( &value.m_x, m_data );
    }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE float Vector::ToFloat() const
    {
        float v;
        StoreFloat( v );
        return v;
    }

    EE_FORCE_INLINE Float2 Vector::ToFloat2() const
    {
        Float2 v;
        StoreFloat2( v );
        return v;
    }

    EE_FORCE_INLINE Float3 Vector::ToFloat3() const
    {
        Float3 v;
        StoreFloat3( v );
        return v;
    }

    EE_FORCE_INLINE Float4 Vector::ToFloat4() const
    {
        Float4 v;
        StoreFloat4( v );
        return v;
    }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE Vector& Vector::Normalize2()
    {
        // Perform the dot product on m_x and m_y only
        auto vLengthSq = _mm_mul_ps( m_data, m_data );
        auto vTemp = _mm_shuffle_ps( vLengthSq, vLengthSq, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        vLengthSq = _mm_add_ss( vLengthSq, vTemp );
        vLengthSq = _mm_shuffle_ps( vLengthSq,  vLengthSq, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        // Prepare for the division
        auto vResult = _mm_sqrt_ps( vLengthSq );
        // Create zero with a single instruction
        auto vZeroMask = _mm_setzero_ps();
        // Test for a divide by zero (Must be FP to detect -0.0)
        vZeroMask = _mm_cmpneq_ps( vZeroMask, vResult );
        // Failsafe on zero (Or epsilon) length planes
        // If the length is infinity, set the elements to zero
        vLengthSq = _mm_cmpneq_ps( vLengthSq, Vector::Infinity );
        // Divide to perform the normalization
        vResult = _mm_div_ps( m_data, vResult );
        // Any that are infinity, set to zero
        vResult = _mm_and_ps( vResult, vZeroMask );
        // Select qnan or result based on infinite length
        auto vTemp1 = _mm_andnot_ps( vLengthSq, Vector::QNaN );
        auto vTemp2 = _mm_and_ps( vResult, vLengthSq );
        m_data = _mm_or_ps( vTemp1, vTemp2 );

        *this = Select( *this, Vector::Zero, Select0011 );

        return *this;
    }

    EE_FORCE_INLINE Vector Vector::GetNormalized2() const
    {
        Vector v = *this;
        v.Normalize2();
        return v;
    }

    EE_FORCE_INLINE Vector& Vector::Normalize3()
    {
        // Perform the dot product on m_x,m_y and m_z only
        auto vLengthSq = _mm_mul_ps( m_data, m_data );
        auto vTemp = _mm_shuffle_ps( vLengthSq, vLengthSq, _MM_SHUFFLE( 2, 1, 2, 1 ) );
        vLengthSq = _mm_add_ss( vLengthSq, vTemp );
        vTemp = _mm_shuffle_ps( vTemp, vTemp, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        vLengthSq = _mm_add_ss( vLengthSq, vTemp );
        vLengthSq = _mm_shuffle_ps( vLengthSq, vLengthSq, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        // Prepare for the division
        auto vResult = _mm_sqrt_ps( vLengthSq );
        // Create zero with a single instruction
        auto vZeroMask = _mm_setzero_ps();
        // Test for a divide by zero (Must be FP to detect -0.0)
        vZeroMask = _mm_cmpneq_ps( vZeroMask, vResult );
        // Failsafe on zero (Or epsilon) length planes
        // If the length is infinity, set the elements to zero
        vLengthSq = _mm_cmpneq_ps( vLengthSq, Vector::Infinity );
        // Divide to perform the normalization
        vResult = _mm_div_ps( m_data, vResult );
        // Any that are infinity, set to zero
        vResult = _mm_and_ps( vResult, vZeroMask );
        // Select qnan or result based on infinite length
        auto vTemp1 = _mm_andnot_ps( vLengthSq, Vector::QNaN );
        auto vTemp2 = _mm_and_ps( vResult, vLengthSq );
        m_data = _mm_or_ps( vTemp1, vTemp2 );

        *this = Select( *this, Vector::Zero, Select0001 );

        return *this;
    }

    EE_FORCE_INLINE Vector Vector::GetNormalized3() const
    {
        Vector v = *this;
        v.Normalize3();
        return v;
    }

    EE_FORCE_INLINE Vector& Vector::Normalize4()
    {
        // Perform the dot product on m_x,m_y,m_z and m_w
        auto vLengthSq = _mm_mul_ps( m_data, m_data );
        // vTemp has m_z and m_w
        auto vTemp = _mm_shuffle_ps( vLengthSq, vLengthSq, _MM_SHUFFLE( 3, 2, 3, 2 ) );
        // m_x+m_z, m_y+m_w
        vLengthSq = _mm_add_ps( vLengthSq, vTemp );
        // m_x+m_z,m_x+m_z,m_x+m_z,m_y+m_w
        vLengthSq = _mm_shuffle_ps( vLengthSq, vLengthSq, _MM_SHUFFLE( 1, 0, 0, 0 ) );
        // ??,??,m_y+m_w,m_y+m_w
        vTemp = _mm_shuffle_ps( vTemp, vLengthSq, _MM_SHUFFLE( 3, 3, 0, 0 ) );
        // ??,??,m_x+m_z+m_y+m_w,??
        vLengthSq = _mm_add_ps( vLengthSq, vTemp );
        // Splat the length
        vLengthSq = _mm_shuffle_ps( vLengthSq, vLengthSq, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        // Prepare for the division
        auto vResult = _mm_sqrt_ps( vLengthSq );
        // Create zero with a single instruction
        auto vZeroMask = _mm_setzero_ps();
        // Test for a divide by zero (Must be FP to detect -0.0)
        vZeroMask = _mm_cmpneq_ps( vZeroMask, vResult );
        // Failsafe on zero (Or epsilon) length planes
        // If the length is infinity, set the elements to zero
        vLengthSq = _mm_cmpneq_ps( vLengthSq, Vector::Infinity );
        // Divide to perform the normalization
        vResult = _mm_div_ps( m_data, vResult );
        // Any that are infinity, set to zero
        vResult = _mm_and_ps( vResult, vZeroMask );
        // Select qnan or result based on infinite length
        auto vTemp1 = _mm_andnot_ps( vLengthSq, Vector::QNaN );
        auto vTemp2 = _mm_and_ps( vResult, vLengthSq );
        m_data = _mm_or_ps( vTemp1, vTemp2 );

        return *this;
    }

    EE_FORCE_INLINE Vector Vector::GetNormalized4() const
    {
        Vector v = *this;
        v.Normalize4();
        return v;
    }

    EE_FORCE_INLINE Vector& Vector::Floor()
    {
        Vector result;

        // To handle NAN, INF and numbers greater than 8388608, use masking
        __m128i vTest = _mm_and_si128( _mm_castps_si128( m_data ), SIMD::g_absMask );
        vTest = _mm_cmplt_epi32( vTest, SIMD::g_noFraction );
        // Truncate
        __m128i vInt = _mm_cvttps_epi32( m_data );
        result = _mm_cvtepi32_ps( vInt );
        __m128 vLarger = _mm_cmpgt_ps( result, m_data );
        // 0 -> 0, 0xffffffff -> -1.0f
        vLarger = _mm_cvtepi32_ps( _mm_castps_si128( vLarger ) );
        result = _mm_add_ps( result, vLarger );
        // All numbers less than 8388608 will use the round to int
        result = _mm_and_ps( result, _mm_castsi128_ps( vTest ) );
        // All others, use the ORIGINAL value
        vTest = _mm_andnot_si128( vTest, _mm_castps_si128( m_data ) );
        result = _mm_or_ps( result, _mm_castsi128_ps( vTest ) );

        m_data = result;
        return *this;
    }

    EE_FORCE_INLINE Vector Vector::GetFloor() const
    {
        Vector v = *this;
        v.Floor();
        return v;
    }

    EE_FORCE_INLINE Vector& Vector::Ceil()
    {
        Vector result;

        // To handle NAN, INF and numbers greater than 8388608, use masking
        __m128i vTest = _mm_and_si128( _mm_castps_si128( m_data ), SIMD::g_absMask );
        vTest = _mm_cmplt_epi32( vTest, SIMD::g_noFraction );
        // Truncate
        __m128i vInt = _mm_cvttps_epi32( m_data );
        result = _mm_cvtepi32_ps( vInt );
        __m128 vSmaller = _mm_cmplt_ps( result, m_data );
        // 0 -> 0, 0xffffffff -> -1.0f
        vSmaller = _mm_cvtepi32_ps( _mm_castps_si128( vSmaller ) );
        result = _mm_sub_ps( result, vSmaller );
        // All numbers less than 8388608 will use the round to int
        result = _mm_and_ps( result, _mm_castsi128_ps( vTest ) );
        // All others, use the ORIGINAL value
        vTest = _mm_andnot_si128( vTest, _mm_castps_si128( m_data ) );
        result = _mm_or_ps( result, _mm_castsi128_ps( vTest ) );

        m_data = result;
        return *this;
    }

    EE_FORCE_INLINE Vector Vector::GetCeil() const
    {
        Vector v = *this;
        v.Ceil();
        return v;
    }

    EE_FORCE_INLINE Vector& Vector::Round()
    {
        __m128 sign = _mm_and_ps( m_data, SIMD::g_signMask );
        __m128 sMagic = _mm_or_ps( SIMD::g_noFraction, sign );
        __m128 R1 = _mm_add_ps( m_data, sMagic );
        R1 = _mm_sub_ps( R1, sMagic );
        __m128 R2 = _mm_and_ps( m_data, SIMD::g_absMask );
        __m128 mask = _mm_cmple_ps( R2, SIMD::g_noFraction );
        R2 = _mm_andnot_ps( mask, m_data );
        R1 = _mm_and_ps( R1, mask );
        m_data = _mm_xor_ps( R1, R2 );
        return  *this;
    }

    EE_FORCE_INLINE Vector Vector::GetRound() const
    {
        Vector v = *this;
        v.Round();
        return v;
    }

    EE_FORCE_INLINE Vector Vector::GetSign() const
    {
        Vector const selectMask = GreaterThanEqual( Vector::Zero );
        return Vector::Select( Vector::NegativeOne, Vector::One, selectMask );
    }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE Vector Vector::Length2() const
    {
        Vector result;

        result = _mm_mul_ps( m_data, m_data );
        auto vTemp = _mm_shuffle_ps( result, result, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        // m_x+m_y
        result = _mm_add_ss( result, vTemp );
        result = _mm_shuffle_ps( result, result, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        result = _mm_sqrt_ps( result );
        return result;
    }

    EE_FORCE_INLINE Vector Vector::Length3() const
    {
        Vector result;

        // Perform the dot product on m_x,m_y and m_z
        result = _mm_mul_ps( m_data, m_data );
        // vTemp has m_z and m_y
        auto vTemp = _mm_shuffle_ps( result, result, _MM_SHUFFLE( 1, 2, 1, 2 ) );
        // m_x+m_z, m_y
        result = _mm_add_ss( result, vTemp );
        // m_y,m_y,m_y,m_y
        vTemp = _mm_shuffle_ps( vTemp, vTemp, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        // m_x+m_z+m_y,??,??,??
        result = _mm_add_ss( result, vTemp );
        // Splat the length squared
        result = _mm_shuffle_ps( result, result, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        // Get the length
        result = _mm_sqrt_ps( result );

        return result;
    }

    EE_FORCE_INLINE Vector Vector::Length4() const
    {
        Vector result;

        // Perform the dot product on m_x,m_y,m_z and m_w
        result = _mm_mul_ps( m_data, m_data );
        // vTemp has m_z and m_w
        auto vTemp = _mm_shuffle_ps( result, result, _MM_SHUFFLE( 3, 2, 3, 2 ) );
        // m_x+m_z, m_y+m_w
        result = _mm_add_ps( result, vTemp );
        // m_x+m_z,m_x+m_z,m_x+m_z,m_y+m_w
        result = _mm_shuffle_ps( result, result, _MM_SHUFFLE( 1, 0, 0, 0 ) );
        // ??,??,m_y+m_w,m_y+m_w
        vTemp = _mm_shuffle_ps( vTemp, result, _MM_SHUFFLE( 3, 3, 0, 0 ) );
        // ??,??,m_x+m_z+m_y+m_w,??
        result = _mm_add_ps( result, vTemp );
        // Splat the length
        result = _mm_shuffle_ps( result, result, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        // Get the length
        result = _mm_sqrt_ps( result );

        return result;
    }

    EE_FORCE_INLINE Vector Vector::InverseLength2() const
    {
        // Perform the dot product on m_x and m_y
        auto vLengthSq = _mm_mul_ps( m_data, m_data );
        // vTemp has m_y splatted
        auto vTemp = _mm_shuffle_ps( vLengthSq, vLengthSq, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        // m_x+m_y
        vLengthSq = _mm_add_ss( vLengthSq, vTemp );
        vLengthSq = _mm_sqrt_ss( vLengthSq );
        vLengthSq = _mm_div_ss( Vector::One, vLengthSq );
        vLengthSq = _mm_shuffle_ps( vLengthSq, vLengthSq, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        return vLengthSq;
    }

    EE_FORCE_INLINE Vector Vector::InverseLength3() const
    {
        // Perform the dot product
        auto vDot = _mm_mul_ps( m_data, m_data );
        // m_x=Dot.m_y, m_y=Dot.m_z
        auto vTemp = _mm_shuffle_ps( vDot, vDot, _MM_SHUFFLE( 2, 1, 2, 1 ) );
        // Result.m_x = m_x+m_y
        vDot = _mm_add_ss( vDot, vTemp );
        // m_x=Dot.m_z
        vTemp = _mm_shuffle_ps( vTemp, vTemp, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        // Result.m_x = (m_x+m_y)+m_z
        vDot = _mm_add_ss( vDot, vTemp );
        // Splat m_x
        vDot = _mm_shuffle_ps( vDot, vDot, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        // Get the reciprocal
        vDot = _mm_sqrt_ps( vDot );
        // Get the reciprocal
        vDot = _mm_div_ps( Vector::One, vDot );
        return vDot;
    }

    EE_FORCE_INLINE Vector Vector::InverseLength4() const
    {
        // Perform the dot product on m_x,m_y,m_z and m_w
        auto vLengthSq = _mm_mul_ps( m_data, m_data );
        // vTemp has m_z and m_w
        auto vTemp = _mm_shuffle_ps( vLengthSq, vLengthSq, _MM_SHUFFLE( 3, 2, 3, 2 ) );
        // m_x+m_z, m_y+m_w
        vLengthSq = _mm_add_ps( vLengthSq, vTemp );
        // m_x+m_z,m_x+m_z,m_x+m_z,m_y+m_w
        vLengthSq = _mm_shuffle_ps( vLengthSq, vLengthSq, _MM_SHUFFLE( 1, 0, 0, 0 ) );
        // ??,??,m_y+m_w,m_y+m_w
        vTemp = _mm_shuffle_ps( vTemp, vLengthSq, _MM_SHUFFLE( 3, 3, 0, 0 ) );
        // ??,??,m_x+m_z+m_y+m_w,??
        vLengthSq = _mm_add_ps( vLengthSq, vTemp );
        // Splat the length
        vLengthSq = _mm_shuffle_ps( vLengthSq, vLengthSq, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        // Get the reciprocal
        vLengthSq = _mm_sqrt_ps( vLengthSq );
        // Accurate!
        vLengthSq = _mm_div_ps( Vector::One, vLengthSq );
        return vLengthSq;
    }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE Vector Vector::NearEqual( Vector const& v, Vector const& epsilon ) const
    {
        // Get the difference
        auto vDelta = _mm_sub_ps( m_data, v );
        // Get the absolute value of the difference
        auto vTemp = _mm_setzero_ps();
        vTemp = _mm_sub_ps( vTemp, vDelta );
        vTemp = _mm_max_ps( vTemp, vDelta );
        vTemp = _mm_cmple_ps( vTemp, epsilon );
        return vTemp;
    }

    EE_FORCE_INLINE Vector Vector::InBounds( Vector const& bounds ) const
    {
        // Test if less than or equal
        auto vTemp1 = _mm_cmple_ps( m_data, bounds );
        // Negate the bounds
        auto vTemp2 = _mm_mul_ps( bounds, Vector::NegativeOne );
        // Test if greater or equal (Reversed)
        vTemp2 = _mm_cmple_ps( vTemp2, m_data );
        // Blend answers
        vTemp1 = _mm_and_ps( vTemp1, vTemp2 );
        return vTemp1;
    }

    EE_FORCE_INLINE bool Vector::IsParallelTo( Vector const& v ) const
    {
        Vector const vAbsDot = Vector::Dot3( *this, v ).GetAbs();
        Vector const vAbsDelta = Vector::One - vAbsDot;
        return vAbsDelta.IsLessThanEqual4( Vector::Epsilon );
    }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE Vector::Vector( Axis axis )
    {
        switch ( axis )
        {
            case Axis::X: *this = Vector::UnitX; break;
            case Axis::Y: *this = Vector::UnitY; break;
            case Axis::Z: *this = Vector::UnitZ; break;
            default: EE_HALT(); break;
        }
    }

    EE_FORCE_INLINE Vector Vector::Orthogonal2D() const
    {
        static Vector const negX( -1.0f, 1.0f, 1.0f, 1.0f );

        Vector result;
        result = _mm_shuffle_ps( *this, *this, _MM_SHUFFLE( 3, 2, 0, 1 ) );
        result = _mm_mul_ps( result, negX );
        return result;
    }

    EE_FORCE_INLINE Vector Vector::Cross2( Vector const& other ) const
    {
        Vector vResult = _mm_shuffle_ps( other.m_data, other.m_data, _MM_SHUFFLE( 0, 1, 0, 1 ) );
        vResult = _mm_mul_ps( vResult, m_data );
        Vector vTemp = vResult.GetSplatY();
        vResult = _mm_sub_ss( vResult, vTemp );
        vResult = vResult.GetSplatX();
        return vResult;
    }

    EE_FORCE_INLINE Vector Vector::Cross3( Vector const& other ) const
    {
        auto vTemp1 = _mm_shuffle_ps( m_data, m_data, _MM_SHUFFLE( 3, 0, 2, 1 ) );
        auto vTemp2 = _mm_shuffle_ps( other, other, _MM_SHUFFLE( 3, 1, 0, 2 ) );
        Vector result = _mm_mul_ps( vTemp1, vTemp2 );
        vTemp1 = _mm_shuffle_ps( vTemp1, vTemp1, _MM_SHUFFLE( 3, 0, 2, 1 ) );
        vTemp2 = _mm_shuffle_ps( vTemp2, vTemp2, _MM_SHUFFLE( 3, 1, 0, 2 ) );
        vTemp1 = _mm_mul_ps( vTemp1, vTemp2 );
        result = _mm_sub_ps( result, vTemp1 );
        result = _mm_and_ps( result, SIMD::g_maskXYZ0 );
        return result;
    }

    EE_FORCE_INLINE Vector Vector::Dot2( Vector const& other ) const
    {
        // Perform the dot product on m_x and m_y
        Vector result = _mm_mul_ps( m_data, other );
        // vTemp has m_y splatted
        auto vTemp = _mm_shuffle_ps( result, result, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        // m_x+m_y
        result = _mm_add_ss( result, vTemp );
        result = _mm_shuffle_ps( result, result, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        return result;
    }

    EE_FORCE_INLINE Vector Vector::Dot3( Vector const& vOther ) const
    {
        // Perform the dot product
        auto vDot = _mm_mul_ps( m_data, vOther );
        // m_x=Dot.vector4_f32[1], m_y=Dot.vector4_f32[2]
        auto vTemp = _mm_shuffle_ps( vDot, vDot, _MM_SHUFFLE( 2, 1, 2, 1 ) );
        // Result.vector4_f32[0] = m_x+m_y
        vDot = _mm_add_ss( vDot, vTemp );
        // m_x=Dot.vector4_f32[2]
        vTemp = _mm_shuffle_ps( vTemp, vTemp, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        // Result.vector4_f32[0] = (m_x+m_y)+m_z
        vDot = _mm_add_ss( vDot, vTemp );
        // Splat m_x
        Vector result = _mm_shuffle_ps( vDot, vDot, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        return result;
    }

    EE_FORCE_INLINE Vector Vector::Dot4( Vector const& other ) const
    {
        auto vTemp2 = other;
        auto vTemp = _mm_mul_ps( m_data, vTemp2 );
        vTemp2 = _mm_shuffle_ps( vTemp2, vTemp, _MM_SHUFFLE( 1, 0, 0, 0 ) ); // Copy X to the Z position and Y to the W position
        vTemp2 = _mm_add_ps( vTemp2, vTemp ); // Add Z = X+Z; W = Y+W;
        vTemp = _mm_shuffle_ps( vTemp, vTemp2, _MM_SHUFFLE( 0, 3, 0, 0 ) );  // Copy W to the Z position
        vTemp = _mm_add_ps( vTemp, vTemp2 ); // Add Z and W together
        return _mm_shuffle_ps( vTemp, vTemp, _MM_SHUFFLE( 2, 2, 2, 2 ) ); // Splat Z and return
    }

    EE_FORCE_INLINE Vector Vector::ScalarProjection( Vector const& other ) const
    {
        Vector const normalizedThis = GetNormalized3();
        Vector const projection = other.Dot3( normalizedThis );
        return projection;
    }

    EE_FORCE_INLINE Vector Vector::VectorProjection( Vector const& other ) const
    {
        Vector const normalizedThis = GetNormalized3();
        Vector const dotOther = other.Dot3( normalizedThis );
        Vector const projection = normalizedThis * dotOther;
        return projection;
    }

    EE_FORCE_INLINE Vector Vector::Average2( Vector const& v0, Vector const& v1 )
    {
        auto avg4 = Average4( v0, v1 );
        return Vector::Select( avg4, Vector::Zero, Vector( 0, 0, 1, 1 ) );
    }

    EE_FORCE_INLINE Vector Vector::Average3( Vector const& v0, Vector const& v1 )
    {
        auto avg4 = Average4( v0, v1 );
        return Vector::Select( avg4, Vector::Zero, Vector( 0, 0, 0, 1 ) );
    }

    EE_FORCE_INLINE Vector Vector::Average4( Vector const& v0, Vector const& v1 )
    {
        return ( v0 + v1 ) * Vector::Half;
    }

    EE_FORCE_INLINE Vector Vector::Min( Vector const& v0, Vector const& v1 )
    {
        Vector result;
        result = _mm_min_ps( v0, v1 );
        return result;
    }

    EE_FORCE_INLINE Vector Vector::Max( Vector const& v0, Vector const& v1 )
    {
        Vector result;
        result = _mm_max_ps( v0, v1 );
        return result;
    }

    EE_FORCE_INLINE Vector Vector::Clamp( Vector const& v, Vector const& min, Vector const& max )
    {
        Vector result;
        result = _mm_max_ps( min, v );
        result = _mm_min_ps( result, max );
        return result;
    }

    EE_FORCE_INLINE Vector Vector::MultiplyAdd( Vector const& v, Vector const& multiplier, Vector const& add )
    {
        Vector result;
        result = _mm_mul_ps( v, multiplier );
        result = _mm_add_ps( result, add );
        return result;
    }

    EE_FORCE_INLINE Vector Vector::NegativeMultiplySubtract( Vector const& vec, Vector const& multiplier, Vector const& subtrahend )
    {
        // result = subtrahend - ( vec * multiplier )
        auto r = _mm_mul_ps( vec, multiplier );
        return _mm_sub_ps( subtrahend, r );
    }

    EE_FORCE_INLINE Vector Vector::Xor( Vector const& v0, Vector const& v1 )
    {
        __m128i V = _mm_xor_si128( _mm_castps_si128( v0 ), _mm_castps_si128( v1 ) );

        Vector result;
        result = _mm_castsi128_ps( V );
        return result;
    }

    EE_FORCE_INLINE Vector Vector::Select( Vector const& v0, Vector const& v1, Vector const& control )
    {
        auto const ctrl = _mm_cmpneq_ps( control, Vector::Zero );

        Vector result;
        auto vTemp1 = _mm_andnot_ps( ctrl, v0 );
        auto vTemp2 = _mm_and_ps( v1, ctrl );
        result = _mm_or_ps( vTemp1, vTemp2 );
        return result;
    }

    template<uint32_t PermuteX, uint32_t PermuteY, uint32_t PermuteZ, uint32_t PermuteW>
    EE_FORCE_INLINE Vector Vector::Permute( Vector const& v0, Vector const& v1 )
    {
        static_assert( PermuteX <= 7, "Element index parameter out of range" );
        static_assert( PermuteY <= 7, "Element index parameter out of range" );
        static_assert( PermuteZ <= 7, "Element index parameter out of range" );
        static_assert( PermuteW <= 7, "Element index parameter out of range" );

        uint32_t const shuffle = _MM_SHUFFLE( PermuteW & 3, PermuteZ & 3, PermuteY & 3, PermuteX & 3 );
        bool const whichX = PermuteX > 3;
        bool const whichY = PermuteY > 3;
        bool const whichZ = PermuteZ > 3;
        bool const whichW = PermuteW > 3;

        static SIMD::UIntMask const selectMask = { whichX ? 0xFFFFFFFF : 0, whichY ? 0xFFFFFFFF : 0, whichZ ? 0xFFFFFFFF : 0, whichW ? 0xFFFFFFFF : 0 };
        __m128 shuffled1 = _mm_shuffle_ps( v0, v0, shuffle );
        __m128 shuffled2 = _mm_shuffle_ps( v1, v1, shuffle );
        __m128 masked1 = _mm_andnot_ps( selectMask, shuffled1 );
        __m128 masked2 = _mm_and_ps( selectMask, shuffled2 );
        return _mm_or_ps( masked1, masked2 );
    }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE Vector Vector::Sin( Vector const& vec )
    {
        // Force the value within the bounds of pi
        auto m_x = Vector::AngleMod2Pi( vec );

        // Map in [-pi/2,pi/2] with sin(m_y) = sin(m_x).
        __m128 sign = _mm_and_ps( m_x, SIMD::g_signMask );
        __m128 c = _mm_or_ps( Vector::Pi, sign );  // pi when m_x >= 0, -pi when m_x < 0
        __m128 absx = _mm_andnot_ps( sign, m_x );  // |m_x|
        __m128 rflx = _mm_sub_ps( c, m_x );
        __m128 comp = _mm_cmple_ps( absx, Vector::PiDivTwo );
        __m128 select0 = _mm_and_ps( comp, m_x );
        __m128 select1 = _mm_andnot_ps( comp, rflx );
        m_x = _mm_or_ps( select0, select1 );

        __m128 x2 = _mm_mul_ps( m_x, m_x );

        // Compute polynomial approximation
        const auto SC1 = SIMD::g_sinCoefficients1;
        auto vConstants = _mm_shuffle_ps( SC1, SC1, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        __m128 Result = _mm_mul_ps( vConstants, x2 );

        const auto SC0 = SIMD::g_sinCoefficients0;
        vConstants = _mm_shuffle_ps( SC0, SC0, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( SC0, SC0, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( SC0, SC0, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( SC0, SC0, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );
        Result = _mm_add_ps( Result, Vector::One );
        Result = _mm_mul_ps( Result, m_x );
        return Result;
    }

    EE_FORCE_INLINE Vector Vector::Cos( Vector const& vec )
    {
        // Map V to m_x in [-pi,pi].
        auto m_x = Vector::AngleMod2Pi( vec );

        // Map in [-pi/2,pi/2] with cos(m_y) = sign*cos(m_x).
        auto sign = _mm_and_ps( m_x, SIMD::g_signMask );
        __m128 c = _mm_or_ps( Vector::Pi, sign );  // pi when m_x >= 0, -pi when m_x < 0
        __m128 absx = _mm_andnot_ps( sign, m_x );  // |m_x|
        __m128 rflx = _mm_sub_ps( c, m_x );
        __m128 comp = _mm_cmple_ps( absx, Vector::PiDivTwo );
        __m128 select0 = _mm_and_ps( comp, m_x );
        __m128 select1 = _mm_andnot_ps( comp, rflx );
        m_x = _mm_or_ps( select0, select1 );
        select0 = _mm_and_ps( comp, Vector::One );
        select1 = _mm_andnot_ps( comp, Vector::NegativeOne );
        sign = _mm_or_ps( select0, select1 );

        __m128 x2 = _mm_mul_ps( m_x, m_x );

        // Compute polynomial approximation
        const auto CC1 = SIMD::g_cosCoefficients1;
        auto vConstants = _mm_shuffle_ps( CC1, CC1, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        __m128 Result = _mm_mul_ps( vConstants, x2 );

        const auto CC0 = SIMD::g_cosCoefficients0;
        vConstants = _mm_shuffle_ps( CC0, CC0, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( CC0, CC0, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( CC0, CC0, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( CC0, CC0, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );
        Result = _mm_add_ps( Result, Vector::One );
        Result = _mm_mul_ps( Result, sign );
        return Result;
    }

    void Vector::SinCos( Vector& sin, Vector& cos, Vector const& angle )
    {
        // Force the value within the bounds of pi
        auto m_x = Vector::AngleMod2Pi( angle );

        // Map in [-pi/2,pi/2] with sin(m_y) = sin(m_x), cos(m_y) = sign*cos(m_x).
        auto sign = _mm_and_ps( m_x, SIMD::g_signMask );
        __m128 c = _mm_or_ps( Vector::Pi, sign );  // pi when m_x >= 0, -pi when m_x < 0
        __m128 absx = _mm_andnot_ps( sign, m_x );  // |m_x|
        __m128 rflx = _mm_sub_ps( c, m_x );
        __m128 comp = _mm_cmple_ps( absx, Vector::PiDivTwo );
        __m128 select0 = _mm_and_ps( comp, m_x );
        __m128 select1 = _mm_andnot_ps( comp, rflx );
        m_x = _mm_or_ps( select0, select1 );
        select0 = _mm_and_ps( comp, Vector::One );
        select1 = _mm_andnot_ps( comp, Vector::NegativeOne );
        sign = _mm_or_ps( select0, select1 );

        __m128 x2 = _mm_mul_ps( m_x, m_x );

        // Compute polynomial approximation of sine
        const auto SC1 = SIMD::g_sinCoefficients1;
        auto vConstants = _mm_shuffle_ps( SC1, SC1, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        __m128 Result = _mm_mul_ps( vConstants, x2 );

        const auto SC0 = SIMD::g_sinCoefficients0;
        vConstants = _mm_shuffle_ps( SC0, SC0, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( SC0, SC0, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( SC0, SC0, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( SC0, SC0, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );
        Result = _mm_add_ps( Result, Vector::One );
        Result = _mm_mul_ps( Result, m_x );
        sin = Result;

        // Compute polynomial approximation of cosine
        const auto CC1 = SIMD::g_cosCoefficients1;
        vConstants = _mm_shuffle_ps( CC1, CC1, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        Result = _mm_mul_ps( vConstants, x2 );

        const auto CC0 = SIMD::g_cosCoefficients0;
        vConstants = _mm_shuffle_ps( CC0, CC0, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( CC0, CC0, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( CC0, CC0, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( CC0, CC0, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );
        Result = _mm_add_ps( Result, Vector::One );
        Result = _mm_mul_ps( Result, sign );
        cos = Result;
    }

    EE_FORCE_INLINE Vector Vector::Tan( Vector const& vec )
    {
        static const Vector tanCoefficients0 = { 1.0f, -4.667168334e-1f, 2.566383229e-2f, -3.118153191e-4f };
        static const Vector tanCoefficients1 = { 4.981943399e-7f, -1.333835001e-1f, 3.424887824e-3f, -1.786170734e-5f };
        static const Vector tanConstants = { 1.570796371f, 6.077100628e-11f, 0.000244140625f, 0.63661977228f /*2 / Pi*/ };
        static const SIMD::UIntMask mask = { 0x1, 0x1, 0x1, 0x1 };

        Vector TwoDivPi = tanConstants.GetSplatW();
        Vector C0 = tanConstants.GetSplatX();
        Vector C1 = tanConstants.GetSplatY();
        Vector vEpsilon = tanConstants.GetSplatZ();

        Vector VA = ( vec * TwoDivPi ).Round();
        Vector VC = Vector::NegativeMultiplySubtract( VA, C0, vec );
        Vector VB = VA.GetAbs();
        VC = Vector::NegativeMultiplySubtract( VA, C1, VC );
        reinterpret_cast<__m128i *>( &VB )[0] = _mm_cvttps_epi32( VB );

        Vector VC2 = VC * VC;
        Vector T7 = tanCoefficients1.GetSplatW();
        Vector T6 = tanCoefficients1.GetSplatZ();
        Vector T4 = tanCoefficients1.GetSplatX();
        Vector T3 = tanCoefficients0.GetSplatW();
        Vector T5 = tanCoefficients1.GetSplatY();
        Vector T2 = tanCoefficients0.GetSplatZ();
        Vector T1 = tanCoefficients0.GetSplatY();
        Vector T0 = tanCoefficients0.GetSplatX();

        Vector VBIsEven = _mm_and_ps( VB, mask );
        VBIsEven = _mm_castsi128_ps( _mm_cmpeq_epi32( _mm_castps_si128( VBIsEven ), _mm_castps_si128( Vector::Zero ) ) );

        Vector N = Vector::MultiplyAdd( VC2, T7, T6 );
        Vector D = Vector::MultiplyAdd( VC2, T4, T3 );
        N = Vector::MultiplyAdd( VC2, N, T5 );
        D = Vector::MultiplyAdd( VC2, D, T2 );
        N = VC2 * N;
        D = Vector::MultiplyAdd( VC2, D, T1 );
        N = Vector::MultiplyAdd( VC, N, VC );
        Vector VCNearZero = VC.InBounds( vEpsilon );
        D = Vector::MultiplyAdd( VC2, D, T0 );

        N = Vector::Select( N, VC, VCNearZero );
        D = Vector::Select( D, Vector::One, VCNearZero );

        Vector R0 = N.GetNegated();
        Vector R1 = N / D;
        R0 = D / R0;

        Vector VIsZero = vec.EqualsZero();
        Vector Result = Vector::Select( R0, R1, VBIsEven );
        Result = Vector::Select( Result, Zero, VIsZero );

        return Result;
    }

    EE_FORCE_INLINE Vector Vector::ASin( Vector const& vec )
    {
        __m128 nonnegative = _mm_cmpge_ps( vec, Vector::Zero );
        __m128 mvalue = _mm_sub_ps( Vector::Zero, vec );
        __m128 m_x = _mm_max_ps( vec, mvalue );  // |vec|

        // Compute (1-|vec|), clamp to zero to avoid sqrt of negative number.
        __m128 oneMValue = _mm_sub_ps( Vector::One, m_x );
        __m128 clampOneMValue = _mm_max_ps( Vector::Zero, oneMValue );
        __m128 root = _mm_sqrt_ps( clampOneMValue );  // sqrt(1-|vec|)

        // Compute polynomial approximation
        const auto AC1 = SIMD::g_arcCoefficients1;
        auto vConstants = _mm_shuffle_ps( AC1, AC1, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        __m128 t0 = _mm_mul_ps( vConstants, m_x );

        vConstants = _mm_shuffle_ps( AC1, AC1, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( AC1, AC1, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( AC1, AC1, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        const auto AC0 = SIMD::g_arcCoefficients0;
        vConstants = _mm_shuffle_ps( AC0, AC0, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( AC0, AC0, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( AC0, AC0, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( AC0, AC0, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, root );

        __m128 t1 = _mm_sub_ps( Vector::Pi, t0 );
        t0 = _mm_and_ps( nonnegative, t0 );
        t1 = _mm_andnot_ps( nonnegative, t1 );
        t0 = _mm_or_ps( t0, t1 );
        t0 = _mm_sub_ps( Vector::PiDivTwo, t0 );
        return t0;
    }

    EE_FORCE_INLINE Vector Vector::ACos( Vector const& vec )
    {
        __m128 nonnegative = _mm_cmpge_ps( vec, Vector::Zero );
        __m128 mvalue = _mm_sub_ps( Vector::Zero, vec );
        __m128 m_x = _mm_max_ps( vec, mvalue );  // |vec|

        // Compute (1-|vec|), clamp to zero to avoid sqrt of negative number.
        __m128 oneMValue = _mm_sub_ps( Vector::One, m_x );
        __m128 clampOneMValue = _mm_max_ps( Vector::Zero, oneMValue );
        __m128 root = _mm_sqrt_ps( clampOneMValue );  // sqrt(1-|vec|)

        // Compute polynomial approximation
        const auto AC1 = SIMD::g_arcCoefficients1;
        auto vConstants = _mm_shuffle_ps( AC1, AC1, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        __m128 t0 = _mm_mul_ps( vConstants, m_x );

        vConstants = _mm_shuffle_ps( AC1, AC1, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( AC1, AC1, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( AC1, AC1, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        const auto AC0 = SIMD::g_arcCoefficients0;
        vConstants = _mm_shuffle_ps( AC0, AC0, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( AC0, AC0, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( AC0, AC0, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( AC0, AC0, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, root );

        __m128 t1 = _mm_sub_ps( Vector::Pi, t0 );
        t0 = _mm_and_ps( nonnegative, t0 );
        t1 = _mm_andnot_ps( nonnegative, t1 );
        t0 = _mm_or_ps( t0, t1 );
        return t0;
    }

    EE_FORCE_INLINE Vector Vector::ATan( Vector const& vec )
    {
        __m128 absV = vec.GetAbs();
        __m128 invV = _mm_div_ps( Vector::One, vec );
        __m128 comp = _mm_cmpgt_ps( vec, Vector::One );
        __m128 select0 = _mm_and_ps( comp, Vector::One );
        __m128 select1 = _mm_andnot_ps( comp, Vector::NegativeOne );
        __m128 sign = _mm_or_ps( select0, select1 );
        comp = _mm_cmple_ps( absV, Vector::One );
        select0 = _mm_and_ps( comp, Vector::Zero );
        select1 = _mm_andnot_ps( comp, sign );
        sign = _mm_or_ps( select0, select1 );
        select0 = _mm_and_ps( comp, vec );
        select1 = _mm_andnot_ps( comp, invV );
        __m128 m_x = _mm_or_ps( select0, select1 );

        __m128 x2 = _mm_mul_ps( m_x, m_x );

        // Compute polynomial approximation
        Vector const TC1 = SIMD::g_aTanCoefficients1;
        Vector vConstants = _mm_shuffle_ps( TC1, TC1, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        __m128 Result = _mm_mul_ps( vConstants, x2 );

        vConstants = _mm_shuffle_ps( TC1, TC1, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( TC1, TC1, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( TC1, TC1, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        Vector const TC0 = SIMD::g_aTanCoefficients0;
        vConstants = _mm_shuffle_ps( TC0, TC0, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( TC0, TC0, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( TC0, TC0, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( TC0, TC0, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );
        Result = _mm_add_ps( Result, Vector::One );
        Result = _mm_mul_ps( Result, m_x );
        __m128 result1 = _mm_mul_ps( sign, Vector::PiDivTwo );
        result1 = _mm_sub_ps( result1, Result );

        comp = _mm_cmpeq_ps( sign, Vector::Zero );
        select0 = _mm_and_ps( comp, Result );
        select1 = _mm_andnot_ps( comp, result1 );
        Result = _mm_or_ps( select0, select1 );
        return Result;
    }

    EE_FORCE_INLINE Vector Vector::ATan2( Vector const& Y, Vector const& X )
    {
        Vector ATanResultValid = Vector( SIMD::g_trueMask );

        Vector vPi = Vector( SIMD::g_aTan2Constants ).GetSplatX();
        Vector vPiOverTwo = Vector( SIMD::g_aTan2Constants ).GetSplatY();
        Vector vPiOverFour = Vector( SIMD::g_aTan2Constants ).GetSplatZ();
        Vector vThreePiOverFour = Vector( SIMD::g_aTan2Constants ).GetSplatW();

        Vector YEqualsZero = Y.EqualsZero();
        Vector XEqualsZero = X.EqualsZero();
        Vector XIsPositive = _mm_and_ps( X, SIMD::g_signMask );
        XIsPositive = _mm_castsi128_ps( _mm_cmpeq_epi32( _mm_castps_si128( XIsPositive ), _mm_castps_si128( Vector::Zero ) ) );
        Vector YEqualsInfinity = Y.EqualsInfinity();
        Vector XEqualsInfinity = X.EqualsInfinity();

        Vector YSign = _mm_and_ps( Y, SIMD::g_signMask );
        vPi = _mm_castsi128_ps( _mm_or_si128( _mm_castps_si128( vPi ), _mm_castps_si128( YSign ) ) );
        vPiOverTwo = _mm_castsi128_ps( _mm_or_si128( _mm_castps_si128( vPiOverTwo ), _mm_castps_si128( YSign ) ) );
        vPiOverFour = _mm_castsi128_ps( _mm_or_si128( _mm_castps_si128( vPiOverFour ), _mm_castps_si128( YSign ) ) );
        vThreePiOverFour = _mm_castsi128_ps( _mm_or_si128( _mm_castps_si128( vThreePiOverFour ), _mm_castps_si128( YSign ) ) );

        Vector R1 = Vector::Select( vPi, YSign, XIsPositive );
        Vector R2 = Vector::Select( ATanResultValid, vPiOverTwo, XEqualsZero );
        Vector R3 = Vector::Select( R2, R1, YEqualsZero );
        Vector R4 = Vector::Select( vThreePiOverFour, vPiOverFour, XIsPositive );
        Vector R5 = Vector::Select( vPiOverTwo, R4, XEqualsInfinity );
        Vector Result = Vector::Select( R3, R5, YEqualsInfinity );
        ATanResultValid = _mm_castsi128_ps( _mm_cmpeq_epi32( _mm_castps_si128( Result ), _mm_castps_si128( ATanResultValid ) ) );

        Vector V = Y / X;
        Vector R0 = Vector::ATan( V );
        R1 = Vector::Select( vPi, Vector( SIMD::g_signMask ), XIsPositive );
        R2 = R0 + R1;

        return Vector::Select( Result, R2, ATanResultValid );
    }

    EE_FORCE_INLINE Vector Vector::SinEst( Vector const& vec )
    {
        // Force the value within the bounds of pi
        auto m_x = Vector::AngleMod2Pi( vec );

        // Map in [-pi/2,pi/2] with sin(m_y) = sin(m_x).
        __m128 sign = _mm_and_ps( m_x, SIMD::g_signMask );
        __m128 c = _mm_or_ps( Vector::Pi, sign );  // pi when m_x >= 0, -pi when m_x < 0
        __m128 absx = _mm_andnot_ps( sign, m_x );  // |m_x|
        __m128 rflx = _mm_sub_ps( c, m_x );
        __m128 comp = _mm_cmple_ps( absx, Vector::PiDivTwo );
        __m128 select0 = _mm_and_ps( comp, m_x );
        __m128 select1 = _mm_andnot_ps( comp, rflx );
        m_x = _mm_or_ps( select0, select1 );

        __m128 x2 = _mm_mul_ps( m_x, m_x );

        // Compute polynomial approximation
        const auto SEC = SIMD::g_sinCoefficients1;
        auto vConstants = _mm_shuffle_ps( SEC, SEC, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        __m128 Result = _mm_mul_ps( vConstants, x2 );

        vConstants = _mm_shuffle_ps( SEC, SEC, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( SEC, SEC, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        Result = _mm_add_ps( Result, Vector::One );
        Result = _mm_mul_ps( Result, m_x );
        return Result;
    }

    EE_FORCE_INLINE Vector Vector::CosEst( Vector const& vec )
    {
        // Map V to m_x in [-pi,pi].
        auto m_x = Vector::AngleMod2Pi( vec );

        // Map in [-pi/2,pi/2] with cos(m_y) = sign*cos(m_x).
        auto sign = _mm_and_ps( m_x, SIMD::g_signMask );
        __m128 c = _mm_or_ps( Vector::Pi, sign );  // pi when m_x >= 0, -pi when m_x < 0
        __m128 absx = _mm_andnot_ps( sign, m_x );  // |m_x|
        __m128 rflx = _mm_sub_ps( c, m_x );
        __m128 comp = _mm_cmple_ps( absx, Vector::PiDivTwo );
        __m128 select0 = _mm_and_ps( comp, m_x );
        __m128 select1 = _mm_andnot_ps( comp, rflx );
        m_x = _mm_or_ps( select0, select1 );
        select0 = _mm_and_ps( comp, Vector::One );
        select1 = _mm_andnot_ps( comp, Vector::NegativeOne );
        sign = _mm_or_ps( select0, select1 );

        __m128 x2 = _mm_mul_ps( m_x, m_x );

        // Compute polynomial approximation
        const auto CEC = SIMD::g_cosCoefficients1;
        auto vConstants = _mm_shuffle_ps( CEC, CEC, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        __m128 Result = _mm_mul_ps( vConstants, x2 );

        vConstants = _mm_shuffle_ps( CEC, CEC, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( CEC, CEC, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        Result = _mm_add_ps( Result, Vector::One );
        Result = _mm_mul_ps( Result, sign );
        return Result;
    }

    EE_FORCE_INLINE Vector Vector::TanEst( Vector const& vec )
    {
        Vector W = Vector( SIMD::g_tanEstCoefficients ).GetSplatW();
        Vector V1 = ( vec * W ).Round();
        V1 = Vector::NegativeMultiplySubtract( Vector::Pi, V1, vec );

        Vector const T0 = Vector( SIMD::g_tanEstCoefficients ).GetSplatX();
        Vector const T1 = Vector( SIMD::g_tanEstCoefficients ).GetSplatY();
        Vector const T2 = Vector( SIMD::g_tanEstCoefficients ).GetSplatZ();

        auto V2T2 = Vector::NegativeMultiplySubtract( V1, V1, T2 );
        auto V2 =  V1 * V1;
        auto V1T0 = V1 * T0;
        auto V1T1 = V1 * T1;

        auto N = Vector::MultiplyAdd( V2, V1T1, V1T0 );
        auto D = V2T2.GetInverseEst();
        return N * D;
    }

    EE_FORCE_INLINE Vector Vector::ASinEst( Vector const& vec )
    {
        __m128 nonnegative = _mm_cmpge_ps( vec, Vector::Zero );
        __m128 mvalue = _mm_sub_ps( Vector::Zero, vec );
        __m128 m_x = _mm_max_ps( vec, mvalue );  // |vec|

        // Compute (1-|vec|), clamp to zero to avoid sqrt of negative number.
        __m128 oneMValue = _mm_sub_ps( Vector::One, m_x );
        __m128 clampOneMValue = _mm_max_ps( Vector::Zero, oneMValue );
        __m128 root = _mm_sqrt_ps( clampOneMValue );  // sqrt(1-|vec|)

        // Compute polynomial approximation
        const auto AEC = SIMD::g_arcEstCoefficients;
        auto vConstants = _mm_shuffle_ps( AEC, AEC, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        __m128 t0 = _mm_mul_ps( vConstants, m_x );

        vConstants = _mm_shuffle_ps( AEC, AEC, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( AEC, AEC, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( AEC, AEC, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, root );

        __m128 t1 = _mm_sub_ps( Vector::Pi, t0 );
        t0 = _mm_and_ps( nonnegative, t0 );
        t1 = _mm_andnot_ps( nonnegative, t1 );
        t0 = _mm_or_ps( t0, t1 );
        t0 = _mm_sub_ps( Vector::PiDivTwo, t0 );
        return t0;
    }

    EE_FORCE_INLINE Vector Vector::ACosEst( Vector const& vec )
    {
        __m128 nonnegative = _mm_cmpge_ps( vec, Vector::Zero );
        __m128 mvalue = _mm_sub_ps( Vector::Zero, vec );
        __m128 m_x = _mm_max_ps( vec, mvalue );  // |vec|

        // Compute (1-|vec|), clamp to zero to avoid sqrt of negative number.
        __m128 oneMValue = _mm_sub_ps( Vector::One, m_x );
        __m128 clampOneMValue = _mm_max_ps( Vector::Zero, oneMValue );
        __m128 root = _mm_sqrt_ps( clampOneMValue );  // sqrt(1-|vec|)

        // Compute polynomial approximation
        auto vConstants = _mm_shuffle_ps( SIMD::g_arcEstCoefficients, SIMD::g_arcEstCoefficients, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        __m128 t0 = _mm_mul_ps( vConstants, m_x );

        vConstants = _mm_shuffle_ps( SIMD::g_arcEstCoefficients, SIMD::g_arcEstCoefficients, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( SIMD::g_arcEstCoefficients, SIMD::g_arcEstCoefficients, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, m_x );

        vConstants = _mm_shuffle_ps( SIMD::g_arcEstCoefficients, SIMD::g_arcEstCoefficients, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        t0 = _mm_add_ps( t0, vConstants );
        t0 = _mm_mul_ps( t0, root );

        __m128 t1 = _mm_sub_ps( Vector::Pi, t0 );
        t0 = _mm_and_ps( nonnegative, t0 );
        t1 = _mm_andnot_ps( nonnegative, t1 );
        t0 = _mm_or_ps( t0, t1 );
        return t0;
    }

    EE_FORCE_INLINE Vector Vector::ATanEst( Vector const& vec )
    {
        __m128 absV = vec.GetAbs();
        __m128 invV = _mm_div_ps( Vector::One, vec );
        __m128 comp = _mm_cmpgt_ps( vec, Vector::One );
        __m128 select0 = _mm_and_ps( comp, Vector::One );
        __m128 select1 = _mm_andnot_ps( comp, Vector::NegativeOne );
        __m128 sign = _mm_or_ps( select0, select1 );
        comp = _mm_cmple_ps( absV, Vector::One );
        select0 = _mm_and_ps( comp, Vector::Zero );
        select1 = _mm_andnot_ps( comp, sign );
        sign = _mm_or_ps( select0, select1 );
        select0 = _mm_and_ps( comp, vec );
        select1 = _mm_andnot_ps( comp, invV );
        __m128 m_x = _mm_or_ps( select0, select1 );

        __m128 x2 = _mm_mul_ps( m_x, m_x );

        // Compute polynomial approximation
        Vector const AEC = SIMD::g_aTanEstCoefficients1;
        Vector vConstants = _mm_shuffle_ps( AEC, AEC, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        __m128 Result = _mm_mul_ps( vConstants, x2 );

        vConstants = _mm_shuffle_ps( AEC, AEC, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( AEC, AEC, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        vConstants = _mm_shuffle_ps( AEC, AEC, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        Result = _mm_add_ps( Result, vConstants );
        Result = _mm_mul_ps( Result, x2 );

        // ATanEstCoefficients0 is already splatted
        Result = _mm_add_ps( Result, SIMD::g_aTanEstCoefficients0 );
        Result = _mm_mul_ps( Result, m_x );
        __m128 result1 = _mm_mul_ps( sign, Vector::PiDivTwo );
        result1 = _mm_sub_ps( result1, Result );

        comp = _mm_cmpeq_ps( sign, Vector::Zero );
        select0 = _mm_and_ps( comp, Result );
        select1 = _mm_andnot_ps( comp, result1 );
        Result = _mm_or_ps( select0, select1 );
        return Result;
    }

    EE_FORCE_INLINE Vector Vector::ATan2Est( Vector const& X, Vector const& Y )
    {
        Vector ATanResultValid = Vector( SIMD::g_trueMask );

        Vector vPi = Vector( SIMD::g_aTan2Constants ).GetSplatX();
        Vector vPiOverTwo = Vector( SIMD::g_aTan2Constants ).GetSplatY();
        Vector vPiOverFour = Vector( SIMD::g_aTan2Constants ).GetSplatZ();
        Vector vThreePiOverFour = Vector( SIMD::g_aTan2Constants ).GetSplatW();

        Vector YEqualsZero = Y.EqualsZero();
        Vector XEqualsZero = X.EqualsZero();
        Vector XIsPositive = _mm_and_ps( X, SIMD::g_signMask );
        XIsPositive = _mm_castsi128_ps( _mm_cmpeq_epi32( _mm_castps_si128( XIsPositive ), _mm_castps_si128( Vector::Zero ) ) );
        Vector YEqualsInfinity = Y.EqualsInfinity();
        Vector XEqualsInfinity = X.EqualsInfinity();

        Vector YSign = _mm_and_ps( Y, SIMD::g_signMask );
        vPi = _mm_castsi128_ps( _mm_or_si128( _mm_castps_si128( vPi ), _mm_castps_si128( YSign ) ) );
        vPiOverTwo = _mm_castsi128_ps( _mm_or_si128( _mm_castps_si128( vPiOverTwo ), _mm_castps_si128( YSign ) ) );
        vPiOverFour = _mm_castsi128_ps( _mm_or_si128( _mm_castps_si128( vPiOverFour ), _mm_castps_si128( YSign ) ) );
        vThreePiOverFour = _mm_castsi128_ps( _mm_or_si128( _mm_castps_si128( vThreePiOverFour ), _mm_castps_si128( YSign ) ) );

        Vector R1 = Vector::Select( vPi, YSign, XIsPositive );
        Vector R2 = Vector::Select( ATanResultValid, vPiOverTwo, XEqualsZero );
        Vector R3 = Vector::Select( R2, R1, YEqualsZero );
        Vector R4 = Vector::Select( vThreePiOverFour, vPiOverFour, XIsPositive );
        Vector R5 = Vector::Select( vPiOverTwo, R4, XEqualsInfinity );
        Vector Result = Vector::Select( R3, R5, YEqualsInfinity );
        ATanResultValid = _mm_castsi128_ps( _mm_cmpeq_epi32( _mm_castps_si128( Result ), _mm_castps_si128( ATanResultValid ) ) );

        Vector Reciprocal = X.GetInverseEst();
        Vector V = Y * Reciprocal;
        Vector R0 = Vector::ATanEst( V );

        R1 = Vector::Select( vPi, Vector( SIMD::g_signMask ), XIsPositive );
        R2 = R0 + R1;
        Result = Vector::Select( Result, R2, ATanResultValid );

        return Result;
    }

    EE_FORCE_INLINE Vector Vector::AngleMod2Pi( Vector const& angles )
    {
        // Modulo the range of the given angles such that -Pi <= Angles < Pi
        Vector result = _mm_mul_ps( angles, Vector::OneDivTwoPi );
        result.Round();
        result = _mm_mul_ps( result, Vector::TwoPi );
        result = _mm_sub_ps( angles, result );
        return result;
    }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE Vector Vector::Lerp( Vector const& from, Vector const& to, float t )
    {
        EE_ASSERT( t >= 0.0f && t <= 1.0f );

        Vector result;
        auto L = _mm_sub_ps( to, from );
        auto S = _mm_set_ps1( t );
        auto Result = _mm_mul_ps( L, S );
        result = _mm_add_ps( Result, from );
        return result;
    }
    
    EE_FORCE_INLINE Vector Vector::NLerp( Vector const& from, Vector const& to, float t )
    {
        EE_ASSERT( t >= 0.0f && t <= 1.0f );

        // Calculate the final length
        auto const fromLength = from.Length3();
        auto const toLength = to.Length3();
        auto const finalLength = Vector::Lerp( fromLength, toLength, t );

        // Normalize vectors
        Vector const normalizedFrom = from / fromLength;
        Vector const normalizedTo = to / toLength;

        // LERP
        auto const finalDirection = Lerp( normalizedFrom, normalizedTo, t );
        auto result = finalDirection.GetNormalized3() * finalLength;
        return result;
    }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE void Vector::ToDirectionAndLength2( Vector& direction, float& length ) const
    {
        Vector const vLength = Length2();
        direction = Vector::Select( *this, Vector::Zero, Select0011 );
        direction /= vLength;
        length = vLength.ToFloat();
    }

    EE_FORCE_INLINE void Vector::ToDirectionAndLength3( Vector& direction, float& length ) const
    {
        Vector const vLength = Length3();
        direction = Vector::Select( *this, Vector::Zero, Select0001 );
        direction /= vLength;
        length = vLength.ToFloat();
    }
}