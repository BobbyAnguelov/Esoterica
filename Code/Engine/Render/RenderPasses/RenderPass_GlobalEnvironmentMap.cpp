#include "RenderPass_GlobalEnvironmentMap.h"
#include "RenderPass_ForwardShading.h"
#include "Base/Render/RHI.h"
#include "Base/Math/ViewVolume.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"

#include "Engine/Render/Shaders/PBR/IrradianceFiltering.esf"
#include "Engine/Render/Shaders/PBR/RadianceFiltering.esf"
#include "Engine/Render/Shaders/PBR/CubemapDownsample.esf"

//-------------------------------------------------------------------------

namespace EE::Render
{
    static constexpr uint32_t g_DFGResolution = 128;
    static constexpr uint32_t g_CaptureResolution = 512;
    static constexpr uint32_t g_RadianceResolution = 128;
    static constexpr uint32_t g_IrradianceResolution = 32;

    void GlobalEnvironmentMapPass::Initialize( RenderPassContext const& context )
    {
        for ( DeviceRenderView& renderView : m_renderViews )
        {
            renderView.Initialize( context.m_pRenderSystem, context.m_materialShaderPipelineBuckets.size() );
        }

        //---------------------------------------------------------------------------------------------------

        static StringID s_IrradianceFilteringShaderID = StringID( "IrradianceFiltering" );
        static StringID s_RadianceFilteringShaderID = StringID( "RadianceFiltering" );
        static StringID s_PrecomputeDFGShaderID = StringID( "PrecomputeDFG" );
        static StringID s_CubemapDownsampleShaderID = StringID( "CubemapDownsample" );

        SurfaceShader const* pIrradianceFilteringShader = context.m_pRenderSystem->FindSurfaceShader( s_IrradianceFilteringShaderID );
        SurfaceShader const* pRadianceFilteringShader = context.m_pRenderSystem->FindSurfaceShader( s_RadianceFilteringShaderID );
        SurfaceShader const* pPrecomputeDFGShader = context.m_pRenderSystem->FindSurfaceShader( s_PrecomputeDFGShaderID );
        SurfaceShader const* pCubemapDownsampleShader = context.m_pRenderSystem->FindSurfaceShader( s_CubemapDownsampleShaderID );

        RHI::Context* pContextRHI = context.m_pRenderSystem->GetContextRHI();

        // TODO: Move pipeline creation to ESF parameters and create pipelines automatically
        RHI::DataFormat radianceColorFormats[] = { RHI::DataFormat::RGBA16_SFloat };
        RHI::DataFormat irradianceColorFormats[] = { RHI::DataFormat::RGBA32_SFloat };
        RHI::DataFormat dfgColorFormats[] = { RHI::DataFormat::RG16_SFloat };
        RHI::DataFormat captureColorFormats[] = { RHI::DataFormat::RG11_B10_UFloat };

        RHI::GraphicsPipelineParameters pipelineParameters = {};
        pipelineParameters.m_colorFormats = irradianceColorFormats;
        pipelineParameters.m_numRenderTargets = 1;
        pipelineParameters.m_pShader = pIrradianceFilteringShader->m_pShader;
        pipelineParameters.m_pRootSignature = pIrradianceFilteringShader->m_pRootSignature;
        pipelineParameters.m_debugName = "Irradiance Filtering Pipeline";

        m_pPipelineIrradianceFiltering = RHI::CreatePipeline( pContextRHI, pipelineParameters );

        pipelineParameters.m_colorFormats = radianceColorFormats;
        pipelineParameters.m_pShader = pRadianceFilteringShader->m_pShader;
        pipelineParameters.m_pRootSignature = pRadianceFilteringShader->m_pRootSignature;
        pipelineParameters.m_debugName = "Radiance Filtering Pipeline";

        m_pPipelineRadianceFiltering = RHI::CreatePipeline( pContextRHI, pipelineParameters );

        pipelineParameters.m_colorFormats = dfgColorFormats;
        pipelineParameters.m_pShader = pPrecomputeDFGShader->m_pShader;
        pipelineParameters.m_pRootSignature = pPrecomputeDFGShader->m_pRootSignature;
        pipelineParameters.m_debugName = "PrecomputeDFG Pipeline";

        m_pPipelinePrecomputeDFG = RHI::CreatePipeline( pContextRHI, pipelineParameters );

        pipelineParameters.m_colorFormats = captureColorFormats;
        pipelineParameters.m_pShader = pCubemapDownsampleShader->m_pShader;
        pipelineParameters.m_pRootSignature = pCubemapDownsampleShader->m_pRootSignature;
        pipelineParameters.m_debugName = "CubemapDownsample Pipeline";

        m_pPipelineCubemapDownsample = RHI::CreatePipeline( pContextRHI, pipelineParameters );
    }

    void GlobalEnvironmentMapPass::Shutdown( RenderSystem* pRenderSystem )
    {
        for ( DeviceRenderView & renderView : m_renderViews )
        {
            renderView.Shutdown( pRenderSystem );
        }

        RHI::DestroyTexture( pRenderSystem->GetContextRHI(), eastl::move( m_pCaptureRenderTarget ) );
        //RHI::DestroyTexture( pRenderSystem->GetContextRHI(), eastl::move( m_pRadianceRenderTarget ) );
        //RHI::DestroyTexture( pRenderSystem->GetContextRHI(), eastl::move( m_pIrradianceRenderTarget ) );
        RHI::DestroyTexture( pRenderSystem->GetContextRHI(), eastl::move( m_pDepthRenderTarget ) );
        RHI::DestroyTexture( pRenderSystem->GetContextRHI(), eastl::move( m_pDfgRenderTarget ) );

        //---------------------------------------------------------------------------------------------------

        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pPipelineIrradianceFiltering ) );
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pPipelineRadianceFiltering ) );
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pPipelinePrecomputeDFG ) );
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pPipelineCubemapDownsample ) );
    }

    void GlobalEnvironmentMapPass::UpdateDeviceResources( RenderSystem*                                             pRenderSystem,
                                                          TArrayView<ForwardShadingMaterialShaderPipelineBucket>    materialShaderPipelineBuckets,
                                                          TArrayView<uint32_t const>                                clusterCapacity )
    {
        EE_PROFILE_FUNCTION_RENDER();

        uint32_t            frameIndex = pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = pRenderSystem->GetContextRHI();

        if ( !m_pCaptureRenderTarget || !m_pDepthRenderTarget )
        {
            RHI::TextureParameters renderTargetParameters = {};

            renderTargetParameters.m_width = g_CaptureResolution;
            renderTargetParameters.m_height = g_CaptureResolution;
            renderTargetParameters.m_arrayLayers = 6;
            renderTargetParameters.m_mipLevels = RHI::ComputeTextureMipLevels( g_CaptureResolution, g_CaptureResolution, 1 );
            renderTargetParameters.m_format = RHI::DataFormat::RG11_B10_UFloat;
            renderTargetParameters.m_descriptorTypes = TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::TextureCube,
                                                                                            RHI::DescriptorTypeFlags::RenderTarget );
            renderTargetParameters.m_clearValue = { { 0.0F, 0.0F, 0.0F, 1.0F } };
            renderTargetParameters.m_debugName = "GlobalEnvironmentMap Capture Target";

            m_pCaptureRenderTarget = RHI::CreateTexture( pContextRHI, renderTargetParameters );

            renderTargetParameters.m_width = g_CaptureResolution;
            renderTargetParameters.m_height = g_CaptureResolution;
            renderTargetParameters.m_mipLevels = 1;
            renderTargetParameters.m_arrayLayers = 1;
            renderTargetParameters.m_format = RHI::DataFormat::D32_SFloat;
            renderTargetParameters.m_descriptorTypes = TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::RenderTarget );
            renderTargetParameters.m_clearValue = {};
            renderTargetParameters.m_initialState = RHI::TextureState::DepthWrite;
            renderTargetParameters.m_debugName = "GlobalEnvironmentMap DepthTarget";

            m_pDepthRenderTarget = RHI::CreateTexture( pContextRHI, renderTargetParameters );
        }

        for ( DeviceRenderView & renderView : m_renderViews )
        {
            renderView.UpdateDeviceResources( pRenderSystem, clusterCapacity );
        }
    }

    void GlobalEnvironmentMapPass::DrawAndFilterGlobalEnvironmentMap( TArrayView<ForwardShadingMaterialShaderPipelineBucket>    materialShaderPipelineBuckets,
                                                                      TArrayView<uint32_t const>                                clusterCapacity,
                                                                      RHI::Buffer*                                              pGlobalParametersBuffer,
                                                                      RHI::CommandBuffer*                                       pCommandBuffer,
                                                                      RHI::Texture*                                             pRadianceRenderTarget,
                                                                      RHI::Texture*                                             pIrradianceRenderTarget,
                                                                      RHI::BufferHandle                                         renderViewBufferHandle,
                                                                      uint32_t                                                  renderViewOffset,
                                                                      TArrayView<ShaderTypes::RenderView>                       dstRenderViews ) const
    {
        EE_PROFILE_FUNCTION_RENDER();

        Matrix mirrorMatrixX = Matrix::Identity;
        mirrorMatrixX.SetScale( Vector( -1.0F, 1.0F, 1.0F, 1.0F ) );

        Matrix mirrorMatrixY = Matrix::Identity;
        mirrorMatrixY.SetScale( Vector( 1.0F, -1.0F, 1.0F, 1.0F ) );

        Matrix projectionMatrix = Math::CreatePerspectiveProjectionMatrix( Math::DegreesToRadians * 90.0F, 1.0F, 0.1F, 1000.0F );

        Vector viewPosition = Vector::Zero;
        viewPosition.SetW1();

        TArray<Matrix, NumRenderViews> viewMatrices;
        viewMatrices[0] = Math::CreateLookAtMatrix( viewPosition, Vector::UnitX, Vector::UnitY ); // Negative X
        viewMatrices[1] = Math::CreateLookAtMatrix( viewPosition, -Vector::UnitX, Vector::UnitY ); // Negative X

        viewMatrices[2] = Math::CreateLookAtMatrix( viewPosition, Vector::UnitY, Vector::UnitZ ); // Positive Y
        viewMatrices[3] = Math::CreateLookAtMatrix( viewPosition, -Vector::UnitY, Vector::UnitZ ); // Negative Y

        viewMatrices[4] = Math::CreateLookAtMatrix( viewPosition, Vector::UnitZ, Vector::UnitY ); // Positive Z
        viewMatrices[5] = Math::CreateLookAtMatrix( viewPosition, -Vector::UnitZ, Vector::UnitY ); // Negative Z

        TArray<Matrix, NumRenderViews> mirrorMatrices;
        mirrorMatrices[0] = mirrorMatrixX; // Negative X
        mirrorMatrices[1] = mirrorMatrixX; // Negative X

        mirrorMatrices[2] = mirrorMatrixY; // Positive Y
        mirrorMatrices[3] = mirrorMatrixX; // Negative Y

        mirrorMatrices[4] = mirrorMatrixX; // Positive Z
        mirrorMatrices[5] = mirrorMatrixX; // Negative Z

        //---------------------------------------------------------------------------------------------------
        // Capture cubemap first

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Global Environment Map Capture" );

            RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( g_CaptureResolution ), float( g_CaptureResolution ), 0.0F, 1.0F );
            RHI::CmdSetScissor( pCommandBuffer, 0, 0, g_CaptureResolution, g_CaptureResolution );

            RHI::CmdBarrier( pCommandBuffer, m_pCaptureRenderTarget,
                             RHI::PipelineStage::PixelShader, RHI::PipelineStage::Draw,
                             RHI::ResourceAccess::ShaderResource, RHI::ResourceAccess::RenderTarget,
                             RHI::TextureState::ShaderResource, RHI::TextureState::RenderTarget,
                             {}, {} );

            for ( uint32_t renderViewIndex = 0; renderViewIndex < NumRenderViews; ++renderViewIndex )
            {
                Matrix viewProjectionMatrix = viewMatrices[renderViewIndex] * projectionMatrix * mirrorMatrices[renderViewIndex];
                Matrix viewProjectionMatrixInverse = viewProjectionMatrix.GetInverse();

                alignas( 32 ) ShaderTypes::RenderView deviceRenderView = {};
                std::memcpy
                (
                    deviceRenderView.m_viewProjectionMatrix,
                    viewProjectionMatrix.m_rows,
                    sizeof( deviceRenderView.m_viewProjectionMatrix )
                );
                std::memcpy
                (
                    deviceRenderView.m_viewMatrix,
                    viewMatrices[renderViewIndex].m_rows,
                    sizeof( deviceRenderView.m_viewMatrix )
                );
                std::memcpy
                (
                    deviceRenderView.m_inverseViewProjectionMatrix,
                    viewProjectionMatrixInverse.m_rows,
                    sizeof( deviceRenderView.m_inverseViewProjectionMatrix )
                );

                deviceRenderView.m_renderTargetSize[0] = float( g_CaptureResolution );
                deviceRenderView.m_renderTargetSize[1] = float( g_CaptureResolution );
                deviceRenderView.m_renderTargetSize[2] = 1.0F / float( g_CaptureResolution );
                deviceRenderView.m_renderTargetSize[3] = 1.0F / float( g_CaptureResolution );

                deviceRenderView.m_projectionP00 = projectionMatrix.m_values[0][0];
                deviceRenderView.m_projectionP11 = projectionMatrix.m_values[1][1];
                deviceRenderView.m_znear = 0.001F;

                deviceRenderView.m_renderViewFlags = ShaderTypes::RENDER_VIEW_FLAG_NONE;
                deviceRenderView.m_renderViewLayerFlags = ShaderTypes::RENDER_VIEW_LAYER_FLAG_GLOBAL_ENVIRONMENT_MAP;

                Memory::CopyToWriteCombined( &dstRenderViews[renderViewIndex], &deviceRenderView, sizeof( deviceRenderView ) );

                {
                    EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Cubemap Face" );

                    ForwardShadingPass::DrawMaterialShaderBuckets
                    (
                        materialShaderPipelineBuckets,
                        clusterCapacity,
                        m_renderViews[renderViewIndex],
                        m_pCaptureRenderTarget,
                        renderViewIndex, 0,
                        m_pDepthRenderTarget,
                        pCommandBuffer
                    );
                }
            }

            for ( uint32_t renderViewIndex = 0; renderViewIndex < NumRenderViews; ++renderViewIndex )
            {
                RHI::CmdBarrier( pCommandBuffer, m_pCaptureRenderTarget,
                                 RHI::PipelineStage::Draw, RHI::PipelineStage::PixelShader,
                                 RHI::ResourceAccess::RenderTarget, RHI::ResourceAccess::ShaderResource,
                                 RHI::TextureState::RenderTarget, RHI::TextureState::ShaderResource,
                                 { 0, 1, renderViewIndex, 1 }, {} );
            }

            RHI::CmdBarrier( pCommandBuffer, pIrradianceRenderTarget,
                             RHI::PipelineStage::PixelShader, RHI::PipelineStage::Draw,
                             RHI::ResourceAccess::ShaderResource, RHI::ResourceAccess::RenderTarget,
                             RHI::TextureState::ShaderResource, RHI::TextureState::RenderTarget,
                             {}, {} );

            RHI::CmdBarrier( pCommandBuffer, pRadianceRenderTarget,
                             RHI::PipelineStage::PixelShader, RHI::PipelineStage::Draw,
                             RHI::ResourceAccess::ShaderResource, RHI::ResourceAccess::RenderTarget,
                             RHI::TextureState::ShaderResource, RHI::TextureState::RenderTarget,
                             {}, {} );

        }

        //---------------------------------------------------------------------------------------------------
        // Downsample cubemap

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Global Environment Map Cubemap Downsample" );

            RHI::CmdSetPipeline( pCommandBuffer, m_pPipelineCubemapDownsample );

            for ( uint32_t mipLevel = 1; mipLevel < m_pCaptureRenderTarget->m_mipLevels; ++mipLevel )
            {
                for ( uint32_t renderViewIndex = 0; renderViewIndex < NumRenderViews; ++renderViewIndex )
                {
                    ShaderTypes::CubemapDownsampleResourceTableData cubemapDownsampleRootConstants = {};
                    cubemapDownsampleRootConstants.SetInputTexture( RHI::GetTextureHandle( m_pCaptureRenderTarget, RHI::DescriptorTypeFlags::TextureCube, 0 ) );
                    cubemapDownsampleRootConstants.SetRenderViewBuffer( renderViewBufferHandle );
                    cubemapDownsampleRootConstants.m_renderViewIndex = renderViewOffset + renderViewIndex;
                    cubemapDownsampleRootConstants.m_mipLevel = float( mipLevel - 1 ) / float( m_pCaptureRenderTarget->m_mipLevels - 1 );

                    RHI::LoadAction loadAction = {};
                    loadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;
                    loadAction.m_colorClearValues[0] = m_pCaptureRenderTarget->m_clearValue;

                    uint32_t resolution = g_CaptureResolution >> mipLevel;

                    RHI::CmdSetRenderTargets( pCommandBuffer, { &m_pCaptureRenderTarget, 1 }, nullptr, &loadAction, { &renderViewIndex, 1 }, { &mipLevel,1 } );
                    RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( resolution ), float( resolution ), 0.0F, 1.0F );
                    RHI::CmdSetScissor( pCommandBuffer, 0, 0, resolution, resolution );

                    RHI::CmdSetRootConstants( pCommandBuffer, 0, &cubemapDownsampleRootConstants, sizeof( cubemapDownsampleRootConstants ) );
                    RHI::CmdDraw( pCommandBuffer, 3, 0 );
                }

                // Issue barriers after all draws, this allows the draws to overlap with each other.
                // But we still need to issue those in the inner loop because next mip level has a dependency on the previous mip level.
                for ( uint32_t renderViewIndex = 0; renderViewIndex < NumRenderViews; ++renderViewIndex )
                {
                    RHI::CmdBarrier( pCommandBuffer, m_pCaptureRenderTarget,
                                     RHI::PipelineStage::Draw, RHI::PipelineStage::PixelShader,
                                     RHI::ResourceAccess::RenderTarget, RHI::ResourceAccess::ShaderResource,
                                     RHI::TextureState::RenderTarget, RHI::TextureState::ShaderResource,
                                     { mipLevel, 1, renderViewIndex, 1 }, {} );
                }
            }

        }

        // Cubemap ready, irradiance filtering first
        //---------------------------------------------------------------------------------------------------
        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Global Environment Map Irradiance Filtering" );

            RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( g_IrradianceResolution ), float( g_IrradianceResolution ), 0.0F, 1.0F );
            RHI::CmdSetScissor( pCommandBuffer, 0, 0, g_IrradianceResolution, g_IrradianceResolution );

            RHI::CmdSetPipeline( pCommandBuffer, m_pPipelineIrradianceFiltering );

            for ( uint32_t renderViewIndex = 0; renderViewIndex < NumRenderViews; ++renderViewIndex )
            {
                ShaderTypes::IrradianceFilteringRootConstants irradianceFilteringRootConstants = {};
                irradianceFilteringRootConstants.m_renderViewIndex = renderViewOffset + renderViewIndex;
                irradianceFilteringRootConstants.m_inputTexture = GetCaptureTextureHandle();
                irradianceFilteringRootConstants.m_renderViewBuffer = renderViewBufferHandle;

                RHI::LoadAction loadAction = {};
                loadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;
                loadAction.m_colorClearValues[0] = pIrradianceRenderTarget->m_clearValue;

                uint32_t mipSlice = 0; // TODO: Make optional in RHI

                RHI::CmdSetRenderTargets( pCommandBuffer, { &pIrradianceRenderTarget, 1 }, nullptr, &loadAction, { &renderViewIndex, 1 }, { &mipSlice, 1 } );

                RHI::CmdSetRootConstants( pCommandBuffer, 0, &irradianceFilteringRootConstants, sizeof( irradianceFilteringRootConstants ) );
                RHI::CmdDraw( pCommandBuffer, 3, 0 );
            }
        }

        // Irradiance ready, radiance filtering last
        //---------------------------------------------------------------------------------------------------
        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Global Environment Map Radiance Filtering" );
            RHI::CmdSetPipeline( pCommandBuffer, m_pPipelineRadianceFiltering );

            for ( uint32_t mipLevel = 0; mipLevel < pRadianceRenderTarget->m_mipLevels; ++mipLevel )
            {
                for ( uint32_t renderViewIndex = 0; renderViewIndex < NumRenderViews; ++renderViewIndex )
                {
                    ShaderTypes::RadianceFilteringRootConstants radianceFilteringRootConstants = {};
                    radianceFilteringRootConstants.m_renderViewIndex = renderViewOffset + renderViewIndex;
                    radianceFilteringRootConstants.m_inputTexture = GetCaptureTextureHandle();
                    radianceFilteringRootConstants.m_renderViewBuffer = renderViewBufferHandle;
                    radianceFilteringRootConstants.m_roughness = float( mipLevel ) / float( pRadianceRenderTarget->m_mipLevels - 1 );
                    radianceFilteringRootConstants.m_cubemapResolution = g_CaptureResolution;

                    RHI::LoadAction loadAction = {};
                    loadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;
                    loadAction.m_colorClearValues[0] = pRadianceRenderTarget->m_clearValue;

                    uint32_t resolution = g_RadianceResolution >> mipLevel;

                    RHI::CmdSetRenderTargets( pCommandBuffer, { &pRadianceRenderTarget, 1 }, nullptr, &loadAction, { &renderViewIndex, 1 }, { &mipLevel, 1 } );
                    RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( resolution ), float( resolution ), 0.0F, 1.0F );
                    RHI::CmdSetScissor( pCommandBuffer, 0, 0, resolution, resolution );

                    RHI::CmdSetRootConstants( pCommandBuffer, 0, &radianceFilteringRootConstants, sizeof( radianceFilteringRootConstants ) );
                    RHI::CmdDraw( pCommandBuffer, 3, 0 );
                }
            }

            RHI::CmdBarrier
            (
                pCommandBuffer, pIrradianceRenderTarget,
                RHI::PipelineStage::Draw, RHI::PipelineStage::PixelShader,
                RHI::ResourceAccess::RenderTarget, RHI::ResourceAccess::ShaderResource,
                RHI::TextureState::RenderTarget, RHI::TextureState::ShaderResource,
                {}, {} );

            RHI::CmdBarrier
            (
                pCommandBuffer, pRadianceRenderTarget,
                RHI::PipelineStage::Draw, RHI::PipelineStage::PixelShader,
                RHI::ResourceAccess::RenderTarget, RHI::ResourceAccess::ShaderResource,
                RHI::TextureState::RenderTarget, RHI::TextureState::ShaderResource,
                {}, {}
            );
        }
    }

    void GlobalEnvironmentMapPass::UpdateDeviceResources_DFG( RHI::Context* pContextRHI )
    {
        EE_PROFILE_FUNCTION_RENDER();
        if ( !m_pDfgRenderTarget )
        {
            RHI::TextureParameters dfgTextureParameters = {};
            dfgTextureParameters.m_width = g_DFGResolution;
            dfgTextureParameters.m_height = g_DFGResolution;
            dfgTextureParameters.m_format = RHI::DataFormat::RG16_SFloat;
            dfgTextureParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Texture, RHI::DescriptorTypeFlags::RenderTarget );
            dfgTextureParameters.m_debugName = "GlobalEnvironmentMap DFG";

            m_pDfgRenderTarget = RHI::CreateTexture( pContextRHI, dfgTextureParameters );
        }
    }

    void GlobalEnvironmentMapPass::PrecomputeDFG( RHI::CommandBuffer* pCommandBuffer )
    {
        EE_PROFILE_FUNCTION_RENDER();

        if ( m_dfgUpdateNeeded )
        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Global Environment Map Compute DFG" );

            RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( g_DFGResolution ), float( g_DFGResolution ), 0.0F, 1.0F );
            RHI::CmdSetScissor( pCommandBuffer, 0, 0, g_DFGResolution, g_DFGResolution );

            RHI::CmdBarrier( pCommandBuffer, m_pDfgRenderTarget,
                             RHI::PipelineStage::PixelShader, RHI::PipelineStage::Draw,
                             RHI::ResourceAccess::ShaderResource, RHI::ResourceAccess::RenderTarget,
                             RHI::TextureState::ShaderResource, RHI::TextureState::RenderTarget,
                             {}, {} );

            RHI::CmdSetPipeline( pCommandBuffer, m_pPipelinePrecomputeDFG );

            RHI::LoadAction loadAction = {};
            loadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;
            loadAction.m_colorClearValues[0] = m_pDfgRenderTarget->m_clearValue;

            RHI::CmdSetRenderTargets( pCommandBuffer, { &m_pDfgRenderTarget, 1 }, nullptr, &loadAction );
            RHI::CmdDraw( pCommandBuffer, 3, 0 );

            RHI::CmdBarrier
            (
                pCommandBuffer, m_pDfgRenderTarget,
                RHI::PipelineStage::Draw, RHI::PipelineStage::PixelShader,
                RHI::ResourceAccess::RenderTarget, RHI::ResourceAccess::ShaderResource,
                RHI::TextureState::RenderTarget, RHI::TextureState::ShaderResource,
                {}, {}
            );

            m_dfgUpdateNeeded = false;
        }
    }

    RHI::TextureHandle GlobalEnvironmentMapPass::GetDFGTextureHandle() const
    {
        return RHI::GetTextureHandle( m_pDfgRenderTarget, RHI::DescriptorTypeFlags::Texture, 0 );
    }

    RHI::TextureHandle GlobalEnvironmentMapPass::GetCaptureTextureHandle() const
    {
        return RHI::GetTextureHandle( m_pCaptureRenderTarget, RHI::DescriptorTypeFlags::TextureCube, 0 );
    }
}
