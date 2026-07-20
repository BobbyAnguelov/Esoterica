
#pragma once

#include "Base/Render/RHI.h"
#include "Engine/Render/Shaders/EngineShader.h"
#include "Engine/Render/RenderPasses/RenderPass.h"

namespace EE::Render
{
    class RenderSystem;
    class RenderViewport;

    class DepthDownsamplePass final
    {
    public:

        void Initialize( RenderPassContext const& context );
        void Shutdown( RenderSystem* pRenderSystem );

        void UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport );

        void DrawToViewport( RenderViewport const* pRenderViewport, DeviceResourceStates& resourceStates, RHI::CommandBuffer* pCommandBuffer, DeviceTextureState& depthBuffer ) const;

    private:

        RHI::Pipeline*                  m_pDownsamplePipeline = nullptr;
        RHI::SamplerStateHandle         m_linearMaxClampSampler = RHI::InvalidResourceHandle;
    };
}
