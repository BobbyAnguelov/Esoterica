#include "RenderPass_DepthDownsample.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/RenderViewport.h"
#include "Engine/Render/Shaders/Downsample.esf"

namespace EE::Render
{
    void DepthDownsamplePass::Initialize( RenderPassContext const& context )
    {
        m_linearMaxClampSampler = RHI::GetSamplerStateHandle( context.m_pRenderSystem->GetLinearClampMaxSampler() );

        static StringID s_DownsampleShaderID = StringID( "Downsample" );

        SurfaceShader const* pDownsampleShader = context.m_pRenderSystem->FindSurfaceShader( s_DownsampleShaderID );

        RHI::DataFormat pipelineColorFormats[] = { RHI::DataFormat::R32_SFloat };

        RHI::GraphicsPipelineParameters pipelineParameters = {};
        pipelineParameters.m_colorFormats = pipelineColorFormats;
        pipelineParameters.m_numRenderTargets = 1;
        pipelineParameters.m_pShader = pDownsampleShader->m_pShader;
        pipelineParameters.m_pRootSignature = pDownsampleShader->m_pRootSignature;
        pipelineParameters.m_debugName.sprintf( "%s Pipeline", pDownsampleShader->m_shaderName.c_str() );

        m_pDownsamplePipeline = RHI::CreatePipeline( context.m_pRenderSystem->GetContextRHI(), pipelineParameters );
    }

    void DepthDownsamplePass::Shutdown( RenderSystem* pRenderSystem )
    {
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pDownsamplePipeline ) );
    }

    void DepthDownsamplePass::UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport )
    {
        Int2 viewportSize = pRenderViewport->GetSize();
        uint32_t textureWidth = uint32_t( viewportSize.m_x );
        uint32_t textureHeight = uint32_t( viewportSize.m_y );

        uint32_t depthDownsampleWidth2 = textureWidth / 2;
        uint32_t depthDownsampleHeight2 = textureHeight / 2;

        uint32_t depthDownsampleWidth4 = textureWidth / 4;
        uint32_t depthDownsampleHeight4 = textureHeight / 4;

        uint32_t depthDownsampleWidth8 = textureWidth / 8;
        uint32_t depthDownsampleHeight8 = textureHeight / 8;

        if ( !pRenderViewport->m_DepthDownsample2 || depthDownsampleWidth2 != pRenderViewport->m_DepthDownsample2->m_width || depthDownsampleHeight2 != pRenderViewport->m_DepthDownsample2->m_height )
        {
            pRenderSystem->QueueResourceDelete
            (
                eastl::move( pRenderViewport->m_DepthDownsample2 ),
                eastl::move( pRenderViewport->m_DepthDownsample4 ),
                eastl::move( pRenderViewport->m_DepthDownsample8 )
            );

            RHI::TextureParameters depthTextureParameters = {};
            depthTextureParameters.m_format = RHI::DataFormat::R32_SFloat;
            depthTextureParameters.m_descriptorTypes = TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::RenderTarget, RHI::DescriptorTypeFlags::Texture );

            depthTextureParameters.m_width = depthDownsampleWidth2;
            depthTextureParameters.m_height = depthDownsampleHeight2;
            depthTextureParameters.m_debugName.sprintf( "Half resolution depth %dx%d", depthTextureParameters.m_width, depthTextureParameters.m_height );
            pRenderViewport->m_DepthDownsample2 = RHI::CreateTexture( pRenderSystem->GetContextRHI(), depthTextureParameters );

            depthTextureParameters.m_width = depthDownsampleWidth4;
            depthTextureParameters.m_height = depthDownsampleHeight4;
            depthTextureParameters.m_debugName.sprintf( "Quarter resolution depth %dx%d", depthTextureParameters.m_width, depthTextureParameters.m_height );
            pRenderViewport->m_DepthDownsample4 = RHI::CreateTexture( pRenderSystem->GetContextRHI(), depthTextureParameters );

            depthTextureParameters.m_width = depthDownsampleWidth8;
            depthTextureParameters.m_height = depthDownsampleHeight8;
            depthTextureParameters.m_debugName.sprintf( "Eighth resolution depth %dx%d", depthTextureParameters.m_width, depthTextureParameters.m_height );
            pRenderViewport->m_DepthDownsample8 = RHI::CreateTexture( pRenderSystem->GetContextRHI(), depthTextureParameters );
        }

    }

    void DepthDownsamplePass::DrawToViewport( RenderViewport const* pRenderViewport, DeviceResourceStates& resourceStates, RHI::CommandBuffer* pCommandBuffer, DeviceTextureState& depthBuffer ) const
    {
        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Depth downsample" );

        RHI::LoadAction loadAction = {};
        loadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;

        //---------------------------------------------------------------------------------------------------
        // 1x -> 2x

        EE_ASSERT( !resourceStates.HasPendingBarriers() );
        resourceStates.Writeable( pRenderViewport->m_DepthDownsample2, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
        resourceStates.FlushBarriers( pCommandBuffer );

        RHI::CmdSetRenderTargets( pCommandBuffer, { &pRenderViewport->m_DepthDownsample2.m_pTexture, 1 }, nullptr, &loadAction );
        RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( pRenderViewport->m_DepthDownsample2->m_width ), float( pRenderViewport->m_DepthDownsample2->m_height ), 0.0F, 1.0F );
        RHI::CmdSetScissor( pCommandBuffer, 0, 0, pRenderViewport->m_DepthDownsample2->m_width, pRenderViewport->m_DepthDownsample2->m_height );

        ShaderTypes::DownsampleResourceTableData rootConstants = {};
        rootConstants.SetInputTexture( resourceStates, RHI::PipelineStage::PixelShader, depthBuffer );
        rootConstants.SetInputSampler( m_linearMaxClampSampler );
        resourceStates.FlushBarriers( pCommandBuffer );

        RHI::CmdSetPipeline( pCommandBuffer, m_pDownsamplePipeline );
        RHI::CmdSetRootConstants( pCommandBuffer, 0, &rootConstants, sizeof( rootConstants ) );
        RHI::CmdDraw( pCommandBuffer, 3, 0 );

        //---------------------------------------------------------------------------------------------------
        // 2x -> 4x

        EE_ASSERT( !resourceStates.HasPendingBarriers() );
        resourceStates.ReadOnly( pRenderViewport->m_DepthDownsample2, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
        resourceStates.Writeable( pRenderViewport->m_DepthDownsample4, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
        resourceStates.FlushBarriers( pCommandBuffer );

        RHI::CmdSetRenderTargets( pCommandBuffer, { &pRenderViewport->m_DepthDownsample4.m_pTexture, 1 }, nullptr, &loadAction );
        RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( pRenderViewport->m_DepthDownsample4->m_width ), float( pRenderViewport->m_DepthDownsample4->m_height ), 0.0F, 1.0F );
        RHI::CmdSetScissor( pCommandBuffer, 0, 0, pRenderViewport->m_DepthDownsample4->m_width, pRenderViewport->m_DepthDownsample4->m_height );

        rootConstants.SetInputTexture( RHI::GetTextureHandle( pRenderViewport->m_DepthDownsample2, RHI::DescriptorTypeFlags::Texture, 0 ) );
        RHI::CmdSetRootConstants( pCommandBuffer, 0, &rootConstants, sizeof( rootConstants ) );
        RHI::CmdDraw( pCommandBuffer, 3, 0 );

        //---------------------------------------------------------------------------------------------------
        // 4x -> 8x
        EE_ASSERT( !resourceStates.HasPendingBarriers() );
        resourceStates.ReadOnly( pRenderViewport->m_DepthDownsample4, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
        resourceStates.Writeable( pRenderViewport->m_DepthDownsample8, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
        resourceStates.FlushBarriers( pCommandBuffer );

        RHI::CmdSetRenderTargets( pCommandBuffer, { &pRenderViewport->m_DepthDownsample8.m_pTexture, 1 }, nullptr, &loadAction );
        RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( pRenderViewport->m_DepthDownsample8->m_width ), float( pRenderViewport->m_DepthDownsample8->m_height ), 0.0F, 1.0F );
        RHI::CmdSetScissor( pCommandBuffer, 0, 0, pRenderViewport->m_DepthDownsample8->m_width, pRenderViewport->m_DepthDownsample8->m_height );

        rootConstants.SetInputTexture( RHI::GetTextureHandle( pRenderViewport->m_DepthDownsample4, RHI::DescriptorTypeFlags::Texture, 0 ) );
        RHI::CmdSetRootConstants( pCommandBuffer, 0, &rootConstants, sizeof( rootConstants ) );
        RHI::CmdDraw( pCommandBuffer, 3, 0 );
    }
}
