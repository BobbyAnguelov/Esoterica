#pragma once
#include "ResourceID.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceRequesterID
    {
    public:

        static constexpr uint64_t s_manualRequestID = 0;
        static constexpr uint64_t s_toolsRequestID = 0xFFFFFFFFFFFFFFFF;

    public:

        // No ID - manual request
        ResourceRequesterID() = default;

        // Install dependency reference
        ResourceRequesterID( ResourceID const& resourceID )
            : m_ID( resourceID.GetResourcePath().GetID() )
            , m_isInstallDependency( true )
        {}

        // Explicit ID - generally refers to an entity or tools ID
        explicit ResourceRequesterID( uint64_t ID )
            : m_ID( ID )
        {
            EE_ASSERT( ID > 0 );
        }

        //-------------------------------------------------------------------------

        // This ID refers to a manual request outside of the default resource loading flow (usually only used for resources like maps)
        inline bool IsManualRequest() const
        {
            return m_ID == s_manualRequestID;
        }

        // This ID refers to a request originating from the tools, this allows for the tools to reload resources that they are editing
        inline bool IsToolsRequest() const
        {
            return m_ID == s_toolsRequestID;
        }

        // A normal request via the entity system
        inline bool IsNormalRequest() const
        {
            return m_ID > 0 && !m_isInstallDependency;
        }

        // A install dependency request, coming from the resource system as part of resource loading
        inline bool IsInstallDependencyRequest() const
        {
            return m_isInstallDependency;
        }

        //-------------------------------------------------------------------------

        // Get the requester ID
        inline uint64_t GetID() const { return m_ID; }

        // Get the ID for the data path for install dependencies, used for reverse look ups
        inline uint32_t GetInstallDependencyResourcePathID() const
        {
            EE_ASSERT( m_isInstallDependency );
            return (uint32_t) m_ID;
        }

        //-------------------------------------------------------------------------

        inline bool operator==( ResourceRequesterID const& rhs ) const { return m_ID == rhs.m_ID; }
        inline bool operator!=( ResourceRequesterID const& rhs ) const { return m_ID != rhs.m_ID; }

    private:

        uint64_t    m_ID = 0;
        bool        m_isInstallDependency = false;
    };
}