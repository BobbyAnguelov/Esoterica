
#pragma once

#include "Engine/Render/Shaders/EngineShader.h"
#include "Engine/Render/RenderPasses/RenderPass.h"

namespace EE::Math
{
    class ViewVolume;
}

namespace EE::Render
{
    class RenderSystem;
    class RenderViewport;

    namespace ShaderTypes
    {
        struct GTAOConstants;
    }

    struct GTAOPass final
    {
        ComputeShader const*                            m_pMainPassComputeShader = nullptr;
        ComputeShader const*                            m_pPrefilterDepthComputeShader = nullptr;
        ComputeShader const*                            m_pDenoiseComputeShader = nullptr;

        //---------------------------------------------------------------------------------------------------

        void Initialize( RenderPassContext const& context );
        void Shutdown( RenderSystem* pRenderSystem );

        void UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport );

        void PrefilterDepth( RenderViewport const*  pRenderViewport,
                             DeviceResourceStates&  resourceStates,
                             RHI::CommandBuffer*    pCommandBuffer,
                             uint32_t               frameIndex );

        void ComputeNoisyResult( RenderViewport const*          pRenderViewport,
                                 DeviceResourceStates&          resourceStates,
                                 RHI::CommandBuffer*            pCommandBuffer,
                                 RHI::BufferHandle              renderViewBuffer,
                                 uint32_t                       mainCameraRenderView,
                                 uint32_t                       frameIndex );

        void Denoise( RenderViewport const*     pRenderViewport,
                      DeviceResourceStates&     resourceStates,
                      RHI::CommandBuffer*       pCommandBuffer,
                      bool                      asyncCompute,
                      uint32_t                  frameIndex );

        void Upsample( RenderViewport const*    pRenderViewport,
                       DeviceResourceStates&    resourceStates,
                       RHI::CommandBuffer*      pCommandBuffer,
                       uint32_t                 frameIndex );

        //---------------------------------------------------------------------------------------------------

    public:

        RHI::Pipeline*  m_pUpsamplePipeline = nullptr;
        bool            m_enableLowResolution = false;
    };
}
