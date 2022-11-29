#include "Encoding.h"
#include <locale>
#include "System/Types/Arrays.h"

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

    Blob Decode( uint8_t const* pDataToDecode, size_t dataSize )
    {
        EE_ASSERT( pDataToDecode != nullptr && dataSize > 0 );
        Blob decodedData;
        EE_UNIMPLEMENTED_FUNCTION();
        return decodedData;
    }
}