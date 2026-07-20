#pragma once
#include "Base/Encoding/Embed.h"

//-------------------------------------------------------------------------

namespace EE::Embed
{
    // LZAV Compressed File: 'fonts/roboto-regular.ttf' (146004 bytes)
    struct Font_Roboto_Regular
    {
        // The size for the source uncompressed file
        constexpr static uint32_t const s_uncompressedSize = 146004;

        // The size for the target buffer when decoding the base85 data
        constexpr static uint32_t const s_decodedSize = 98984;

        // The compressed data encoded in base85
        static char const s_data[123731];

        // Decompress and get the file data
        EE_FORCE_INLINE static Blob GetFileData() { return DecompressEmbeddedFile( s_data, s_decodedSize, s_uncompressedSize ); }
    };
}