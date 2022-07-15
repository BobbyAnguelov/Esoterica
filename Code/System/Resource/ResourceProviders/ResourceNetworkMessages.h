#pragma once
#include "System/Resource/ResourceID.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Resource
    {
        enum class NetworkMessageID
        {
            RequestResource = 1,
            ResourceRequestComplete = 2,
            ResourceUpdated = 3,
        };

        //-------------------------------------------------------------------------

        struct NetworkResourceRequest
        {
            EE_SERIALIZE( m_path );

            NetworkResourceRequest() = default;

            NetworkResourceRequest( ResourceID const& ID )
                : m_path( ID )
            {}

            ResourceID              m_path;
        };

        //-------------------------------------------------------------------------

        struct NetworkResourceResponse
        {
            EE_SERIALIZE( m_resourceID, m_filePath );

            ResourceID              m_resourceID;
            String                  m_filePath;
        };
    }
}