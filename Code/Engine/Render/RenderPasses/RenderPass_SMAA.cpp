
#include "RenderPass_SMAA.h"
#include "Engine/Render/RenderSystem.h"
#include "Base/Render/RHI.h"

#include "Engine/Render/RenderViewport.h"

#include "Engine/Render/Shaders/SMAA/SMAA_EdgeDetection.esf"
#include "Engine/Render/Shaders/SMAA/SMAA_BlendingWeightCalculation.esf"
#include "Engine/Render/Shaders/SMAA/SMAA_NeighborhoodBlending.esf"

namespace EE::Render
{
    void SMAAPass::Initialize( RenderPassContext const& context )
    {
        static StringID s_EdgeDetectionShaderID = StringID( "SMAA_EdgeDetection" );
        static StringID s_BlendingWeightCalculationShaderID = StringID( "SMAA_BlendingWeightCalculation" );
        static StringID s_NeighborhoodBlendingShaderID = StringID( "SMAA_NeighborhoodBlending" );

        SurfaceShader const* pEdgeDetectionShader = context.m_pRenderSystem->FindSurfaceShader( s_EdgeDetectionShaderID );
        SurfaceShader const* pBlendingWeightCalculationShader = context.m_pRenderSystem->FindSurfaceShader( s_BlendingWeightCalculationShaderID );
        SurfaceShader const* pNeighborhoodBlendingShader = context.m_pRenderSystem->FindSurfaceShader( s_NeighborhoodBlendingShaderID );

        RHI::DataFormat edgeDetectionDataFormats[] = { RHI::DataFormat::RGBA8_UNorm };
        RHI::DataFormat neighborhoodBlendingDataFormats[] = { RHI::DataFormat::RG11_B10_UFloat };

        RHI::DepthStencilState edgeDetectionDepthStencil = {};
        edgeDetectionDepthStencil.m_stencilTest = true;
        edgeDetectionDepthStencil.m_stencilFrontCompareMode = RHI::CompareMode::Always;
        edgeDetectionDepthStencil.m_stencilFrontPass = RHI::StencilOp::Replace;
        edgeDetectionDepthStencil.m_stencilWriteMask = 0xFF;

        RHI::GraphicsPipelineParameters edgeDetectionPipelineParameters = {};
        edgeDetectionPipelineParameters.m_colorFormats = edgeDetectionDataFormats;
        edgeDetectionPipelineParameters.m_depthStencilFormat = RHI::DataFormat::S8_Uint;
        edgeDetectionPipelineParameters.m_numRenderTargets = 1;
        edgeDetectionPipelineParameters.m_pShader = pEdgeDetectionShader->m_pShader;
        edgeDetectionPipelineParameters.m_pRootSignature = pEdgeDetectionShader->m_pRootSignature;
        edgeDetectionPipelineParameters.m_depthStencilState = edgeDetectionDepthStencil;
        edgeDetectionPipelineParameters.m_debugName = "SMAA EdgeDetection Pipeline";

        m_pPipelineEdgeDetection = RHI::CreatePipeline( context.m_pRenderSystem->GetContextRHI(), edgeDetectionPipelineParameters );

        RHI::DepthStencilState blendingWeightCalculationDepthStencil = {};
        blendingWeightCalculationDepthStencil.m_stencilTest = true;
        blendingWeightCalculationDepthStencil.m_stencilFrontCompareMode = RHI::CompareMode::Equal;
        blendingWeightCalculationDepthStencil.m_stencilReadMask = 0xFF;

        RHI::GraphicsPipelineParameters blendingWeightCalculationPipelineParameters = edgeDetectionPipelineParameters;
        blendingWeightCalculationPipelineParameters.m_depthStencilState = blendingWeightCalculationDepthStencil;
        blendingWeightCalculationPipelineParameters.m_pShader = pBlendingWeightCalculationShader->m_pShader;
        blendingWeightCalculationPipelineParameters.m_pRootSignature = pBlendingWeightCalculationShader->m_pRootSignature;
        blendingWeightCalculationPipelineParameters.m_debugName = "SMAA BlendingWeightCalculation Pipeline";

        m_pPipelineBlendingWeightCalculation = RHI::CreatePipeline( context.m_pRenderSystem->GetContextRHI(), blendingWeightCalculationPipelineParameters );

        RHI::GraphicsPipelineParameters neighborhoodBlendingPipelineParameters = edgeDetectionPipelineParameters;
        neighborhoodBlendingPipelineParameters.m_depthStencilState = {};
        neighborhoodBlendingPipelineParameters.m_depthStencilFormat = RHI::DataFormat::Undefined;
        neighborhoodBlendingPipelineParameters.m_colorFormats = neighborhoodBlendingDataFormats;
        neighborhoodBlendingPipelineParameters.m_pShader = pNeighborhoodBlendingShader->m_pShader;
        neighborhoodBlendingPipelineParameters.m_pRootSignature = pNeighborhoodBlendingShader->m_pRootSignature;
        neighborhoodBlendingPipelineParameters.m_debugName = "SMAA NeighborhoodBlending Pipeline";

        m_pPipelineNeighborhoodBlending = RHI::CreatePipeline( context.m_pRenderSystem->GetContextRHI(), neighborhoodBlendingPipelineParameters );
    }

    void SMAAPass::Shutdown( RenderSystem* pRenderSystem )
    {
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pPipelineEdgeDetection ) );
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pPipelineBlendingWeightCalculation ) );
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pPipelineNeighborhoodBlending ) );
    }

    void SMAAPass::UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport )
    {
        Int2 textureSize = pRenderViewport->GetSize();
        uint32_t textureWidth = uint32_t( textureSize.m_x );
        uint32_t textureHeight = uint32_t( textureSize.m_y );

        if ( !pRenderViewport->m_SMAA_StencilTexture || !pRenderViewport->m_SMAA_EdgesTexture || !pRenderViewport->m_SMAA_BlendTexture || !pRenderViewport->m_SMAA_ResultTexture ||
             pRenderViewport->m_SMAA_EdgesTexture->m_width != textureWidth || pRenderViewport->m_SMAA_EdgesTexture->m_height != textureHeight ||
             pRenderViewport->m_SMAA_BlendTexture->m_width != textureWidth || pRenderViewport->m_SMAA_BlendTexture->m_height != textureHeight ||
             pRenderViewport->m_SMAA_ResultTexture->m_width != textureWidth || pRenderViewport->m_SMAA_ResultTexture->m_height != textureHeight ||
             pRenderViewport->m_SMAA_StencilTexture->m_width != textureWidth || pRenderViewport->m_SMAA_StencilTexture->m_height != textureHeight )
        {
            pRenderSystem->QueueResourceDelete
            (
                eastl::move( pRenderViewport->m_SMAA_StencilTexture ),
                eastl::move( pRenderViewport->m_SMAA_EdgesTexture ),
                eastl::move( pRenderViewport->m_SMAA_BlendTexture ),
                eastl::move( pRenderViewport->m_SMAA_ResultTexture )
            );

            RHI::TextureParameters edgesTextureParameters = {};
            edgesTextureParameters.m_width = textureWidth;
            edgesTextureParameters.m_height = textureHeight;
            edgesTextureParameters.m_format = RHI::DataFormat::RGBA8_UNorm;
            edgesTextureParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::RenderTarget, RHI::DescriptorTypeFlags::Texture );
            edgesTextureParameters.m_debugName.sprintf( "SMAA Edges Target %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_SMAA_EdgesTexture = RHI::CreateTexture( pRenderSystem->GetContextRHI(), edgesTextureParameters );

            RHI::TextureParameters blendTextureParameters = edgesTextureParameters;
            blendTextureParameters.m_debugName.sprintf( "SMAA Blend Target %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_SMAA_BlendTexture = RHI::CreateTexture( pRenderSystem->GetContextRHI(), blendTextureParameters );

            RHI::TextureParameters resultTextureParameters = edgesTextureParameters;
            resultTextureParameters.m_format = RHI::DataFormat::RG11_B10_UFloat;
            resultTextureParameters.m_debugName.sprintf( "SMAA Result Target %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_SMAA_ResultTexture = RHI::CreateTexture( pRenderSystem->GetContextRHI(), resultTextureParameters );

            RHI::TextureParameters stencilTextureParameters = edgesTextureParameters;
            stencilTextureParameters.m_format = RHI::DataFormat::S8_Uint;
            stencilTextureParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::RenderTarget;
            stencilTextureParameters.m_initialState = RHI::TextureState::DepthWrite;
            stencilTextureParameters.m_debugName.sprintf( "SMAA Stencil Target %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_SMAA_StencilTexture = RHI::CreateTexture( pRenderSystem->GetContextRHI(), stencilTextureParameters );
        }
    }

    void SMAAPass::DrawToViewport( RenderViewport const*                pRenderViewport,
                                   DeviceResourceStates&                resourceStates,
                                   RHI::CommandBuffer*                  pCommandBuffer,
                                   RHI::Texture*                        pSMAAAreaTexture,
                                   RHI::Texture*                        pSMAASearchTexture ) const
    {

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "SMAA" );

        Float2 viewportSize = pRenderViewport->GetSize();

        {
            EE_ASSERT( !resourceStates.HasPendingBarriers() );
            resourceStates.Writeable( pRenderViewport->m_SMAA_EdgesTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
            resourceStates.Writeable( pRenderViewport->m_SMAA_StencilTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::DepthWrite, RHI::TextureState::DepthWrite );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::LoadAction edgeDetectionLoadAction = {};
            edgeDetectionLoadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;
            edgeDetectionLoadAction.m_colorClearValues[0] = pRenderViewport->m_SMAA_EdgesTexture->m_clearValue;

            edgeDetectionLoadAction.m_loadActionStencil = RHI::LoadActionType::Clear;
            edgeDetectionLoadAction.m_depthClearValue = pRenderViewport->m_SMAA_StencilTexture->m_clearValue;

            RHI::CmdSetRenderTargets( pCommandBuffer, { &pRenderViewport->m_SMAA_EdgesTexture.m_pTexture, 1 }, pRenderViewport->m_SMAA_StencilTexture, &edgeDetectionLoadAction );

            ShaderTypes::SMAAEdgeDetectionResourceTableData edgeDetectionRootConstants = {};

            EE_ASSERT( !resourceStates.HasPendingBarriers() );
            edgeDetectionRootConstants.SetColorTexture( resourceStates, RHI::PipelineStage::PixelShader, pRenderViewport->m_ForwardShading_ColorTexture );
            resourceStates.FlushBarriers( pCommandBuffer );

            edgeDetectionRootConstants.m_rtMetrics[0] = 1.0F / viewportSize.m_x;
            edgeDetectionRootConstants.m_rtMetrics[1] = 1.0F / viewportSize.m_y;
            edgeDetectionRootConstants.m_rtMetrics[2] = viewportSize.m_x;
            edgeDetectionRootConstants.m_rtMetrics[3] = viewportSize.m_y;

            RHI::CmdSetPipeline( pCommandBuffer, m_pPipelineEdgeDetection );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &edgeDetectionRootConstants, sizeof( edgeDetectionRootConstants ) );
            RHI::CmdSetStencilReference( pCommandBuffer, 0x1 );
            RHI::CmdDraw( pCommandBuffer, 3, 0 );
        }

        {
            EE_ASSERT( !resourceStates.HasPendingBarriers() );
            resourceStates.ReadOnly( pRenderViewport->m_SMAA_StencilTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::DepthRead, RHI::TextureState::DepthRead );
            resourceStates.Writeable( pRenderViewport->m_SMAA_BlendTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::LoadAction blendingWeightCalculationLoadAction = {};
            blendingWeightCalculationLoadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;
            blendingWeightCalculationLoadAction.m_colorClearValues[0] = pRenderViewport->m_SMAA_BlendTexture->m_clearValue;

            blendingWeightCalculationLoadAction.m_loadActionStencil = RHI::LoadActionType::Load;

            RHI::CmdSetRenderTargets( pCommandBuffer, { &pRenderViewport->m_SMAA_BlendTexture.m_pTexture, 1 }, pRenderViewport->m_SMAA_StencilTexture, &blendingWeightCalculationLoadAction );

            EE_ASSERT( !resourceStates.HasPendingBarriers() );

            ShaderTypes::SMAABlendingWeightCalculationResourceTableData blendingWeightCalculationRootConstants = {};
            blendingWeightCalculationRootConstants.SetEdgesTexture( resourceStates, RHI::PipelineStage::PixelShader, pRenderViewport->m_SMAA_EdgesTexture );
            blendingWeightCalculationRootConstants.SetAreaTexture( pSMAAAreaTexture );
            blendingWeightCalculationRootConstants.SetSearchTexture( pSMAASearchTexture );
            resourceStates.FlushBarriers( pCommandBuffer );

            blendingWeightCalculationRootConstants.m_rtMetrics[0] = 1.0F / viewportSize.m_x;
            blendingWeightCalculationRootConstants.m_rtMetrics[1] = 1.0F / viewportSize.m_y;
            blendingWeightCalculationRootConstants.m_rtMetrics[2] = viewportSize.m_x;
            blendingWeightCalculationRootConstants.m_rtMetrics[3] = viewportSize.m_y;

            RHI::CmdSetPipeline( pCommandBuffer, m_pPipelineBlendingWeightCalculation );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &blendingWeightCalculationRootConstants, sizeof( blendingWeightCalculationRootConstants ) );
            RHI::CmdSetStencilReference( pCommandBuffer, 0x1 );
            RHI::CmdDraw( pCommandBuffer, 3, 0 );
        }

        {
            EE_ASSERT( !resourceStates.HasPendingBarriers() );
            resourceStates.Writeable( pRenderViewport->m_SMAA_ResultTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::LoadAction neighborhoodBlendingLoadAction = {};
            neighborhoodBlendingLoadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;
            neighborhoodBlendingLoadAction.m_colorClearValues[0] = pRenderViewport->m_SMAA_ResultTexture->m_clearValue;

            RHI::CmdSetRenderTargets( pCommandBuffer, { &pRenderViewport->m_SMAA_ResultTexture.m_pTexture, 1 }, nullptr, &neighborhoodBlendingLoadAction );

            EE_ASSERT( !resourceStates.HasPendingBarriers() );

            ShaderTypes::SMAANeighborhoodBlendingResourceTableData neighborhoodBlendingRootConstants = {};
            neighborhoodBlendingRootConstants.SetColorTexture( resourceStates, RHI::PipelineStage::PixelShader, pRenderViewport->m_ForwardShading_ColorTexture );
            neighborhoodBlendingRootConstants.SetBlendTexture( resourceStates, RHI::PipelineStage::PixelShader, pRenderViewport->m_SMAA_BlendTexture );
            resourceStates.FlushBarriers( pCommandBuffer );

            neighborhoodBlendingRootConstants.m_rtMetrics[0] = 1.0F / viewportSize.m_x;
            neighborhoodBlendingRootConstants.m_rtMetrics[1] = 1.0F / viewportSize.m_y;
            neighborhoodBlendingRootConstants.m_rtMetrics[2] = viewportSize.m_x;
            neighborhoodBlendingRootConstants.m_rtMetrics[3] = viewportSize.m_y;

            RHI::CmdSetPipeline( pCommandBuffer, m_pPipelineNeighborhoodBlending );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &neighborhoodBlendingRootConstants, sizeof( neighborhoodBlendingRootConstants ) );
            RHI::CmdSetStencilReference( pCommandBuffer, 0 );
            RHI::CmdDraw( pCommandBuffer, 3, 0 );
        }

        RHI::CmdSetRenderTargets( pCommandBuffer, {}, nullptr );
    }
}
