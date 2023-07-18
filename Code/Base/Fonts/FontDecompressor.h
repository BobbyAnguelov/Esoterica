#pragma once

#include "../_Module/API.h"
#include "Base/Types/Arrays.h"
#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Fonts
    {
        EE_BASE_API void GetDecompressedFontData( uint8_t const* pSourceData, Blob& fontData );
    }
}