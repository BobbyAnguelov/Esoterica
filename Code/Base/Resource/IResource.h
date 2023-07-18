#pragma once

#include "ResourceID.h"

//-------------------------------------------------------------------------
// Base for all EE resources
//-------------------------------------------------------------------------
// Note: Virtual resources are resources which dont have explicit resource descriptors or are generated as a side-effect of another resource's compilation
// e.g., We can generate the navmesh for a map as part of compiling the map
// e.g., We generate an anim graph dataset whenever we compile a graph variation

namespace EE::Resource
{
    class EE_BASE_API IResource
    {
        friend class ResourceLoader;

    public:

        static ResourceTypeID GetStaticResourceTypeID() { return ResourceTypeID(); }

    public:

        IResource( IResource const& ) = default;
        virtual ~IResource() = default;

        inline ResourceID const& GetResourceID() const { return m_resourceID; }
        inline ResourcePath const& GetResourcePath() const { return m_resourceID.GetResourcePath(); }

        virtual bool IsValid() const = 0;
        virtual ResourceTypeID const& GetResourceType() const = 0;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetFriendlyName() const = 0;
        uint64_t GetSourceResourceHash() const { return m_sourceResourceHash; }
        #endif

    protected:

        IResource() {}

    private:

        ResourceID      m_resourceID;

        #if EE_DEVELOPMENT_TOOLS
        uint64_t        m_sourceResourceHash = 0;
        #endif
    };
}

//-------------------------------------------------------------------------

// Define a resource
// Note: The expected fourCC can only contain lowercase letters and digits
#define EE_RESOURCE( typeFourCC, friendlyName ) \
    public: \
        static bool const IsVirtualResource = false;\
        static ResourceTypeID const& GetStaticResourceTypeID() { static ResourceTypeID const typeID( typeFourCC ); return typeID; } \
        virtual ResourceTypeID const& GetResourceType() const override { static ResourceTypeID const typeID( typeFourCC ); return typeID; } \
        EE_DEVELOPMENT_TOOLS_LINE_IN_MACRO( constexpr static char const* const s_friendlyName = #friendlyName; )\
        EE_DEVELOPMENT_TOOLS_LINE_IN_MACRO( virtual char const* GetFriendlyName() const override { return friendlyName; } )\
    private: