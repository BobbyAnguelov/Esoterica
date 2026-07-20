#include "RenderPass_CascadedShadow.h"
#include "RenderPass_ForwardShading.h" // TODO: Decouple forward shading
#include "Engine/Render/RenderViewport.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/Shaders/Renderer/RendererTypes.esh"
#include "Base/Render/RHI.h"
#include "Base/Render/Settings/Settings_Render.h"
#include "Base/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    void CascadedShadowPass::Initialize( RenderPassContext const& context )
    {
        m_pRenderSettings = context.m_pRenderSettings;
        for ( DeviceRenderView& renderView : m_renderViews )
        {
            renderView.Initialize( context.m_pRenderSystem, context.m_materialShaderPipelineBuckets.size() );
        }
    }

    void CascadedShadowPass::Shutdown( RenderSystem* pRenderSystem )
    {
        for ( size_t cascadeIndex = 0; cascadeIndex < NumShadowCascades; ++cascadeIndex )
        {
            m_renderViews[cascadeIndex].Shutdown( pRenderSystem );
        }
        RHI::DestroyTexture( pRenderSystem->GetContextRHI(), eastl::move( m_depthTargetArray ) );
    }

    void CascadedShadowPass::UpdateDeviceResources( RenderSystem* pRenderSystem, TArrayView<uint32_t const> clusterCapacity )
    {
        EE_PROFILE_FUNCTION_RENDER();

        if ( !m_depthTargetArray ||
             m_depthTargetArray->m_width != m_pRenderSettings->m_cascadedShadowResolution ||
             m_depthTargetArray->m_height != m_pRenderSettings->m_cascadedShadowResolution )
        {
            pRenderSystem->QueueResourceDelete( eastl::move( m_depthTargetArray ) );

            RHI::TextureParameters depthParameters = {};
            depthParameters.m_width = m_pRenderSettings->m_cascadedShadowResolution;
            depthParameters.m_height = m_pRenderSettings->m_cascadedShadowResolution;
            depthParameters.m_arrayLayers = uint32_t( NumShadowCascades );
            depthParameters.m_format = RHI::DataFormat::D32_SFloat;
            depthParameters.m_descriptorTypes = TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::RenderTarget, RHI::DescriptorTypeFlags::Texture );
            depthParameters.m_debugName.sprintf( "CascadedShadowPass Shadow Depth Target %i Cascades", NumShadowCascades );

            m_depthTargetArray = RHI::CreateTexture( pRenderSystem->GetContextRHI(), depthParameters );
        }

        for ( size_t cascadeIndex = 0; cascadeIndex < NumShadowCascades; ++cascadeIndex )
        {
            m_renderViews[cascadeIndex].UpdateDeviceResources( pRenderSystem, clusterCapacity );
        }
    }

    void CascadedShadowPass::DrawShadowCascades( TArrayView<ForwardShadingMaterialShaderPipelineBucket const>   materialShaderPipelineBuckets,
                                                 TArrayView<uint32_t const>                                     clusterCapacity,
                                                 RenderViewport const*                                          pRenderViewport,
                                                 Vector                                                         lightDirection,
                                                 DeviceResourceStates&                                          resourceStates,
                                                 RHI::CommandBuffer*                                            pCommandBuffer,
                                                 ShaderTypes::CascadedShadow*                                   pOutCascadedShadow_WriteCombined,
                                                 TArrayView<ShaderTypes::RenderView>                            dstRenderViews_WriteCombined )
    {
        EE_PROFILE_FUNCTION_RENDER();

        float const ShadowMapResolution = float( m_pRenderSettings->m_cascadedShadowResolution );
        float const ShadowMapTexelSize = 1.0F / ShadowMapResolution;

        Matrix reverseZMatrix( Vector( 1.0f, 0.0f, 0.0f, 0.0f ),
                               Vector( 0.0f, 1.0f, 0.0f, 0.0f ),
                               Vector( 0.0f, 0.0f, -1.0f, 0.0f ),
                               Vector( 0.0f, 0.0f, 1.0f, 1.0f ) );

        Matrix textureScaleBiasMatrix( Vector( 0.5F, 0.0F, 0.0F, 0.0F ),
                                       Vector( 0.0F, -0.5F, 0.0F, 0.0F ),
                                       Vector( 0.0F, 0.0F, 1.0F, 0.0F ),
                                       Vector( 0.5F, 0.5F, 0.0F, 1.0F ) );

        Math::ViewVolume::VolumeCorners frustumCorners = pRenderViewport->GetViewVolume().GetCorners();

        pOutCascadedShadow_WriteCombined->m_cascadeSize[0] = ShadowMapResolution;
        pOutCascadedShadow_WriteCombined->m_cascadeSize[1] = ShadowMapResolution;
        pOutCascadedShadow_WriteCombined->m_cascadeSize[2] = ShadowMapTexelSize;
        pOutCascadedShadow_WriteCombined->m_cascadeSize[3] = ShadowMapTexelSize;

        pOutCascadedShadow_WriteCombined->m_shadowCascades = RHI::GetTextureHandle( m_depthTargetArray, RHI::DescriptorTypeFlags::Texture, 0 );

        lightDirection.SetW0();

        //---------------------------------------------------------------------------------------------------
        // Compute shadow matrix
        Matrix globalShadowMatrix = Matrix::Identity;
        {
            Vector frustumCenter = Vector::Zero;
            for ( Vector const& corner : frustumCorners.m_points )
            {
                frustumCenter += corner;
            }
            frustumCenter *= 1.0F / 8.0F;
            frustumCenter.SetW1();

            Matrix shadowProjectionMatrix = Math::CreateOrthographicProjectionMatrixOffCenter(
                -0.5F, 0.5F, -0.5F, 0.5F,
                0.0F, 1.0F );
            shadowProjectionMatrix = shadowProjectionMatrix * reverseZMatrix;

            Matrix shadowViewMatrix = Math::CreateLookAtMatrix( frustumCenter + lightDirection * 0.5F, frustumCenter, Vector::WorldUp );
            Matrix shadowViewProjectionMatrix = shadowViewMatrix * shadowProjectionMatrix;
            globalShadowMatrix = shadowViewProjectionMatrix * textureScaleBiasMatrix;

            std::memcpy(
                pOutCascadedShadow_WriteCombined->m_shadowMatrix,
                globalShadowMatrix.m_rows,
                sizeof( globalShadowMatrix ) );
        }

        //---------------------------------------------------------------------------------------------------
        // Compute splits

        TArray<float, NumShadowCascades> cascadeSplits = {};

        float lambda = 0.94F;
        float minDistance = 0.0F;
        float maxDistance = 1.0F;

        FloatRange depthRange = pRenderViewport->GetViewVolume().GetDepthRange();
        float      depthRangeLength = depthRange.GetLength();

        float minZ = depthRange.m_begin + minDistance * depthRangeLength;
        float maxZ = depthRange.m_begin + maxDistance * depthRangeLength;
        float zRange = maxZ - minZ;
        float zRatio = maxZ / minZ;

        for ( size_t split = 0; split < cascadeSplits.size(); ++split )
        {
            float power = float( split + 1 ) / float( NumShadowCascades );
            float log = minZ * Math::Pow( zRatio, power );
            float uniform = minZ + zRange * power;
            float distance = lambda * ( log - uniform ) + uniform;
            cascadeSplits[split] = ( distance - depthRange.m_begin ) / zRange;
        }

        //-------------------------------------------------------------------------------------------------------
        // Begin rendering

        Float2 const viewTopLeft = Float2( 0.0F, 0.0F );
        Float2 const viewSize = Float2( ShadowMapResolution, ShadowMapResolution );

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Cascaded Shadows" );

        RHI::CmdSetViewport( pCommandBuffer,
                     viewTopLeft.m_x, viewTopLeft.m_y, viewSize.m_x, viewSize.m_y,
                     0.0F, 1.0F );
        RHI::CmdSetScissor( pCommandBuffer,
                            uint32_t( viewTopLeft.m_x ), uint32_t( viewTopLeft.m_y ),
                            uint32_t( viewSize.m_x ), uint32_t( viewSize.m_y ) );

        for ( uint32_t cascadeIndex = 0; cascadeIndex < NumShadowCascades; ++cascadeIndex )
        {
            //---------------------------------------------------------------------------------------------------
            // Compute splits

            float const previousSplitDistance = cascadeIndex ? cascadeSplits[cascadeIndex - 1] : minDistance;
            float const splitDistance = cascadeSplits[cascadeIndex];

            TArray<Vector, 8> splitFrustumCorners = {};
            for ( size_t cornerIndex = 0; cornerIndex < 4; ++cornerIndex )
            {
                Vector frustumRay = frustumCorners.m_points[cornerIndex + 4] - frustumCorners.m_points[cornerIndex];
                splitFrustumCorners[cornerIndex] = frustumCorners.m_points[cornerIndex] + frustumRay * previousSplitDistance;
                splitFrustumCorners[cornerIndex + 4] = frustumCorners.m_points[cornerIndex] + frustumRay * splitDistance;
            }

            Vector splitFrustumCenter = Vector::Zero;
            for ( Vector const& corner : splitFrustumCorners )
            {
                splitFrustumCenter += corner;
            }
            splitFrustumCenter *= 1.0F / 8.0F;
            splitFrustumCenter.SetW1();

            float sphereRadius = 0.0F;
            for ( Vector const& corner : splitFrustumCorners )
            {
                sphereRadius = Math::Max( sphereRadius, corner.GetDistance3( splitFrustumCenter ) );
            }

            sphereRadius = Math::Ceiling( sphereRadius * 16.0F ) / 16.0F;

            Matrix shadowProjectionMatrix = Math::CreateOrthographicProjectionMatrixOffCenter(
                -sphereRadius, sphereRadius, -sphereRadius, sphereRadius,
                -sphereRadius * 2.0F, sphereRadius * 2.0F );
            shadowProjectionMatrix = shadowProjectionMatrix * reverseZMatrix;

            Matrix shadowViewMatrix = Math::CreateLookAtMatrix( splitFrustumCenter, splitFrustumCenter - lightDirection, Vector::WorldUp );
            Matrix shadowViewProjectionMatrix = shadowViewMatrix * shadowProjectionMatrix;

            //---------------------------------------------------------------------------------------------------
            // Snap shadowmap to texel center
            Vector shadowOrigin = shadowViewProjectionMatrix.TransformVector4( Vector( 0.0F, 0.0F, 0.0F, 1.0F ) );
            shadowOrigin *= ShadowMapResolution * 0.5F;

            Vector roundedOffset = shadowOrigin.GetRound() - shadowOrigin;
            roundedOffset *= 2.0F / ShadowMapResolution;
            roundedOffset.SetZ( 0.0F );
            roundedOffset.SetW( 0.0F );

            shadowProjectionMatrix.m_rows[3] += roundedOffset;
            shadowViewProjectionMatrix = shadowViewMatrix * shadowProjectionMatrix;

            Matrix shadowViewProjectionInverseMatrix = shadowViewProjectionMatrix.GetInverse();

            // Write render view data
            //---------------------------------------------------------------------------------------------------

            alignas( 32 ) ShaderTypes::RenderView shadowRenderView = {};
            std::memcpy
            (
                shadowRenderView.m_viewProjectionMatrix,
                shadowViewProjectionMatrix.m_rows,
                sizeof( shadowRenderView.m_viewProjectionMatrix )
            );
            std::memcpy
            (
                shadowRenderView.m_viewMatrix,
                shadowViewMatrix.m_rows,
                sizeof( shadowRenderView.m_viewMatrix )
            );
            std::memcpy
            (
                shadowRenderView.m_inverseViewProjectionMatrix,
                shadowViewProjectionInverseMatrix.m_rows,
                sizeof( shadowRenderView.m_inverseViewProjectionMatrix )
            );

            shadowRenderView.m_renderTargetSize[0] = ShadowMapResolution;
            shadowRenderView.m_renderTargetSize[1] = ShadowMapResolution;
            shadowRenderView.m_renderTargetSize[2] = ShadowMapTexelSize;
            shadowRenderView.m_renderTargetSize[3] = ShadowMapTexelSize;

            shadowRenderView.m_projectionP00 = shadowProjectionMatrix.m_values[0][0];
            shadowRenderView.m_projectionP11 = shadowProjectionMatrix.m_values[1][1];
            shadowRenderView.m_znear = -sphereRadius * 2.0F;

            shadowRenderView.m_renderViewFlags = ShaderTypes::RENDER_VIEW_FLAG_DEPTH_ONLY;
            shadowRenderView.m_renderViewLayerFlags = ShaderTypes::RENDER_VIEW_LAYER_FLAG_SHADOW_MAP;

            Memory::CopyToWriteCombined( &dstRenderViews_WriteCombined[cascadeIndex], &shadowRenderView, sizeof( shadowRenderView ) );

            //-------------------------------------------------------------------------------------------------------
            // Write proxy data

            Matrix cascadeShadowMatrixInverse = ( shadowViewProjectionMatrix * textureScaleBiasMatrix ).GetInverse();

            Vector cascadeCorner0 = globalShadowMatrix.TransformVector3( cascadeShadowMatrixInverse.TransformVector3( Vector::Zero ) );
            Vector cascadeCorner1 = globalShadowMatrix.TransformVector3( cascadeShadowMatrixInverse.TransformVector3( Vector::One ) );

            Vector cascadeScale = Vector::One / ( cascadeCorner1 - cascadeCorner0 );
            cascadeScale.SetW0();

            Vector cascadeOffset = -cascadeCorner0;
            cascadeOffset.SetW0();

            cascadeOffset.Store( pOutCascadedShadow_WriteCombined->m_cascadeOffsets[cascadeIndex] );
            cascadeScale.Store( pOutCascadedShadow_WriteCombined->m_cascadeScales[cascadeIndex] );

            // Depth only pass
            //---------------------------------------------------------------------------------------------------

            EE_ASSERT( !resourceStates.HasPendingBarriers() );

            resourceStates.Writeable( m_depthTargetArray, RHI::PipelineStage::Draw, RHI::ResourceAccess::DepthWrite, RHI::TextureState::DepthWrite );
            resourceStates.FlushBarriers( pCommandBuffer );

            RHI::LoadAction depthTargetLoadAction = {};
            depthTargetLoadAction.m_loadActionDepth = RHI::LoadActionType::Clear;

            RHI::CmdSetRenderTargets( pCommandBuffer, {}, m_depthTargetArray, &depthTargetLoadAction, {}, {}, cascadeIndex );

            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Cascade" );

            for ( size_t shaderIndex = 0; shaderIndex < materialShaderPipelineBuckets.size(); ++shaderIndex )
            {
                ForwardShadingMaterialShaderPipelineBucket const& shaderPipelineBucket = materialShaderPipelineBuckets[shaderIndex];

                uint32_t const bucketIndirectCommandCapacity = Math::IntegerDivideCeiling<uint32_t>( clusterCapacity[shaderIndex], RHI::MaxDispatchSize );
                DeviceRenderViewBucket const& renderViewBucket = m_renderViews[cascadeIndex].m_renderViewBuckets[shaderIndex];

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

                    RHI::CmdSetPipeline( pCommandBuffer, shaderPipelineBucket.m_pDepthOnlyPipeline );
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
    }
}
