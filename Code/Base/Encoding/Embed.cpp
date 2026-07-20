#include "Embed.h"
#include "Encoding.h"
#include "Base/Memory/Memory.h"
#include "Base/ThirdParty/lzav/lzav_estoerica.h"

//-------------------------------------------------------------------------

namespace EE::Embed
{
    Blob DecompressEmbeddedFile( char const* pDataBase85, uint32_t decodedSize, uint32_t uncompressedSize )
    {
        // Decode data
        EE_ASSERT( decodedSize < ( 2 * 1024 * 1024 ) ); // Restrict for now
        uint8_t* pDecodedData = EE_STACK_ARRAY_ALLOC( uint8_t, decodedSize );
        Encoding::Base85::Decode( (uint8_t const*) pDataBase85, pDecodedData );

        // Decompress data
        Blob embeddedFileData;
        embeddedFileData.resize( uncompressedSize );
        if ( lzav::lzav_decompress( pDecodedData, embeddedFileData.data(), decodedSize, uncompressedSize ) < 0 )
        {
            EE_HALT();
        }

        return embeddedFileData;
    }
}