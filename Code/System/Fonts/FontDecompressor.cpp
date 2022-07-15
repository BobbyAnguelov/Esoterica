#include "FontDecompressor.h"

//-------------------------------------------------------------------------

namespace EE
{
    //-----------------------------------------------------------------------------
    // EMBEDDED FONT DECOMPRESSION
    //-----------------------------------------------------------------------------
    // Use the font compressor in external which uses stb_compress() to compress font into byte array.
    // Decompression from stb.h (public domain) by Sean Barrett https://github.com/nothings/stb/blob/master/stb.h
    //-----------------------------------------------------------------------------

    namespace
    {
        static uint32_t stb_decompress_length( uint8_t *input )
        {
            return ( input[8] << 24 ) + ( input[9] << 16 ) + ( input[10] << 8 ) + input[11];
        }

        static uint8_t *stb__barrier, *stb__barrier2, *stb__barrier3, *stb__barrier4;
        static uint8_t *stb__dout;
        static void stb__match( uint8_t *data, uint32_t length )
        {
            // INVERSE of memmove... write each byte before copying the next...
            EE_ASSERT( stb__dout + length <= stb__barrier );
            if ( stb__dout + length > stb__barrier ) { stb__dout += length; return; }
            if ( data < stb__barrier4 ) { stb__dout = stb__barrier + 1; return; }
            while ( length-- ) *stb__dout++ = *data++;
        }

        static void stb__lit( uint8_t *data, uint32_t length )
        {
            EE_ASSERT( stb__dout + length <= stb__barrier );
            if ( stb__dout + length > stb__barrier ) { stb__dout += length; return; }
            if ( data < stb__barrier2 ) { stb__dout = stb__barrier + 1; return; }
            memcpy( stb__dout, data, length );
            stb__dout += length;
        }

        #define stb__in2(x)   ((i[x] << 8) + i[(x)+1])
        #define stb__in3(x)   ((i[x] << 16) + stb__in2((x)+1))
        #define stb__in4(x)   ((i[x] << 24) + stb__in3((x)+1))

        static uint8_t *stb_decompress_token( uint8_t *i )
        {
            if ( *i >= 0x20 ) { // use fewer if's for cases that expand small
                if ( *i >= 0x80 )       stb__match( stb__dout - i[1] - 1, i[0] - 0x80 + 1 ), i += 2;
                else if ( *i >= 0x40 )  stb__match( stb__dout - ( stb__in2( 0 ) - 0x4000 + 1 ), i[2] + 1 ), i += 3;
                else /* *i >= 0x20 */ stb__lit( i + 1, i[0] - 0x20 + 1 ), i += 1 + ( i[0] - 0x20 + 1 );
            }
            else { // more ifs for cases that expand large, since overhead is amortized
                if ( *i >= 0x18 )       stb__match( stb__dout - ( stb__in3( 0 ) - 0x180000 + 1 ), i[3] + 1 ), i += 4;
                else if ( *i >= 0x10 )  stb__match( stb__dout - ( stb__in3( 0 ) - 0x100000 + 1 ), stb__in2( 3 ) + 1 ), i += 5;
                else if ( *i >= 0x08 )  stb__lit( i + 2, stb__in2( 0 ) - 0x0800 + 1 ), i += 2 + ( stb__in2( 0 ) - 0x0800 + 1 );
                else if ( *i == 0x07 )  stb__lit( i + 3, stb__in2( 1 ) + 1 ), i += 3 + ( stb__in2( 1 ) + 1 );
                else if ( *i == 0x06 )  stb__match( stb__dout - ( stb__in3( 1 ) + 1 ), i[4] + 1 ), i += 5;
                else if ( *i == 0x04 )  stb__match( stb__dout - ( stb__in3( 1 ) + 1 ), stb__in2( 4 ) + 1 ), i += 6;
            }
            return i;
        }

        static uint32_t stb_adler32( uint32_t adler32, uint8_t *buffer, uint32_t buflen )
        {
            const unsigned long ADLER_MOD = 65521;
            unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
            unsigned long blocklen, i;

            blocklen = buflen % 5552;
            while ( buflen ) {
                for ( i = 0; i + 7 < blocklen; i += 8 ) {
                    s1 += buffer[0], s2 += s1;
                    s1 += buffer[1], s2 += s1;
                    s1 += buffer[2], s2 += s1;
                    s1 += buffer[3], s2 += s1;
                    s1 += buffer[4], s2 += s1;
                    s1 += buffer[5], s2 += s1;
                    s1 += buffer[6], s2 += s1;
                    s1 += buffer[7], s2 += s1;

                    buffer += 8;
                }

                for ( ; i < blocklen; ++i )
                    s1 += *buffer++, s2 += s1;

                s1 %= ADLER_MOD, s2 %= ADLER_MOD;
                buflen -= blocklen;
                blocklen = 5552;
            }
            return (uint32_t) ( s2 << 16 ) + (uint32_t) s1;
        }

        static uint32_t stb_decompress( uint8_t *output, uint8_t *i, uint32_t length )
        {
            uint32_t olen;
            if ( stb__in4( 0 ) != 0x57bC0000 ) return 0;
            if ( stb__in4( 4 ) != 0 )          return 0; // error! stream is > 4GB
            olen = stb_decompress_length( i );
            stb__barrier2 = i;
            stb__barrier3 = i + length;
            stb__barrier = output + olen;
            stb__barrier4 = output;
            i += 16;

            stb__dout = output;
            for ( ;;) {
                uint8_t *old_i = i;
                i = stb_decompress_token( i );
                if ( i == old_i ) {
                    if ( *i == 0x05 && i[1] == 0xfa ) {
                        EE_ASSERT( stb__dout == output + olen );
                        if ( stb__dout != output + olen ) return 0;
                        if ( stb_adler32( 1, output, olen ) != (uint32_t) stb__in4( 2 ) )
                            return 0;
                        return olen;
                    }
                    else {
                        EE_ASSERT( 0 ); /* NOTREACHED */
                        return 0;
                    }
                }
                EE_ASSERT( stb__dout <= output + olen );
                if ( stb__dout > output + olen )
                    return 0;
            }
        }

        static uint32_t Decode85Byte( uint8_t c ) { return c >= '\\' ? c - 36 : c - 35; }

        static void DecodeFontData( uint8_t const* src, uint8_t* dst )
        {
            while ( *src )
            {
                int32_t tmp = Decode85Byte( src[0] ) + 85 * ( Decode85Byte( src[1] ) + 85 * ( Decode85Byte( src[2] ) + 85 * ( Decode85Byte( src[3] ) + 85 * Decode85Byte( src[4] ) ) ) );
                dst[0] = ( ( tmp >> 0 ) & 0xFF );
                dst[1] = ( ( tmp >> 8 ) & 0xFF );
                dst[2] = ( ( tmp >> 16 ) & 0xFF );
                dst[3] = ( ( tmp >> 24 ) & 0xFF );
                src += 5;
                dst += 4;
            }
        }
    }

    //-------------------------------------------------------------------------

    namespace Fonts
    {
        void GetDecompressedFontData( uint8_t const* pSourceData, Blob& fontData )
        {
            // Decode font data
            uint32_t const decodedDataSize = uint32_t( ( strlen( (char*) pSourceData ) + 4 ) / 5 ) * 4;
            uint8_t* pDecodedData = EE_STACK_ARRAY_ALLOC( uint8_t, decodedDataSize );
            DecodeFontData( (uint8_t const*) pSourceData, pDecodedData );

            // Decompress font data
            int32_t const decompressedDataSize = stb_decompress_length( (uint8_t*) pDecodedData );
            fontData.resize( decompressedDataSize );
            stb_decompress( fontData.data(), pDecodedData, decodedDataSize );
        }
    }
}