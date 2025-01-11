#pragma once

#include "Base/_Module/API.h"
#include "MathConstants.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Esoterica.h"
#include <math.h>

// Compiler Specific Math Functions
//-------------------------------------------------------------------------

#if _WIN32
#include "Platform/Math_Win32.h"
#endif

// General Math Functions
//-------------------------------------------------------------------------

namespace EE
{
    namespace Math
    {
        EE_FORCE_INLINE float Sin( float value ) { return sinf( value ); }
        EE_FORCE_INLINE float Cos( float value ) { return cosf( value ); }
        EE_FORCE_INLINE float Tan( float value ) { return tanf( value ); }

        EE_FORCE_INLINE float ASin( float value ) { return asinf( value ); }
        EE_FORCE_INLINE float ACos( float value ) { return acosf( value ); }
        EE_FORCE_INLINE float ATan( float value ) { return atanf( value ); }
        EE_FORCE_INLINE float ATan2( float y, float x ) { return atan2f( y, x ); }

        EE_FORCE_INLINE float Cosec( float value ) { return 1.0f / sinf( value ); }
        EE_FORCE_INLINE float Sec( float value ) { return 1.0f / cosf( value ); }
        EE_FORCE_INLINE float Cot( float value ) { return 1.0f / tanf( PiDivTwo - value ); }

        EE_FORCE_INLINE float Pow( float x, float y ) { return powf( x, y ); }
        EE_FORCE_INLINE float Sqr( float value ) { return value * value; }
        EE_FORCE_INLINE float Sqrt( float value ) { return sqrtf( value ); }

        // Computes Euler's number (2.7182818...) raised to the given power N ( 2.7182818...^N )
        EE_FORCE_INLINE float Exp( float N ) { return expf( N ); }

        // Computes 2 raised to the given power N ( 2^N )
        EE_FORCE_INLINE float Exp2( float N ) { return exp2f( N ); }

        // Computes the base 10 log of value
        EE_FORCE_INLINE float Log( float value ) { return logf( value ); }

        // Computes the base 2 log of value
        EE_FORCE_INLINE float Log2( float value ) { return log2f( value ); }

        EE_FORCE_INLINE float AddToMovingAverage( float currentAverage, uint64_t numCurrentSamples, float newValue )
        {
            return currentAverage + ( ( newValue - currentAverage ) / float( numCurrentSamples + 1 ) );
        }

        EE_FORCE_INLINE float Abs( float a ) { return fabsf( a ); }
        EE_FORCE_INLINE double Abs( double a ) { return fabs( a ); }
        EE_FORCE_INLINE int8_t Abs( int8_t a ) { return (int8_t) abs( a ); }
        EE_FORCE_INLINE int16_t Abs( int16_t a ) { return (int16_t) abs( a ); }
        EE_FORCE_INLINE int32_t Abs( int32_t a ) { return labs( a ); }
        EE_FORCE_INLINE int64_t Abs( int64_t a ) { return llabs( a ); }

        EE_FORCE_INLINE float Reciprocal( float r ) { return 1.0f / r; }
        EE_FORCE_INLINE double Reciprocal( double r ) { return 1.0 / r; }

        template<typename T>
        EE_FORCE_INLINE T Min( T a, T b ) { return a <= b ? a : b; }

        template<typename T>
        EE_FORCE_INLINE T Max( T a, T b ) { return a >= b ? a : b; }

        template<typename T>
        EE_FORCE_INLINE T AbsMin( T a, T b ) { return Abs( a ) <= Abs( b ) ? a : b; }

        template<typename T>
        EE_FORCE_INLINE T AbsMax( T a, T b ) { return Abs( a ) >= Abs( b ) ? a : b; }

        template<typename T>
        EE_FORCE_INLINE T Sqrt( T a ) { return sqrt( a ); }

        template<typename T>
        EE_FORCE_INLINE T Clamp( T value, T lowerBound, T upperBound )
        {
            EE_ASSERT( lowerBound <= upperBound );
            return Min( Max( value, lowerBound ), upperBound );
        }

        template<typename T>
        EE_FORCE_INLINE bool IsInRangeInclusive( T value, T lowerBound, T upperBound )
        {
            EE_ASSERT( lowerBound < upperBound );
            return value >= lowerBound && value <= upperBound;
        }

        template<typename T>
        EE_FORCE_INLINE bool IsInRangeExclusive( T value, T lowerBound, T upperBound )
        {
            EE_ASSERT( lowerBound < upperBound );
            return value > lowerBound && value < upperBound;
        }

        // Decomposes a float into integer and remainder portions, remainder is return and the integer result is stored in the integer portion
        EE_FORCE_INLINE float ModF( float value, float& integerPortion )
        {
            return modff( value, &integerPortion );
        }

        // Returns the floating point remainder of x/y
        EE_FORCE_INLINE float FModF( float x, float y )
        {
            return fmodf( x, y );
        }

        EE_FORCE_INLINE float PercentageThroughRange( float value, float lowerBound, float upperBound )
        {
            EE_ASSERT( lowerBound < upperBound );
            return Clamp( value, lowerBound, upperBound ) / ( upperBound - lowerBound );
        }

        EE_FORCE_INLINE bool IsNearEqual( float value, float comparand, float epsilon = Epsilon )
        {
            return fabsf( value - comparand ) <= epsilon;
        }

        EE_FORCE_INLINE bool IsNearZero( float value, float epsilon = Epsilon )
        {
            return fabsf( value ) <= epsilon;
        }

        EE_FORCE_INLINE bool IsNearEqual( double value, double comparand, double epsilon = Epsilon )
        {
            return fabs( value - comparand ) <= epsilon;
        }

        EE_FORCE_INLINE bool IsNearZero( double value, double epsilon = Epsilon )
        {
            return fabs( value ) <= epsilon;
        }

        EE_FORCE_INLINE float Ceiling( float value )
        {
            return ceilf( value );
        }

        EE_FORCE_INLINE int32_t CeilingToInt( float value )
        {
            return (int32_t) ceilf( value );
        }

        EE_FORCE_INLINE float Floor( float value )
        {
            return floorf( value );
        }

        EE_FORCE_INLINE int32_t FloorToInt( float value )
        {
            return (int32_t) floorf( value );
        }

        EE_FORCE_INLINE float Round( float value )
        {
            return roundf( value );
        }

        EE_FORCE_INLINE int32_t RoundToInt( float value )
        {
            return (int32_t) roundf( value );
        }

        template<typename T>
        EE_FORCE_INLINE T FloatToInt( float value )
        {
            return static_cast<T>( roundf( value ) );
        }

        template<typename T>
        EE_FORCE_INLINE T DoubleToInt( double value )
        {
            return static_cast<T>( round( value ) );
        }

        inline int32_t GreatestCommonDivisor( int32_t a, int32_t b )
        {
            return ( b == 0 ) ? Abs( a ) : GreatestCommonDivisor( b, a % b );
        }

        inline int32_t LowestCommonMultiple( int32_t a, int32_t b )
        {
            int32_t const gcd = GreatestCommonDivisor( a, b );
            int32_t const lcm = ( a / gcd ) * b;
            return lcm;
        }

        inline float RemapRange( float value, float fromRangeBegin, float fromRangeEnd, float toRangeBegin, float toRangeEnd )
        {
            float const fromRangeLength = fromRangeEnd - fromRangeBegin;
            float const percentageThroughFromRange = Math::Clamp( ( value - fromRangeBegin ) / fromRangeLength, 0.0f, 1.0f );
            float const toRangeLength = toRangeEnd - toRangeBegin;
            float const result = toRangeBegin + ( percentageThroughFromRange * toRangeLength );

            return result;
        }

        template<typename T>
        EE_FORCE_INLINE bool IsOdd( T n )
        {
            static_assert( std::numeric_limits<T>::is_integer, "Integer type required" );
            return n % 2 != 0; 
        }

        template<typename T>
        EE_FORCE_INLINE bool IsEven( T n )
        {
            static_assert( std::numeric_limits<T>::is_integer, "Integer type required." );
            return n % 2 == 0; 
        }

        template<typename T>
        EE_FORCE_INLINE T MakeOdd( T n )
        {
            static_assert( std::numeric_limits<T>::is_integer, "Integer type required." );
            return IsOdd( n ) ? n : n + 1;
        }

        template<typename T>
        EE_FORCE_INLINE T MakeEven( T n )
        {
            static_assert( std::numeric_limits<T>::is_integer, "Integer type required." );
            return IsEven( n ) ? n : n + 1;
        }

        EE_FORCE_INLINE bool IsNaN( float n )
        {
            return isnan( n );
        }

        EE_FORCE_INLINE bool IsInf( float n )
        {
            return isinf( n );
        }

        EE_FORCE_INLINE bool IsNaNOrInf( float n )
        {
            return isnan( n ) || isinf( n );
        }

        //-------------------------------------------------------------------------

        // Note: returns true for 0
        EE_FORCE_INLINE bool IsPowerOf2( int32_t x ) { return ( x & ( x - 1 ) ) == 0; }

        EE_FORCE_INLINE uint32_t GetClosestPowerOfTwo( uint32_t x )
        {
            uint32_t i = 1;
            while ( i < x ) i += i;
            if ( 4 * x < 3 * i ) i >>= 1;
            return i;
        }

        EE_FORCE_INLINE uint32_t GetUpperPowerOfTwo( uint32_t x )
        {
            uint32_t i = 1;
            while ( i < x ) i += i;
            return i;
        }

        EE_FORCE_INLINE uint32_t GetLowerPowerOfTwo( uint32_t x )
        {
            uint32_t i = 1;
            while ( i <= x ) i += i;
            return i >> 1;
        }

        EE_FORCE_INLINE uint32_t RoundUpToNearestMultiple32( uint32_t value, uint32_t multiple ) { return ( ( value + multiple - 1 ) / multiple ) * multiple; }
        EE_FORCE_INLINE uint64_t RoundUpToNearestMultiple64( uint64_t value, uint64_t multiple ) { return ( ( value + multiple - 1 ) / multiple ) * multiple; }
        EE_FORCE_INLINE uint32_t RoundDownToNearestMultiple32( uint32_t value, uint32_t multiple ) { return value - value % multiple; }
        EE_FORCE_INLINE uint64_t RoundDownToNearestMultiple64( uint64_t value, uint64_t multiple ) { return value - value % multiple; }

        EE_FORCE_INLINE uint32_t GetMaxNumberOfBitsForValue( uint64_t value ) { return Math::GetMostSignificantBit( value ) + 1; }
    }

    //-------------------------------------------------------------------------

    enum NoInit_t { NoInit };
    enum ZeroInit_t { ZeroInit };
    enum IdentityInit_t { IdentityInit };

    //-------------------------------------------------------------------------
    // Axes
    //-------------------------------------------------------------------------

    enum class Axis : uint8_t
    {
        X = 0,
        Y,
        Z,
        NegX,
        NegY,
        NegZ
    };

    EE_FORCE_INLINE Axis GetOppositeAxis( Axis axis) 
    {
        int32_t const v = (int32_t) axis;
        if ( v < 3 )
        {
            return Axis( v + 3 );
        }
        else
        {
            return Axis( v - 3 );
        }
    }

    // Returns the orthogonal axis based on the right hand rule for cross-products: Cross( axis1, axis2 )
    EE_BASE_API Axis GetOrthogonalAxis( Axis axis1, Axis axis2 );

    //-------------------------------------------------------------------------
    // Storage Types
    //-------------------------------------------------------------------------

    struct Float2;
    struct Float3;
    struct Float4;

    //-------------------------------------------------------------------------

    struct EE_BASE_API Int2
    {
        EE_SERIALIZE( m_x, m_y );

        static Int2 const Zero;

    public:

        inline Int2() {}
        inline Int2( ZeroInit_t ) : m_x( 0 ), m_y( 0 ) {}
        inline Int2( Float2 const& v );
        inline explicit Int2( int32_t v ) : m_x( v ), m_y( v ) {}
        inline explicit Int2( int32_t ix, int32_t iy ) : m_x( ix ), m_y( iy ) {}

        inline bool IsZero() const { return *this == Zero; }

        inline int32_t& operator[]( uint32_t i ) { EE_ASSERT( i < 2 ); return ( (int32_t*) this )[i]; }
        inline int32_t const& operator[]( uint32_t i ) const { EE_ASSERT( i < 2 ); return ( (int32_t*) this )[i]; }

        inline bool operator==( Int2 const rhs ) const { return m_x == rhs.m_x && m_y == rhs.m_y; }
        inline bool operator!=( Int2 const rhs ) const { return m_x != rhs.m_x || m_y != rhs.m_y; }

        inline Int2 operator+( Int2 const& rhs ) const { return Int2( m_x + rhs.m_x, m_y + rhs.m_y ); }
        inline Int2 operator-( Int2 const& rhs ) const { return Int2( m_x - rhs.m_x, m_y - rhs.m_y ); }
        inline Int2 operator*( Int2 const& rhs ) const { return Int2( m_x * rhs.m_x, m_y * rhs.m_y ); }
        inline Int2 operator/( Int2 const& rhs ) const { return Int2( m_x / rhs.m_x, m_y / rhs.m_y ); }

        inline Int2& operator+=( int32_t const& rhs ) { m_x += rhs; m_y += rhs; return *this; }
        inline Int2& operator-=( int32_t const& rhs ) { m_x -= rhs; m_y -= rhs; return *this; }
        inline Int2& operator*=( int32_t const& rhs ) { m_x *= rhs; m_y *= rhs; return *this; }
        inline Int2& operator/=( int32_t const& rhs ) { m_x /= rhs; m_y /= rhs; return *this; }

        // Component wise operation
        inline Int2 operator+( int32_t const& rhs ) const { return Int2( m_x + rhs, m_y + rhs ); }
        inline Int2 operator-( int32_t const& rhs ) const { return Int2( m_x - rhs, m_y - rhs ); }
        inline Int2 operator*( int32_t const& rhs ) const { return Int2( m_x * rhs, m_y * rhs ); }
        inline Int2 operator/( int32_t const& rhs ) const { return Int2( m_x / rhs, m_y / rhs ); }

        inline Int2& operator+=( Int2 const& rhs ) { m_x += rhs.m_x; m_y += rhs.m_y; return *this; }
        inline Int2& operator-=( Int2 const& rhs ) { m_x -= rhs.m_x; m_y -= rhs.m_y; return *this; }
        inline Int2& operator*=( Int2 const& rhs ) { m_x *= rhs.m_x; m_y *= rhs.m_y; return *this; }
        inline Int2& operator/=( Int2 const& rhs ) { m_x /= rhs.m_x; m_y /= rhs.m_y; return *this; }

    public:

        int32_t m_x, m_y;
    };

    //-------------------------------------------------------------------------

    struct EE_BASE_API Int4
    {
        EE_SERIALIZE( m_x, m_y, m_z, m_w );

        static Int4 const Zero;

    public:

        inline Int4() {}
        inline Int4( ZeroInit_t ) : m_x( 0 ), m_y( 0 ), m_z( 0 ), m_w( 0 ) {}
        inline explicit Int4( int32_t v ) : m_x( v ), m_y( v ), m_z( v ), m_w( v ) {}
        inline explicit Int4( int32_t ix, int32_t iy, int32_t iz, int32_t iw ) : m_x( ix ), m_y( iy ), m_z( iz ), m_w( iw ) {}

        inline bool IsZero() const { return *this == Zero; }

        inline int32_t& operator[]( uint32_t i ) { EE_ASSERT( i < 4 ); return ( (int32_t*) this )[i]; }
        inline int32_t const& operator[]( uint32_t i ) const { EE_ASSERT( i < 4 ); return ( (int32_t*) this )[i]; }

        inline bool operator==( Int4 const rhs ) const { return m_x == rhs.m_x && m_y == rhs.m_y && m_z == rhs.m_z && m_w == rhs.m_w; }
        inline bool operator!=( Int4 const rhs ) const { return m_x != rhs.m_x || m_y != rhs.m_y || m_z != rhs.m_z || m_w != rhs.m_w; }

        inline Int4 operator+( int32_t const& rhs ) const { return Int4( m_x + rhs, m_y + rhs, m_z + rhs, m_w + rhs ); }
        inline Int4 operator-( int32_t const& rhs ) const { return Int4( m_x - rhs, m_y - rhs, m_z - rhs, m_w - rhs ); }
        inline Int4 operator*( int32_t const& rhs ) const { return Int4( m_x * rhs, m_y * rhs, m_z * rhs, m_w * rhs ); }
        inline Int4 operator/( int32_t const& rhs ) const { return Int4( m_x / rhs, m_y / rhs, m_z / rhs, m_w / rhs ); }

        inline Int4& operator+=( int32_t const& rhs ) { m_x += rhs; m_y += rhs; m_z += rhs; m_w += rhs; return *this; }
        inline Int4& operator-=( int32_t const& rhs ) { m_x -= rhs; m_y -= rhs; m_z -= rhs; m_w -= rhs; return *this; }
        inline Int4& operator*=( int32_t const& rhs ) { m_x *= rhs; m_y *= rhs; m_z *= rhs; m_w *= rhs; return *this; }
        inline Int4& operator/=( int32_t const& rhs ) { m_x /= rhs; m_y /= rhs; m_z /= rhs; m_w /= rhs; return *this; }

        // Component wise operation
        inline Int4 operator+( Int4 const& rhs ) const { return Int4( m_x + rhs.m_x, m_y + rhs.m_y, m_z + rhs.m_z, m_w + rhs.m_w ); }
        inline Int4 operator-( Int4 const& rhs ) const { return Int4( m_x - rhs.m_x, m_y - rhs.m_y, m_z - rhs.m_z, m_w - rhs.m_w ); }
        inline Int4 operator*( Int4 const& rhs ) const { return Int4( m_x * rhs.m_x, m_y * rhs.m_y, m_z * rhs.m_z, m_w * rhs.m_w ); }
        inline Int4 operator/( Int4 const& rhs ) const { return Int4( m_x / rhs.m_x, m_y / rhs.m_y, m_z / rhs.m_z, m_w / rhs.m_w ); }

        inline Int4& operator+=( Int4 const& rhs ) { m_x += rhs.m_x; m_y += rhs.m_y; m_z += rhs.m_z; m_w += rhs.m_w; return *this; }
        inline Int4& operator-=( Int4 const& rhs ) { m_x -= rhs.m_x; m_y -= rhs.m_y; m_z -= rhs.m_z; m_w -= rhs.m_w; return *this; }
        inline Int4& operator*=( Int4 const& rhs ) { m_x *= rhs.m_x; m_y *= rhs.m_y; m_z *= rhs.m_z; m_w *= rhs.m_w; return *this; }
        inline Int4& operator/=( Int4 const& rhs ) { m_x /= rhs.m_x; m_y /= rhs.m_y; m_z /= rhs.m_z; m_w /= rhs.m_w; return *this; }

    public:

        int32_t m_x, m_y, m_z, m_w;
    };

    //-------------------------------------------------------------------------

    struct EE_BASE_API Float2
    {
        EE_SERIALIZE( m_x, m_y );

        static Float2 const Zero;
        static Float2 const One;
        static Float2 const UnitX;
        static Float2 const UnitY;

    public:

        inline Float2() {}
        EE_FORCE_INLINE Float2( ZeroInit_t ) : m_x( 0 ), m_y( 0 ) {}
        EE_FORCE_INLINE explicit Float2( float v ) : m_x( v ), m_y( v ) {}
        EE_FORCE_INLINE explicit Float2( float ix, float iy ) : m_x( ix ), m_y( iy ) {}
        EE_FORCE_INLINE explicit Float2( int32_t ix, int32_t iy ) : m_x( (float) ix ), m_y( (float) iy ) {}
        inline explicit Float2( Int2 const& v ) : m_x( (float) v.m_x ), m_y( (float) v.m_y ) {}
        inline explicit Float2( Float3 const& v );
        inline explicit Float2( Float4 const& v );

        inline bool IsZero() const { return *this == Zero; }
        inline bool IsNearZero() const { return Math::IsNearZero( m_x ) && Math::IsNearZero( m_y ); }

        inline float& operator[]( uint32_t i ) { EE_ASSERT( i < 2 ); return ( (float*) this )[i]; }
        inline float const& operator[]( uint32_t i ) const { EE_ASSERT( i < 2 ); return ( (float*) this )[i]; }

        EE_FORCE_INLINE Float2 operator-() const { return Float2( -m_x, -m_y ); }

        inline bool operator==( Float2 const rhs ) const { return m_x == rhs.m_x && m_y == rhs.m_y; }
        inline bool operator!=( Float2 const rhs ) const { return m_x != rhs.m_x || m_y != rhs.m_y; }

        inline Float2 operator+( Float2 const& rhs ) const { return Float2( m_x + rhs.m_x, m_y + rhs.m_y ); }
        inline Float2 operator-( Float2 const& rhs ) const { return Float2( m_x - rhs.m_x, m_y - rhs.m_y ); }
        inline Float2 operator*( Float2 const& rhs ) const { return Float2( m_x * rhs.m_x, m_y * rhs.m_y ); }
        inline Float2 operator/( Float2 const& rhs ) const { return Float2( m_x / rhs.m_x, m_y / rhs.m_y ); }

        inline Float2 operator+( float const& rhs ) const { return Float2( m_x + rhs, m_y + rhs ); }
        inline Float2 operator-( float const& rhs ) const { return Float2( m_x - rhs, m_y - rhs ); }
        inline Float2 operator*( float const& rhs ) const { return Float2( m_x * rhs, m_y * rhs ); }
        inline Float2 operator/( float const& rhs ) const { return Float2( m_x / rhs, m_y / rhs ); }

        inline Float2& operator+=( Float2 const& rhs ) { m_x += rhs.m_x; m_y += rhs.m_y; return *this; }
        inline Float2& operator-=( Float2 const& rhs ) { m_x -= rhs.m_x; m_y -= rhs.m_y; return *this; }
        inline Float2& operator*=( Float2 const& rhs ) { m_x *= rhs.m_x; m_y *= rhs.m_y; return *this; }
        inline Float2& operator/=( Float2 const& rhs ) { m_x /= rhs.m_x; m_y /= rhs.m_y; return *this; }

        inline Float2& operator+=( float const& rhs ) { m_x += rhs; m_y += rhs; return *this; }
        inline Float2& operator-=( float const& rhs ) { m_x -= rhs; m_y -= rhs; return *this; }
        inline Float2& operator*=( float const& rhs ) { m_x *= rhs; m_y *= rhs; return *this; }
        inline Float2& operator/=( float const& rhs ) { m_x /= rhs; m_y /= rhs; return *this; }

        float m_x, m_y;
    };

    //-------------------------------------------------------------------------

    struct EE_BASE_API Float3
    {
        EE_SERIALIZE( m_x, m_y, m_z );

        static Float3 const Zero;
        static Float3 const One;
        static Float3 const UnitX;
        static Float3 const UnitY;
        static Float3 const UnitZ;

        static Float3 const WorldForward;
        static Float3 const WorldUp;
        static Float3 const WorldRight;

    public:

        inline Float3() {}
        EE_FORCE_INLINE Float3( ZeroInit_t ) : m_x( 0 ), m_y( 0 ), m_z( 0 ) {}
        EE_FORCE_INLINE explicit Float3( float v ) : m_x( v ), m_y( v ), m_z( v ) {}
        EE_FORCE_INLINE explicit Float3( float ix, float iy, float iz ) : m_x( ix ), m_y( iy ), m_z( iz ) {}
        inline explicit Float3( Float2 const& v, float iz = 0.0f ) : m_x( v.m_x ), m_y( v.m_y ), m_z( iz ) {}
        inline explicit Float3( Float4 const& v );

        inline bool IsZero() const { return *this == Zero; }
        inline bool IsNearZero() const { return Math::IsNearZero( m_x ) && Math::IsNearZero( m_y ) && Math::IsNearZero( m_z ); }

        inline float& operator[]( uint32_t i ) { EE_ASSERT( i < 3 ); return ( (float*) this )[i]; }
        inline float const& operator[]( uint32_t i ) const { EE_ASSERT( i < 3 ); return ( (float*) this )[i]; }

        EE_FORCE_INLINE Float3 operator-() const { return Float3( -m_x, -m_y, -m_z ); }

        inline bool operator==( Float3 const rhs ) const { return m_x == rhs.m_x && m_y == rhs.m_y && m_z == rhs.m_z; }
        inline bool operator!=( Float3 const rhs ) const { return m_x != rhs.m_x || m_y != rhs.m_y || m_z != rhs.m_z; }

        inline operator Float2() const { return Float2( m_x, m_y ); }

        inline Float3 operator+( Float3 const& rhs ) const { return Float3( m_x + rhs.m_x, m_y + rhs.m_y, m_z + rhs.m_z ); }
        inline Float3 operator-( Float3 const& rhs ) const { return Float3( m_x - rhs.m_x, m_y - rhs.m_y, m_z - rhs.m_z ); }
        inline Float3 operator*( Float3 const& rhs ) const { return Float3( m_x * rhs.m_x, m_y * rhs.m_y, m_z * rhs.m_z ); }
        inline Float3 operator/( Float3 const& rhs ) const { return Float3( m_x / rhs.m_x, m_y / rhs.m_y, m_z / rhs.m_z ); }

        inline Float3 operator+( float const& rhs ) const { return Float3( m_x + rhs, m_y + rhs, m_z + rhs ); }
        inline Float3 operator-( float const& rhs ) const { return Float3( m_x - rhs, m_y - rhs, m_z - rhs ); }
        inline Float3 operator*( float const& rhs ) const { return Float3( m_x * rhs, m_y * rhs, m_z * rhs ); }
        inline Float3 operator/( float const& rhs ) const { return Float3( m_x / rhs, m_y / rhs, m_z / rhs ); }

        inline Float3& operator+=( Float3 const& rhs ) { m_x += rhs.m_x; m_y += rhs.m_y; m_z += rhs.m_z; return *this; }
        inline Float3& operator-=( Float3 const& rhs ) { m_x -= rhs.m_x; m_y -= rhs.m_y; m_z -= rhs.m_z; return *this; }
        inline Float3& operator*=( Float3 const& rhs ) { m_x *= rhs.m_x; m_y *= rhs.m_y; m_z *= rhs.m_z; return *this; }
        inline Float3& operator/=( Float3 const& rhs ) { m_x /= rhs.m_x; m_y /= rhs.m_y; m_z /= rhs.m_z; return *this; }

        inline Float3& operator+=( float const& rhs ) { m_x += rhs; m_y += rhs; m_z += rhs; return *this; }
        inline Float3& operator-=( float const& rhs ) { m_x -= rhs; m_y -= rhs; m_z -= rhs; return *this; }
        inline Float3& operator*=( float const& rhs ) { m_x *= rhs; m_y *= rhs; m_z *= rhs; return *this; }
        inline Float3& operator/=( float const& rhs ) { m_x /= rhs; m_y /= rhs; m_z /= rhs; return *this; }

        float m_x, m_y, m_z;
    };

    //-------------------------------------------------------------------------

    struct EE_BASE_API Float4
    {
        EE_SERIALIZE( m_x, m_y, m_z, m_w );

        static Float4 const Zero;
        static Float4 const One;
        static Float4 const UnitX;
        static Float4 const UnitY;
        static Float4 const UnitZ;
        static Float4 const UnitW;

        static Float4 const WorldForward;
        static Float4 const WorldUp;
        static Float4 const WorldRight;

    public:

        Float4() {}
        EE_FORCE_INLINE Float4( ZeroInit_t ) : m_x( 0 ), m_y( 0 ), m_z( 0 ), m_w( 0 ) {}
        EE_FORCE_INLINE explicit Float4( float v ) : m_x( v ), m_y( v ), m_z( v ), m_w( v ) {}
        EE_FORCE_INLINE explicit Float4( float ix, float iy, float iz, float iw ) : m_x( ix ), m_y( iy ), m_z( iz ), m_w( iw ) {}
        explicit Float4( Float2 const& v, float iz = 0.0f, float iw = 0.0f ) : m_x( v.m_x ), m_y( v.m_y ), m_z( iz ), m_w( iw ) {}
        explicit Float4( Float3 const& v, float iw = 0.0f ) : m_x( v.m_x ), m_y( v.m_y ), m_z( v.m_z ), m_w( iw ) {}

        inline bool IsZero() const { return *this == Zero; }
        inline bool IsNearZero() const { return Math::IsNearZero( m_x ) && Math::IsNearZero( m_y ) && Math::IsNearZero( m_z ) && Math::IsNearZero( m_w ); }

        float& operator[]( uint32_t i ) { EE_ASSERT( i < 4 ); return ( (float*) this )[i]; }
        float const& operator[]( uint32_t i ) const { EE_ASSERT( i < 4 ); return ( (float*) this )[i]; }

        EE_FORCE_INLINE Float4 operator-() const { return Float4( -m_x, -m_y, -m_z, -m_w ); }

        bool operator==( Float4 const rhs ) const { return m_x == rhs.m_x && m_y == rhs.m_y && m_z == rhs.m_z && m_w == rhs.m_w; }
        bool operator!=( Float4 const rhs ) const { return m_x != rhs.m_x || m_y != rhs.m_y || m_z != rhs.m_z || m_w != rhs.m_w; }

        inline operator Float2() const { return Float2( m_x, m_y ); }
        inline operator Float3() const { return Float3( m_x, m_y, m_z ); }

        inline Float4 operator+( Float4 const& rhs ) const { return Float4( m_x + rhs.m_x, m_y + rhs.m_y, m_z + rhs.m_z, m_w + rhs.m_w ); }
        inline Float4 operator-( Float4 const& rhs ) const { return Float4( m_x - rhs.m_x, m_y - rhs.m_y, m_z - rhs.m_z, m_w - rhs.m_w ); }
        inline Float4 operator*( Float4 const& rhs ) const { return Float4( m_x * rhs.m_x, m_y * rhs.m_y, m_z * rhs.m_z, m_w * rhs.m_w ); }
        inline Float4 operator/( Float4 const& rhs ) const { return Float4( m_x / rhs.m_x, m_y / rhs.m_y, m_z / rhs.m_z, m_w / rhs.m_w ); }

        inline Float4 operator+( float const& rhs ) const { return Float4( m_x + rhs, m_y + rhs, m_z + rhs, m_w + rhs ); }
        inline Float4 operator-( float const& rhs ) const { return Float4( m_x - rhs, m_y - rhs, m_z - rhs, m_w - rhs ); }
        inline Float4 operator*( float const& rhs ) const { return Float4( m_x * rhs, m_y * rhs, m_z * rhs, m_w * rhs ); }
        inline Float4 operator/( float const& rhs ) const { return Float4( m_x / rhs, m_y / rhs, m_z / rhs, m_w / rhs ); }

        inline Float4& operator+=( Float4 const& rhs ) { m_x += rhs.m_x; m_y += rhs.m_y; m_z += rhs.m_z; m_w += rhs.m_w; return *this; }
        inline Float4& operator-=( Float4 const& rhs ) { m_x -= rhs.m_x; m_y -= rhs.m_y; m_z -= rhs.m_z; m_w -= rhs.m_w; return *this; }
        inline Float4& operator*=( Float4 const& rhs ) { m_x *= rhs.m_x; m_y *= rhs.m_y; m_z *= rhs.m_z; m_w *= rhs.m_w; return *this; }
        inline Float4& operator/=( Float4 const& rhs ) { m_x /= rhs.m_x; m_y /= rhs.m_y; m_z /= rhs.m_z; m_w /= rhs.m_w; return *this; }

        inline Float4& operator+=( float const& rhs ) { m_x += rhs; m_y += rhs; m_z += rhs; m_w += rhs; return *this; }
        inline Float4& operator-=( float const& rhs ) { m_x -= rhs; m_y -= rhs; m_z -= rhs; m_w -= rhs; return *this; }
        inline Float4& operator*=( float const& rhs ) { m_x *= rhs; m_y *= rhs; m_z *= rhs; m_w *= rhs; return *this; }
        inline Float4& operator/=( float const& rhs ) { m_x /= rhs; m_y /= rhs; m_z /= rhs; m_w /= rhs; return *this; }

        float m_x, m_y, m_z, m_w;
    };

    // Implicit conversions
    //-------------------------------------------------------------------------

    inline Int2::Int2( Float2 const& v )
        : m_x( (int32_t) v.m_x )
        , m_y( (int32_t) v.m_y )
    {}

    inline Float2::Float2( Float3 const& v )
        : m_x( v.m_x )
        , m_y( v.m_y )
    {}

    inline Float2::Float2( Float4 const& v )
        : m_x( v.m_x )
        , m_y( v.m_y )
    {}

    inline Float3::Float3( Float4 const& v )
        : m_x( v.m_x )
        , m_y( v.m_y )
        , m_z( v.m_z )
    {}

    //-------------------------------------------------------------------------
    // Type safe angle definitions
    //-------------------------------------------------------------------------

    struct Radians;
    struct Degrees;

    //-------------------------------------------------------------------------

    struct Degrees
    {
        EE_SERIALIZE( m_value );

    public:

        inline Degrees() = default;
        inline Degrees( float degrees ) : m_value( degrees ) {}
        inline explicit Degrees( Radians const& radians );

        EE_FORCE_INLINE explicit operator float() const { return m_value; }
        EE_FORCE_INLINE operator Radians() const;
        EE_FORCE_INLINE float ToFloat() const { return m_value; }
        EE_FORCE_INLINE Radians ToRadians() const;

        inline Degrees operator-() const { return Degrees( -m_value ); }

        inline Degrees operator+( Degrees const& rhs ) const { return Degrees( m_value + rhs.m_value ); }
        inline Degrees operator-( Degrees const& rhs ) const { return Degrees( m_value - rhs.m_value ); }
        inline Degrees operator*( Degrees const& rhs ) const { return Degrees( m_value * rhs.m_value ); }
        inline Degrees operator/( Degrees const& rhs ) const { return Degrees( m_value / rhs.m_value ); }

        inline Degrees& operator+=( Degrees const& rhs ) { m_value += rhs.m_value; return *this; }
        inline Degrees& operator-=( Degrees const& rhs ) { m_value -= rhs.m_value; return *this; }
        inline Degrees& operator*=( Degrees const& rhs ) { m_value *= rhs.m_value; return *this; }
        inline Degrees& operator/=( Degrees const& rhs ) { m_value /= rhs.m_value; return *this; }

        inline Degrees operator+( float const& rhs ) const { return Degrees( m_value + rhs ); }
        inline Degrees operator-( float const& rhs ) const { return Degrees( m_value - rhs ); }
        inline Degrees operator*( float const& rhs ) const { return Degrees( m_value * rhs ); }
        inline Degrees operator/( float const& rhs ) const { return Degrees( m_value / rhs ); }

        inline Degrees& operator+=( float const& rhs ) { m_value += rhs; return *this; }
        inline Degrees& operator-=( float const& rhs ) { m_value -= rhs; return *this; }
        inline Degrees& operator*=( float const& rhs ) { m_value *= rhs; return *this; }
        inline Degrees& operator/=( float const& rhs ) { m_value /= rhs; return *this; }

        inline Degrees operator+( int32_t const& rhs ) const { return Degrees( m_value + rhs ); }
        inline Degrees operator-( int32_t const& rhs ) const { return Degrees( m_value - rhs ); }
        inline Degrees operator*( int32_t const& rhs ) const { return Degrees( m_value * rhs ); }
        inline Degrees operator/( int32_t const& rhs ) const { return Degrees( m_value / rhs ); }

        inline Degrees& operator+=( int32_t const& rhs ) { m_value += rhs; return *this; }
        inline Degrees& operator-=( int32_t const& rhs ) { m_value -= rhs; return *this; }
        inline Degrees& operator*=( int32_t const& rhs ) { m_value *= rhs; return *this; }
        inline Degrees& operator/=( int32_t const& rhs ) { m_value /= rhs; return *this; }

        inline Degrees operator+( uint32_t const& rhs ) const { return Degrees( m_value + rhs ); }
        inline Degrees operator-( uint32_t const& rhs ) const { return Degrees( m_value - rhs ); }
        inline Degrees operator*( uint32_t const& rhs ) const { return Degrees( m_value * rhs ); }
        inline Degrees operator/( uint32_t const& rhs ) const { return Degrees( m_value / rhs ); }

        inline Degrees& operator+=( uint32_t const& rhs ) { m_value += rhs; return *this; }
        inline Degrees& operator-=( uint32_t const& rhs ) { m_value -= rhs; return *this; }
        inline Degrees& operator*=( uint32_t const& rhs ) { m_value *= rhs; return *this; }
        inline Degrees& operator/=( uint32_t const& rhs ) { m_value /= rhs; return *this; }

        inline bool operator>( float const& rhs ) const { return m_value > rhs; };
        inline bool operator<( float const& rhs ) const { return m_value < rhs; }
        inline bool operator>=( float const& rhs ) const { return m_value >= rhs; }
        inline bool operator<=( float const& rhs ) const { return m_value <= rhs; }

        inline bool operator>( Degrees const& rhs ) const { return m_value > rhs.m_value; }
        inline bool operator<( Degrees const& rhs ) const { return m_value < rhs.m_value; }
        inline bool operator>=( Degrees const& rhs ) const { return m_value >= rhs.m_value; }
        inline bool operator<=( Degrees const& rhs ) const { return m_value <= rhs.m_value; }

        inline bool operator>( Radians const& rhs ) const;
        inline bool operator<( Radians const& rhs ) const;
        inline bool operator>=( Radians const& rhs ) const;
        inline bool operator<=( Radians const& rhs ) const;

        inline bool operator==( float const& v ) const { return Math::IsNearEqual( m_value, v ); }
        inline bool operator!=( float const& v ) const { return !Math::IsNearEqual( m_value, v ); }

        inline bool operator==( Degrees const& rhs ) const  { return m_value == rhs.m_value; }
        inline bool operator!=( Degrees const& rhs ) const  { return m_value != rhs.m_value; }

        inline bool operator==( Radians const& rhs ) const;
        inline bool operator!=( Radians const& rhs ) const;

        //-------------------------------------------------------------------------

        inline void Clamp( Degrees min, Degrees max )
        {
            m_value = Math::Clamp( m_value, min.m_value, max.m_value );
        }

        // Clamps between -360 and 360
        inline void Clamp360()
        {
            m_value -= ( int32_t( m_value / 360.0f ) * 360.0f );
        }

        // Clamps between -360 and 360
        inline Degrees GetClamped360() const
        {
            Degrees d( m_value );
            d.Clamp360();
            return d;
        }

        // Clamps to -180 to 180
        inline void Clamp180()
        {
            Clamp360();

            float delta = 180 - Math::Abs( m_value );
            if ( delta < 0 )
            {
                delta += 180;
                m_value = ( m_value < 0 ) ? delta : -delta;
            }
        }

        // Clamps to -180 to 180
        inline Degrees GetClamped180() const
        {
            Degrees r( m_value );
            r.Clamp180();
            return r;
        }

        // Clamps between 0 to 360
        inline Degrees& ClampPositive360()
        {
            Clamp360();
            if ( m_value < 0 )
            {
                m_value += 360;
            }
            return *this;
        }

        // Clamps between 0 to 360
        inline Degrees GetClampedPositive360() const
        {
            Degrees d( m_value );
            d.ClampPositive360();
            return d;
        }

    private:

        float m_value = 0;
    };

    //-------------------------------------------------------------------------

    struct EE_BASE_API Radians
    {
        EE_SERIALIZE( m_value );

        static Radians const Pi;
        static Radians const TwoPi;
        static Radians const OneDivPi;
        static Radians const OneDivTwoPi;
        static Radians const PiDivTwo;
        static Radians const PiDivFour;

    public:

        inline Radians() = default;
        inline Radians( float radians ) : m_value( radians ) {}
        inline explicit Radians( Degrees const& degrees );

        EE_FORCE_INLINE explicit operator float() const { return m_value; }
        EE_FORCE_INLINE operator Degrees() const { return ToDegrees(); }
        EE_FORCE_INLINE float ToFloat() const { return m_value; }
        EE_FORCE_INLINE Degrees ToDegrees() const { return Degrees( m_value * Math::RadiansToDegrees ); }

        inline Radians operator-() const { return Radians( -m_value ); }

        inline Radians operator+( Radians const& rhs ) const { return Radians( m_value + rhs.m_value ); }
        inline Radians operator-( Radians const& rhs ) const { return Radians( m_value - rhs.m_value ); }
        inline Radians operator*( Radians const& rhs ) const { return Radians( m_value * rhs.m_value ); }
        inline Radians operator/( Radians const& rhs ) const { return Radians( m_value / rhs.m_value ); }

        inline Radians& operator+=( Radians const& rhs ) { m_value += rhs.m_value; return *this; }
        inline Radians& operator-=( Radians const& rhs ) { m_value -= rhs.m_value; return *this; }
        inline Radians& operator*=( Radians const& rhs ) { m_value *= rhs.m_value; return *this; }
        inline Radians& operator/=( Radians const& rhs ) { m_value /= rhs.m_value; return *this; }

        inline Radians operator+( float const& rhs ) const { return Radians( m_value + rhs ); }
        inline Radians operator-( float const& rhs ) const { return Radians( m_value - rhs ); }
        inline Radians operator*( float const& rhs ) const { return Radians( m_value * rhs ); }
        inline Radians operator/( float const& rhs ) const { return Radians( m_value / rhs ); }

        inline Radians& operator+=( float const& rhs ) { m_value += rhs; return *this; }
        inline Radians& operator-=( float const& rhs ) { m_value -= rhs; return *this; }
        inline Radians& operator*=( float const& rhs ) { m_value *= rhs; return *this; }
        inline Radians& operator/=( float const& rhs ) { m_value /= rhs; return *this; }

        inline Radians operator+( int32_t const& rhs ) const { return Radians( m_value + rhs ); }
        inline Radians operator-( int32_t const& rhs ) const { return Radians( m_value - rhs ); }
        inline Radians operator*( int32_t const& rhs ) const { return Radians( m_value * rhs ); }
        inline Radians operator/( int32_t const& rhs ) const { return Radians( m_value / rhs ); }

        inline Radians& operator+=( int32_t const& rhs ) { m_value += rhs; return *this; }
        inline Radians& operator-=( int32_t const& rhs ) { m_value -= rhs; return *this; }
        inline Radians& operator*=( int32_t const& rhs ) { m_value *= rhs; return *this; }
        inline Radians& operator/=( int32_t const& rhs ) { m_value /= rhs; return *this; }

        inline Radians operator+( uint32_t const& rhs ) const { return Radians( m_value + rhs ); }
        inline Radians operator-( uint32_t const& rhs ) const { return Radians( m_value - rhs ); }
        inline Radians operator*( uint32_t const& rhs ) const { return Radians( m_value * rhs ); }
        inline Radians operator/( uint32_t const& rhs ) const { return Radians( m_value / rhs ); }

        inline Radians& operator+=( uint32_t const& rhs ) { m_value += rhs; return *this; }
        inline Radians& operator-=( uint32_t const& rhs ) { m_value -= rhs; return *this; }
        inline Radians& operator*=( uint32_t const& rhs ) { m_value *= rhs; return *this; }
        inline Radians& operator/=( uint32_t const& rhs ) { m_value /= rhs; return *this; }

        inline bool operator>( float const& rhs ) const { return m_value > rhs; };
        inline bool operator<( float const& rhs ) const { return m_value < rhs; }
        inline bool operator>=( float const& rhs ) const { return m_value >= rhs; }
        inline bool operator<=( float const& rhs ) const { return m_value <= rhs; }

        inline bool operator>( Radians const& rhs ) const { return m_value > rhs.m_value; }
        inline bool operator<( Radians const& rhs ) const { return m_value < rhs.m_value; }
        inline bool operator>=( Radians const& rhs ) const { return m_value >= rhs.m_value; }
        inline bool operator<=( Radians const& rhs ) const { return m_value <= rhs.m_value; }

        inline bool operator>( Degrees const& rhs ) const;
        inline bool operator<( Degrees const& rhs ) const;
        inline bool operator>=( Degrees const& rhs ) const;
        inline bool operator<=( Degrees const& rhs ) const;

        inline bool operator==( float const& v ) const { return Math::IsNearEqual( m_value, v ); }
        inline bool operator!=( float const& v ) const { return !Math::IsNearEqual( m_value, v ); }

        inline bool operator==( Radians const& rhs ) const { return m_value == rhs.m_value; }
        inline bool operator!=( Radians const& rhs ) const { return m_value != rhs.m_value; }

        inline bool operator==( Degrees const& rhs ) const;
        inline bool operator!=( Degrees const& rhs ) const;

        //-------------------------------------------------------------------------

        inline void Clamp( Radians min, Radians max )
        {
            m_value = Math::Clamp( m_value, min.m_value, max.m_value );
        }

        // Clamps between -2Pi to 2Pi
        inline void Clamp360()
        {
            m_value -= int32_t( m_value / Math::TwoPi ) * Math::TwoPi;
        }

        // Clamps between -2Pi to 2Pi
        inline Radians GetClamped360() const
        {
            Radians r( m_value );
            r.Clamp360();
            return r;
        }

        // Clamps between 0 to 2Pi
        inline void ClampPositive360()
        {
            Clamp360();
            if( m_value < 0 )
            {
                m_value += Math::TwoPi;
            }
        }

        // Clamps between 0 to 2Pi
        inline Radians GetClampedToPositive360() const
        {
            Radians r( m_value );
            r.ClampPositive360();
            return r;
        }

        // Clamps to -Pi to Pi
        inline void Clamp180()
        {
            Clamp360();

            float delta = Math::Pi - Math::Abs( m_value );
            if ( delta < 0 )
            {
                delta += Math::Pi;
                m_value = ( m_value < 0 ) ? delta : -delta;
            }
        }

        // Clamps to -Pi to Pi
        inline Radians GetClamped180() const
        {
            Radians r( m_value );
            r.Clamp180();
            return r;
        }

        // Inverts angle between [0;2Pi] and [-2Pi;0]
        inline void Invert()
        {
            Clamp360();
            float const delta = Math::TwoPi - Math::Abs( m_value );
            m_value = ( m_value < 0 ) ? delta : -delta;
        }

        // Inverts angle between [0;2Pi] and [-2Pi;0]
        inline Radians GetInverse() const
        {
            Radians r( m_value );
            r.Invert();
            return r;
        }

        // Flips the front and rear 180 degree arc i.e. 135 becomes -45, -90 becomes 90, etc.
        inline void Flip()
        {
            Clamp180();
            float const delta = Math::Pi - Math::Abs( m_value );
            m_value = ( m_value < 0 ) ? delta : -delta;
        }

        // Flips the front and rear 180 degree arc i.e. 135 becomes -45, -90 becomes 90, etc.
        inline Radians GetFlipped() const
        {
            Radians r( m_value );
            r.Flip();
            return r;
        }

    private:

        float m_value = 0;
    };

    //-------------------------------------------------------------------------

    inline Degrees::Degrees( Radians const& radians )
        : m_value( radians.ToDegrees() )
    {}

    inline Radians Degrees::ToRadians() const
    {
        return Radians( m_value * Math::DegreesToRadians );
    }

    inline Degrees::operator Radians() const
    { 
        return ToRadians(); 
    }

    inline bool Degrees::operator>( Radians const& rhs ) const { return m_value > rhs.ToDegrees().m_value; }
    inline bool Degrees::operator<( Radians const& rhs ) const { return m_value < rhs.ToDegrees().m_value; }
    inline bool Degrees::operator>=( Radians const& rhs ) const { return m_value >= rhs.ToDegrees().m_value; }
    inline bool Degrees::operator<=( Radians const& rhs ) const { return m_value <= rhs.ToDegrees().m_value; }

    inline bool Degrees::operator==( Radians const& rhs ) const { return Math::IsNearEqual( m_value, rhs.ToDegrees().m_value ); }
    inline bool Degrees::operator!=( Radians const& rhs ) const { return !Math::IsNearEqual( m_value, rhs.ToDegrees().m_value ); }

    //-------------------------------------------------------------------------

    inline Radians::Radians( Degrees const& degrees )
        : m_value( degrees.ToRadians() )
    {}

    inline bool Radians::operator>( Degrees const& rhs ) const { return m_value > rhs.ToRadians().m_value; }
    inline bool Radians::operator<( Degrees const& rhs ) const { return m_value < rhs.ToRadians().m_value; }
    inline bool Radians::operator>=( Degrees const& rhs ) const { return m_value >= rhs.ToRadians().m_value; }
    inline bool Radians::operator<=( Degrees const& rhs ) const { return m_value <= rhs.ToRadians().m_value; }

    inline bool Radians::operator==( Degrees const& rhs ) const { return Math::IsNearEqual( m_value, rhs.ToRadians().m_value ); }
    inline bool Radians::operator!=( Degrees const& rhs ) const { return !Math::IsNearEqual( m_value, rhs.ToRadians().m_value ); }

    //-------------------------------------------------------------------------
    // Euler Angles - Order of rotation in EE is XYZ
    //-------------------------------------------------------------------------

    struct EulerAngles
    {
        EE_SERIALIZE( m_x, m_y, m_z );

    public:

        EulerAngles() = default;

        inline explicit EulerAngles( Degrees inX, Degrees inY, Degrees inZ )
            : m_x( inX )
            , m_y( inY )
            , m_z( inZ )
        {}

        inline explicit EulerAngles( Radians inX, Radians inY, Radians inZ )
            : m_x( inX )
            , m_y( inY )
            , m_z( inZ )
        {}

        inline explicit EulerAngles( float inDegreesX, float inDegreesY, float inDegreesZ )
            : m_x( Math::DegreesToRadians * inDegreesX )
            , m_y( Math::DegreesToRadians * inDegreesY )
            , m_z( Math::DegreesToRadians * inDegreesZ )
        {}

        inline EulerAngles( Float3 const& anglesInDegrees )
            : m_x( Math::DegreesToRadians * anglesInDegrees.m_x )
            , m_y( Math::DegreesToRadians * anglesInDegrees.m_y )
            , m_z( Math::DegreesToRadians * anglesInDegrees.m_z )
        {}

        inline void Clamp()
        {
            m_x.Clamp360();
            m_y.Clamp360();
            m_z.Clamp360();
        }

        inline EulerAngles GetClamped() const
        { 
            EulerAngles clamped = *this;
            clamped.Clamp();
            return clamped;
        }

        inline Radians GetYaw() const { return m_z; }
        inline Radians GetPitch() const { return m_x; }
        inline Radians GetRoll() const { return m_y; }

        inline Float3 GetAsRadians() const { return Float3( m_x.ToFloat(), m_y.ToFloat(), m_z.ToFloat() ); }
        inline Float3 GetAsDegrees() const { return Float3( m_x.ToDegrees().ToFloat(), m_y.ToDegrees().ToFloat(), m_z.ToDegrees().ToFloat() ); }

        inline bool operator==( EulerAngles const& other ) const { return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z; }
        inline bool operator!=( EulerAngles const& other ) const { return m_x != other.m_x || m_y != other.m_y || m_z != other.m_z; }

        inline Radians& operator[]( uint32_t i ) { EE_ASSERT( i < 3 ); return ( (Radians*) this )[i]; }
        inline Radians const& operator[]( uint32_t i ) const { EE_ASSERT( i < 3 ); return ( (Radians*) this )[i]; }

        inline Float3 ToFloat3() const { return Float3( Math::RadiansToDegrees * m_x.ToFloat(), Math::RadiansToDegrees * m_y.ToFloat(), Math::RadiansToDegrees * m_z.ToFloat() ); }

    public:

        Radians m_x = 0.0f;
        Radians m_y = 0.0f;
        Radians m_z = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Axis Angle
    //-------------------------------------------------------------------------

    struct AxisAngle
    {
        EE_SERIALIZE( m_axis, m_angle );

    public:

        inline AxisAngle() = default;
        inline explicit AxisAngle( Float3&& axis, Radians angle ) : m_axis( axis ), m_angle( angle ) {}
        inline explicit AxisAngle( Float3&& axis, Degrees angle ) : m_axis( axis ), m_angle( angle.ToRadians() ) {}
        inline explicit AxisAngle( Float3 const& axis, Radians angle ) : m_axis( axis ), m_angle( angle ) {}
        inline explicit AxisAngle( Float3 const& axis, Degrees angle ) : m_axis( axis ), m_angle( angle.ToRadians() ) {}

        inline bool IsValid() const
        {
            float const lengthSq = m_axis.m_x * m_axis.m_x + m_axis.m_y * m_axis.m_y + m_axis.m_z * m_axis.m_z;
            return Math::Abs( lengthSq - 1.0f ) < Math::Epsilon;
        }

    public:

        Float3      m_axis = Float3::Zero;
        Radians     m_angle = Radians( 0.0f );
    };
}