
#pragma once

#include "Engine/Render/Shaders/EngineShader.h"
#include "Engine/Render/RenderPasses/RenderPass.h"

namespace EE::Render
{
    class RenderSystem;
    class RenderViewport;

    struct SMAAPass final
    {
        RHI::Pipeline*      m_pPipelineEdgeDetection = nullptr;
        RHI::Pipeline*      m_pPipelineBlendingWeightCalculation = nullptr;
        RHI::Pipeline*      m_pPipelineNeighborhoodBlending = nullptr;

        //---------------------------------------------------------------------------------------------------

        void Initialize( RenderPassContext const& context );
        void Shutdown( RenderSystem* pRenderSystem );

        void UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport );

        void DrawToViewport( RenderViewport const*                  pRenderViewport,
                             DeviceResourceStates&                  resourceStates,
                             RHI::CommandBuffer*                    pCommandBuffer,
                             RHI::Texture*                          pSMAAAreaTexture,
                             RHI::Texture*                          pSMAASearchTexture ) const;
    };
}
