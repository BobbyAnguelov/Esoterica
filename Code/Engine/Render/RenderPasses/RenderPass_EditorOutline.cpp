#include "Base/Esoterica.h"
#include "RenderPass_EditorOutline.h"

#if EE_DEVELOPMENT_TOOLS

#include "Engine/Render/RenderViewport.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Engine/Render/RenderPasses/RenderPass_ForwardShading.h"
#include "Engine/Render/Shaders/Renderer/RendererTypes.esh"
#include "Engine/Render/Shaders/EditorOutline/EditorOutline_Initialize.esf"
#include "Engine/Render/Shaders/EditorOutline/EditorOutline_JumpFlood.esf"
#include "Engine/Render/Shaders/EditorOutline/EditorOutline_Composite.esf"
#include "Base/Profiling.h"

namespace EE::Render
{
    void EditorOutlineRenderPass::Initialize( RenderPassContext const& context )
    {
        m_pRenderSettings = context.m_pRenderSettings;

        static StringID s_InitializeShaderID = StringID( "EditorOutline_Initialize" );
        static StringID s_JumpFloodShaderID = StringID( "EditorOutline_JumpFlood" );
        static StringID s_CompositeShaderID = StringID( "EditorOutline_Composite" );

        SurfaceShader const* pInitializeShader = context.m_pRenderSystem->FindSurfaceShader( s_InitializeShaderID );
        SurfaceShader const* pJumpFloodShader = context.m_pRenderSystem->FindSurfaceShader( s_JumpFloodShaderID );
        SurfaceShader const* pCompositeShader = context.m_pRenderSystem->FindSurfaceShader( s_CompositeShaderID );

        RHI::DataFormat seedTextureFormats[] = { RHI::DataFormat::RG16_UNorm };

        RHI::GraphicsPipelineParameters initializePipelineParameters = {};
        initializePipelineParameters.m_colorFormats = seedTextureFormats;
        initializePipelineParameters.m_numRenderTargets = 1;
        initializePipelineParameters.m_pShader = pInitializeShader->m_pShader;
        initializePipelineParameters.m_pRootSignature = pInitializeShader->m_pRootSignature;
        initializePipelineParameters.m_debugName.sprintf( "%s Pipeline", pInitializeShader->m_shaderName.c_str() );

        m_pInitializePipeline = RHI::CreatePipeline( context.m_pRenderSystem->GetContextRHI(), initializePipelineParameters );

        RHI::GraphicsPipelineParameters jumpFloodPipelineParameters = initializePipelineParameters;
        jumpFloodPipelineParameters.m_pShader = pJumpFloodShader->m_pShader;
        jumpFloodPipelineParameters.m_pRootSignature = pJumpFloodShader->m_pRootSignature;
        jumpFloodPipelineParameters.m_debugName.sprintf( "%s Pipeline", pJumpFloodShader->m_shaderName.c_str() );

        m_pJumpFloodPipeline = RHI::CreatePipeline( context.m_pRenderSystem->GetContextRHI(), jumpFloodPipelineParameters );

        RHI::DataFormat compositeColorFormats[] = { RHI::DataFormat::RGBA8_sRGB };

        RHI::BlendState compositeBlendState = {};
        compositeBlendState.m_blendEnabled = true;
        compositeBlendState.m_srcFactors[0] = RHI::BlendConstant::SrcAlpha;
        compositeBlendState.m_dstFactors[0] = RHI::BlendConstant::OneMinusSrcAlpha;
        compositeBlendState.m_srcAlphaFactors[0] = RHI::BlendConstant::SrcAlpha;
        compositeBlendState.m_dstAlphaFactors[0] = RHI::BlendConstant::OneMinusSrcAlpha;
        compositeBlendState.m_blendModes[0] = RHI::BlendMode::Add;
        compositeBlendState.m_blendModesAlpha[0] = RHI::BlendMode::Add;
        compositeBlendState.m_writeMasks[0] = 0x0F;
        compositeBlendState.m_renderTargetMask = RHI::BlendStateTargetFlags::Target0;

        RHI::GraphicsPipelineParameters compositePipelineParameters = {};
        compositePipelineParameters.m_colorFormats = compositeColorFormats;
        compositePipelineParameters.m_numRenderTargets = 1;
        compositePipelineParameters.m_blendState = compositeBlendState;
        compositePipelineParameters.m_pShader = pCompositeShader->m_pShader;
        compositePipelineParameters.m_pRootSignature = pCompositeShader->m_pRootSignature;
        compositePipelineParameters.m_debugName.sprintf( "%s Pipeline", pCompositeShader->m_shaderName.c_str() );

        m_pCompositePipeline = RHI::CreatePipeline( context.m_pRenderSystem->GetContextRHI(), compositePipelineParameters );
    }

    void EditorOutlineRenderPass::Shutdown( RenderSystem* pRenderSystem )
    {
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pInitializePipeline ) );
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pJumpFloodPipeline ) );
        RHI::DestroyPipeline( pRenderSystem->GetContextRHI(), eastl::move( m_pCompositePipeline ) );
    }

    void EditorOutlineRenderPass::UpdateWorldDeviceResources( EntityWorld* pWorld )
    {
        EE_PROFILE_FUNCTION_RENDER();

        for ( Viewport* pViewport : pWorld->GetViewports() )
        {
            if ( !pViewport->IsValid() )
            {
                continue;
            }

            RenderViewport* pRenderViewport = static_cast<RenderViewport*>( pViewport );

            if ( !pRenderViewport->IsPickingEnabled() )
            {
                continue;
            }

            RenderViewProxy& renderViewProxy = pRenderViewport->m_editorOutlineRenderViewProxy;

            EE_ASSERT( renderViewProxy.IsValid() );
            EE_ASSERT( renderViewProxy.GetNumRenderViews() == 1 );

            renderViewProxy.StartRenderViewWrite();
            renderViewProxy.WriteRenderView( 0, pRenderViewport->GetViewVolume(), pRenderViewport->GetSize(), ShaderTypes::RENDER_VIEW_FLAG_DEPTH_ONLY );
            renderViewProxy.SubmitRenderViewWrite();
        }
    }

    void EditorOutlineRenderPass::UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport )
    {
        EE_PROFILE_FUNCTION_RENDER();

        Int2 textureSize = pRenderViewport->GetSize();
        uint32_t textureWidth = uint32_t( textureSize.m_x );
        uint32_t textureHeight = uint32_t( textureSize.m_y );

        if ( !pRenderViewport->m_editorOutline_depthTexture || pRenderViewport->m_editorOutline_depthTexture->m_width != textureWidth || pRenderViewport->m_editorOutline_depthTexture->m_height != textureHeight )
        {
            pRenderSystem->QueueResourceDelete( eastl::move( pRenderViewport->m_editorOutline_depthTexture ) );

            RHI::TextureParameters depthParameters = {};
            depthParameters.m_width = textureWidth;
            depthParameters.m_height = textureHeight;
            depthParameters.m_format = RHI::DataFormat::D16_UNorm;
            depthParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::RenderTarget, RHI::DescriptorTypeFlags::Texture };
            depthParameters.m_debugName.sprintf( "EditorOutlinePass Depth Target %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_editorOutline_depthTexture = RHI::CreateTexture( pRenderSystem->GetContextRHI(), depthParameters );
        }

        if ( !pRenderViewport->m_editorOutline_JFA_texture0 || pRenderViewport->m_editorOutline_JFA_texture0->m_width != textureWidth || pRenderViewport->m_editorOutline_JFA_texture0->m_height != textureHeight )
        {
            pRenderSystem->QueueResourceDelete
            (
                eastl::move( pRenderViewport->m_editorOutline_JFA_texture0 ),
                eastl::move( pRenderViewport->m_editorOutline_JFA_texture1 )
            );

            RHI::TextureParameters seedTextureParameters = {};
            seedTextureParameters.m_width = textureWidth;
            seedTextureParameters.m_height = textureHeight;
            seedTextureParameters.m_format = RHI::DataFormat::RG16_UNorm;
            seedTextureParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::RenderTarget, RHI::DescriptorTypeFlags::Texture };
            seedTextureParameters.m_debugName.sprintf( "EditorOutlinePass JFA Target 0 %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_editorOutline_JFA_texture0 = RHI::CreateTexture( pRenderSystem->GetContextRHI(), seedTextureParameters );

            seedTextureParameters.m_debugName.sprintf( "EditorOutlinePass JFA Target 1 %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_editorOutline_JFA_texture1 = RHI::CreateTexture( pRenderSystem->GetContextRHI(), seedTextureParameters );
        }

        if ( !pRenderViewport->m_editorOutline_idTexture || pRenderViewport->m_editorOutline_idTexture->m_width != textureWidth || pRenderViewport->m_editorOutline_idTexture->m_height != textureHeight )
        {
            pRenderSystem->QueueResourceDelete( eastl::move( pRenderViewport->m_editorOutline_idTexture ) );

            RHI::TextureParameters idTextureParameters = {};
            idTextureParameters.m_width = textureWidth;
            idTextureParameters.m_height = textureHeight;
            idTextureParameters.m_format = RHI::DataFormat::R32_UInt;
            idTextureParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::RenderTarget, RHI::DescriptorTypeFlags::Texture, RHI::DescriptorTypeFlags::RWTexture };
            idTextureParameters.m_debugName.sprintf( "EditorOutlinePass Object ID Target %dx%d", textureWidth, textureHeight );

            pRenderViewport->m_editorOutline_idTexture = RHI::CreateTexture( pRenderSystem->GetContextRHI(), idTextureParameters );
        }
    }

    void EditorOutlineRenderPass::DrawToViewport
    (
        TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
        ActiveRenderView const&                                         activeRenderView,
        RenderViewport const*                                           pRenderViewport,
        DeviceResourceStates&                                           resourceStates,
        RHI::CommandBuffer*                                             pCommandBuffer
    ) const
    {
        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Editor Outline Object ID Pass" );

        Float2 const viewSize = Float2
        (
            float( pRenderViewport->m_editorOutline_depthTexture->m_width ),
            float( pRenderViewport->m_editorOutline_depthTexture->m_height )
        );

        RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, viewSize.m_x, viewSize.m_y, 0.0F, 1.0F );
        RHI::CmdSetScissor( pCommandBuffer, 0, 0, uint32_t( viewSize.m_x ), uint32_t( viewSize.m_y ) );

        //-------------------------------------------------------------------------

        {
            EE_ASSERT( !resourceStates.HasPendingBarriers() );
            resourceStates.Writeable( pRenderViewport->m_editorOutline_idTexture, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::TextureState::UnorderedAccess );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::CmdClearTexture( pCommandBuffer, pRenderViewport->m_editorOutline_idTexture.m_pTexture, ~0U );

            resourceStates.Writeable( pRenderViewport->m_editorOutline_idTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
            resourceStates.Writeable( pRenderViewport->m_editorOutline_depthTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::DepthWrite, RHI::TextureState::DepthWrite );
            resourceStates.FlushBarriers( pCommandBuffer );

            ForwardShadingPass::DrawMaterialShaderBuckets_OutlineID
            (
                materialShaderBuckets,
                activeRenderView,
                pRenderViewport->m_editorOutline_idTexture.m_pTexture,
                pRenderViewport->m_editorOutline_depthTexture.m_pTexture,
                pCommandBuffer
            );
        }
    }

    void EditorOutlineRenderPass::ResolveToViewport( RenderViewport const* pRenderViewport, DeviceResourceStates& resourceStates, RHI::CommandBuffer* pCommandBuffer ) const
    {
        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Editor Outline Resolve" );

        uint32_t const textureWidth = pRenderViewport->m_editorOutline_JFA_texture0->m_width;
        uint32_t const textureHeight = pRenderViewport->m_editorOutline_JFA_texture0->m_height;

        Float2 const viewportSize = pRenderViewport->GetSize();

        RHI::LoadAction seedLoadAction = {};
        seedLoadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;

        float const editorOutlineThickness = Math::Max( m_pRenderSettings->m_editorOutlineThickness, 0.0F );

        //-------------------------------------------------------------------------

        {
            EE_ASSERT( !resourceStates.HasPendingBarriers() );
            resourceStates.ReadOnly( pRenderViewport->m_editorOutline_idTexture, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
            resourceStates.Writeable( pRenderViewport->m_editorOutline_JFA_texture0, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::CmdSetRenderTargets( pCommandBuffer, { &pRenderViewport->m_editorOutline_JFA_texture0.m_pTexture, 1 }, nullptr, &seedLoadAction );
            RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( textureWidth ), float( textureHeight ), 0.0F, 1.0F );
            RHI::CmdSetScissor( pCommandBuffer, 0, 0, textureWidth, textureHeight );

            ShaderTypes::EditorOutlineInitializeResourceTableData initializeRootConstants = {};
            initializeRootConstants.SetObjectIDTexture( resourceStates, RHI::PipelineStage::PixelShader, pRenderViewport->m_editorOutline_idTexture );
            initializeRootConstants.m_targetSize[0] = float( textureWidth );
            initializeRootConstants.m_targetSize[1] = float( textureHeight );
            initializeRootConstants.m_targetSize[2] = 1.0F / float( textureWidth );
            initializeRootConstants.m_targetSize[3] = 1.0F / float( textureHeight );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::CmdSetPipeline( pCommandBuffer, m_pInitializePipeline );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &initializeRootConstants, sizeof( initializeRootConstants ) );
            RHI::CmdDraw( pCommandBuffer, 3, 0 );
        }

        //-------------------------------------------------------------------------

        uint32_t jumpFloodStepSize = 1;
        while ( jumpFloodStepSize < uint32_t( editorOutlineThickness * 0.5F ) + 3 )
        {
            jumpFloodStepSize <<= 1;
        }

        bool readIsTexture0 = true;

        for ( uint32_t stepSize = jumpFloodStepSize; stepSize >= 1; stepSize >>= 1 )
        {
            DeviceTextureState& inputTexture = readIsTexture0 ? pRenderViewport->m_editorOutline_JFA_texture0 : pRenderViewport->m_editorOutline_JFA_texture1;
            DeviceTextureState& outputTexture = readIsTexture0 ? pRenderViewport->m_editorOutline_JFA_texture1 : pRenderViewport->m_editorOutline_JFA_texture0;

            EE_ASSERT( !resourceStates.HasPendingBarriers() );
            resourceStates.ReadOnly( inputTexture, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
            resourceStates.Writeable( outputTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::CmdSetRenderTargets( pCommandBuffer, { &outputTexture.m_pTexture, 1 }, nullptr, &seedLoadAction );
            RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, float( textureWidth ), float( textureHeight ), 0.0F, 1.0F );
            RHI::CmdSetScissor( pCommandBuffer, 0, 0, textureWidth, textureHeight );

            ShaderTypes::EditorOutlineJumpFloodResourceTableData jumpFloodRootConstants = {};
            jumpFloodRootConstants.SetInputTexture( RHI::GetTextureHandle( inputTexture, RHI::DescriptorTypeFlags::Texture, 0 ) );
            jumpFloodRootConstants.m_stepSize = stepSize;
            jumpFloodRootConstants.m_screenSize[0] = float( textureWidth );
            jumpFloodRootConstants.m_screenSize[1] = float( textureHeight );
            jumpFloodRootConstants.m_screenSize[2] = 1.0F / float( textureWidth );
            jumpFloodRootConstants.m_screenSize[3] = 1.0F / float( textureHeight );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::CmdSetPipeline( pCommandBuffer, m_pJumpFloodPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &jumpFloodRootConstants, sizeof( jumpFloodRootConstants ) );
            RHI::CmdDraw( pCommandBuffer, 3, 0 );

            readIsTexture0 = !readIsTexture0;
        }

        //-------------------------------------------------------------------------

        DeviceTextureState& distanceTexture = readIsTexture0 ? pRenderViewport->m_editorOutline_JFA_texture0 : pRenderViewport->m_editorOutline_JFA_texture1;

        RHI::LoadAction compositeLoadAction = {};
        compositeLoadAction.m_loadActionsColor[0] = RHI::LoadActionType::Load;

        EE_ASSERT( !resourceStates.HasPendingBarriers() );
        resourceStates.ReadOnly( distanceTexture, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
        resourceStates.Writeable( pRenderViewport->m_finalTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
        resourceStates.FlushBarriers( pCommandBuffer );

        RHI::CmdSetRenderTargets( pCommandBuffer, { &pRenderViewport->m_finalTexture.m_pTexture, 1 }, nullptr, &compositeLoadAction );
        RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F, viewportSize.m_x, viewportSize.m_y, 0.0F, 1.0F );
        RHI::CmdSetScissor( pCommandBuffer, 0, 0, uint32_t( viewportSize.m_x ), uint32_t( viewportSize.m_y ) );

        Float4 outlineColor = m_pRenderSettings->m_editorOutlineColor.ToFloat4();

        ShaderTypes::EditorOutlineCompositeResourceTableData compositeRootConstants = {};
        compositeRootConstants.SetDistanceTexture( resourceStates, RHI::PipelineStage::PixelShader, distanceTexture );
        compositeRootConstants.m_parameters[0] = editorOutlineThickness;
        compositeRootConstants.m_parameters[1] = outlineColor.m_x;
        compositeRootConstants.m_parameters[2] = outlineColor.m_y;
        compositeRootConstants.m_parameters[3] = outlineColor.m_z;
        resourceStates.FlushBarriers( pCommandBuffer );

        RHI::CmdSetPipeline( pCommandBuffer, m_pCompositePipeline );
        RHI::CmdSetRootConstants( pCommandBuffer, 0, &compositeRootConstants, sizeof( compositeRootConstants ) );
        RHI::CmdDraw( pCommandBuffer, 3, 0 );
    }
}

#endif
