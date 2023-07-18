#pragma once
#include "Base/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

namespace EE::Encoding
{
    //-------------------------------------------------------------------------
    // Base 64 Encoding
    //-------------------------------------------------------------------------

    namespace Base64
    {
        EE_BASE_API Blob Encode( uint8_t const* pDataToEncode, size_t dataSize );
        EE_BASE_API Blob Decode( uint8_t const* pDataToDecode, size_t dataSize );
    }

    //-------------------------------------------------------------------------
    // Base 85 Encoding
    //-------------------------------------------------------------------------

    namespace Base85
    {
        EE_BASE_API Blob Encode( uint8_t const* pDataToEncode, size_t dataSize );
        EE_BASE_API Blob Decode( uint8_t const* pDataToDecode, size_t dataSize );
    }
}