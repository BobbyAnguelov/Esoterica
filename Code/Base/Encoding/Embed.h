#pragma once
#include "Base/Types/Containers_ForwardDecl.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::Embed
{
    EE_BASE_API Blob DecompressEmbeddedFile( char const* pDataBase85, uint32_t decodedSize, uint32_t uncompressedSize );
}