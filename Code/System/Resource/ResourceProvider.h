#pragma once

#include "System/_Module/API.h"
#include "ResourceID.h"
#include "ResourceSettings.h"

//-------------------------------------------------------------------------

namespace EE { class IniFile; }

//-------------------------------------------------------------------------

namespace EE::Resource
{
    //-------------------------------------------------------------------------
    // The resource provider is the system that is responsible for resolving a resource request to raw resource data
    // It is responsible for loading the request resource from the disk/network as well decompressing it
    // The raw decompressed data is then provided to the resource system for installation
    //-------------------------------------------------------------------------

    class ResourceRequest;

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API ResourceProvider
    {

    public:

        ResourceProvider( ResourceSettings const& settings ) : m_settings( settings ) {}
        virtual ~ResourceProvider() {}

        virtual bool IsReady() const = 0;
        virtual bool Initialize() = 0;
        virtual void Shutdown() {}

        // Get general resource settings
        ResourceSettings const& GetSettings() const { return m_settings; }

        // The resource provider update function
        virtual void Update() {};

        // Request a resource to be loaded
        virtual void RequestRawResource( ResourceRequest* pRequest ) = 0;

        // Cancel a prior request
        virtual void CancelRequest( ResourceRequest* pRequest ) = 0;

        // Get any externally updated resource for this update
        #if EE_DEVELOPMENT_TOOLS
        virtual TVector<ResourceID> const& GetExternallyUpdatedResources() const
        {
            static TVector<ResourceID> const emptyVector;
            return emptyVector;
        }
        #endif

    protected:

        ResourceSettings const m_settings;
    };
}