
#include "RenderPass_ForwardShading.h"
#include "Base/Render/RHI.h"
#include "Base/Render/RenderWindow.h"
#include "Engine/Render/RenderViewport.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/Shaders/Renderer/RendererTypes.esh"

namespace EE::Render
{
    void ForwardShadingMaterialShaderPipelineBucket::Shutdown( RHI::Context* pContextRHI )
    {
        RHI::DestroyPipeline( pContextRHI, eastl::move( m_pDepthOnlyPipeline ) );
        RHI::DestroyPipeline( pContextRHI, eastl::move( m_pDepthOnlyAlphaTestPipeline ) );
        RHI::DestroyPipeline( pContextRHI, eastl::move( m_pOpaquePipeline ) );
        RHI::DestroyPipeline( pContextRHI, eastl::move( m_pAlphaBlendPipeline ) );
    }

    //-------------------------------------------------------------------------------------------------------

    TVector<ForwardShadingMaterialShaderPipelineBucket> ForwardShadingPass::InitializeMaterialShaderBuckets( RenderSystem* pRenderSystem )
    {
        // Initialize pipeline buckets (TODO: Make it configurable)
        RHI::DataFormat pipelineColorFormats[] = { RHI::DataFormat::RG11_B10_UFloat };

        RHI::DepthStencilState depthStencilStateDepthOnlyPass = {};
        depthStencilStateDepthOnlyPass.m_depthTest = true;
        depthStencilStateDepthOnlyPass.m_depthWrite = true;
        depthStencilStateDepthOnlyPass.m_depthCompareMode = RHI::CompareMode::GreaterEqual;

        RHI::DepthStencilState depthStencilStateColorPass = {};
        depthStencilStateColorPass.m_depthTest = true;
        depthStencilStateColorPass.m_depthWrite = false;
        depthStencilStateColorPass.m_depthCompareMode = RHI::CompareMode::Equal;

        RHI::DepthStencilState depthStencilStateColorAlphaBlendPass = {};
        depthStencilStateColorAlphaBlendPass.m_depthTest = true;
        depthStencilStateColorAlphaBlendPass.m_depthWrite = false;
        depthStencilStateColorAlphaBlendPass.m_depthCompareMode = RHI::CompareMode::Equal;

        RHI::MeshPipelineParameters opaquePipelineParameters = {};
        opaquePipelineParameters.m_colorFormats = pipelineColorFormats;
        opaquePipelineParameters.m_numRenderTargets = 1;
        opaquePipelineParameters.m_depthStencilFormat = RHI::DataFormat::D32_SFloat;
        opaquePipelineParameters.m_depthStencilState = depthStencilStateColorPass;
        opaquePipelineParameters.m_rasterizerState.m_depthClip = true;
        opaquePipelineParameters.m_rasterizerState.m_scissor = true;
        opaquePipelineParameters.m_rasterizerState.m_cullMode = RHI::CullMode::None; // Backface culling is done in the shader
        opaquePipelineParameters.m_blendState.m_writeMasks[0] = 0x0F;
        opaquePipelineParameters.m_blendState.m_renderTargetMask = RHI::BlendStateTargetFlags::Target0;

        RHI::MeshPipelineParameters alphaBlendPipelineParameters = opaquePipelineParameters;
        alphaBlendPipelineParameters.m_depthStencilState = depthStencilStateColorAlphaBlendPass;
        alphaBlendPipelineParameters.m_blendState.m_srcFactors[0] = RHI::BlendConstant::SrcAlpha;
        alphaBlendPipelineParameters.m_blendState.m_srcAlphaFactors[0] = RHI::BlendConstant::SrcAlpha;
        alphaBlendPipelineParameters.m_blendState.m_dstFactors[0] = RHI::BlendConstant::OneMinusSrcAlpha;
        alphaBlendPipelineParameters.m_blendState.m_dstAlphaFactors[0] = RHI::BlendConstant::OneMinusSrcAlpha;
        alphaBlendPipelineParameters.m_blendState.m_blendModes[0] = RHI::BlendMode::Add;
        alphaBlendPipelineParameters.m_blendState.m_blendModesAlpha[0] = RHI::BlendMode::Add;
        alphaBlendPipelineParameters.m_blendState.m_blendEnabled = true;

        RHI::MeshPipelineParameters depthOnlyPipelineParameters = opaquePipelineParameters;
        depthOnlyPipelineParameters.m_depthStencilState = depthStencilStateDepthOnlyPass;
        depthOnlyPipelineParameters.m_numRenderTargets = 0;
        depthOnlyPipelineParameters.m_colorFormats = {};

        RHI::MeshPipelineParameters depthOnlyAlphaTestPipelineParameters = opaquePipelineParameters;
        depthOnlyAlphaTestPipelineParameters.m_depthStencilState = depthStencilStateDepthOnlyPass;
        depthOnlyAlphaTestPipelineParameters.m_numRenderTargets = 0;
        depthOnlyAlphaTestPipelineParameters.m_colorFormats = {};

        RHI::Context* pContextRHI = pRenderSystem->GetContextRHI();
        TVector<MaterialShader> const& materialShaders = pRenderSystem->GetMaterialShaders();

        TVector<ForwardShadingMaterialShaderPipelineBucket> materialShaderBuckets;
        materialShaderBuckets.reserve( materialShaders.size() );

        for ( MaterialShader const& shader : materialShaders )
        {
            opaquePipelineParameters.m_debugName.sprintf( "%s Opaque Pipeline", shader.m_shaderName.c_str() );
            opaquePipelineParameters.m_pRootSignature = shader.m_pRootSignature;
            opaquePipelineParameters.m_pShader = shader.m_shaders[0];

            alphaBlendPipelineParameters.m_debugName.sprintf( "%s AlphaBlend Pipeline", shader.m_shaderName.c_str() );
            alphaBlendPipelineParameters.m_pRootSignature = shader.m_pRootSignature;
            alphaBlendPipelineParameters.m_pShader = shader.m_shaders[0];

            depthOnlyPipelineParameters.m_debugName.sprintf( "%s DepthOnly Pipeline", shader.m_shaderName.c_str() );
            depthOnlyPipelineParameters.m_pRootSignature = shader.m_pRootSignature;
            depthOnlyPipelineParameters.m_pShader = shader.m_shaders[MaterialShader::DepthOnly];

            depthOnlyAlphaTestPipelineParameters.m_debugName.sprintf( "%s DepthOnly_AlphaTest Pipeline", shader.m_shaderName.c_str() );
            depthOnlyAlphaTestPipelineParameters.m_pRootSignature = shader.m_pRootSignature;
            depthOnlyAlphaTestPipelineParameters.m_pShader = shader.m_shaders[MaterialShader::DepthOnly | MaterialShader::AlphaTest];

            ForwardShadingMaterialShaderPipelineBucket shaderPipelineBucket = {};

            shaderPipelineBucket.m_shaderName = shader.m_shaderName.c_str();
            shaderPipelineBucket.m_pCommandSignature = shader.m_pCommandSignature;

            shaderPipelineBucket.m_pDepthOnlyPipeline = RHI::CreatePipeline( pContextRHI, depthOnlyPipelineParameters );
            shaderPipelineBucket.m_pDepthOnlyAlphaTestPipeline = RHI::CreatePipeline( pContextRHI, depthOnlyAlphaTestPipelineParameters );
            shaderPipelineBucket.m_pOpaquePipeline = RHI::CreatePipeline( pContextRHI, opaquePipelineParameters );
            shaderPipelineBucket.m_pAlphaBlendPipeline = RHI::CreatePipeline( pContextRHI, alphaBlendPipelineParameters );

            materialShaderBuckets.emplace_back( eastl::move( shaderPipelineBucket ) );
        }

        return materialShaderBuckets;
    }

    void ForwardShadingPass::DrawMaterialShaderBuckets_DepthOnly( TArrayView<ForwardShadingMaterialShaderPipelineBucket const> materialShaderBuckets,
                                                                  TArrayView<uint32_t const>                                   clusterCapacity,
                                                                  DeviceRenderView const&                                      renderView,
                                                                  RHI::Texture*                                                pDepthTexture,
                                                                  RHI::CommandBuffer*                                          pCommandBuffer )
    {
        RHI::LoadAction depthOnlyLoadAction = {};
        depthOnlyLoadAction.m_loadActionDepth = RHI::LoadActionType::Clear;

        RHI::CmdSetRenderTargets( pCommandBuffer, {}, pDepthTexture, &depthOnlyLoadAction );

        // Depth only pass
        //---------------------------------------------------------------------------------------------------

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Forward Shading Depth Only Pass" );

        for ( size_t shaderIndex = 0; shaderIndex < materialShaderBuckets.size(); ++shaderIndex )
        {
            ForwardShadingMaterialShaderPipelineBucket const& shaderPipelineBucket = materialShaderBuckets[shaderIndex];

            uint32_t const bucketIndirectCommandCapacity = Math::IntegerDivideCeiling<uint32_t>( clusterCapacity[shaderIndex], RHI::MaxDispatchSize );
            DeviceRenderViewBucket const& renderViewBucket = renderView.m_renderViewBuckets[shaderIndex];

            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, shaderPipelineBucket.m_shaderName.data() );

                RHI::CmdSetPipeline( pCommandBuffer, shaderPipelineBucket.m_pDepthOnlyPipeline );
                RHI::CmdSetRootConstants( pCommandBuffer, 0, nullptr, sizeof( ShaderTypes::DrawRootConstants ) );
                {
                    MaterialShaderRenderBucket const& renderBucket = renderViewBucket.m_opaqueBucket;
                    RHI::CmdExecuteIndirect( pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                                             renderBucket.m_drawArgumentBuffer.m_buffer, 0,
                                             renderBucket.m_drawCounterBuffer, 0 );
                }
            }

            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, shaderPipelineBucket.m_shaderName.data() );

                RHI::CmdSetPipeline( pCommandBuffer, shaderPipelineBucket.m_pDepthOnlyAlphaTestPipeline );
                RHI::CmdSetRootConstants( pCommandBuffer, 0, nullptr, sizeof( ShaderTypes::DrawRootConstants ) );
                {
                    MaterialShaderRenderBucket const& renderBucket = renderViewBucket.m_alphaTestBucket;
                    RHI::CmdExecuteIndirect( pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                                             renderBucket.m_drawArgumentBuffer.m_buffer, 0,
                                             renderBucket.m_drawCounterBuffer, 0 );
                }
            }
        }
    }

    void ForwardShadingPass::DrawMaterialShaderBuckets_Shading( TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
                                                                TArrayView<uint32_t const>                                      clusterCapacity,
                                                                DeviceRenderView const&                                         renderView,
                                                                RHI::Texture*                                                   pColorTexture,
                                                                uint32_t                                                        colorTargetSlice,
                                                                uint32_t                                                        colorTargetMipSlice,
                                                                RHI::Texture*                                                   pDepthTexture,
                                                                RHI::CommandBuffer*                                             pCommandBuffer )
    {
        // Opaque Color pass
        //---------------------------------------------------------------------------------------------------

        RHI::LoadAction opaqueColorLoadAction = {};
        opaqueColorLoadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;
        opaqueColorLoadAction.m_colorClearValues[0] = pColorTexture->m_clearValue;
        opaqueColorLoadAction.m_loadActionDepth = RHI::LoadActionType::Load;

        TArrayView<uint32_t> colorArraySlices = {};
        TArrayView<uint32_t> colorMipSlices = {};
        if ( colorTargetSlice != ~0U )
        {
            colorArraySlices = { &colorTargetSlice, 1 };
            colorMipSlices = { &colorTargetMipSlice, 1 };
        }

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Forward Shading Opaque + AlphaTest Pass" );

            RHI::CmdSetRenderTargets( pCommandBuffer, { &pColorTexture, 1 }, pDepthTexture, &opaqueColorLoadAction, colorArraySlices, colorMipSlices );

            for ( size_t shaderIndex = 0; shaderIndex < materialShaderBuckets.size(); ++shaderIndex )
            {
                ForwardShadingMaterialShaderPipelineBucket const& shaderPipelineBucket = materialShaderBuckets[shaderIndex];

                uint32_t const bucketIndirectCommandCapacity = Math::IntegerDivideCeiling<uint32_t>( clusterCapacity[shaderIndex], RHI::MaxDispatchSize );
                DeviceRenderViewBucket const& renderViewBucket = renderView.m_renderViewBuckets[shaderIndex];

                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, shaderPipelineBucket.m_shaderName.data() );

                RHI::CmdSetPipeline( pCommandBuffer, shaderPipelineBucket.m_pOpaquePipeline );
                RHI::CmdSetRootConstants( pCommandBuffer, 0, nullptr, sizeof( ShaderTypes::DrawRootConstants ) );
                RHI::CmdExecuteIndirect( pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                                         renderViewBucket.m_opaqueBucket.m_drawArgumentBuffer.m_buffer, 0,
                                         renderViewBucket.m_opaqueBucket.m_drawCounterBuffer, 0 );
                RHI::CmdExecuteIndirect( pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                                         renderViewBucket.m_alphaTestBucket.m_drawArgumentBuffer.m_buffer, 0,
                                         renderViewBucket.m_alphaTestBucket.m_drawCounterBuffer, 0 );
            }
        }

        // Alpha Blend Color pass
        //---------------------------------------------------------------------------------------------------

        RHI::LoadAction alphaBlendColorLoadAction = {};
        alphaBlendColorLoadAction.m_loadActionsColor[0] = RHI::LoadActionType::Load;
        alphaBlendColorLoadAction.m_loadActionDepth = RHI::LoadActionType::Load;

        RHI::CmdSetRenderTargets( pCommandBuffer, { &pColorTexture, 1 }, pDepthTexture, &alphaBlendColorLoadAction, colorArraySlices, colorMipSlices );

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Forward Shading Alpha Blend Pass" );

        for ( size_t shaderIndex = 0; shaderIndex < materialShaderBuckets.size(); ++shaderIndex )
        {
            ForwardShadingMaterialShaderPipelineBucket const& shaderPipelineBucket = materialShaderBuckets[shaderIndex];

            uint32_t const bucketIndirectCommandCapacity = Math::IntegerDivideCeiling<uint32_t>( clusterCapacity[shaderIndex], RHI::MaxDispatchSize );
            DeviceRenderViewBucket const& renderViewBucket = renderView.m_renderViewBuckets[shaderIndex];

            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, shaderPipelineBucket.m_shaderName.data() );

                RHI::CmdSetPipeline( pCommandBuffer, shaderPipelineBucket.m_pDepthOnlyPipeline );
                RHI::CmdSetRootConstants( pCommandBuffer, 0, nullptr, sizeof( ShaderTypes::DrawRootConstants ) );
                RHI::CmdExecuteIndirect( pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                                         renderViewBucket.m_alphaBlendBucket.m_drawArgumentBuffer.m_buffer, 0,
                                         renderViewBucket.m_alphaBlendBucket.m_drawCounterBuffer, 0 );
            }

            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, shaderPipelineBucket.m_shaderName.data() );


                RHI::CmdSetPipeline( pCommandBuffer, shaderPipelineBucket.m_pAlphaBlendPipeline );
                RHI::CmdSetRootConstants( pCommandBuffer, 0, nullptr, sizeof( ShaderTypes::DrawRootConstants ) );
                RHI::CmdExecuteIndirect( pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                                         renderViewBucket.m_alphaBlendBucket.m_drawArgumentBuffer.m_buffer, 0,
                                         renderViewBucket.m_alphaBlendBucket.m_drawCounterBuffer, 0 );
            }
        }
    }

    void ForwardShadingPass::DrawMaterialShaderBuckets( TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
                                                        TArrayView<uint32_t const>                                      clusterCapacity,
                                                        DeviceRenderView const&                                         renderView,
                                                        RHI::Texture*                                                   pColorTexture,
                                                        uint32_t                                                        colorTargetSlice,
                                                        uint32_t                                                        colorTargetMipSlice,
                                                        RHI::Texture*                                                   pDepthTexture,
                                                        RHI::CommandBuffer*                                             pCommandBuffer )
    {
        DrawMaterialShaderBuckets_DepthOnly
        (
            materialShaderBuckets,
            clusterCapacity,
            renderView,
            pDepthTexture,
            pCommandBuffer
        );
        DrawMaterialShaderBuckets_Shading
        (
            materialShaderBuckets,
            clusterCapacity,
            renderView,
            pColorTexture,
            colorTargetSlice,
            colorTargetMipSlice,
            pDepthTexture,
            pCommandBuffer
        );
    }

    //-------------------------------------------------------------------------------------------------------

    void ForwardShadingPass::Initialize( RenderPassContext const& context )
    {
        m_renderView.Initialize( context.m_pRenderSystem, context.m_materialShaderPipelineBuckets.size() );
    }

    void ForwardShadingPass::Shutdown( RenderSystem* pRenderSystem )
    {
        m_renderView.Shutdown( pRenderSystem );
    }

    void ForwardShadingPass::UpdateDeviceResources( RenderSystem*                                                   pRenderSystem,
                                                    TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
                                                    TArrayView<uint32_t const>                                      clusterCapacity )
    {
        EE_PROFILE_FUNCTION_RENDER();

        m_renderView.UpdateDeviceResources( pRenderSystem, clusterCapacity );
    }

    void ForwardShadingPass::UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport )
    {
        EE_PROFILE_FUNCTION_RENDER();

        Int2 textureSize = pRenderViewport->GetSize();
        uint32_t textureWidth = uint32_t( textureSize.m_x );
        uint32_t textureHeight = uint32_t( textureSize.m_y );

        if ( pRenderViewport->TextureNeedsResize( pRenderViewport->m_ForwardShading_ColorTexture ) )
        {
            pRenderSystem->QueueResourceDelete
            (
                eastl::move( pRenderViewport->m_ForwardShading_DepthTexture ),
                eastl::move( pRenderViewport->m_ForwardShading_ColorTexture )
            );

            RHI::TextureParameters depthParameters = {};
            depthParameters.m_width = textureWidth;
            depthParameters.m_height = textureHeight;
            depthParameters.m_format = RHI::DataFormat::D32_SFloat;
            depthParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::RenderTarget, RHI::DescriptorTypeFlags::Texture );
            depthParameters.m_debugName.sprintf( "ForwardShadingPass Depth Target %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_ForwardShading_DepthTexture = RHI::CreateTexture( pRenderSystem->GetContextRHI(), depthParameters );

            RHI::TextureParameters hdrParameters = depthParameters;
            hdrParameters.m_format = RHI::DataFormat::RG11_B10_UFloat;
            hdrParameters.m_clearValue = { { 0.0F, 0.0F, 0.0F, 1.0F } };
            hdrParameters.m_debugName.sprintf( "ForwardShadingPass HDR Target %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_ForwardShading_ColorTexture = RHI::CreateTexture( pRenderSystem->GetContextRHI(), hdrParameters );
        }
    }

    void ForwardShadingPass::DepthOnlyPass( TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
                                            TArrayView<uint32_t const>                                      clusterCapacity,
                                            RenderViewport const*                                           pRenderViewport,
                                            DeviceResourceStates&                                           resourceStates,
                                            RHI::CommandBuffer*                                             pCommandBuffer,
                                            TArrayView<ShaderTypes::RenderView>                             dstRenderViews_WriteCombined ) const
    {
        // Copy render view data
        //---------------------------------------------------------------------------------------------------

        Math::ViewVolume const& viewVolume = pRenderViewport->GetViewVolume();
        Float2 const viewSize = pRenderViewport->GetSize();

        Matrix reverseZ( Vector( 1.0f, 0.0f, 0.0f, 0.0f ),
                         Vector( 0.0f, 1.0f, 0.0f, 0.0f ),
                         Vector( 0.0f, 0.0f, -1.0f, 0.0f ),
                         Vector( 0.0f, 0.0f, 1.0f, 1.0f ) );

        Matrix projectionMatrix = viewVolume.GetProjectionMatrix() * reverseZ;
        Matrix viewProjectionMatrix = viewVolume.GetViewMatrix() * projectionMatrix;

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
            viewVolume.GetViewMatrix().m_rows,
            sizeof( deviceRenderView.m_viewMatrix )
        );
        std::memcpy
        (
            deviceRenderView.m_inverseViewProjectionMatrix,
            viewProjectionMatrix.GetInverse().m_rows,
            sizeof( deviceRenderView.m_inverseViewProjectionMatrix )
        );
        std::memcpy
        (
            deviceRenderView.m_inverseViewMatrix,
            viewVolume.GetViewMatrix().GetInverse().m_rows,
            sizeof( viewVolume.GetViewMatrix() )
        );
        std::memcpy
        (
            deviceRenderView.m_inverseProjectionMatrix,
            projectionMatrix.GetInverse().m_rows,
            sizeof( projectionMatrix )
        );

        deviceRenderView.m_renderTargetSize[0] = viewSize.m_x;
        deviceRenderView.m_renderTargetSize[1] = viewSize.m_y;
        deviceRenderView.m_renderTargetSize[2] = 1.0F / viewSize.m_x;
        deviceRenderView.m_renderTargetSize[3] = 1.0F / viewSize.m_y;

        deviceRenderView.m_projectionP00 = projectionMatrix.m_values[0][0];
        deviceRenderView.m_projectionP11 = projectionMatrix.m_values[1][1];
        deviceRenderView.m_znear = 0.001F;

        deviceRenderView.m_renderViewFlags = ShaderTypes::RENDER_VIEW_FLAG_NONE;
        deviceRenderView.m_renderViewLayerFlags = ShaderTypes::RENDER_VIEW_LAYER_FLAG_FORWARD_SHADING;

        Memory::CopyToWriteCombined( dstRenderViews_WriteCombined.data(), &deviceRenderView, sizeof( deviceRenderView ) );

        RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, viewSize.m_x, viewSize.m_y, 0.0F, 1.0F );
        RHI::CmdSetScissor( pCommandBuffer, 0, 0, uint32_t( viewSize.m_x ), uint32_t( viewSize.m_y ) );

        EE_ASSERT( !resourceStates.HasPendingBarriers() );
        resourceStates.Writeable( pRenderViewport->m_ForwardShading_DepthTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::DepthWrite, RHI::TextureState::DepthWrite );
        resourceStates.FlushBarriers( pCommandBuffer );

        DrawMaterialShaderBuckets_DepthOnly
        (
            materialShaderBuckets,
            clusterCapacity,
            m_renderView,
            pRenderViewport->m_ForwardShading_DepthTexture,
            pCommandBuffer
        );
    }

    void ForwardShadingPass::ShadingPass( TArrayView<ForwardShadingMaterialShaderPipelineBucket const>      materialShaderBuckets,
                                          TArrayView<uint32_t const>                                        clusterCapacity,
                                          RenderViewport const*                                             pRenderViewport,
                                          DeviceResourceStates&                                             resourceStates,
                                          RHI::CommandBuffer*                                               pCommandBuffer ) const
    {
        Float2 const viewSize = pRenderViewport->GetSize();

        RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, viewSize.m_x, viewSize.m_y, 0.0F, 1.0F );
        RHI::CmdSetScissor( pCommandBuffer, 0, 0, uint32_t( viewSize.m_x ), uint32_t( viewSize.m_y ) );

        EE_ASSERT( !resourceStates.HasPendingBarriers() );
        resourceStates.Writeable( pRenderViewport->m_ForwardShading_ColorTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
        resourceStates.Writeable( pRenderViewport->m_ForwardShading_DepthTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::DepthWrite, RHI::TextureState::DepthWrite );
        resourceStates.FlushBarriers( pCommandBuffer );

        DrawMaterialShaderBuckets_Shading
        (
            materialShaderBuckets,
            clusterCapacity,
            m_renderView,
            pRenderViewport->m_ForwardShading_ColorTexture, ~0U, 0,
            pRenderViewport->m_ForwardShading_DepthTexture,
            pCommandBuffer
        );
    }
}
