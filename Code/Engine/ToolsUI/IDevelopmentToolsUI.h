#pragma once
#include "Engine/_Module/API.h"
#include "Base/Resource/ResourceRequesterID.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
}

//-------------------------------------------------------------------------
// Development Tools Framework
//-------------------------------------------------------------------------
// Base class for any runtime/editor development UI tools

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    class ImageCache;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API IDevelopmentToolsUI
    {
    public:

        virtual ~IDevelopmentToolsUI() = default;

        virtual void Initialize( UpdateContext const& context, ImGuiX::ImageCache* pImageCache ) = 0;
        virtual void Shutdown( UpdateContext const& context ) = 0;

        // This is called at the absolute start of the frame before we update the resource system, start updating any entities, etc...
        // Any entity/world/map state changes need to be done via this update!
        virtual void StartFrame( UpdateContext const& context ) {}

        // Optional update run before we update the world at each stage
        virtual void Update( UpdateContext const& context ) {}

        // This is called at the absolute end of the frame just before we kick off rendering. It is generally NOT safe to modify any world/map/entity during this update!!!
        virtual void EndFrame( UpdateContext const& context ) {}

        // Hot Reload Support
        virtual void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToReload, TVector<ResourceID> const& resourcesToBeReloaded ) = 0;
        virtual void EndHotReload() = 0;
    };
}
#endif