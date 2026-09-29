#include "Engine/Render/RenderPasses/RenderPass_PunctualShadow.h"
#include "Engine/Render/RenderPasses/RenderPass_ForwardShading.h" // TODO: Decouple forward shading
#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Engine/Render/RenderSystem.h"
#include "Base/Render/RHI.h"
#include "Base/Profiling.h"


//-------------------------------------------------------------------------

namespace EE::Render
{
    void PunctualShadowPass::Initialize( RenderPassContext const& context )
    {}

    void PunctualShadowPass::Shutdown( RenderSystem* pRenderSystem )
    {}

    //-------------------------------------------------------------------------

    void PunctualShadowPass::BarrierWriteable( DeviceRenderWorld const& deviceRenderWorld, ActiveRenderViewList const& activeRenderViewList, DeviceResourceStates& resourceStates ) const
    {
        for ( uint32_t activeRenderViewIndex = 0; activeRenderViewIndex < activeRenderViewList.m_numActiveRenderViews; ++activeRenderViewIndex )
        {
            DeviceRenderView const* pShadowView = deviceRenderWorld.GetDeviceRenderView( activeRenderViewList.GetDeviceRenderViewIndex( activeRenderViewIndex ) );
            if ( pShadowView->m_deviceRenderViewType != DeviceRenderViewType::PointShadowMap && pShadowView->m_deviceRenderViewType != DeviceRenderViewType::SpotShadowMap )
            {
                continue;
            }

            resourceStates.Writeable( pShadowView->m_depthTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::DepthWrite, RHI::TextureState::DepthWrite );
        }
    }

    void PunctualShadowPass::BarrierReadOnly( DeviceRenderWorld const& deviceRenderWorld, ActiveRenderViewList const& activeRenderViewList, DeviceResourceStates& resourceStates ) const
    {
        for ( uint32_t activeRenderViewIndex = 0; activeRenderViewIndex < activeRenderViewList.m_numActiveRenderViews; ++activeRenderViewIndex )
        {
            DeviceRenderView const* pShadowView = deviceRenderWorld.GetDeviceRenderView( activeRenderViewList.GetDeviceRenderViewIndex( activeRenderViewIndex ) );
            if ( pShadowView->m_deviceRenderViewType != DeviceRenderViewType::PointShadowMap && pShadowView->m_deviceRenderViewType != DeviceRenderViewType::SpotShadowMap )
            {
                continue;
            }

            resourceStates.ReadOnly( pShadowView->m_depthTexture, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
        }
    }

    void PunctualShadowPass::DrawPunctualShadows
    (
        TArrayView<ForwardShadingMaterialShaderPipelineBucket const>   materialShaderPipelineBuckets,
        ActiveRenderViewList const&                                    activeRenderViewList,
        DeviceRenderWorld const&                                       deviceRenderWorld,
        DeviceResourceStates&                                          resourceStates,
        RHI::CommandBuffer*                                            pCommandBuffer
    ) const
    {
        EE_PROFILE_FUNCTION_RENDER();

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Punctual Shadows" );

        //-------------------------------------------------------------------------

        for ( uint32_t activeRenderViewIndex = 0; activeRenderViewIndex < activeRenderViewList.m_numActiveRenderViews; ++activeRenderViewIndex )
        {
            DeviceRenderView const* pShadowView = deviceRenderWorld.GetDeviceRenderView( activeRenderViewList.GetDeviceRenderViewIndex( activeRenderViewIndex ) );
            if ( pShadowView->m_deviceRenderViewType != DeviceRenderViewType::PointShadowMap && pShadowView->m_deviceRenderViewType != DeviceRenderViewType::SpotShadowMap )
            {
                continue;
            }

            RHI::Texture* const pShadowTexture = pShadowView->m_depthTexture.m_pTexture;
            EE_ASSERT( pShadowTexture != nullptr );

            uint32_t const resolution = pShadowTexture->m_width;
            uint32_t const numViews = GetRenderViewGroupSize( pShadowView->m_deviceRenderViewType );

            EE_ASSERT( activeRenderViewIndex + numViews <= activeRenderViewList.m_numActiveRenderViews );

            RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( resolution ), float( resolution ), 0.0F, 1.0F );
            RHI::CmdSetScissor( pCommandBuffer, 0, 0, resolution, resolution );

            for ( uint32_t viewIndex = 0; viewIndex < numViews; ++viewIndex )
            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Punctual Shadow" );

                ForwardShadingPass::DrawMaterialShaderBuckets_DepthOnly
                (
                    materialShaderPipelineBuckets,
                    activeRenderViewList.GetActiveRenderView( activeRenderViewIndex + viewIndex ),
                    pShadowTexture,
                    viewIndex,
                    pCommandBuffer
                );
            }
        }
    }
}
