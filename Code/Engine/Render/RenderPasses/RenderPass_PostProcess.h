
#pragma once

#include "Engine/Render/Shaders/EngineShader.h"
#include "Engine/Render/RenderPasses/RenderPass.h"

namespace EE::Render
{
    class RenderSystem;
    class RenderViewport;

    class PostProcessPass final
    {
    public:

        void Initialize( RenderPassContext const& context );
        void Shutdown( RenderSystem* pRenderSystem );

        void UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport );

        void DrawToViewport( DeviceResourceStates&      resourceStates,
                             RHI::CommandBuffer*        pCommandBuffer,
                             RHI::Texture*              pTonemapLUT,
                             DeviceTextureState&        hdrTexture,
                             DeviceTextureState&        depthTexture ) const;

    private:

        RHI::Pipeline* m_pPipelineCombined = nullptr;
    };
}
