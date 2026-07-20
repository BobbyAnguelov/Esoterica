
#include "RenderPass_PostProcess.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/RenderViewport.h"
#include "Engine/Render/Shaders/PostProcess.esf"
#include "Base/Render/RHI.h"
#include "Base/Render/RenderWindow.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    void PostProcessPass::Initialize( RenderPassContext const& context )
    {
        static StringID s_PostProcessShaderID = StringID( "PostProcess" );

        SurfaceShader const* pPostProcessShader = context.m_pRenderSystem->FindSurfaceShader( s_PostProcessShaderID );

        RHI::DataFormat pipelineCombinedColorFormats[] = { RHI::DataFormat::RGBA8_sRGB };

        RHI::GraphicsPipelineParameters pipelineCombinedParameters = {};
        pipelineCombinedParameters.m_colorFormats = pipelineCombinedColorFormats;
        pipelineCombinedParameters.m_numRenderTargets = 1;
        pipelineCombinedParameters.m_pShader = pPostProcessShader->m_pShader;
        pipelineCombinedParameters.m_pRootSignature = pPostProcessShader->m_pRootSignature;
        pipelineCombinedParameters.m_debugName.sprintf( "%s Pipeline", pPostProcessShader->m_shaderName.c_str() );

        m_pPipelineCombined = RHI::CreatePipeline( context.m_pRenderSystem->GetContextRHI(), pipelineCombinedParameters );
    }

    void PostProcessPass::Shutdown( RenderSystem* pRenderSystem )
    {
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pPipelineCombined ) );
    }

    void PostProcessPass::UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport )
    {
        Int2 viewportSize = pRenderViewport->GetSize();
        uint32_t textureWidth = uint32_t( viewportSize.m_x );
        uint32_t textureHeight = uint32_t( viewportSize.m_y );

        if ( pRenderViewport->IsStandalone() )
        {
            pRenderViewport->m_finalTexture.m_pTexture = pRenderViewport->m_pWindow->GetCurrentRenderTarget();
            pRenderViewport->m_finalTexture.m_currentSync = RHI::PipelineStage::Draw;
            pRenderViewport->m_finalTexture.m_currentAccess = RHI::ResourceAccess::RenderTarget;
            pRenderViewport->m_finalTexture.m_currentState = RHI::TextureState::RenderTarget;
        }

        #if EE_DEVELOPMENT_TOOLS
        if ( !pRenderViewport->m_finalTexture || textureWidth != pRenderViewport->m_finalTexture->m_width || textureHeight != pRenderViewport->m_finalTexture->m_height )
        {
            pRenderSystem->QueueResourceDelete( eastl::move( pRenderViewport->m_finalTexture ) );

            RHI::TextureParameters finalTextureParameters = {};
            finalTextureParameters.m_width = textureWidth;
            finalTextureParameters.m_height = textureHeight;
            finalTextureParameters.m_format = RHI::DataFormat::RGBA8_sRGB;
            finalTextureParameters.m_descriptorTypes = TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::RenderTarget, RHI::DescriptorTypeFlags::Texture );
            finalTextureParameters.m_debugName.sprintf( "Viewport Final Texture %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_finalTexture = RHI::CreateTexture( pRenderSystem->GetContextRHI(), finalTextureParameters );
        }
        #endif
    }

    void PostProcessPass::DrawToViewport( DeviceResourceStates&     resourceStates,
                                          RHI::CommandBuffer*       pCommandBuffer,
                                          RHI::Texture*             pTonemapLUT,
                                          DeviceTextureState&       hdrTexture,
                                          DeviceTextureState&       depthTexture ) const
    {
        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Post Processing" );

        EE_ASSERT( !resourceStates.HasPendingBarriers() );

        ShaderTypes::PostProcessResourceTableData postProcessRootConstants = {};
        postProcessRootConstants.SetTonemapLUT( pTonemapLUT );
        postProcessRootConstants.SetHDRTexture( resourceStates, RHI::PipelineStage::PixelShader, hdrTexture );
        postProcessRootConstants.SetDepthTexture( resourceStates, RHI::PipelineStage::PixelShader, depthTexture );

        resourceStates.FlushBarriers( pCommandBuffer );

        RHI::CmdSetPipeline( pCommandBuffer, m_pPipelineCombined );
        RHI::CmdSetRootConstants( pCommandBuffer, 0, &postProcessRootConstants, sizeof( postProcessRootConstants ) );
        RHI::CmdDraw( pCommandBuffer, 3, 0 );
    }
}
