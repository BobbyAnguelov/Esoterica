#include "RenderPass_GlobalEnvironmentMap.h"
#include "RenderPass_ForwardShading.h"
#include "Base/Memory/Memory.h"
#include "Base/Render/RHI.h"
#include "Base/Math/ViewVolume.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/RenderViewport.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"

#include "Engine/Render/Shaders/PBR/IrradianceShProject.esf"
#include "Engine/Render/Shaders/PBR/ProbeTableRadiance.esf"
#include "Engine/Render/Shaders/PBR/CubemapDownsample.esf"

//-------------------------------------------------------------------------

namespace EE::Render
{
    static const uint32_t g_CaptureResolution = 128;
    static const uint32_t g_CaptureReducedLevels = RHI::ComputeUncompressedMipLevels( g_CaptureResolution, g_CaptureResolution, 1 ) - 1;

    static const uint32_t g_RadianceResolution = 128;
    static const uint32_t g_RadianceMipLevels = g_CaptureReducedLevels;

    static constexpr uint32_t g_NumShCoefficients = 9;

    void GlobalEnvironmentMapPass::Initialize( RenderPassContext const& context )
    {
        //-------------------------------------------------------------------------

        static StringID s_IrradianceShProjectShaderID = StringID( "IrradianceSHProject" );
        static StringID s_ProbeTableRadianceShaderID = StringID( "ProbeTableRadiance" );
        static StringID s_CubemapDownsampleShaderID = StringID( "CubemapDownsample" );

        SurfaceShader const* pCubemapDownsampleShader = context.m_pRenderSystem->FindSurfaceShader( s_CubemapDownsampleShaderID );

        m_pIrradianceSHProjectShader = context.m_pRenderSystem->FindComputeShader( s_IrradianceShProjectShaderID );
        m_pProbeTableRadianceShader = context.m_pRenderSystem->FindComputeShader( s_ProbeTableRadianceShaderID );

        RHI::Context* pContextRHI = context.m_pRenderSystem->GetContextRHI();

        //-------------------------------------------------------------------------

        RHI::DataFormat cubemapDownsampleColorFormats[] = { RHI::DataFormat::RG11_B10_UFloat };

        RHI::GraphicsPipelineParameters cubemapDownsamplePipelineParameters = {};
        cubemapDownsamplePipelineParameters.m_numRenderTargets = 1;

        cubemapDownsamplePipelineParameters.m_colorFormats = cubemapDownsampleColorFormats;
        cubemapDownsamplePipelineParameters.m_pShader = pCubemapDownsampleShader->m_pShader;
        cubemapDownsamplePipelineParameters.m_pRootSignature = pCubemapDownsampleShader->m_pRootSignature;
        cubemapDownsamplePipelineParameters.m_debugName = "CubemapDownsample Pipeline";

        m_pPipelineCubemapDownsample = RHI::CreatePipeline( pContextRHI, cubemapDownsamplePipelineParameters );
    }

    void GlobalEnvironmentMapPass::Shutdown( RenderSystem* pRenderSystem )
    {
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pPipelineCubemapDownsample ) );
    }

    void GlobalEnvironmentMapPass::UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport )
    {
        EE_PROFILE_FUNCTION_RENDER();

        RHI::Context* pContextRHI = pRenderSystem->GetContextRHI();

        DeviceRenderView& view = pRenderViewport->m_globalEnvironmentMapRenderViewProxy.GetRenderView( 0 );

        //-------------------------------------------------------------------------

        if ( !view.m_colorTexture )
        {
            RHI::TextureParameters renderTargetParameters = {};

            renderTargetParameters.m_width = g_CaptureResolution;
            renderTargetParameters.m_height = g_CaptureResolution;
            renderTargetParameters.m_arrayLayers = 6;
            renderTargetParameters.m_mipLevels = RHI::ComputeUncompressedMipLevels( g_CaptureResolution, g_CaptureResolution, 1 );
            renderTargetParameters.m_format = RHI::DataFormat::RG11_B10_UFloat;
            renderTargetParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::TextureCube, RHI::DescriptorTypeFlags::RenderTarget };
            renderTargetParameters.m_clearValue = { { 0.0F, 0.0F, 0.0F, 1.0F } };
            renderTargetParameters.m_debugName = "GlobalEnvironmentMap Capture Target";

            view.m_colorTexture = RHI::CreateTexture( pContextRHI, renderTargetParameters );

            renderTargetParameters.m_width = g_CaptureResolution;
            renderTargetParameters.m_height = g_CaptureResolution;
            renderTargetParameters.m_mipLevels = 1;
            renderTargetParameters.m_arrayLayers = 1;
            renderTargetParameters.m_format = RHI::DataFormat::D32_SFloat;
            renderTargetParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::RenderTarget;
            renderTargetParameters.m_clearValue = {};
            renderTargetParameters.m_initialState = RHI::TextureState::DepthWrite;
            renderTargetParameters.m_debugName = "GlobalEnvironmentMap DepthTarget";

            view.m_depthTexture = RHI::CreateTexture( pContextRHI, renderTargetParameters );

            renderTargetParameters.m_width = g_RadianceResolution;
            renderTargetParameters.m_height = g_RadianceResolution;
            renderTargetParameters.m_mipLevels = g_RadianceMipLevels;
            renderTargetParameters.m_arrayLayers = 6;
            renderTargetParameters.m_format = RHI::DataFormat::RGBA16_SFloat;
            renderTargetParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::TextureCube, RHI::DescriptorTypeFlags::RWTexture };
            renderTargetParameters.m_clearValue = { { 0.0F, 0.0F, 0.0F, 1.0F } };
            renderTargetParameters.m_initialState = RHI::TextureState::ShaderResource;
            renderTargetParameters.m_debugName = "GlobalEnvironmentMap Radiance Target";

            view.m_pEnvironmentMapRadianceTexture = RHI::CreateTexture( pContextRHI, renderTargetParameters );
        }

        if ( !view.m_pSHCoefficientsBuffer )
        {
            RHI::BufferParameters shCoefficientsParameters = {};
            shCoefficientsParameters.m_bufferSize = g_NumShCoefficients * 3 * sizeof( float );
            shCoefficientsParameters.m_bufferStride = g_NumShCoefficients * 3 * sizeof( float );
            shCoefficientsParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer };
            shCoefficientsParameters.m_debugName = "GlobalEnvironmentMap SH Coefficients";

            view.m_pSHCoefficientsBuffer = RHI::CreateBuffer( pContextRHI, shCoefficientsParameters );
        }
    }

    void GlobalEnvironmentMapPass::UpdateWorldDeviceResources( EntityWorld* pWorld )
    {
        EE_PROFILE_FUNCTION_RENDER();

        Vector viewPosition = Vector::Zero;
        viewPosition.SetW1();

        for ( Viewport* pViewport : pWorld->GetViewports() )
        {
            if ( !pViewport->IsValid() )
            {
                continue;
            }

            RenderViewport* pRenderViewport = static_cast<RenderViewport*>( pViewport );
            RenderViewProxy& renderViewProxy = pRenderViewport->m_globalEnvironmentMapRenderViewProxy;

            EE_ASSERT( renderViewProxy.IsValid() );
            EE_ASSERT( renderViewProxy.GetNumRenderViews() == g_NumGlobalEnvironmentMapViews );

            renderViewProxy.StartRenderViewWrite();

            for ( uint32_t renderViewIndex = 0; renderViewIndex < g_NumGlobalEnvironmentMapViews; ++renderViewIndex )
            {
                renderViewProxy.WriteGlobalEnvironmentMapRenderView( renderViewIndex, viewPosition, 0.1F, 1000.0F, g_CaptureResolution );
            }

            renderViewProxy.SubmitRenderViewWrite();
        }
    }

    void GlobalEnvironmentMapPass::CaptureGlobalEnvironmentMap
    (
        TArrayView<ForwardShadingMaterialShaderPipelineBucket>    materialShaderPipelineBuckets,
        ActiveRenderViewList const&                               activeRenderViewList,
        DeviceRenderWorld const&                                  deviceRenderWorld,
        RenderViewport const*                                     pRenderViewport,
        RHI::Buffer*                                              pGlobalParametersBuffer,
        RHI::CommandBuffer*                                       pCommandBuffer
    ) const
    {
        EE_PROFILE_FUNCTION_RENDER();

        DeviceRenderView const* const pGlobalEnvironmentMapView = &pRenderViewport->m_globalEnvironmentMapRenderViewProxy.m_renderViewHandle.m_data[0];
        uint32_t const deviceRenderViewBaseIndex = pRenderViewport->m_globalEnvironmentMapRenderViewProxy.GetBaseRenderViewIndex();

        RHI::Texture* pCaptureRenderTarget = pGlobalEnvironmentMapView->m_colorTexture;
        RHI::Texture* pDepthRenderTarget = pGlobalEnvironmentMapView->m_depthTexture;

        uint32_t const activeRenderViewIndex = activeRenderViewList.FindActiveRenderViewIndex( deviceRenderViewBaseIndex );
        EE_ASSERT( activeRenderViewIndex != ActiveRenderViewList::s_InvalidActiveRenderViewIndex );
        EE_ASSERT( activeRenderViewIndex + g_NumGlobalEnvironmentMapViews <= activeRenderViewList.m_numActiveRenderViews );

        //-------------------------------------------------------------------------

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Global Environment Map Capture" );

            RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( g_CaptureResolution ), float( g_CaptureResolution ), 0.0F, 1.0F );
            RHI::CmdSetScissor( pCommandBuffer, 0, 0, g_CaptureResolution, g_CaptureResolution );

            for ( uint32_t renderViewIndex = 0; renderViewIndex < g_NumGlobalEnvironmentMapViews; ++renderViewIndex )
            {
                RHI::CmdBarrier
                (
                    pCommandBuffer, pCaptureRenderTarget,
                    RHI::PipelineStage::AllShader, RHI::PipelineStage::Draw,
                    RHI::ResourceAccess::ShaderResource, RHI::ResourceAccess::RenderTarget,
                    RHI::TextureState::ShaderResource, RHI::TextureState::RenderTarget,
                    { 0, 1, renderViewIndex, 1 }, {}
                );
            }

            for ( uint32_t renderViewIndex = 0; renderViewIndex < g_NumGlobalEnvironmentMapViews; ++renderViewIndex )
            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Cubemap Face" );

                ForwardShadingPass::DrawMaterialShaderBuckets
                (
                    materialShaderPipelineBuckets,
                    activeRenderViewList.GetActiveRenderView( activeRenderViewIndex + renderViewIndex ),
                    pCaptureRenderTarget,
                    renderViewIndex, 0,
                    pDepthRenderTarget,
                    pCommandBuffer
                );
            }

            for ( uint32_t renderViewIndex = 0; renderViewIndex < g_NumGlobalEnvironmentMapViews; ++renderViewIndex )
            {
                RHI::CmdBarrier
                (
                    pCommandBuffer, pCaptureRenderTarget,
                    RHI::PipelineStage::Draw, RHI::PipelineStage::AllShader,
                    RHI::ResourceAccess::RenderTarget, RHI::ResourceAccess::ShaderResource,
                    RHI::TextureState::RenderTarget, RHI::TextureState::ShaderResource,
                    { 0, 1, renderViewIndex, 1 }, {}
                );
            }
        }

        //-------------------------------------------------------------------------

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Global Environment Map Cubemap Downsample" );

            EE_ASSERT( pCaptureRenderTarget->m_mipLevels == g_CaptureReducedLevels + 1 );

            RHI::LoadAction reduceLoadAction = {};
            reduceLoadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;
            reduceLoadAction.m_colorClearValues[0] = pCaptureRenderTarget->m_clearValue;

            RHI::CmdSetPipeline( pCommandBuffer, m_pPipelineCubemapDownsample );

            for ( uint32_t mipLevel = 1; mipLevel <= g_CaptureReducedLevels; ++mipLevel )
            {
                RHI::CmdBarrier
                (
                    pCommandBuffer, pCaptureRenderTarget,
                    RHI::PipelineStage::AllShader, RHI::PipelineStage::Draw,
                    RHI::ResourceAccess::ShaderResource, RHI::ResourceAccess::RenderTarget,
                    RHI::TextureState::ShaderResource, RHI::TextureState::RenderTarget,
                    { mipLevel, 1, 0, 0 }, {}
                );

                uint32_t const resolution = g_CaptureResolution >> mipLevel;

                RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( resolution ), float( resolution ), 0.0F, 1.0F );
                RHI::CmdSetScissor( pCommandBuffer, 0, 0, resolution, resolution );

                for ( uint32_t face = 0; face < g_NumGlobalEnvironmentMapViews; ++face )
                {
                    uint32_t const colorArraySlices[] = { face };
                    uint32_t const colorMipSlices[] = { mipLevel };

                    RHI::CmdSetRenderTargets
                    (
                        pCommandBuffer, { &pCaptureRenderTarget, 1 }, nullptr, &reduceLoadAction,
                        { colorArraySlices, 1 }, { colorMipSlices, 1 }
                    );

                    ShaderTypes::CubemapDownsampleResourceTableData cubemapDownsampleRootConstants = {};

                    cubemapDownsampleRootConstants.SetInputTexture( RHI::GetTextureHandle( pCaptureRenderTarget, RHI::DescriptorTypeFlags::TextureCube, 0 ) );
                    cubemapDownsampleRootConstants.SetRenderViewBuffer( deviceRenderWorld.GetRenderViewBufferHandle() );
                    cubemapDownsampleRootConstants.m_renderViewIndex = deviceRenderViewBaseIndex + face;
                    cubemapDownsampleRootConstants.m_sourceLevel = mipLevel - 1;

                    RHI::CmdSetRootConstants( pCommandBuffer, 0, &cubemapDownsampleRootConstants, sizeof( cubemapDownsampleRootConstants ) );
                    RHI::CmdDraw( pCommandBuffer, 3, 0 );
                }

                RHI::CmdBarrier
                (
                    pCommandBuffer, pCaptureRenderTarget,
                    RHI::PipelineStage::Draw, RHI::PipelineStage::AllShader,
                    RHI::ResourceAccess::RenderTarget, RHI::ResourceAccess::ShaderResource,
                    RHI::TextureState::RenderTarget, RHI::TextureState::ShaderResource,
                    { mipLevel, 1, 0, 0 }, {}
                );
            }
        }


        //-------------------------------------------------------------------------

        RHI::CmdBarrier( pCommandBuffer, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::ShaderResource );
    }

    //-------------------------------------------------------------------------

    void GlobalEnvironmentMapPass::FilterEnvironmentMap
    (
        DeviceRenderWorld const&    deviceRenderWorld,
        RenderViewport const*       pRenderViewport,
        RHI::CommandBuffer*         pCommandBuffer,
        RHI::Buffer*                pProbeTableBuffer
    ) const
    {
        EE_PROFILE_FUNCTION_RENDER();

        DeviceRenderView const* const pGlobalEnvironmentMapView = &pRenderViewport->m_globalEnvironmentMapRenderViewProxy.m_renderViewHandle.m_data[0];
        uint32_t const deviceRenderViewBaseIndex = pRenderViewport->m_globalEnvironmentMapRenderViewProxy.GetBaseRenderViewIndex();

        RHI::Texture* const pCaptureRenderTarget = pGlobalEnvironmentMapView->m_colorTexture;
        RHI::Texture* const pRadianceRenderTarget = pGlobalEnvironmentMapView->m_pEnvironmentMapRadianceTexture;

        RHI::CmdBarrier
        (
            pCommandBuffer, pRadianceRenderTarget,
            RHI::PipelineStage::AllShader, RHI::PipelineStage::ComputeShader,
            RHI::ResourceAccess::ShaderResource, RHI::ResourceAccess::UnorderedAccess,
            RHI::TextureState::ShaderResource, RHI::TextureState::UnorderedAccess,
            {}, {}
        );

        //-------------------------------------------------------------------------

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Global Environment Map Irradiance Projection" );

            ShaderTypes::IrradianceSHProjectResourceTableData projectRootConstants = {};
            projectRootConstants.m_renderViewIndex = deviceRenderViewBaseIndex;
            projectRootConstants.SetInputTexture( RHI::GetTextureHandle( pCaptureRenderTarget, RHI::DescriptorTypeFlags::TextureCube, 0 ) );
            projectRootConstants.SetShCoefficients( pGlobalEnvironmentMapView->m_pSHCoefficientsBuffer );
            projectRootConstants.SetRenderViewBuffer( deviceRenderWorld.GetRenderViewBufferHandle() );

            RHI::CmdSetPipeline( pCommandBuffer, m_pIrradianceSHProjectShader->m_pPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &projectRootConstants, sizeof( projectRootConstants ) );
            RHI::CmdDispatchCompute( pCommandBuffer, 1, 1, 1 );
        }

        //-------------------------------------------------------------------------

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Global Environment Map Radiance Filtering (table)" );

            RHI::CmdSetPipeline( pCommandBuffer, m_pProbeTableRadianceShader->m_pPipeline );

            for ( uint32_t mipLevel = 0; mipLevel < g_RadianceMipLevels; ++mipLevel )
            {
                ShaderTypes::ProbeTableRadianceResourceTableData rootConstants = {};

                rootConstants.SetInputTexture( RHI::GetTextureHandle( pCaptureRenderTarget, RHI::DescriptorTypeFlags::TextureCube, 0 ) );
                rootConstants.SetProbeTable( RHI::GetBufferHandle( pProbeTableBuffer, RHI::DescriptorTypeFlags::Buffer ) );
                rootConstants.SetRenderViewBuffer( deviceRenderWorld.GetRenderViewBufferHandle() );
                rootConstants.SetOutputTexture( RHI::GetTextureHandle( pRadianceRenderTarget, RHI::DescriptorTypeFlags::RWTexture, mipLevel ) );
                rootConstants.m_renderViewIndex = deviceRenderViewBaseIndex;
                rootConstants.m_level = mipLevel;

                uint32_t const resolution = g_RadianceResolution >> mipLevel;
                uint32_t const numGroups = ( resolution + 7 ) / 8;

                RHI::CmdSetRootConstants( pCommandBuffer, 0, &rootConstants, sizeof( rootConstants ) );
                RHI::CmdDispatchCompute( pCommandBuffer, numGroups, numGroups, g_NumGlobalEnvironmentMapViews );
            }
        }

        //-------------------------------------------------------------------------

        RHI::CmdBarrier
        (
            pCommandBuffer, pRadianceRenderTarget,
            RHI::PipelineStage::ComputeShader, RHI::PipelineStage::AllShader,
            RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::ShaderResource,
            RHI::TextureState::UnorderedAccess, RHI::TextureState::ShaderResource,
            {}, {}
        );
    }
}
