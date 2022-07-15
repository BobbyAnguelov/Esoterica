#pragma once

#include "ResourceID.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Resource
    {
        // Describes the contents of a resource, every resource has a header
        struct ResourceHeader
        {
            EE_SERIALIZE( m_version, m_resourceType, m_installDependencies );

        public:

            ResourceHeader()
                : m_version( -1 )
            {}

            ResourceHeader( int32_t version, ResourceTypeID type )
                : m_version( version )
                , m_resourceType( type )
            {}

            ResourceTypeID GetResourceTypeID() const { return m_resourceType; }
            void AddInstallDependency( ResourceID resourceID ) { m_installDependencies.push_back( resourceID ); }

        public:

            int32_t                     m_version;
            ResourceTypeID          m_resourceType;
            TVector<ResourceID>     m_installDependencies;
        };
    }
}