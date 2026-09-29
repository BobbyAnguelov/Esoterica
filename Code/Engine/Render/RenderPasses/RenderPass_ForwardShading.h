

#pragma once

#include "Base/Esoterica.h"
#include "Base/Render/RHI.h"
#include "Engine/Render/ActiveRenderView.h"
#include "Engine/Render/Device/DeviceRenderView.h"
#include "Engine/Render/Device/DeviceResizeBuffer.h"
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

    class RenderSystem;
    class RenderViewport;

    struct ForwardShadingMaterialShaderPipelineBucket
    {
        void Initialize( RHI::Context* pContextRHI, MaterialShader const& shader );
        void Shutdown( RHI::Context* pContextRHI );

        //-------------------------------------------------------------------------

        StringView                                  m_shaderName;
        RHI::CommandSignature*                      m_pCommandSignature = nullptr;
        RHI::Pipeline*                              m_pDepthOnlyPipeline = nullptr;
        RHI::Pipeline*                              m_pDepthOnlyAlphaTestPipeline = nullptr;
        RHI::Pipeline*                              m_pOpaquePipeline = nullptr;
        RHI::Pipeline*                              m_pAlphaBlendPipeline = nullptr;

        // HACK: Special treatment for 16-bit depth only pipelines, used by shadows and editor outlines
        RHI::Pipeline*                              m_pDepthOnlyPipeline_LowPrecision = nullptr;
        RHI::Pipeline*                              m_pDepthOnlyAlphaTestPipeline_LowPrecision = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        RHI::Pipeline*                              m_pOutlinePipeline = nullptr;
        #endif
    };

    //-------------------------------------------------------------------------

    struct ForwardShadingPass
    {
        static TVector<ForwardShadingMaterialShaderPipelineBucket> InitializeMaterialShaderBuckets( RenderSystem* pRenderSystem );

        // Depth only pass
        static void DrawMaterialShaderBuckets_DepthOnly
        (
            TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
            ActiveRenderView const&                                         activeRenderView,
            RHI::Texture*                                                   pDepthTexture,
            uint32_t                                                        depthTargetSlice,
            RHI::CommandBuffer*                                             pCommandBuffer
        );

        #if EE_DEVELOPMENT_TOOLS
        static void DrawMaterialShaderBuckets_OutlineID
        (
            TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
            ActiveRenderView const&                                         activeRenderView,
            RHI::Texture*                                                   pObjectIDTexture,
            RHI::Texture*                                                   pDepthTexture,
            RHI::CommandBuffer*                                             pCommandBuffer
        );
        #endif

        // Shading pass (assumes depth pass was rendered separately, will not work without a depth pass)
        static void DrawMaterialShaderBuckets_Shading
        (
            TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
            ActiveRenderView const&                                         activeRenderView,
            RHI::Texture*                                                   pColorTexture,
            uint32_t                                                        colorTargetSlice,
            uint32_t                                                        colorTargetMipSlice,
            RHI::Texture*                                                   pDepthTexture,
            RHI::CommandBuffer*                                             pCommandBuffer
        );

        // Depth + Shading passes combined
        static void DrawMaterialShaderBuckets
        (
            TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
            ActiveRenderView const&                                         activeRenderView,
            RHI::Texture*                                                   pColorTexture,
            uint32_t                                                        colorTargetSlice,
            uint32_t                                                        colorTargetMipSlice,
            RHI::Texture*                                                   pDepthTexture,
            RHI::CommandBuffer*                                             pCommandBuffer
        );

        //-------------------------------------------------------------------------

        void Initialize( RenderPassContext const& context );
        void Shutdown( RenderSystem* pRenderSystem );

        void UpdateWorldDeviceResources( EntityWorld* pWorld );
        void UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport );

        void DepthOnlyPass
        (
            TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
            ActiveRenderView const&                                         activeRenderView,
            RenderViewport const*                                           pRenderViewport,
            DeviceResourceStates&                                           resourceStates,
            RHI::CommandBuffer*                                             pCommandBuffer
        ) const;

        void ShadingPass
        (
            TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
            ActiveRenderView const&                                         activeRenderView,
            RenderViewport const*                                           pRenderViewport,
            DeviceResourceStates&                                           resourceStates,
            RHI::CommandBuffer*                                             pCommandBuffer
        ) const;
    };
}
