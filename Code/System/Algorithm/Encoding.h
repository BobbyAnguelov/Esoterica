#pragma once
#include "System/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::Encoding
{
    //-------------------------------------------------------------------------
    // Base 64 Encoding
    //-------------------------------------------------------------------------

    namespace Base64
    {
        EE_SYSTEM_API Blob Encode( uint8_t const* pDataToEncode, size_t dataSize );
        EE_SYSTEM_API Blob Decode( uint8_t const* pDataToDecode, size_t dataSize );
    }

    //-------------------------------------------------------------------------
    // Base 85 Encoding
    //-------------------------------------------------------------------------

    namespace Base85
    {
        EE_SYSTEM_API Blob Encode( uint8_t const* pDataToEncode, size_t dataSize );
        EE_SYSTEM_API Blob Decode( uint8_t const* pDataToDecode, size_t dataSize );
    }
}