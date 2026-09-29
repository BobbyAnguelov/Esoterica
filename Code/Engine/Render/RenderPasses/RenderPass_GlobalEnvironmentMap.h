
#pragma once

#include "Engine/Render/Shaders/EngineShader.h"
#include "Engine/Render/ActiveRenderView.h"
#include "Engine/Render/Device/DeviceRenderView.h"
#include "Engine/Render/RenderPasses/RenderPass.h"

namespace EE
{
    class EntityWorld;
}

namespace EE::Render
{
    namespace ShaderTypes
    {
        struct RenderView;
    }

    class DeviceRenderWorld;
    class RenderViewport;

    //-------------------------------------------------------------------------

    class GlobalEnvironmentMapPass final
    {
    public:

        //-------------------------------------------------------------------------

        void Initialize( RenderPassContext const& context );
        void Shutdown( RenderSystem* pRenderSystem );

        void UpdateWorldDeviceResources( EntityWorld* pWorld );
        void UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport );

        void CaptureGlobalEnvironmentMap
        (
            TArrayView<ForwardShadingMaterialShaderPipelineBucket>      materialShaderPipelineBuckets,
            ActiveRenderViewList const&                                 activeRenderViewList,
            DeviceRenderWorld const&                                    deviceRenderWorld,
            RenderViewport const*                                       pRenderViewport,
            RHI::Buffer*                                                pGlobalParametersBuffer,
            RHI::CommandBuffer*                                         pCommandBuffer
        ) const;

        void FilterEnvironmentMap
        (
            DeviceRenderWorld const&                                    deviceRenderWorld,
            RenderViewport const*                                       pRenderViewport,
            RHI::CommandBuffer*                                         pCommandBuffer,
            RHI::Buffer*                                                pProbeTableBuffer
        ) const;

    private:

        //-------------------------------------------------------------------------

        RHI::Pipeline*                              m_pPipelineCubemapDownsample = nullptr;

        ComputeShader const*                        m_pIrradianceSHProjectShader = nullptr;
        ComputeShader const*                        m_pProbeTableRadianceShader = nullptr;
    };
}
