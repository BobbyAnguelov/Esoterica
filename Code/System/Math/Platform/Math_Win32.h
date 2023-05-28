#pragma once
#include "System/Esoterica.h"
#include <intrin.h>

namespace EE::Math
{
    EE_FORCE_INLINE uint32_t GetMostSignificantBit( uint32_t value )
    {
        unsigned long index;
        _BitScanReverse( &index, (unsigned long) value );
        return index;
    }
}