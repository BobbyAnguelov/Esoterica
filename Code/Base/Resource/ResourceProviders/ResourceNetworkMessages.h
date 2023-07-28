#pragma once
#include "Base/Resource/ResourceID.h"
#include "Base/Serialization/BinarySerialization.h"

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
            EE_SERIALIZE( m_resourceIDs );

            TVector<ResourceID>     m_resourceIDs;
        };

        //-------------------------------------------------------------------------

        struct NetworkResourceResponse
        {
            struct Result
            {
                EE_SERIALIZE( m_resourceID, m_filePath );

                Result() = default;

                Result( ResourceID const& ID, String const& path )
                    : m_resourceID( ID )
                    , m_filePath( path )
                {}

                ResourceID              m_resourceID;
                String                  m_filePath;
            };

        public:

            EE_SERIALIZE( m_results );

            TVector<Result>     m_results;
        };
    }
}