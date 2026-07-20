#pragma once
#include "Base/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

namespace EE::Encoding
{
    //-------------------------------------------------------------------------
    // Base 64 Encoding
    //-------------------------------------------------------------------------

    namespace Base64
    {
        EE_BASE_API Blob Encode( uint8_t const* pDataToEncode, size_t dataSize );
        EE_BASE_API Blob Decode( uint8_t const* pDataToDecode, size_t dataSize );
    }

    //-------------------------------------------------------------------------
    // Base 85 Encoding
    //-------------------------------------------------------------------------

    namespace Base85
    {
        EE_BASE_API Blob Encode( uint8_t const* pDataToEncode, size_t dataSize );
        EE_BASE_API Blob Decode( uint8_t const* pDataToDecode, size_t outDataSize );
        EE_BASE_API void Decode( uint8_t const* pDataToDecode, uint8_t* pOutData );

        EE_FORCE_INLINE char EncodeByte( unsigned int x )
        {
            x = ( x % 85 ) + 35;
            return (char) ( ( x >= '\\' ) ? x + 1 : x );
        }

        EE_FORCE_INLINE uint32_t DecodeByte( uint8_t c ) { return c >= '\\' ? c - 36 : c - 35; }
    }

    //-------------------------------------------------------------------------
    // Four CC
    //-------------------------------------------------------------------------

    namespace FourCC
    {
        // Const-eval generator
        template <int32_t N>
        static inline consteval uint32_t Encode_ConstEval( const char( &stringLiteral )[N] )
        {
            static_assert( N > 1, "At least one character required" );
            static_assert( N <= 5, "Only up to 4 characters are supported" );

            uint32_t result = uint32_t( stringLiteral[N - 2] );
            if constexpr ( N > 2 ) { result |= uint32_t( stringLiteral[N - 3] ) << 8ULL; }
            if constexpr ( N > 3 ) { result |= uint32_t( stringLiteral[N - 4] ) << 16ULL; }
            if constexpr ( N > 4 ) { result |= uint32_t( stringLiteral[N - 5] ) << 24ULL; }
            return result;
        }

        EE_BASE_API uint32_t Encode( char const* pStr );
        EE_BASE_API TInlineString<5> Decode( uint32_t code );
    }

    //-------------------------------------------------------------------------
    // Eight CC
    //-------------------------------------------------------------------------

    namespace EightCC
    {
        // Const-eval generator
        template <int32_t N>
        static inline consteval uint64_t Encode_ConstEval( const char( &stringLiteral )[N] )
        {
            static_assert( N > 1, "At least one character required" );
            static_assert( N <= 9, "Only up to 8 characters are supported" );

            uint64_t result = uint64_t( stringLiteral[N - 2] );
            if constexpr ( N > 2 ) { result |= uint64_t( stringLiteral[N - 3] ) << 8ULL; }
            if constexpr ( N > 3 ) { result |= uint64_t( stringLiteral[N - 4] ) << 16ULL; }
            if constexpr ( N > 4 ) { result |= uint64_t( stringLiteral[N - 5] ) << 24ULL; }
            if constexpr ( N > 5 ) { result |= uint64_t( stringLiteral[N - 6] ) << 32ULL; }
            if constexpr ( N > 6 ) { result |= uint64_t( stringLiteral[N - 7] ) << 40ULL; }
            if constexpr ( N > 7 ) { result |= uint64_t( stringLiteral[N - 8] ) << 48ULL; }
            if constexpr ( N > 8 ) { result |= uint64_t( stringLiteral[N - 9] ) << 56ULL; }
            return result;
        }

        EE_BASE_API uint64_t Encode( char const* pStr );
        EE_BASE_API TInlineString<9> Decode( uint64_t code );
    }
}