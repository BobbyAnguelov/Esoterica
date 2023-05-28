#pragma once

#include "System/Math/Math.h"
#include "System/Math/Quaternion.h"

//-------------------------------------------------------------------------

namespace EE::Quantization
{
    //-------------------------------------------------------------------------
    // Normalized float quantization
    //-------------------------------------------------------------------------
    // Supports variable output bits (0-16)

    template<unsigned N>
    inline uint16_t EncodeUnsignedNormalizedFloat( float value )
    {
        static_assert( N > 0, "Invalid number of bits specified" );
        static_assert( N <= 16, "Encoding into more than 16bits is not allowed" );

        EE_ASSERT( value >= 0 && value <= 1.0f );

        float const quantizedValue = value * ( ( 1 << ( N ) ) - 1 ) + 0.5f;
        return uint16_t( quantizedValue );
    }

    template<unsigned N>
    inline float DecodeUnsignedNormalizedFloat( uint16_t encodedValue )
    {
        static_assert( N > 0, "Invalid number of bits specified" );
        static_assert( N <= 16, "Decoding from more than 16bits is not allowed" );
        return encodedValue / float( ( 1 << ( N ) ) - 1 );
    }

    template<unsigned N>
    inline uint16_t EncodeSignedNormalizedFloat( float value )
    {
        static_assert( N > 0, "Invalid number of bits specified" );
        static_assert( N <= 16, "Encoding into more than 16bits is not allowed" );

        EE_ASSERT( value >= -1.0f && value <= 1.0f );

        // Track the sign and create a unsigned normalized float
        uint16_t sign = 0;
        if ( value < 0 )
        {
            sign = 1;
            value = -value;
        }

        return sign | ( EncodeUnsignedNormalizedFloat<N - 1>( value ) << 1 );
    }

    template<unsigned N>
    inline float DecodeSignedNormalizedFloat( uint16_t encodedValue )
    {
        static_assert( N > 0, "Invalid number of bits specified" );
        static_assert( N <= 16, "Decoding from more than 16bits is not allowed" );

        float const sign = float( encodedValue & 1 ? -1 : 1 );
        return DecodeUnsignedNormalizedFloat<N - 1>( encodedValue >> 1 ) * sign;
    }

    //-------------------------------------------------------------------------
    // Float quantization
    //-------------------------------------------------------------------------
    // 32 bit float to 16bit uint

    inline uint16_t EncodeFloat( float value, float const quantizationRangeStartValue, float const quantizationRangeLength )
    {
        EE_ASSERT( quantizationRangeLength != 0 );

        float const normalizedValue = ( value - quantizationRangeStartValue ) / quantizationRangeLength;
        uint16_t const encodedValue = EncodeUnsignedNormalizedFloat<16>( normalizedValue );
        return encodedValue;
    }

    inline float DecodeFloat( uint16_t encodedValue, float const quantizationRangeStartValue, float const quantizationRangeLength )
    {
        EE_ASSERT( quantizationRangeLength != 0 );

        float const normalizedValue = DecodeUnsignedNormalizedFloat<16>( encodedValue );
        float const decodedValue = ( normalizedValue * quantizationRangeLength ) + quantizationRangeStartValue;
        return decodedValue;
    }

    //-------------------------------------------------------------------------
    // Quaternion Encoding
    //-------------------------------------------------------------------------
    // Encode a quaternion into 48 bits -> 2 bits for largest component index, 3x15bit component values, 1 wasted bit
    // Storage:
    // m_data0 => [Largest Component High Bit] [15 bit for component 0]
    // m_data1 => [Largest Component Low Bit ] [15 bit for component 1]
    // m_data2 => [Unused Bit                ] [15 bit for component 2]

    struct EncodedQuaternion
    {
        EE_SERIALIZE( m_data0, m_data2 );

    private:

        static constexpr float const s_valueRangeMin = -Math::OneDivSqrtTwo;
        static constexpr float const s_valueRangeMax = Math::OneDivSqrtTwo;
        static constexpr float const s_valueRangeLength = s_valueRangeMax - s_valueRangeMin;

    public:

        EncodedQuaternion() = default;

        EncodedQuaternion( uint16_t const& a, uint16_t const& b, uint16_t const& c )
            : m_data0( a )
            , m_data1( b )
            , m_data2( c )
        {}

        EncodedQuaternion( Quaternion const& value )
        {
            EE_ASSERT( value.IsNormalized() );

            Float4 const floatValues = value.ToFloat4();

            // X
            uint16_t largestValueIndex = 0;
            float maxAbsValue = Math::Abs( floatValues.m_x );
            float signMultiplier = ( floatValues.m_x < 0 ) ? -1.0f : 1.0f;

            // Y
            float absValue = Math::Abs( floatValues.m_y );
            if ( absValue > maxAbsValue )
            {
                largestValueIndex = 1;
                maxAbsValue = absValue;
                signMultiplier = ( floatValues.m_y < 0 ) ? -1.0f : 1.0f;
            }

            // Z
            absValue = Math::Abs( floatValues.m_z );
            if ( absValue > maxAbsValue )
            {
                largestValueIndex = 2;
                maxAbsValue = absValue;
                signMultiplier = ( floatValues.m_z < 0 ) ? -1.0f : 1.0f;
            }

            // W
            absValue = Math::Abs( floatValues.m_w );
            if ( absValue > maxAbsValue )
            {
                largestValueIndex = 3;
                maxAbsValue = absValue;
                signMultiplier = ( floatValues.m_w < 0 ) ? -1.0f : 1.0f;
            }

            //-------------------------------------------------------------------------

            static constexpr float const rangeMultiplier15Bit = float( 0x7FFF ) / s_valueRangeLength;

            uint16_t a = 0, b = 0, c = 0;

            if ( largestValueIndex == 0 )
            {
                a = (uint16_t) Math::RoundToInt( ( ( floatValues.m_y * signMultiplier ) - s_valueRangeMin ) * rangeMultiplier15Bit );
                b = (uint16_t) Math::RoundToInt( ( ( floatValues.m_z * signMultiplier ) - s_valueRangeMin ) * rangeMultiplier15Bit );
                c = (uint16_t) Math::RoundToInt( ( ( floatValues.m_w * signMultiplier ) - s_valueRangeMin ) * rangeMultiplier15Bit );
            }
            else if ( largestValueIndex == 1 )
            {
                a = (uint16_t) Math::RoundToInt( ( ( floatValues.m_x * signMultiplier ) - s_valueRangeMin ) * rangeMultiplier15Bit );
                b = (uint16_t) Math::RoundToInt( ( ( floatValues.m_z * signMultiplier ) - s_valueRangeMin ) * rangeMultiplier15Bit );
                c = (uint16_t) Math::RoundToInt( ( ( floatValues.m_w * signMultiplier ) - s_valueRangeMin ) * rangeMultiplier15Bit );

                m_data1 = 0x8000; // 1 << 16
            }
            else if ( largestValueIndex == 2 )
            {
                a = (uint16_t) Math::RoundToInt( ( ( floatValues.m_x * signMultiplier ) - s_valueRangeMin ) * rangeMultiplier15Bit );
                b = (uint16_t) Math::RoundToInt( ( ( floatValues.m_y * signMultiplier ) - s_valueRangeMin ) * rangeMultiplier15Bit );
                c = (uint16_t) Math::RoundToInt( ( ( floatValues.m_w * signMultiplier ) - s_valueRangeMin ) * rangeMultiplier15Bit );

                m_data0 = 0x8000; // 1 << 16
            }
            else if ( largestValueIndex == 3 )
            {
                a = (uint16_t) Math::RoundToInt( ( ( floatValues.m_x * signMultiplier ) - s_valueRangeMin ) * rangeMultiplier15Bit );
                b = (uint16_t) Math::RoundToInt( ( ( floatValues.m_y * signMultiplier ) - s_valueRangeMin ) * rangeMultiplier15Bit );
                c = (uint16_t) Math::RoundToInt( ( ( floatValues.m_z * signMultiplier ) - s_valueRangeMin ) * rangeMultiplier15Bit );

                m_data0 = 0x8000; // 1 << 16
                m_data1 = 0x8000; // 1 << 16
            }
            else
            {
                EE_HALT();
            }

            //-------------------------------------------------------------------------

            m_data0 = m_data0 | a;
            m_data1 = m_data1 | b;
            m_data2 = c;
        }

        inline Quaternion ToQuaternion() const
        {
            uint16_t const largestValueIndex = ( m_data0 >> 14 & 0x0002 ) | m_data1 >> 15;
            uint16_t const da = m_data0 & 0x7FFF;
            uint16_t const db = m_data1 & 0x7FFF;
            uint16_t const dc = m_data2;

            static constexpr float const rangeMultiplier15Bit = s_valueRangeLength / float( 0x7FFF );
            float const a = ( da * rangeMultiplier15Bit ) + s_valueRangeMin;
            float const b = ( db * rangeMultiplier15Bit ) + s_valueRangeMin;
            float const c = ( dc * rangeMultiplier15Bit ) + s_valueRangeMin;

            float const sum = ( a * a + b * b + c * c );
            EE_ASSERT( sum <= 1.0f );
            float const d = Math::Sqrt( 1.0f - sum );

            if ( largestValueIndex == 0 )
            {
                return Quaternion( d, a, b, c );
            }
            else if ( largestValueIndex == 1 )
            {
                return Quaternion( a, d, b, c );
            }
            else if ( largestValueIndex == 2 )
            {
                return Quaternion( a, b, d, c );
            }
            else if ( largestValueIndex == 3 )
            {
                return Quaternion( a, b, c, d );
            }
            else
            {
                EE_HALT();
                return Quaternion();
            }
        }

        inline uint16_t GetData0() const { return m_data0; }
        inline uint16_t GetData1() const { return m_data1; }
        inline uint16_t GetData2() const { return m_data2; }

    public:

        uint16_t m_data0 = 0;
        uint16_t m_data1 = 0;
        uint16_t m_data2 = 0;
    };
}