#pragma once
#include "Base/Encoding/Embed.h"

//-------------------------------------------------------------------------

namespace EE::Embed
{
    // LZAV Compressed File: 'D:\Work\Plastic\Esoterica\External\FilterFitter\ReflectionProbeTable_ggx_esoterica_cube.bin' (3504 bytes)
    struct ProbeTable_GGX_Esoterica_Cube
    {
        // The size for the source uncompressed file
        constexpr static uint32_t const s_uncompressedSize = 3504;

        // The size for the target buffer when decoding the base85 data
        constexpr static uint32_t const s_decodedSize = 3056;

        // The compressed data encoded in base85
        static char const s_data[3821];

        // Decompress and get the file data
        EE_FORCE_INLINE static Blob GetFileData() { return DecompressEmbeddedFile( s_data, s_decodedSize, s_uncompressedSize ); }
    };
}