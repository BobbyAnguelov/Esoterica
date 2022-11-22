#include "Hash.h"

#define XXH_INLINE_ALL
#include "System/ThirdParty/xxhash/xxhash.h"

//-------------------------------------------------------------------------

namespace EE::Hash
{
    namespace XXHash
    {
        constexpr static uint32_t const g_hashSeed = 'EE8';
    }

    uint32_t XXHash::GetHash32( void const* pData, size_t size )
    {
        return XXH32( pData, size, g_hashSeed );
    }

    uint64_t XXHash::GetHash64( void const* pData, size_t size )
    {
        return XXH64( pData, size, g_hashSeed );
    }
}