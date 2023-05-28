#pragma once

#include "System/Encoding/Quantization.h"
#include "EASTL/bitvector.h"

//-------------------------------------------------------------------------

namespace EE::Serialization
{
    template<size_t N>
    class BitArchive
    {
    public:

        BitArchive()
            : m_isReading( false )
        {}

        BitArchive( Blob const& inData )
            : m_isReading( true )
        {
            size_t const numBytesToCopy = inData.size();
            EE_ASSERT( numBytesToCopy < N / 8 );
            memcpy( m_bits.data(), inData.data(), numBytesToCopy );
        }

        inline bool IsReading() const { return m_isReading; }
        inline bool IsWriting() const { return !m_isReading; }

        // Write
        //-------------------------------------------------------------------------

        // Writes a 1-bit bool value
        void WriteBool( bool value );

        // Writes a unsigned int value out, using the specified number of bits
        // Use this for enum value though clients are expected to do any conversions themselves
        void WriteUInt( uint32_t value, uint32_t maxBitsToUse );

        // Writes a 16bit float - expected to be in the range [0:1]
        void WriteNormalizedFloat( float value );

        // Writes a 16bit float - requires you to provide a quantization range with the min/max values
        void WriteFloat( float value, float minPossibleValue, float maxPossibleValue );

        // Get the final data for the serialization operation
        void GetWrittenData( Blob& outData );

        // Read
        //-------------------------------------------------------------------------

        // Read back a 1-bit bool value
        bool ReadBool();

        // Reads back an unsigned int stored in the specified number of bits
        // Use this for enum value though clients are expected to do any conversions themselves
        uint32_t ReadUInt( uint32_t maxBitsToUse );

        // Read back a 16bit float - expected to be in the range [0:1]
        float ReadNormalizedFloat();

        // Read back a 16bit float - requires you to provide a quantization range with the min/max values
        float ReadFloat( float minPossibleValue, float maxPossibleValue );

    private:

        eastl::bitset<N, uint8_t>       m_bits;
        uint32_t                        m_bitPos = 0; // where are we currently in the bit set (from right to left!)
        bool                            m_isReading = false;
    };

    //-------------------------------------------------------------------------

    template<size_t N>
    void BitArchive<N>::WriteBool( bool value )
    {
        EE_ASSERT( !m_isReading );
        EE_ASSERT( ( m_bitPos + 1 ) < N );

        if ( value )
        {
            m_bits.set( m_bitPos );
        }
        m_bitPos++;
    }

    template<size_t N>
    void BitArchive<N>::WriteUInt( uint32_t value, uint32_t maxBitsToUse )
    {
        EE_ASSERT( !m_isReading );
        EE_ASSERT( ( Math::GetMostSignificantBit( value ) + 1 ) <= maxBitsToUse );
        EE_ASSERT( ( m_bitPos + maxBitsToUse ) < N );

        for ( uint32_t i = 0u; i < maxBitsToUse; i++ )
        {
            uint32_t const valueMask = 1 << i;
            if ( valueMask & value )
            {
                m_bits.set( m_bitPos );
            }
            m_bitPos++;
        }
    }

    template<size_t N>
    void BitArchive<N>::WriteNormalizedFloat( float value )
    {
        EE_ASSERT( !m_isReading );
        uint16_t const encodedFloat = Quantization::EncodeUnsignedNormalizedFloat<16>( value );
        WriteUInt( encodedFloat, 16 );
    }

    template<size_t N>
    void BitArchive<N>::WriteFloat( float value, float minPossibleValue, float maxPossibleValue )
    {
        EE_ASSERT( !m_isReading );
        EE_ASSERT( maxPossibleValue > minPossibleValue );
        uint16_t const encodedFloat = Quantization::EncodeFloat( value, minPossibleValue, maxPossibleValue - minPossibleValue );
        WriteUInt( encodedFloat, 16 );
    }

    template<size_t N>
    void BitArchive<N>::GetWrittenData( Blob& outData )
    {
        EE_ASSERT( !m_isReading );

        size_t const numBytesToCopy = Math::CeilingToInt( m_bitPos / 8.0f );
        outData.resize( numBytesToCopy );
        memcpy( outData.data(), m_bits.data(), numBytesToCopy );
    }

    //-------------------------------------------------------------------------

    template<size_t N>
    bool BitArchive<N>::ReadBool()
    {
        EE_ASSERT( m_isReading );

        bool value = m_bits[m_bitPos];
        m_bitPos++;

        return value;
    }

    template<size_t N>
    uint32_t BitArchive<N>::ReadUInt( uint32_t maxBitsToUse )
    {
        EE_ASSERT( m_isReading );
        uint32_t value = 0;

        for ( uint32_t i = 0u; i < maxBitsToUse; i++ )
        {
            if ( m_bits[m_bitPos] )
            {
                value |= ( 1 << i );
            }

            m_bitPos++;
        }

        return value;
    }

    template<size_t N>
    float BitArchive<N>::ReadNormalizedFloat()
    {
        EE_ASSERT( m_isReading );
        uint16_t const encodedFloat = (uint16_t) ReadUInt( 16 );
        return Quantization::DecodeUnsignedNormalizedFloat<16>( encodedFloat );
    }

    template<size_t N>
    float BitArchive<N>::ReadFloat( float minPossibleValue, float maxPossibleValue )
    {
        EE_ASSERT( m_isReading );
        EE_ASSERT( maxPossibleValue > minPossibleValue );
        uint16_t const encodedFloat = (uint16_t) ReadUInt( 16 );
        return Quantization::DecodeFloat( encodedFloat, minPossibleValue, maxPossibleValue - minPossibleValue );
    }
}