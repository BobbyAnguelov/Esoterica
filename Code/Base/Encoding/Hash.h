#pragma once

#include "Base/_Module/API.h"
#include "Base/Types/String.h"
#include "Base/Types/Arrays.h"

//-----------------------------------------------------------------------------

namespace EE::Hash
{
    // XXHash
    //-------------------------------------------------------------------------
    // This is the default hashing algorithm for the engine

    namespace XXHash
    {
        EE_BASE_API uint32_t GetHash32( void const* pData, size_t size );

        EE_FORCE_INLINE uint32_t GetHash32( String const& string )
        {
            return GetHash32( string.c_str(), string.length() );
        }

        EE_FORCE_INLINE uint32_t GetHash32( char const* pString )
        {
            return GetHash32( pString, strlen( pString ) );
        }

        EE_FORCE_INLINE uint32_t GetHash32( Blob const& data )
        {
            return GetHash32( data.data(), data.size() );
        }

        //-------------------------------------------------------------------------

        EE_BASE_API uint64_t GetHash64( void const* pData, size_t size );

        EE_FORCE_INLINE uint64_t GetHash64( String const& string )
        {
            return GetHash64( string.c_str(), string.length() );
        }

        EE_FORCE_INLINE uint64_t GetHash64( char const* pString )
        {
            return GetHash64( pString, strlen( pString ) );
        }

        EE_FORCE_INLINE uint64_t GetHash64( Blob const& data )
        {
            return GetHash64( data.data(), data.size() );
        }
    }

    // FNV1a
    //-------------------------------------------------------------------------
    // This is a const expression hash
    // Should not be used for anything other than code only features i.e. custom RTTI etc...

    namespace FNV1a
    {
        constexpr uint32_t const g_constValue32 = 0x811c9dc5;
        constexpr uint32_t const g_defaultOffsetBasis32 = 0x1000193;
        constexpr uint64_t const g_constValue64 = 0xcbf29ce484222325;
        constexpr uint64_t const g_defaultOffsetBasis64 = 0x100000001b3;

        constexpr static inline uint32_t GetHash32( char const* const str, const uint32_t val = g_constValue32 )
        {
            return ( str[0] == '\0' ) ? val : GetHash32( &str[1], ( (uint64_t) val ^ uint32_t( str[0] ) ) * g_defaultOffsetBasis32 );
        }

        constexpr static inline uint64_t GetHash64( char const* const str, const uint64_t val = g_constValue64 )
        {
            return ( str[0] == '\0' ) ? val : GetHash64( &str[1], ( (uint64_t) val ^ uint64_t( str[0] ) ) * g_defaultOffsetBasis64 );
        }
    }

    // Default EE hashing functions
    //-------------------------------------------------------------------------

    EE_FORCE_INLINE uint32_t GetHash32( String const& string )
    {
        return XXHash::GetHash32( string.c_str(), string.length() );
    }

    template<size_t S>
    EE_FORCE_INLINE uint32_t GetHash32( TInlineString<S> const& string )
    {
        return XXHash::GetHash32( string.c_str(), string.length() );
    }

    EE_FORCE_INLINE uint32_t GetHash32( char const* pString )
    {
        return XXHash::GetHash32( pString, strlen( pString ) );
    }

    EE_FORCE_INLINE uint32_t GetHash32( void const* pPtr, size_t size )
    {
        return XXHash::GetHash32( pPtr, size );
    }

    EE_FORCE_INLINE uint32_t GetHash32( Blob const& data )
    {
        return XXHash::GetHash32( data.data(), data.size() );
    }

    EE_FORCE_INLINE uint64_t GetHash64( String const& string )
    {
        return XXHash::GetHash64( string.c_str(), string.length() );
    }

    template<size_t S>
    EE_FORCE_INLINE uint64_t GetHash64( TInlineString<S> const& string )
    {
        return XXHash::GetHash64( string.c_str(), string.length() );
    }

    EE_FORCE_INLINE uint64_t GetHash64( char const* pString )
    {
        return XXHash::GetHash64( pString, strlen( pString ) );
    }

    EE_FORCE_INLINE uint64_t GetHash64( void const* pPtr, size_t size )
    {
        return XXHash::GetHash64( pPtr, size );
    }

    EE_FORCE_INLINE uint64_t GetHash64( Blob const& data )
    {
        return XXHash::GetHash64( data.data(), data.size() );
    }
}