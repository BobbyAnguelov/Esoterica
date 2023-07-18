#pragma once

#include "Base/_Module/API.h"
#include "Base/Esoterica.h"

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

        EE_BASE_API Texture const* GetMissingTexture();
    };
}