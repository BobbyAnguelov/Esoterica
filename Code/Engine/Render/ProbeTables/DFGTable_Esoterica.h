#pragma once
#include "Base/Encoding/Embed.h"

//-------------------------------------------------------------------------

namespace EE::Embed
{
    // LZAV Compressed File: 'D:\Work\Plastic\Esoterica\External\FilterFitter\DFGTable_esoterica.bin' (65600 bytes)
    struct DFGTable_Esoterica
    {
        // The size for the source uncompressed file
        constexpr static uint32_t const s_uncompressedSize = 65600;

        // The size for the target buffer when decoding the base85 data
        constexpr static uint32_t const s_decodedSize = 65388;

        // The compressed data encoded in base85
        static char const s_data[81736];

        // Decompress and get the file data
        EE_FORCE_INLINE static Blob GetFileData() { return DecompressEmbeddedFile( s_data, s_decodedSize, s_uncompressedSize ); }
    };
}