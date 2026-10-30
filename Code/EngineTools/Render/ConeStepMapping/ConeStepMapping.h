#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    // Relaxed cone step map generation, Policarpo and Oliveira describe it (GPU Gems 3, chapter 18)
    //-------------------------------------------------------------------------

    namespace ConeStepMap
    {
        EE_ENGINETOOLS_API void Generate( Resource::CompileContext const& ctx, uint8_t* pSurfaceData, int32_t width, int32_t height, int32_t strideInBytes, int32_t searchRadiusInTexels );
    }
}
