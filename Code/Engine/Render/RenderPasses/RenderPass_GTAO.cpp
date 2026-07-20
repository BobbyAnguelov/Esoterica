#include "RenderPass_GTAO.h"

#include "Base/Render/RHI.h"
#include "Base/Render/RenderWindow.h"
#include "Base/Math/ViewVolume.h"

#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/RenderMaterialShaderClusterCapacity.h"
#include "Engine/Render/RenderViewport.h"

#include "Engine/Render/Shaders/GTAO/PrefilterDepth.esf"
#include "Engine/Render/Shaders/GTAO/MainPass.esf"
#include "Engine/Render/Shaders/GTAO/Denoise.esf"
#include "Engine/Render/Shaders/GTAO/XeGTAO_Types.esh"

#include "Engine/Render/Shaders/BilateralUpsample.esf"

//-------------------------------------------------------------------------

namespace EE::Render
{
    static const XeGTAO::GTAOSettings g_XeGTAO_Settings = {};

    void GTAOPass::Initialize( RenderPassContext const& context )
    {
        static StringID s_XeGTAOPrefilterDepthShaderID = StringID( "PrefilterDepth" );
        static StringID s_XeGTAOMainPassShaderID = StringID( "MainPass" );
        static StringID s_XeGTAODenoiseShaderID = StringID( "Denoise" );

        m_pPrefilterDepthComputeShader = context.m_pRenderSystem->FindComputeShader( s_XeGTAOPrefilterDepthShaderID );
        m_pMainPassComputeShader = context.m_pRenderSystem->FindComputeShader( s_XeGTAOMainPassShaderID );
        m_pDenoiseComputeShader = context.m_pRenderSystem->FindComputeShader( s_XeGTAODenoiseShaderID );

        RHI::DataFormat pipelineColorFormats[] = { RHI::DataFormat::R16_SFloat };

        static StringID s_BilateralUpsampleShaderID = StringID( "BilateralUpsample" );

        SurfaceShader const* pBilateralUpsampleShader = context.m_pRenderSystem->FindSurfaceShader( s_BilateralUpsampleShaderID );

        RHI::GraphicsPipelineParameters pipelineParameters = {};
        pipelineParameters.m_colorFormats = pipelineColorFormats;
        pipelineParameters.m_numRenderTargets = 1;
        pipelineParameters.m_pShader = pBilateralUpsampleShader->m_pShader;
        pipelineParameters.m_pRootSignature = pBilateralUpsampleShader->m_pRootSignature;
        pipelineParameters.m_debugName.sprintf( "%s Pipeline", pBilateralUpsampleShader->m_shaderName.c_str() );

        m_pUpsamplePipeline = RHI::CreatePipeline( context.m_pRenderSystem->GetContextRHI(), pipelineParameters );
    }

    void GTAOPass::Shutdown( RenderSystem* pRenderSystem )
    {
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pUpsamplePipeline ) );
    }

    void GTAOPass::UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport )
    {
        uint32_t            frameIndex = pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = pRenderSystem->GetContextRHI();

        //---------------------------------------------------------------------------------------------------

        uint32_t fullResolutionWidth = pRenderViewport->m_ForwardShading_DepthTexture->m_width;
        uint32_t fullResolutionHeight = pRenderViewport->m_ForwardShading_DepthTexture->m_height;

        uint32_t depthWidth = 0;
        uint32_t depthHeight = 0;

        if ( m_enableLowResolution )
        {
            depthWidth = pRenderViewport->m_DepthDownsample4->m_width;
            depthHeight = pRenderViewport->m_DepthDownsample4->m_height;
        }
        else
        {
            depthWidth = pRenderViewport->m_ForwardShading_DepthTexture->m_width;
            depthHeight = pRenderViewport->m_ForwardShading_DepthTexture->m_height;
        }

        if ( !pRenderViewport->m_GTAO_ResultTextureNoisy0 || pRenderViewport->m_GTAO_ResultTextureNoisy0->m_width != depthWidth || pRenderViewport->m_GTAO_ResultTextureNoisy0->m_height != depthHeight )
        {
            pRenderSystem->QueueResourceDelete( eastl::move( pRenderViewport->m_GTAO_ResultTextureNoisy0 ), eastl::move( pRenderViewport->m_GTAO_ResultTextureNoisy1 ) );

            RHI::TextureParameters resultTextureParameters = {};
            resultTextureParameters.m_width = depthWidth;
            resultTextureParameters.m_height = depthHeight;
            resultTextureParameters.m_format = RHI::DataFormat::R16_SFloat;
            resultTextureParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::RWTexture );
            resultTextureParameters.m_debugName = "GTAO Result Noisy 0";

            pRenderViewport->m_GTAO_ResultTextureNoisy0 = RHI::CreateTexture( pContextRHI, resultTextureParameters );

            resultTextureParameters.m_debugName = "GTAO Result Noisy 1";

            pRenderViewport->m_GTAO_ResultTextureNoisy1 = RHI::CreateTexture( pContextRHI, resultTextureParameters );
        }

        bool forceNewResultTexture = pRenderViewport->m_GTAO_ResultTexture && m_enableLowResolution && !pRenderViewport->m_GTAO_ResultTexture->m_descriptorTypes.IsFlagSet( RHI::DescriptorTypeFlags::RenderTarget );

        if ( forceNewResultTexture || !pRenderViewport->m_GTAO_ResultTexture || pRenderViewport->m_GTAO_ResultTexture->m_width != fullResolutionWidth || pRenderViewport->m_GTAO_ResultTexture->m_height != fullResolutionHeight )
        {
            pRenderSystem->QueueResourceDelete
            (
                eastl::move( pRenderViewport->m_GTAO_ResultTexture ),
                eastl::move( pRenderViewport->m_GTAO_ResultTextureHalfResolution )
            );

            RHI::TextureParameters resultTextureParameters = {};
            resultTextureParameters.m_width = fullResolutionWidth;
            resultTextureParameters.m_height = fullResolutionHeight;
            resultTextureParameters.m_format = RHI::DataFormat::R16_SFloat;
            resultTextureParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Texture, RHI::DescriptorTypeFlags::RWTexture );
            resultTextureParameters.m_debugName = "GTAO Result";

            if ( m_enableLowResolution )
            {
                resultTextureParameters.m_descriptorTypes.AppendFlags( RHI::DescriptorTypeFlags::RenderTarget );
            }

            pRenderViewport->m_GTAO_ResultTexture = RHI::CreateTexture( pContextRHI, resultTextureParameters );

            if ( m_enableLowResolution )
            {
                RHI::TextureParameters halfResResultTextureParameters = resultTextureParameters;
                halfResResultTextureParameters.m_width /= 2;
                halfResResultTextureParameters.m_height /= 2;
                halfResResultTextureParameters.m_debugName = "GTAO Result Half Resolution";

                pRenderViewport->m_GTAO_ResultTextureHalfResolution = RHI::CreateTexture( pContextRHI, halfResResultTextureParameters );
            }
        }

        if ( !pRenderViewport->m_GTAO_PrefilterDepthTexture || pRenderViewport->m_GTAO_PrefilterDepthTexture->m_width != depthWidth || pRenderViewport->m_GTAO_PrefilterDepthTexture->m_height != depthHeight )
        {
            pRenderSystem->QueueResourceDelete( eastl::move( pRenderViewport->m_GTAO_PrefilterDepthTexture ) );

            RHI::TextureParameters prefilterDepthTextureParameters = {};
            prefilterDepthTextureParameters.m_width = depthWidth;
            prefilterDepthTextureParameters.m_height = depthHeight;
            prefilterDepthTextureParameters.m_mipLevels = 5;
            prefilterDepthTextureParameters.m_format = RHI::DataFormat::R32_SFloat;
            prefilterDepthTextureParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::RWTexture );
            prefilterDepthTextureParameters.m_debugName = "GTAO Prefilter Depth";

            pRenderViewport->m_GTAO_PrefilterDepthTexture = RHI::CreateTexture( pContextRHI, prefilterDepthTextureParameters );
        }

        if ( !pRenderViewport->m_GTAO_EdgesTexture || pRenderViewport->m_GTAO_EdgesTexture->m_width != depthWidth || pRenderViewport->m_GTAO_EdgesTexture->m_height != depthHeight )
        {
            pRenderSystem->QueueResourceDelete( eastl::move( pRenderViewport->m_GTAO_EdgesTexture ) );

            RHI::TextureParameters edgesTextureParameters = {};
            edgesTextureParameters.m_width = depthWidth;
            edgesTextureParameters.m_height = depthHeight;
            edgesTextureParameters.m_format = RHI::DataFormat::R8_UNorm;
            edgesTextureParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::RWTexture );
            edgesTextureParameters.m_debugName = "GTAO Edges Texture";

            pRenderViewport->m_GTAO_EdgesTexture = RHI::CreateTexture( pContextRHI, edgesTextureParameters );
        }

        if ( !pRenderViewport->m_GTAO_parametersBuffers[frameIndex] )
        {
            RHI::BufferParameters gtaoBufferParameters = {};
            gtaoBufferParameters.m_bufferSize = sizeof( XeGTAO::GTAOConstants );
            gtaoBufferParameters.m_bufferStride = sizeof( XeGTAO::GTAOConstants );
            gtaoBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            gtaoBufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::ConstantBuffer;
            gtaoBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            gtaoBufferParameters.m_debugName.sprintf( "GTAO Parameters Buffer %i", frameIndex );

            pRenderViewport->m_GTAO_parametersBuffers[frameIndex] = RHI::CreateBuffer( pContextRHI, gtaoBufferParameters );
        }

        //---------------------------------------------------------------------------------------------------
        // GTAO per frame data

        Math::ViewVolume const& viewVolume = pRenderViewport->GetViewVolume();

        alignas( 32 ) XeGTAO::GTAOConstants constants = {};

        constants.ViewportSize.x = depthWidth;
        constants.ViewportSize.y = depthHeight;

        constants.ViewportPixelSize.x = 1.0f / float( depthWidth );
        constants.ViewportPixelSize.y = 1.0f / float( depthHeight );

        // We reversing here end and begin because we have reverse Z
        const float clipNear = viewVolume.GetDepthRange().m_end;
        const float clipFar = viewVolume.GetDepthRange().m_begin;

        float depthLinearizeMul = ( clipFar * clipNear ) / ( clipFar - clipNear );
        float depthLinearizeAdd = clipFar / ( clipFar - clipNear );

        constants.DepthUnpackConsts.x = depthLinearizeMul;
        constants.DepthUnpackConsts.y = depthLinearizeAdd;

        Matrix const& projectionMatrix = viewVolume.GetProjectionMatrix();

        float tanHalfFOVX = 1.0F / projectionMatrix[0][0];
        float tanHalfFOVY = 1.0F / projectionMatrix[1][1];

        constants.CameraTanHalfFOV.x = tanHalfFOVX;
        constants.CameraTanHalfFOV.y = tanHalfFOVY;

        constants.NDCToViewMul.x = constants.CameraTanHalfFOV.x * 2.0f;
        constants.NDCToViewMul.y = constants.CameraTanHalfFOV.y * -2.0f;

        constants.NDCToViewAdd.x = constants.CameraTanHalfFOV.x * -1.0f;
        constants.NDCToViewAdd.y = constants.CameraTanHalfFOV.y * 1.0f;

        constants.NDCToViewMul_x_PixelSize.x = constants.NDCToViewMul.x * constants.ViewportPixelSize.x;
        constants.NDCToViewMul_x_PixelSize.y = constants.NDCToViewMul.y * constants.ViewportPixelSize.y;

        constants.EffectRadius = g_XeGTAO_Settings.Radius;
        constants.EffectFalloffRange = g_XeGTAO_Settings.FalloffRange;
        constants.DenoiseBlurBeta = ( g_XeGTAO_Settings.DenoisePasses == 0 ) ? ( 1e4f ) : ( 1.2f );    // high value disables denoise - more elegant & correct way would be do set all edges to 0
        constants.RadiusMultiplier = g_XeGTAO_Settings.RadiusMultiplier;
        constants.SampleDistributionPower = g_XeGTAO_Settings.SampleDistributionPower;
        constants.ThinOccluderCompensation = g_XeGTAO_Settings.ThinOccluderCompensation;
        constants.FinalValuePower = g_XeGTAO_Settings.FinalValuePower;
        constants.DepthMIPSamplingOffset = g_XeGTAO_Settings.DepthMIPSamplingOffset;

        constants.NoiseIndex = 0;

        // When TAA is enabled uncomment this and alternate noise index every frame
        //constants.NoiseIndex = ( g_XeGTAO_Settings.DenoisePasses > 0 ) ? pRenderViewport->m_GTAO_noiseIndex : 0;
        //pRenderViewport->m_GTAO_noiseIndex = ( pRenderViewport->m_GTAO_noiseIndex + 1 ) % 64;

        Memory::CopyToWriteCombined( pRenderViewport->m_GTAO_parametersBuffers[frameIndex]->m_pMappedAddress_WriteCombined, &constants, sizeof( constants ) );
    }

    void GTAOPass::PrefilterDepth( RenderViewport const*    pRenderViewport,
                                   DeviceResourceStates&    resourceStates,
                                   RHI::CommandBuffer*      pCommandBuffer,
                                   uint32_t                 frameIndex )
    {
        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "GTAO Prefilter Depth" );

        EE_ASSERT( !resourceStates.HasPendingBarriers() );

        uint32_t depthWidth = 0;
        uint32_t depthHeight = 0;

        ShaderTypes::PrefilterDepthResourceTableData rootConstants = {};
        if ( m_enableLowResolution )
        {
            rootConstants.SetSrcRawDepth( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_DepthDownsample4 );

            depthWidth = pRenderViewport->m_DepthDownsample4->m_width;
            depthHeight = pRenderViewport->m_DepthDownsample4->m_height;
        }
        else
        {
            rootConstants.SetSrcRawDepth( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_ForwardShading_DepthTexture );

            depthWidth = pRenderViewport->m_ForwardShading_DepthTexture->m_width;
            depthHeight = pRenderViewport->m_ForwardShading_DepthTexture->m_height;
        }

        resourceStates.Writeable( pRenderViewport->m_GTAO_PrefilterDepthTexture, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::TextureState::UnorderedAccess );
        resourceStates.FlushBarriers( pCommandBuffer );

        RHI::TextureHandle mip0Handle = RHI::GetTextureHandle( pRenderViewport->m_GTAO_PrefilterDepthTexture, RHI::DescriptorTypeFlags::RWTexture, 0 );
        RHI::TextureHandle mip1Handle = RHI::GetTextureHandle( pRenderViewport->m_GTAO_PrefilterDepthTexture, RHI::DescriptorTypeFlags::RWTexture, 1 );
        RHI::TextureHandle mip2Handle = RHI::GetTextureHandle( pRenderViewport->m_GTAO_PrefilterDepthTexture, RHI::DescriptorTypeFlags::RWTexture, 2 );
        RHI::TextureHandle mip3Handle = RHI::GetTextureHandle( pRenderViewport->m_GTAO_PrefilterDepthTexture, RHI::DescriptorTypeFlags::RWTexture, 3 );
        RHI::TextureHandle mip4Handle = RHI::GetTextureHandle( pRenderViewport->m_GTAO_PrefilterDepthTexture, RHI::DescriptorTypeFlags::RWTexture, 4 );

        rootConstants.SetOutMip0( mip0Handle );
        rootConstants.SetOutMip1( mip1Handle );
        rootConstants.SetOutMip2( mip2Handle );
        rootConstants.SetOutMip3( mip3Handle );
        rootConstants.SetOutMip4( mip4Handle );

        RHI::CmdSetPipeline( pCommandBuffer, m_pPrefilterDepthComputeShader->m_pPipeline );
        RHI::CmdSetRootConstants( pCommandBuffer, 0, &rootConstants, sizeof( rootConstants ) );
        RHI::CmdSetRootParameter( pCommandBuffer, 1, pRenderViewport->m_GTAO_parametersBuffers[frameIndex], 0 );
        RHI::CmdDispatchCompute( pCommandBuffer, ( depthWidth + 15 ) / 16, ( depthHeight + 15 ) / 16, 1 );
    }

    void GTAOPass::ComputeNoisyResult( RenderViewport const*    pRenderViewport,
                                       DeviceResourceStates&    resourceStates,
                                       RHI::CommandBuffer*      pCommandBuffer,
                                       RHI::BufferHandle        renderViewBuffer,
                                       uint32_t                 mainCameraRenderView,
                                       uint32_t                 frameIndex )
    {
        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "GTAO Main Pass" );

        uint32_t depthWidth = 0;
        uint32_t depthHeight = 0;

        EE_ASSERT( !resourceStates.HasPendingBarriers() );

        ShaderTypes::MainPassResourceTableData rootConstants = {};

        if ( m_enableLowResolution )
        {
            rootConstants.SetDepthTexture( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_DepthDownsample4 );
            depthWidth = pRenderViewport->m_DepthDownsample4->m_width;
            depthHeight = pRenderViewport->m_DepthDownsample4->m_height;
        }
        else
        {
            rootConstants.SetDepthTexture( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_ForwardShading_DepthTexture );
            depthWidth = pRenderViewport->m_ForwardShading_DepthTexture->m_width;
            depthHeight = pRenderViewport->m_ForwardShading_DepthTexture->m_height;
        }

        rootConstants.SetEdgesTexture( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_GTAO_EdgesTexture, 0 );
        rootConstants.SetPrefilteredDepthTexture( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_GTAO_PrefilterDepthTexture );
        rootConstants.SetResultTextureNoisy( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_GTAO_ResultTextureNoisy0, 0 );
        resourceStates.FlushBarriers( pCommandBuffer );

        rootConstants.SetRenderViewBuffer( renderViewBuffer );
        rootConstants.m_mainCameraRenderView = mainCameraRenderView;

        RHI::CmdSetPipeline( pCommandBuffer, m_pMainPassComputeShader->m_pPipeline );
        RHI::CmdSetRootConstants( pCommandBuffer, 0, &rootConstants, sizeof( rootConstants ) );
        RHI::CmdSetRootParameter( pCommandBuffer, 1, pRenderViewport->m_GTAO_parametersBuffers[frameIndex], 0 );
        RHI::CmdDispatchCompute( pCommandBuffer, ( depthWidth + 7 ) / 8, ( depthHeight + 7 ) / 8, 1 );
    }

    void GTAOPass::Denoise( RenderViewport const*   pRenderViewport,
                            DeviceResourceStates&   resourceStates,
                            RHI::CommandBuffer*     pCommandBuffer,
                            bool                    asyncCompute,
                            uint32_t                frameIndex )
    {
        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "GTAO Denoise Pass" );

        uint32_t depthWidth = 0;
        uint32_t depthHeight = 0;

        if ( m_enableLowResolution )
        {
            depthWidth = pRenderViewport->m_DepthDownsample4->m_width;
            depthHeight = pRenderViewport->m_DepthDownsample4->m_height;
        }
        else
        {
            depthWidth = pRenderViewport->m_ForwardShading_DepthTexture->m_width;
            depthHeight = pRenderViewport->m_ForwardShading_DepthTexture->m_height;
        }

        EE_ASSERT( !resourceStates.HasPendingBarriers() );

        ShaderTypes::DenoiseResourceTableData rootConstants = {};
        rootConstants.SetEdgesTexture( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_GTAO_EdgesTexture );

        resourceStates.FlushBarriers( pCommandBuffer );

        rootConstants.m_finalApply = 0;

        //---------------------------------------------------------------------------------------------------
        // Pass 0
        {
            EE_ASSERT( !resourceStates.HasPendingBarriers() );

            rootConstants.SetResultTextureNoisy( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_GTAO_ResultTextureNoisy0 );
            rootConstants.SetResultTexture( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_GTAO_ResultTextureNoisy1, 0 );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::CmdSetPipeline( pCommandBuffer, m_pDenoiseComputeShader->m_pPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &rootConstants, sizeof( rootConstants ) );
            RHI::CmdSetRootParameter( pCommandBuffer, 1, pRenderViewport->m_GTAO_parametersBuffers[frameIndex], 0 );
            RHI::CmdDispatchCompute( pCommandBuffer, ( depthWidth + 7 ) / 8, ( depthHeight + 7 ) / 8, 1 );
        }

        //---------------------------------------------------------------------------------------------------
        // Pass 1
        {
            EE_ASSERT( !resourceStates.HasPendingBarriers() );

            rootConstants.SetResultTextureNoisy( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_GTAO_ResultTextureNoisy1 );
            rootConstants.SetResultTexture( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_GTAO_ResultTextureNoisy0, 0 );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::CmdSetRootConstants( pCommandBuffer, 0, &rootConstants, sizeof( rootConstants ) );
            RHI::CmdSetRootParameter( pCommandBuffer, 1, pRenderViewport->m_GTAO_parametersBuffers[frameIndex], 0 );
            RHI::CmdDispatchCompute( pCommandBuffer, ( depthWidth + 7 ) / 8, ( depthHeight + 7 ) / 8, 1 );
        }

        //---------------------------------------------------------------------------------------------------
        // Final pass
        {
            EE_ASSERT( !resourceStates.HasPendingBarriers() );
            rootConstants.SetResultTextureNoisy( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_GTAO_ResultTextureNoisy0 );
            if ( m_enableLowResolution )
            {
                rootConstants.SetResultTexture( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_GTAO_ResultTextureNoisy1, 0 );
            }
            else
            {
                rootConstants.SetResultTexture( resourceStates, RHI::PipelineStage::ComputeShader, pRenderViewport->m_GTAO_ResultTexture, 0 );
            }
            resourceStates.FlushBarriers( pCommandBuffer );

            rootConstants.m_finalApply = 1;

            RHI::CmdSetRootConstants( pCommandBuffer, 0, &rootConstants, sizeof( rootConstants ) );
            RHI::CmdSetRootParameter( pCommandBuffer, 1, pRenderViewport->m_GTAO_parametersBuffers[frameIndex], 0 );
            RHI::CmdDispatchCompute( pCommandBuffer, ( depthWidth + 7 ) / 8, ( depthHeight + 7 ) / 8, 1 );
        }
    }

    void GTAOPass::Upsample( RenderViewport const*  pRenderViewport,
                             DeviceResourceStates&  resourceStates,
                             RHI::CommandBuffer *   pCommandBuffer,
                             uint32_t               frameIndex )
    {

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "GTAO Bilateral Upsample" );

        RHI::LoadAction loadAction = {};
        loadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;

        {
            EE_ASSERT( !resourceStates.HasPendingBarriers() );

            resourceStates.Writeable( pRenderViewport->m_GTAO_ResultTextureHalfResolution, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::CmdSetRenderTargets( pCommandBuffer, { &pRenderViewport->m_GTAO_ResultTextureHalfResolution.m_pTexture, 1 }, nullptr, &loadAction );
            RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( pRenderViewport->m_GTAO_ResultTextureHalfResolution->m_width ), float( pRenderViewport->m_GTAO_ResultTextureHalfResolution->m_height ), 0.0F, 1.0F );
            RHI::CmdSetScissor( pCommandBuffer, 0, 0, pRenderViewport->m_GTAO_ResultTextureHalfResolution->m_width, pRenderViewport->m_GTAO_ResultTextureHalfResolution->m_height );

            EE_ASSERT( !resourceStates.HasPendingBarriers() );

            ShaderTypes::BilateralUpsampleResourceTableData rootConstants = {};
            rootConstants.SetFullResolutionDepthTexture( resourceStates, RHI::PipelineStage::PixelShader, pRenderViewport->m_DepthDownsample2 );
            rootConstants.SetLowResolutionDepthTexture( resourceStates, RHI::PipelineStage::PixelShader, pRenderViewport->m_DepthDownsample4 );
            rootConstants.SetSourceTexture( resourceStates, RHI::PipelineStage::PixelShader, pRenderViewport->m_GTAO_ResultTextureNoisy1 );
            resourceStates.FlushBarriers( pCommandBuffer );

            rootConstants.m_sourceResolution[0] = float( pRenderViewport->m_GTAO_ResultTextureNoisy1->m_width );
            rootConstants.m_sourceResolution[1] = float( pRenderViewport->m_GTAO_ResultTextureNoisy1->m_height );
            rootConstants.m_sourceResolution[2] = 1.0F / float( pRenderViewport->m_GTAO_ResultTextureNoisy1->m_width );
            rootConstants.m_sourceResolution[3] = 1.0F / float( pRenderViewport->m_GTAO_ResultTextureNoisy1->m_height );

            RHI::CmdSetPipeline( pCommandBuffer, m_pUpsamplePipeline );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &rootConstants, sizeof( rootConstants ) );
            RHI::CmdDraw( pCommandBuffer, 3, 0 );
        }

        {
            EE_ASSERT( !resourceStates.HasPendingBarriers() );

            resourceStates.Writeable( pRenderViewport->m_GTAO_ResultTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::CmdSetRenderTargets( pCommandBuffer, { &pRenderViewport->m_GTAO_ResultTexture.m_pTexture, 1 }, nullptr, &loadAction );
            RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( pRenderViewport->m_GTAO_ResultTexture->m_width ), float( pRenderViewport->m_GTAO_ResultTexture->m_height ), 0.0F, 1.0F );
            RHI::CmdSetScissor( pCommandBuffer, 0, 0, pRenderViewport->m_GTAO_ResultTexture->m_width, pRenderViewport->m_GTAO_ResultTexture->m_height );

            EE_ASSERT( !resourceStates.HasPendingBarriers() );

            ShaderTypes::BilateralUpsampleResourceTableData rootConstants = {};
            rootConstants.SetFullResolutionDepthTexture( resourceStates, RHI::PipelineStage::PixelShader, pRenderViewport->m_ForwardShading_DepthTexture );
            rootConstants.SetLowResolutionDepthTexture( resourceStates, RHI::PipelineStage::PixelShader, pRenderViewport->m_DepthDownsample2 );
            rootConstants.SetSourceTexture( resourceStates, RHI::PipelineStage::PixelShader, pRenderViewport->m_GTAO_ResultTextureHalfResolution );
            resourceStates.FlushBarriers( pCommandBuffer );

            rootConstants.m_sourceResolution[0] = float( pRenderViewport->m_GTAO_ResultTextureHalfResolution->m_width );
            rootConstants.m_sourceResolution[1] = float( pRenderViewport->m_GTAO_ResultTextureHalfResolution->m_height );
            rootConstants.m_sourceResolution[2] = 1.0F / float( pRenderViewport->m_GTAO_ResultTextureHalfResolution->m_width );
            rootConstants.m_sourceResolution[3] = 1.0F / float( pRenderViewport->m_GTAO_ResultTextureHalfResolution->m_height );

            RHI::CmdSetRootConstants( pCommandBuffer, 0, &rootConstants, sizeof( rootConstants ) );
            RHI::CmdDraw( pCommandBuffer, 3, 0 );
        }
    }
}
