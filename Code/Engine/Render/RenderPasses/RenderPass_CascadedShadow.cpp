#include "Engine/Render/RenderPasses/RenderPass_CascadedShadow.h"
#include "Engine/Render/RenderPasses/RenderPass_ForwardShading.h" // TODO: Decouple forward shading
#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Engine/Render/RenderSystem.h"
#include "Base/Render/RHI.h"
#include "Base/Render/Settings/Settings_Render.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    void CascadedShadowPass::Initialize( RenderPassContext const& context )
    {
        m_pRenderSettings = context.m_pRenderSettings;
    }

    void CascadedShadowPass::Shutdown( RenderSystem* pRenderSystem )
    {}

    //-------------------------------------------------------------------------

    void CascadedShadowPass::BarrierWriteable( DeviceRenderWorld const& deviceRenderWorld, ActiveRenderViewList const& activeRenderViewList, DeviceResourceStates& resourceStates ) const
    {
        for ( uint32_t activeRenderViewIndex = 0; activeRenderViewIndex < activeRenderViewList.m_numActiveRenderViews; ++activeRenderViewIndex )
        {
            DeviceRenderView const* pShadowView = deviceRenderWorld.GetDeviceRenderView( activeRenderViewList.GetDeviceRenderViewIndex( activeRenderViewIndex ) );
            if ( pShadowView->m_deviceRenderViewType != DeviceRenderViewType::CascadedShadowMap || !pShadowView->m_depthTexture.IsValid() )
            {
                continue;
            }

            resourceStates.Writeable( pShadowView->m_depthTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::DepthWrite, RHI::TextureState::DepthWrite );
        }
    }

    void CascadedShadowPass::BarrierReadOnly( DeviceRenderWorld const& deviceRenderWorld, ActiveRenderViewList const& activeRenderViewList, DeviceResourceStates& resourceStates ) const
    {
        for ( uint32_t activeRenderViewIndex = 0; activeRenderViewIndex < activeRenderViewList.m_numActiveRenderViews; ++activeRenderViewIndex )
        {
            DeviceRenderView const* pShadowView = deviceRenderWorld.GetDeviceRenderView( activeRenderViewList.GetDeviceRenderViewIndex( activeRenderViewIndex ) );
            if ( pShadowView->m_deviceRenderViewType != DeviceRenderViewType::CascadedShadowMap || !pShadowView->m_depthTexture.IsValid() )
            {
                continue;
            }

            resourceStates.ReadOnly( pShadowView->m_depthTexture, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
        }
    }

    void CascadedShadowPass::DrawShadowCascades
    (
        TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderPipelineBuckets,
        ActiveRenderViewList const&                                     activeRenderViewList,
        DeviceRenderWorld const&                                        deviceRenderWorld,
        DeviceResourceStates&                                           resourceStates,
        RHI::CommandBuffer*                                             pCommandBuffer
    ) const
    {
        EE_PROFILE_FUNCTION_RENDER();

        float const ShadowMapResolution = float( m_pRenderSettings->m_cascadedShadowResolution );

        //-------------------------------------------------------------------------

        Float2 const viewTopLeft = Float2( 0.0F, 0.0F );
        Float2 const viewSize = Float2( ShadowMapResolution, ShadowMapResolution );

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Cascaded Shadows" );

        RHI::CmdSetViewport( pCommandBuffer, viewTopLeft.m_x, viewTopLeft.m_y, viewSize.m_x, viewSize.m_y, 0.0F, 1.0F );
        RHI::CmdSetScissor( pCommandBuffer, uint32_t( viewTopLeft.m_x ), uint32_t( viewTopLeft.m_y ), uint32_t( viewSize.m_x ), uint32_t( viewSize.m_y ) );

        for ( uint32_t activeRenderViewIndex = 0; activeRenderViewIndex < activeRenderViewList.m_numActiveRenderViews; ++activeRenderViewIndex )
        {
            DeviceRenderView const* pShadowView = deviceRenderWorld.GetDeviceRenderView( activeRenderViewList.GetDeviceRenderViewIndex( activeRenderViewIndex ) );
            if ( pShadowView->m_deviceRenderViewType != DeviceRenderViewType::CascadedShadowMap || !pShadowView->m_depthTexture.IsValid() )
            {
                continue;
            }

            RHI::Texture* const pShadowDepthArray = pShadowView->m_depthTexture.m_pTexture;
            EE_ASSERT( pShadowDepthArray != nullptr );
            EE_ASSERT( activeRenderViewIndex + g_NumCascadedShadowViews <= activeRenderViewList.m_numActiveRenderViews );

            for ( uint32_t cascadeIndex = 0; cascadeIndex < g_NumCascadedShadowViews; ++cascadeIndex )
            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Cascade" );

                ForwardShadingPass::DrawMaterialShaderBuckets_DepthOnly
                (
                    materialShaderPipelineBuckets,
                    activeRenderViewList.GetActiveRenderView( activeRenderViewIndex + cascadeIndex ),
                    pShadowDepthArray,
                    cascadeIndex,
                    pCommandBuffer
                );
            }
        }
    }
}
