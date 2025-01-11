#pragma once

#include "ResourceID.h"
#include "Base/FileSystem/FileSystemPath.h"

//-------------------------------------------------------------------------
// Base for all EE resources
//-------------------------------------------------------------------------
// Note: Derived resources are resources which dont have explicit resource descriptors or are generated as a side-effect of another resource's compilation
// e.g., We can generate the navmesh for a map as part of compiling the map
// e.g., Anim graph variation resources are generated from the same source file as the anim graph definition

namespace EE::Resource
{
    class EE_BASE_API IResource
    {
        friend class ResourceLoader;

    public:

        constexpr static char const *const s_additionalDataFileExtension = "bin";

        // Get the path for the additional data file for a given resource
        EE_FORCE_INLINE static FileSystem::Path GetAdditionalDataFilePath( FileSystem::Path const& resourcePath )
        {
            FileSystem::Path additionalDataFilePath = resourcePath.GetWithAppendedExtension( s_additionalDataFileExtension );
            return additionalDataFilePath;
        }

        static ResourceTypeID GetStaticResourceTypeID() { return ResourceTypeID(); }

    public:

        IResource( IResource const& ) = default;
        virtual ~IResource() = default;

        virtual bool IsValid() const = 0;

        // Get the type for this resource
        virtual ResourceTypeID const& GetResourceTypeID() const = 0;

        // Get the ID for the resource
        inline ResourceID const& GetResourceID() const { return m_resourceID; }

        // Get the path for this resource (from the ID)
        inline DataPath const& GetDataPath() const { return m_resourceID.GetDataPath(); }

        // Does this resource require an additional set of binary data
        virtual bool RequiresAdditionalDataFile() const { return false; }

        IResource& operator=( IResource const& ) = default;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Get the friendly name for this resource type
        virtual char const* GetFriendlyName() const = 0;

        // Get the recorded hash of all compile dependencies (sources)
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
#define EE_RESOURCE( typeFourCC, friendlyName, version, requiresAdditionalDataFile ) \
    public: \
        constexpr static int32_t const s_version = version;\
        constexpr static bool const s_requiresAdditionalDataFile = requiresAdditionalDataFile;\
        static ResourceTypeID const& GetStaticResourceTypeID() { static ResourceTypeID const typeID( typeFourCC ); return typeID; } \
        virtual ResourceTypeID const& GetResourceTypeID() const override { static ResourceTypeID const typeID( typeFourCC ); return typeID; } \
        virtual bool RequiresAdditionalDataFile() const override { return requiresAdditionalDataFile; }\
        EE_DEVELOPMENT_TOOLS_LINE_IN_MACRO( constexpr static char const* const s_friendlyName = #friendlyName; )\
        EE_DEVELOPMENT_TOOLS_LINE_IN_MACRO( virtual char const* GetFriendlyName() const override { return friendlyName; } )\
    private: