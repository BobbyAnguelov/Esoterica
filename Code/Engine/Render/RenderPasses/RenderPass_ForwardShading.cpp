
#include "RenderPass_ForwardShading.h"
#include "Base/Render/RHI.h"
#include "Base/Render/RenderWindow.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Render/RenderViewport.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/Shaders/Renderer/RendererTypes.esh"

namespace EE::Render
{
    void ForwardShadingMaterialShaderPipelineBucket::Initialize( RHI::Context* pContextRHI, MaterialShader const& shader )
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

        RHI::MeshPipelineParameters depthOnlyPipelineParameters_LowPrecision = depthOnlyPipelineParameters;
        depthOnlyPipelineParameters_LowPrecision.m_depthStencilFormat = RHI::DataFormat::D16_UNorm;

        RHI::MeshPipelineParameters depthOnlyAlphaTestPipelineParameters_LowPrecision = depthOnlyAlphaTestPipelineParameters;
        depthOnlyAlphaTestPipelineParameters_LowPrecision.m_depthStencilFormat = RHI::DataFormat::D16_UNorm;

        //-------------------------------------------------------------------------

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

        depthOnlyPipelineParameters_LowPrecision.m_debugName.sprintf( "%s DepthOnly LowPrecision Pipeline", shader.m_shaderName.c_str() );
        depthOnlyPipelineParameters_LowPrecision.m_pRootSignature = shader.m_pRootSignature;
        depthOnlyPipelineParameters_LowPrecision.m_pShader = shader.m_shaders[MaterialShader::DepthOnly];

        depthOnlyAlphaTestPipelineParameters_LowPrecision.m_debugName.sprintf( "%s DepthOnly_AlphaTest LowPrecision Pipeline", shader.m_shaderName.c_str() );
        depthOnlyAlphaTestPipelineParameters_LowPrecision.m_pRootSignature = shader.m_pRootSignature;
        depthOnlyAlphaTestPipelineParameters_LowPrecision.m_pShader = shader.m_shaders[MaterialShader::DepthOnly | MaterialShader::AlphaTest];

        #if EE_DEVELOPMENT_TOOLS
        RHI::DataFormat outlineIDColorFormats[] = { RHI::DataFormat::R32_UInt };

        RHI::MeshPipelineParameters outlineIDPipelineParameters = depthOnlyPipelineParameters_LowPrecision;
        outlineIDPipelineParameters.m_colorFormats = outlineIDColorFormats;
        outlineIDPipelineParameters.m_numRenderTargets = 1;
        outlineIDPipelineParameters.m_debugName.sprintf( "%s OutlineID Pipeline", shader.m_shaderName.c_str() );
        outlineIDPipelineParameters.m_pShader = shader.m_pOutlineShader;
        #endif

        //-------------------------------------------------------------------------

        m_shaderName = shader.m_shaderName.c_str();
        m_pCommandSignature = shader.m_pCommandSignature;

        m_pDepthOnlyPipeline = RHI::CreatePipeline( pContextRHI, depthOnlyPipelineParameters );
        m_pDepthOnlyAlphaTestPipeline = RHI::CreatePipeline( pContextRHI, depthOnlyAlphaTestPipelineParameters );
        m_pOpaquePipeline = RHI::CreatePipeline( pContextRHI, opaquePipelineParameters );
        m_pAlphaBlendPipeline = RHI::CreatePipeline( pContextRHI, alphaBlendPipelineParameters );
        m_pDepthOnlyPipeline_LowPrecision = RHI::CreatePipeline( pContextRHI, depthOnlyPipelineParameters_LowPrecision );
        m_pDepthOnlyAlphaTestPipeline_LowPrecision = RHI::CreatePipeline( pContextRHI, depthOnlyAlphaTestPipelineParameters_LowPrecision );

        #if EE_DEVELOPMENT_TOOLS
        if ( shader.m_pOutlineShader )
        {
            m_pOutlinePipeline = RHI::CreatePipeline( pContextRHI, outlineIDPipelineParameters );
        }
        #endif
    }

    void ForwardShadingMaterialShaderPipelineBucket::Shutdown( RHI::Context* pContextRHI )
    {
        RHI::DestroyPipeline( pContextRHI, eastl::move( m_pDepthOnlyPipeline ) );
        RHI::DestroyPipeline( pContextRHI, eastl::move( m_pDepthOnlyAlphaTestPipeline ) );
        RHI::DestroyPipeline( pContextRHI, eastl::move( m_pOpaquePipeline ) );
        RHI::DestroyPipeline( pContextRHI, eastl::move( m_pAlphaBlendPipeline ) );
        RHI::DestroyPipeline( pContextRHI, eastl::move( m_pDepthOnlyPipeline_LowPrecision ) );
        RHI::DestroyPipeline( pContextRHI, eastl::move( m_pDepthOnlyAlphaTestPipeline_LowPrecision ) );

        #if EE_DEVELOPMENT_TOOLS
        RHI::DestroyPipeline( pContextRHI, eastl::move( m_pOutlinePipeline ) );
        #endif
    }

    //-------------------------------------------------------------------------

    TVector<ForwardShadingMaterialShaderPipelineBucket> ForwardShadingPass::InitializeMaterialShaderBuckets( RenderSystem* pRenderSystem )
    {
        RHI::Context* pContextRHI = pRenderSystem->GetContextRHI();
        TVector<MaterialShader> const& materialShaders = pRenderSystem->GetMaterialShaders();

        TVector<ForwardShadingMaterialShaderPipelineBucket> materialShaderBuckets;
        materialShaderBuckets.reserve( materialShaders.size() );

        for ( MaterialShader const& shader : materialShaders )
        {
            ForwardShadingMaterialShaderPipelineBucket& shaderPipelineBucket = materialShaderBuckets.emplace_back();
            shaderPipelineBucket.Initialize( pContextRHI, shader );
        }

        return materialShaderBuckets;
    }

    void ForwardShadingPass::DrawMaterialShaderBuckets_DepthOnly
    (
        TArrayView<ForwardShadingMaterialShaderPipelineBucket const> materialShaderBuckets,
        ActiveRenderView const&                                      renderView,
        RHI::Texture*                                                pDepthTexture,
        uint32_t                                                     depthTargetSlice,
        RHI::CommandBuffer*                                          pCommandBuffer
    )
    {
        bool const isLowPrecisionDepth = pDepthTexture->m_format == RHI::DataFormat::D16_UNorm;

        RHI::LoadAction depthOnlyLoadAction = {};
        depthOnlyLoadAction.m_loadActionDepth = RHI::LoadActionType::Clear;

        RHI::CmdSetRenderTargets( pCommandBuffer, {}, pDepthTexture, &depthOnlyLoadAction, {}, {}, depthTargetSlice );

        // Depth only pass
        //-------------------------------------------------------------------------

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Forward Shading Depth Only Pass" );

        for ( size_t shaderIndex = 0; shaderIndex < materialShaderBuckets.size(); ++shaderIndex )
        {
            ForwardShadingMaterialShaderPipelineBucket const& shaderPipelineBucket = materialShaderBuckets[shaderIndex];

            ActiveRenderViewBucket const& renderViewBucket = renderView.m_renderViewBuckets[shaderIndex];
            uint32_t const bucketIndirectCommandCapacity = uint32_t( renderViewBucket.m_opaqueBucket.m_drawArgumentBuffer.m_pBuffer->m_size / sizeof( ShaderTypes::DrawArgument ) );

            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, shaderPipelineBucket.m_shaderName.data() );

                RHI::CmdSetPipeline( pCommandBuffer, isLowPrecisionDepth ? shaderPipelineBucket.m_pDepthOnlyPipeline_LowPrecision : shaderPipelineBucket.m_pDepthOnlyPipeline );
                RHI::CmdSetRootConstants( pCommandBuffer, 0, nullptr, sizeof( ShaderTypes::DrawRootConstants ) );
                {
                    ActiveRenderViewMaterialShaderBucket const& renderBucket = renderViewBucket.m_opaqueBucket;

                    RHI::CmdExecuteIndirect
                    (
                        pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                        renderBucket.m_drawArgumentBuffer.m_pBuffer, 0,
                        renderBucket.m_pDrawCounterBuffer, 0
                    );
                }
            }

            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, shaderPipelineBucket.m_shaderName.data() );

                RHI::CmdSetPipeline( pCommandBuffer, isLowPrecisionDepth ? shaderPipelineBucket.m_pDepthOnlyAlphaTestPipeline_LowPrecision : shaderPipelineBucket.m_pDepthOnlyAlphaTestPipeline );
                RHI::CmdSetRootConstants( pCommandBuffer, 0, nullptr, sizeof( ShaderTypes::DrawRootConstants ) );
                {
                    ActiveRenderViewMaterialShaderBucket const& renderBucket = renderViewBucket.m_alphaTestBucket;

                    RHI::CmdExecuteIndirect
                    (
                        pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                        renderBucket.m_drawArgumentBuffer.m_pBuffer, 0,
                        renderBucket.m_pDrawCounterBuffer, 0
                    );
                }
            }
        }
    }

    #if EE_DEVELOPMENT_TOOLS

    void ForwardShadingPass::DrawMaterialShaderBuckets_OutlineID
    (
        TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
        ActiveRenderView const&                                         renderView,
        RHI::Texture*                                                   pObjectIDTexture,
        RHI::Texture*                                                   pDepthTexture,
        RHI::CommandBuffer*                                             pCommandBuffer
    )
    {
        RHI::LoadAction outlineIDLoadAction = {};
        outlineIDLoadAction.m_loadActionsColor[0] = RHI::LoadActionType::Load;
        outlineIDLoadAction.m_loadActionDepth = RHI::LoadActionType::Clear;

        RHI::CmdSetRenderTargets( pCommandBuffer, { &pObjectIDTexture, 1 }, pDepthTexture, &outlineIDLoadAction );

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Forward Shading Object ID Pass" );

        for ( size_t shaderIndex = 0; shaderIndex < materialShaderBuckets.size(); ++shaderIndex )
        {
            ForwardShadingMaterialShaderPipelineBucket const& shaderPipelineBucket = materialShaderBuckets[shaderIndex];

            if ( !shaderPipelineBucket.m_pOutlinePipeline )
            {
                continue;
            }

            ActiveRenderViewBucket const& renderViewBucket = renderView.m_renderViewBuckets[shaderIndex];
            uint32_t const bucketIndirectCommandCapacity = uint32_t( renderViewBucket.m_opaqueBucket.m_drawArgumentBuffer.m_pBuffer->m_size / sizeof( ShaderTypes::DrawArgument ) );

            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, shaderPipelineBucket.m_shaderName.data() );

                RHI::CmdSetPipeline( pCommandBuffer, shaderPipelineBucket.m_pOutlinePipeline );
                RHI::CmdSetRootConstants( pCommandBuffer, 0, nullptr, sizeof( ShaderTypes::DrawRootConstants ) );
                {
                    ActiveRenderViewMaterialShaderBucket const& renderBucket = renderViewBucket.m_opaqueBucket;

                    RHI::CmdExecuteIndirect
                    (
                        pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                        renderBucket.m_drawArgumentBuffer.m_pBuffer, 0,
                        renderBucket.m_pDrawCounterBuffer, 0
                    );
                }
            }

            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, shaderPipelineBucket.m_shaderName.data() );

                RHI::CmdSetPipeline( pCommandBuffer, shaderPipelineBucket.m_pOutlinePipeline );
                RHI::CmdSetRootConstants( pCommandBuffer, 0, nullptr, sizeof( ShaderTypes::DrawRootConstants ) );
                {
                    ActiveRenderViewMaterialShaderBucket const& renderBucket = renderViewBucket.m_alphaTestBucket;

                    RHI::CmdExecuteIndirect
                    (
                        pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                        renderBucket.m_drawArgumentBuffer.m_pBuffer, 0,
                        renderBucket.m_pDrawCounterBuffer, 0
                    );
                }
            }
        }
    }

    #endif

    void ForwardShadingPass::DrawMaterialShaderBuckets_Shading
    (
        TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
        ActiveRenderView const&                                         renderView,
        RHI::Texture*                                                   pColorTexture,
        uint32_t                                                        colorTargetSlice,
        uint32_t                                                        colorTargetMipSlice,
        RHI::Texture*                                                   pDepthTexture,
        RHI::CommandBuffer*                                             pCommandBuffer
    )
    {
        // Opaque Color pass
        //-------------------------------------------------------------------------

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

                uint32_t const bucketIndirectCommandCapacity = uint32_t( renderView.m_renderViewBuckets[shaderIndex].m_opaqueBucket.m_drawArgumentBuffer.m_pBuffer->m_size / sizeof( ShaderTypes::DrawArgument ) );
                ActiveRenderViewBucket const& renderViewBucket = renderView.m_renderViewBuckets[shaderIndex];

                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, shaderPipelineBucket.m_shaderName.data() );

                RHI::CmdSetPipeline( pCommandBuffer, shaderPipelineBucket.m_pOpaquePipeline );
                RHI::CmdSetRootConstants( pCommandBuffer, 0, nullptr, sizeof( ShaderTypes::DrawRootConstants ) );

                RHI::CmdExecuteIndirect
                (
                    pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                    renderViewBucket.m_opaqueBucket.m_drawArgumentBuffer.m_pBuffer, 0,
                    renderViewBucket.m_opaqueBucket.m_pDrawCounterBuffer, 0
                );

                RHI::CmdExecuteIndirect
                (
                    pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                    renderViewBucket.m_alphaTestBucket.m_drawArgumentBuffer.m_pBuffer, 0,
                    renderViewBucket.m_alphaTestBucket.m_pDrawCounterBuffer, 0
                );
            }
        }

        // Alpha Blend Color pass
        //-------------------------------------------------------------------------

        RHI::LoadAction alphaBlendColorLoadAction = {};
        alphaBlendColorLoadAction.m_loadActionsColor[0] = RHI::LoadActionType::Load;
        alphaBlendColorLoadAction.m_loadActionDepth = RHI::LoadActionType::Load;

        RHI::CmdSetRenderTargets( pCommandBuffer, { &pColorTexture, 1 }, pDepthTexture, &alphaBlendColorLoadAction, colorArraySlices, colorMipSlices );

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Forward Shading Alpha Blend Pass" );

        for ( size_t shaderIndex = 0; shaderIndex < materialShaderBuckets.size(); ++shaderIndex )
        {
            ForwardShadingMaterialShaderPipelineBucket const& shaderPipelineBucket = materialShaderBuckets[shaderIndex];

            uint32_t const bucketIndirectCommandCapacity = uint32_t( renderView.m_renderViewBuckets[shaderIndex].m_alphaBlendBucket.m_drawArgumentBuffer.m_pBuffer->m_size / sizeof( ShaderTypes::DrawArgument ) );
            ActiveRenderViewBucket const& renderViewBucket = renderView.m_renderViewBuckets[shaderIndex];

            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, shaderPipelineBucket.m_shaderName.data() );

                RHI::CmdSetPipeline( pCommandBuffer, shaderPipelineBucket.m_pDepthOnlyPipeline );
                RHI::CmdSetRootConstants( pCommandBuffer, 0, nullptr, sizeof( ShaderTypes::DrawRootConstants ) );
                RHI::CmdExecuteIndirect
                (
                    pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                    renderViewBucket.m_alphaBlendBucket.m_drawArgumentBuffer.m_pBuffer, 0,
                    renderViewBucket.m_alphaBlendBucket.m_pDrawCounterBuffer, 0
                );
            }

            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, shaderPipelineBucket.m_shaderName.data() );


                RHI::CmdSetPipeline( pCommandBuffer, shaderPipelineBucket.m_pAlphaBlendPipeline );
                RHI::CmdSetRootConstants( pCommandBuffer, 0, nullptr, sizeof( ShaderTypes::DrawRootConstants ) );
                RHI::CmdExecuteIndirect
                (
                    pCommandBuffer, shaderPipelineBucket.m_pCommandSignature, bucketIndirectCommandCapacity,
                    renderViewBucket.m_alphaBlendBucket.m_drawArgumentBuffer.m_pBuffer, 0,
                    renderViewBucket.m_alphaBlendBucket.m_pDrawCounterBuffer, 0
                );
            }
        }
    }

    void ForwardShadingPass::DrawMaterialShaderBuckets
    (
        TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
        ActiveRenderView const&                                         renderView,
        RHI::Texture*                                                   pColorTexture,
        uint32_t                                                        colorTargetSlice,
        uint32_t                                                        colorTargetMipSlice,
        RHI::Texture*                                                   pDepthTexture,
        RHI::CommandBuffer*                                             pCommandBuffer
    )
    {
        DrawMaterialShaderBuckets_DepthOnly
        (
            materialShaderBuckets,
            renderView,
            pDepthTexture,
            0,
            pCommandBuffer
        );
        DrawMaterialShaderBuckets_Shading
        (
            materialShaderBuckets,
            renderView,
            pColorTexture,
            colorTargetSlice,
            colorTargetMipSlice,
            pDepthTexture,
            pCommandBuffer
        );
    }

    //-------------------------------------------------------------------------

    void ForwardShadingPass::Initialize( RenderPassContext const& context )
    {}

    void ForwardShadingPass::Shutdown( RenderSystem* pRenderSystem )
    {}

    void ForwardShadingPass::UpdateWorldDeviceResources( EntityWorld* pWorld )
    {
        EE_PROFILE_FUNCTION_RENDER();

        for ( Viewport* pViewport : pWorld->GetViewports() )
        {
            if ( !pViewport->IsValid() )
            {
                continue;
            }

            RenderViewport* pRenderViewport = static_cast<RenderViewport*>( pViewport );
            RenderViewProxy& renderViewProxy = pRenderViewport->m_mainRenderViewProxy;

            EE_ASSERT( renderViewProxy.IsValid() );
            EE_ASSERT( renderViewProxy.GetNumRenderViews() == 1 );

            renderViewProxy.StartRenderViewWrite();
            renderViewProxy.WriteRenderView( 0, pRenderViewport->GetViewVolume(), pRenderViewport->GetSize(), ShaderTypes::RENDER_VIEW_FLAG_NONE );
            renderViewProxy.SubmitRenderViewWrite();
        }
    }

    void ForwardShadingPass::UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport )
    {
        EE_PROFILE_FUNCTION_RENDER();

        Int2 textureSize = pRenderViewport->GetSize();
        uint32_t textureWidth = uint32_t( textureSize.m_x );
        uint32_t textureHeight = uint32_t( textureSize.m_y );

        if ( pRenderViewport->TextureNeedsResize( pRenderViewport->m_forwardShading_colorTexture ) )
        {
            pRenderSystem->QueueResourceDelete
            (
                eastl::move( pRenderViewport->m_forwardShading_depthTexture ),
                eastl::move( pRenderViewport->m_forwardShading_colorTexture )
            );

            RHI::TextureParameters depthParameters = {};
            depthParameters.m_width = textureWidth;
            depthParameters.m_height = textureHeight;
            depthParameters.m_format = RHI::DataFormat::D32_SFloat;
            depthParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::RenderTarget, RHI::DescriptorTypeFlags::Texture };
            depthParameters.m_debugName.sprintf( "ForwardShadingPass Depth Target %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_forwardShading_depthTexture = RHI::CreateTexture( pRenderSystem->GetContextRHI(), depthParameters );

            RHI::TextureParameters hdrParameters = depthParameters;
            hdrParameters.m_format = RHI::DataFormat::RG11_B10_UFloat;
            hdrParameters.m_clearValue = { { 0.0F, 0.0F, 0.0F, 1.0F } };
            hdrParameters.m_debugName.sprintf( "ForwardShadingPass HDR Target %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_forwardShading_colorTexture = RHI::CreateTexture( pRenderSystem->GetContextRHI(), hdrParameters );
        }
    }

    void ForwardShadingPass::DepthOnlyPass
    (
        TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
        ActiveRenderView const&                                         activeRenderView,
        RenderViewport const*                                           pRenderViewport,
        DeviceResourceStates&                                           resourceStates,
        RHI::CommandBuffer*                                             pCommandBuffer
    ) const
    {
        Float2 const viewSize = pRenderViewport->GetSize();

        RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, viewSize.m_x, viewSize.m_y, 0.0F, 1.0F );
        RHI::CmdSetScissor( pCommandBuffer, 0, 0, uint32_t( viewSize.m_x ), uint32_t( viewSize.m_y ) );

        EE_ASSERT( !resourceStates.HasPendingBarriers() );
        resourceStates.Writeable( pRenderViewport->m_forwardShading_depthTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::DepthWrite, RHI::TextureState::DepthWrite );
        resourceStates.FlushBarriers( pCommandBuffer );

        DrawMaterialShaderBuckets_DepthOnly
        (
            materialShaderBuckets,
            activeRenderView,
            pRenderViewport->m_forwardShading_depthTexture,
            0,
            pCommandBuffer
        );
    }

    void ForwardShadingPass::ShadingPass
    (
        TArrayView<ForwardShadingMaterialShaderPipelineBucket const>      materialShaderBuckets,
        ActiveRenderView const&                                           activeRenderView,
        RenderViewport const*                                             pRenderViewport,
        DeviceResourceStates&                                             resourceStates,
        RHI::CommandBuffer*                                               pCommandBuffer
    ) const
    {
        Float2 const viewSize = pRenderViewport->GetSize();

        RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, viewSize.m_x, viewSize.m_y, 0.0F, 1.0F );
        RHI::CmdSetScissor( pCommandBuffer, 0, 0, uint32_t( viewSize.m_x ), uint32_t( viewSize.m_y ) );

        EE_ASSERT( !resourceStates.HasPendingBarriers() );
        resourceStates.Writeable( pRenderViewport->m_forwardShading_colorTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
        resourceStates.Writeable( pRenderViewport->m_forwardShading_depthTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::DepthWrite, RHI::TextureState::DepthWrite );
        resourceStates.FlushBarriers( pCommandBuffer );

        DrawMaterialShaderBuckets_Shading
        (
            materialShaderBuckets,
            activeRenderView,
            pRenderViewport->m_forwardShading_colorTexture, ~0U, 0,
            pRenderViewport->m_forwardShading_depthTexture,
            pCommandBuffer
        );
    }
}
