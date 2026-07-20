#pragma once
#include "Base/Esoterica.h"
#include "ResourceID.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceRequesterID
    {
    public:

        enum class Source : uint8_t
        {
            Invalid = 0,
            Entity,
            InstallDependency,
            Tool,
            Manual,
        };

        static inline ResourceRequesterID InstallDependencyRequest( ResourceID dependentResourceID ) 
        {
            EE_ASSERT( dependentResourceID.IsValid() );
            return ResourceRequesterID( dependentResourceID.GetPathID(), Source::InstallDependency );
        }

        static inline ResourceRequesterID ManualRequest()
        { 
            return ResourceRequesterID( 0, Source::Manual );
        }

        static inline ResourceRequesterID ToolRequest( uint64_t toolID )
        {
            EE_ASSERT( toolID != 0 );
            return ResourceRequesterID( toolID, Source::Tool );
        }

    public:

        // No ID - manual request
        ResourceRequesterID() = default;

        // Explicit 64bit ID - generally refers to an entity ID
        explicit ResourceRequesterID( uint64_t requesterID )
            : ResourceRequesterID( requesterID, Source::Entity )
        {}

        //-------------------------------------------------------------------------

        inline bool IsValid() const { return m_source != Source::Invalid; }

        // A normal request via the entity system
        inline bool IsEntitySystemRequest() const { return m_source == Source::Entity; }

        // A install dependency request, coming from the resource system as part of resource loading
        inline bool IsInstallDependencyRequest() const { return m_source == Source::InstallDependency; }

        // This ID refers to a request originating from the tools, this allows for the tools to reload resources that they are editing
        inline bool IsToolsRequest() const { return m_source == Source::Tool; }

        // This ID refers to a manual request outside of the default resource loading flow (usually only used for resources like maps)
        inline bool IsManualRequest() const { return m_source == Source::Manual; }

        //-------------------------------------------------------------------------

        // Get the requester ID
        inline uint64_t GetID() const { return m_ID; }

        // Get the ID for the data path for install dependencies, used for reverse look ups
        inline uint64_t GetInstallDependencyResourcePathID() const
        {
            EE_ASSERT( IsInstallDependencyRequest() );
            return m_ID;
        }

        // Get the ID for the tool that requested this, used for reverse look ups
        inline uint64_t GetToolID() const
        {
            EE_ASSERT( IsToolsRequest() );
            return m_ID;
        }

        //-------------------------------------------------------------------------

        inline bool operator==( ResourceRequesterID const& rhs ) const { return m_ID == rhs.m_ID; }
        inline bool operator!=( ResourceRequesterID const& rhs ) const { return m_ID != rhs.m_ID; }

    private:

        explicit ResourceRequesterID( uint64_t requesterID, Source source )
            : m_ID( requesterID )
            , m_source( source )
        {}

    private:

        uint64_t        m_ID = 0;
        Source          m_source = Source::Invalid;
    };
}