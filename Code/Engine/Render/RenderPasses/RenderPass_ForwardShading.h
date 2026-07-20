

#pragma once

#include "Base/Render/RHI.h"
#include "Engine/Render/Device/DeviceRenderView.h"
#include "Engine/Render/RenderPasses/RenderPass.h"

namespace EE::Render
{
    namespace ShaderTypes
    {
        struct RenderView;
    }

    class RenderSystem;
    class RenderViewport;

    struct ForwardShadingMaterialShaderPipelineBucket
    {
        StringView                                  m_shaderName;
        RHI::CommandSignature*                      m_pCommandSignature = nullptr;
        RHI::Pipeline*                              m_pDepthOnlyPipeline = nullptr;
        RHI::Pipeline*                              m_pDepthOnlyAlphaTestPipeline = nullptr;
        RHI::Pipeline*                              m_pOpaquePipeline = nullptr;
        RHI::Pipeline*                              m_pAlphaBlendPipeline = nullptr;

        void Shutdown( RHI::Context* pContextRHI );
    };

    struct ForwardShadingPass
    {
        DeviceRenderView                            m_renderView;

        //---------------------------------------------------------------------------------------------------

        static TVector<ForwardShadingMaterialShaderPipelineBucket> InitializeMaterialShaderBuckets( RenderSystem* pRenderSystem );

        // Depth only pass
        static void DrawMaterialShaderBuckets_DepthOnly( TArrayView<ForwardShadingMaterialShaderPipelineBucket const>   materialShaderBuckets,
                                                         TArrayView<uint32_t const>                                     clusterCapacity,
                                                         DeviceRenderView const&                                        renderView,
                                                         RHI::Texture*                                                  pDepthTexture,
                                                         RHI::CommandBuffer*                                            pCommandBuffer );

        // Shading pass (assumes depth pass was rendered separately, will not work without a depth pass)
        static void DrawMaterialShaderBuckets_Shading( TArrayView<ForwardShadingMaterialShaderPipelineBucket const>     materialShaderBuckets,
                                                       TArrayView<uint32_t const>                                       clusterCapacity,
                                                       DeviceRenderView const&                                          renderView,
                                                       RHI::Texture*                                                    pColorTexture,
                                                       uint32_t                                                         colorTargetSlice,
                                                       uint32_t                                                         colorTargetMipSlice,
                                                       RHI::Texture*                                                    pDepthTexture,
                                                       RHI::CommandBuffer*                                              pCommandBuffer );

        // Depth + Shading passes combined
        static void DrawMaterialShaderBuckets( TArrayView<ForwardShadingMaterialShaderPipelineBucket const>     materialShaderBuckets,
                                               TArrayView<uint32_t const>                                       clusterCapacity,
                                               DeviceRenderView const&                                          renderView,
                                               RHI::Texture*                                                    pColorTexture,
                                               uint32_t                                                         colorTargetSlice,
                                               uint32_t                                                         colorTargetMipSlice,
                                               RHI::Texture*                                                    pDepthTexture,
                                               RHI::CommandBuffer*                                              pCommandBuffer );

        //---------------------------------------------------------------------------------------------------

        void Initialize( RenderPassContext const& context );
        void Shutdown( RenderSystem* pRenderSystem );

        void UpdateDeviceResources( RenderSystem*                                                       pRenderSystem,
                                    TArrayView<ForwardShadingMaterialShaderPipelineBucket const>        materialShaderBuckets,
                                    TArrayView<uint32_t const>                                          clusterCapacity );

        void UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport );

        void DepthOnlyPass( TArrayView<ForwardShadingMaterialShaderPipelineBucket const>                materialShaderBuckets,
                            TArrayView<uint32_t const>                                                  clusterCapacity,
                            RenderViewport const*                                                       pRenderViewport,
                            DeviceResourceStates&                                                       resourceStates,
                            RHI::CommandBuffer*                                                         pCommandBuffer,
                            TArrayView<ShaderTypes::RenderView>                                         dstRenderViews_WriteCombined ) const;

        void ShadingPass( TArrayView<ForwardShadingMaterialShaderPipelineBucket const>                  materialShaderBuckets,
                          TArrayView<uint32_t const>                                                    clusterCapacity,
                          RenderViewport const*                                                         pRenderViewport,
                          DeviceResourceStates&                                                         resourceStates,
                          RHI::CommandBuffer*                                                           pCommandBuffer ) const;
    };
}
