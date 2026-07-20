
#pragma once

#include "Engine/Render/Shaders/EngineShader.h"
#include "Engine/Render/Device/DeviceRenderView.h"
#include "Engine/Render/RenderPasses/RenderPass.h"

namespace EE::Render
{
    namespace ShaderTypes
    {
        struct RenderView;
    }

    class GlobalEnvironmentMapPass final
    {
    public:

        static constexpr uint32_t                   NumRenderViews = 6;

    public:

        //---------------------------------------------------------------------------------------------------

        void Initialize( RenderPassContext const& context );
        void Shutdown( RenderSystem* pRenderSystem );

        void UpdateDeviceResources( RenderSystem*                                               pRenderSystem,
                                    TArrayView<ForwardShadingMaterialShaderPipelineBucket>      materialShaderPipelineBuckets,
                                    TArrayView<uint32_t const>                                  clusterCapacity );

        void DrawAndFilterGlobalEnvironmentMap( TArrayView<ForwardShadingMaterialShaderPipelineBucket>      materialShaderPipelineBuckets,
                                                TArrayView<uint32_t const>                                  clusterCapacity,
                                                RHI::Buffer*                                                pGlobalParametersBuffer,
                                                RHI::CommandBuffer*                                         pCommandBuffer,
                                                RHI::Texture*                                               pRadianceRenderTarget,
                                                RHI::Texture*                                               pIrradianceRenderTarget,
                                                RHI::BufferHandle                                           renderViewBufferHandle,
                                                uint32_t                                                    renderViewOffset,
                                                TArrayView<ShaderTypes::RenderView>                         dstRenderViews ) const;

        void UpdateDeviceResources_DFG( RHI::Context* pContextRHI );
        void PrecomputeDFG( RHI::CommandBuffer* pCommandBuffer );

        RHI::TextureHandle GetDFGTextureHandle() const;
        RHI::TextureHandle GetCaptureTextureHandle() const;

    public:

        //---------------------------------------------------------------------------------------------------

        TArray<DeviceRenderView, NumRenderViews>    m_renderViews;

    private:

        //---------------------------------------------------------------------------------------------------

        bool                                        m_dfgUpdateNeeded = true;

        RHI::Texture*                               m_pCaptureRenderTarget = nullptr;
        RHI::Texture*                               m_pDepthRenderTarget = nullptr;
        RHI::Texture*                               m_pDfgRenderTarget = nullptr;

        RHI::Pipeline*                              m_pPipelineIrradianceFiltering = nullptr;
        RHI::Pipeline*                              m_pPipelineRadianceFiltering = nullptr;
        RHI::Pipeline*                              m_pPipelinePrecomputeDFG = nullptr;
        RHI::Pipeline*                              m_pPipelineCubemapDownsample = nullptr;
    };
}
