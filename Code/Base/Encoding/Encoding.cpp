#include "Encoding.h"
#include <locale>
#include "Base/Types/Arrays.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

namespace EE::Encoding::Base64
{
    static uint8_t const g_charTable[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    //-------------------------------------------------------------------------

    Blob Encode( uint8_t const* pDataToEncode, size_t dataSize )
    {
        EE_ASSERT( pDataToEncode != nullptr && dataSize > 0 );

        Blob encodedData;
        int32_t i = 0, j = 0;
        uint8_t byte3[3];
        uint8_t byte4[4];

        while ( dataSize-- )
        {
            byte3[i++] = *( pDataToEncode++ );
            if ( i == 3 )
            {
                byte4[0] = static_cast<uint8_t>( ( byte3[0] & 0xfc ) >> 2 );
                byte4[1] = static_cast<uint8_t>( ( ( byte3[0] & 0x03 ) << 4 ) + ( ( byte3[1] & 0xf0 ) >> 4 ) );
                byte4[2] = static_cast<uint8_t>( ( ( byte3[1] & 0x0f ) << 2 ) + ( ( byte3[2] & 0xc0 ) >> 6 ) );
                byte4[3] = static_cast<uint8_t>( byte3[2] & 0x3f );

                for ( i = 0; ( i < 4 ); i++ )
                {
                    encodedData.emplace_back( g_charTable[byte4[i]] );
                }
                i = 0;
            }
        }

        if ( i )
        {
            for ( j = i; j < 3; j++ )
            {
                byte3[j] = '\0';
            }

            byte4[0] = ( byte3[0] & 0xfc ) >> 2;
            byte4[1] = ( ( byte3[0] & 0x03 ) << 4 ) + ( ( byte3[1] & 0xf0 ) >> 4 );
            byte4[2] = ( ( byte3[1] & 0x0f ) << 2 ) + ( ( byte3[2] & 0xc0 ) >> 6 );
            byte4[3] = byte3[2] & 0x3f;

            for ( j = 0; ( j < i + 1 ); j++ )
            {
                encodedData.emplace_back( g_charTable[byte4[j]] );
            }

            while ( ( i++ < 3 ) )
            {
                encodedData.emplace_back( '=' );
            }
        }

        return encodedData;
    }

    static bool IsBase64( uint8_t c )
    {
        return ( std::isalnum( c ) || ( c == '+' ) || ( c == '/' ) );
    }

    static uint8_t FindCharIndex( uint8_t c )
    {
        for ( uint8_t i = 0; i < 65; i++ )
        {
            if ( g_charTable[i] == c )
            {
                return i;
            }
        }

        EE_UNREACHABLE_CODE();
        return (uint8_t) -1;
    }

    Blob Decode( uint8_t const* pDataToDecode, size_t dataSize )
    {
        EE_ASSERT( pDataToDecode != nullptr && dataSize > 0 );
        Blob decodedData;

        int32_t idx = 0;
        size_t i = 0, j = 0;
        uint8_t byte4[4], byte3[3];

        while ( dataSize-- && ( pDataToDecode[idx] != '=' ) && IsBase64( pDataToDecode[idx] ) )
        {
            byte4[i++] = pDataToDecode[idx]; idx++;
            if ( i == 4 )
            {
                for ( i = 0; i < 4; i++ )
                {
                    byte4[i] = FindCharIndex( byte4[i] );
                }

                byte3[0] = ( byte4[0] << 2 ) + ( ( byte4[1] & 0x30 ) >> 4 );
                byte3[1] = ( ( byte4[1] & 0xf ) << 4 ) + ( ( byte4[2] & 0x3c ) >> 2 );
                byte3[2] = ( ( byte4[2] & 0x3 ) << 6 ) + byte4[3];

                for ( i = 0; ( i < 3 ); i++ )
                {
                    decodedData.emplace_back( byte3[i] );
                }

                i = 0;
            }
        }

        //-------------------------------------------------------------------------

        if ( i )
        {
            for ( j = i; j < 4; j++ )
            {
                byte4[j] = 0;
            }

            for ( j = 0; j < 4; j++ )
            {
                byte4[j] = FindCharIndex( byte4[j] );
            }

            byte3[0] = ( byte4[0] << 2 ) + ( ( byte4[1] & 0x30 ) >> 4 );
            byte3[1] = ( ( byte4[1] & 0xf ) << 4 ) + ( ( byte4[2] & 0x3c ) >> 2 );
            byte3[2] = ( ( byte4[2] & 0x3 ) << 6 ) + byte4[3];

            for ( j = 0; ( j < i - 1 ); j++ )
            {
                decodedData.emplace_back( byte3[j] );
            }
        }

        return decodedData;
    }
}

//-------------------------------------------------------------------------

namespace EE::Encoding::Base85
{
    Blob Encode( uint8_t const* pDataToEncode, size_t dataSize )
    {
        EE_ASSERT( pDataToEncode != nullptr && dataSize > 0 );
        Blob encodedData;
        EE_UNIMPLEMENTED_FUNCTION();
        return encodedData;
    }

    Blob Decode( uint8_t const* pDataToDecode, size_t outDataSize )
    {
        EE_ASSERT( pDataToDecode != nullptr && outDataSize > 0 );

        Blob decodedData;
        decodedData.resize( outDataSize );
        Decode( pDataToDecode, decodedData.data() );
        return decodedData;
    }

    void Decode( uint8_t const* pDataToDecode, uint8_t* pOutData )
    {
        uint8_t const* pSrc = pDataToDecode;
        uint8_t* pDst = pOutData;

        while ( *pSrc )
        {
            int32_t tmp = DecodeByte( pSrc[0] ) + 85 * ( DecodeByte( pSrc[1] ) + 85 * ( DecodeByte( pSrc[2] ) + 85 * ( DecodeByte( pSrc[3] ) + 85 * DecodeByte( pSrc[4] ) ) ) );
            pDst[0] = ( ( tmp >> 0 ) & 0xFF );
            pDst[1] = ( ( tmp >> 8 ) & 0xFF );
            pDst[2] = ( ( tmp >> 16 ) & 0xFF );
            pDst[3] = ( ( tmp >> 24 ) & 0xFF );
            pSrc += 5;
            pDst += 4;
        }
    }
}

//-------------------------------------------------------------------------

namespace EE::Encoding::FourCC
{
    uint32_t Encode( char const* pStr )
    {
        if ( pStr == nullptr )
        {
            return 0;
        }

        size_t const length = strlen( pStr );
        if ( length > 4 )
        {
            return 0;
        }

        uint32_t code = 0;
        for ( size_t i = 0; i < length; i++ )
        {
            code |= ( uint32_t( pStr[i] ) << ( length - 1 - i ) * 8 );
        }

        return code;
    }

    TInlineString<5> Decode( uint32_t code )
    {
        TInlineString<5> str;

        //-------------------------------------------------------------------------

        if ( code == 0 )
        {
            return str;
        }

        //-------------------------------------------------------------------------

        char values[4];
        values[0] = (char) ( code >> 24 );
        values[1] = (char) ( ( code & 0x00FF0000 ) >> 16 );
        values[2] = (char) ( ( code & 0x0000FF00 ) >> 8 );
        values[3] = (char) ( code & 0x000000FF );

        //-------------------------------------------------------------------------

        bool validValidDetected = false;
        for ( int32_t i = 0; i < 4; i++ )
        {
            if ( values[i] != 0 )
            {
                str.push_back( values[i] );
                validValidDetected = true;
            }
            else // Zero value detected
            {
                // We do not allow zero values between non-zero values
                if ( validValidDetected )
                {
                    str.clear();
                    break;
                }
            }
        }

        return str;
    }
}

//-------------------------------------------------------------------------

namespace EE::Encoding::EightCC
{
    uint64_t Encode( char const* pStr )
    {
        if ( pStr == nullptr )
        {
            return 0;
        }

        size_t const length = strlen( pStr );
        if ( length > 8 )
        {
            return 0;
        }

        uint64_t code = 0;
        for ( size_t i = 0; i < length; i++ )
        {
            code |= ( uint64_t( pStr[i] ) << ( length - 1 - i ) * 8 );
        }

        return code;
    }

    TInlineString<9> Decode( uint64_t code )
    {
        TInlineString<9> str;

        //-------------------------------------------------------------------------

        if ( code == 0 )
        {
            return str;
        }

        //-------------------------------------------------------------------------

        char values[8];
        values[0] = (char) ( code >> 56 );
        values[1] = (char) ( ( code & 0x00FF000000000000 ) >> 48 );
        values[2] = (char) ( ( code & 0x0000FF0000000000 ) >> 40 );
        values[3] = (char) ( ( code & 0x000000FF00000000 ) >> 32 );
        values[4] = (char) ( ( code & 0x00000000FF000000 ) >> 24 );
        values[5] = (char) ( ( code & 0x0000000000FF0000 ) >> 16 );
        values[6] = (char) ( ( code & 0x000000000000FF00 ) >> 8 );
        values[7] = (char) ( ( code & 0x00000000000000FF ) );

        //-------------------------------------------------------------------------

        bool validValidDetected = false;
        for ( int32_t i = 0; i < 8; i++ )
        {
            if ( values[i] != 0 )
            {
                str.push_back( values[i] );
                validValidDetected = true;
            }
            else // Zero value detected
            {
                // We do not allow zero values between non-zero values
                if ( validValidDetected )
                {
                    str.clear();
                    break;
                }
            }
        }

        return str;
    }
}
