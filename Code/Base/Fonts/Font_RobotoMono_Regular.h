#pragma once
#include "Base/Encoding/Embed.h"

//-------------------------------------------------------------------------

namespace EE::Embed
{
    // LZAV Compressed File: 'fonts/robotomono-regular.ttf' (87540 bytes)
    struct EE_BASE_API Font_RobotoMono_Regular
    {
        // The size for the source uncompressed file
        constexpr static uint32_t const s_uncompressedSize = 87540;

        // The size for the target buffer when decoding the base85 data
        constexpr static uint32_t const s_decodedSize = 61676;

        // The compressed data encoded in base85
        static char const s_data[77096];

        // Decompress and get the file data
        EE_FORCE_INLINE static Blob GetFileData() { return DecompressEmbeddedFile( s_data, s_decodedSize, s_uncompressedSize ); }
    };
}