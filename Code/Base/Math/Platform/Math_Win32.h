#pragma once
#include "Base/Esoterica.h"
#include <intrin.h>

namespace EE::Math
{
    EE_FORCE_INLINE uint32_t GetMostSignificantBit( uint64_t value )
    {
        // The intrinsic produces an undefined value if the input is 0, so we need to handle it explicitly
        if ( value == 0 )
        {
            return 0;
        }

        //-------------------------------------------------------------------------

        unsigned long index = 0;
        _BitScanReverse64( &index, (unsigned long) value );
        return index;
    }
}