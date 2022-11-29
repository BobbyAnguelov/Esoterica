#pragma once

#include "System/_Module/API.h"
#include "System/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class RenderDevice;
    class Texture;

    //-------------------------------------------------------------------------

    namespace CoreResources
    {
        void Initialize( RenderDevice* pRenderDevice );
        void Shutdown( RenderDevice* pRenderDevice );

        EE_SYSTEM_API Texture const* GetMissingTexture();
    };
}