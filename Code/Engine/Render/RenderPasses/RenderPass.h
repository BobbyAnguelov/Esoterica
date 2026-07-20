
#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/Device/DeviceResourceState.h"
#include "Base/Types/Arrays.h"

namespace EE::Render
{
    class RenderSystem;
    class RenderSettings;
    struct ForwardShadingMaterialShaderPipelineBucket;

    struct RenderPassContext
    {
        RenderSystem*                                           m_pRenderSystem = nullptr;
        RenderSettings const*                                   m_pRenderSettings = nullptr;
        TArrayView<ForwardShadingMaterialShaderPipelineBucket>  m_materialShaderPipelineBuckets;
    };
}
