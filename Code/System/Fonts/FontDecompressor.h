#pragma once

#include "../_Module/API.h"
#include "System/Types/Arrays.h"
#include "System/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Fonts
    {
        EE_SYSTEM_API void GetDecompressedFontData( uint8_t const* pSourceData, Blob& fontData );
    }
}