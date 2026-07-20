#include "Renderer_ForwardShading.h"
#include "Engine/Render/RenderViewport.h"
#include "Engine/Render/RenderMaterialShaderClusterCapacity.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/Systems/WorldSystem_Render.h"
#include "Engine/Render/Settings/ViewportSettings_Render.h"
#include "Engine/Render/Components/Component_Lights.h" // TODO: Move light components to DeviceRenderWorld
#include "Engine/Render/DebugMesh/DebugMeshRegistry.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/Profiling.h"
#include "Base/Render/Settings/Settings_Render.h"
#include "Base/Render/RenderWindow.h"
#include "Base/Render/RHI.h"
#include "Base/Math/ViewVolume.h"
#include "EASTL/bit.h"

#include "Engine/Render/Shaders/Renderer/RendererTypes.esh"
#include "Engine/Render/Shaders/Picking/Picking.esh"

//-------------------------------------------------------------------------

namespace EE::Render
{
    template<typename F>
    inline void ForwardShadingRenderer::ForEachRenderBucket( uint32_t numCascadedShadowPasses, F fn )
    {
        for ( DeviceRenderView& renderView : m_renderPass_GlobalEnvironmentMap.m_renderViews )
        {
            renderView.ForEachRenderBucket( fn );
        }
        for ( uint32_t cascadedShadowPassIndex = 0; cascadedShadowPassIndex < numCascadedShadowPasses; ++cascadedShadowPassIndex )
        {
            for ( DeviceRenderView& renderView : m_renderPass_CascadedShadows[cascadedShadowPassIndex].m_renderViews )
            {
                renderView.ForEachRenderBucket( fn );
            }
        }
        m_renderPass_ForwardShading.m_renderView.ForEachRenderBucket( fn );
    }

    template <typename F>
    inline void ForwardShadingRenderer::ForEachRenderPass( F fn )
    {
        fn( m_renderPass_ForwardShading );

        for ( CascadedShadowPass& cascadedShadowPass : m_renderPass_CascadedShadows )
        {
            fn( cascadedShadowPass );
        }

        fn( m_renderPass_DepthDownsample );
        fn( m_renderPass_GlobalEnvironmentMap );
        fn( m_renderPass_SMAA );
        fn( m_renderPass_GTAO );
        fn( m_renderPass_PostProcess );

        #if EE_DEVELOPMENT_TOOLS
        fn( m_debugDrawPass );
        #endif
    }

    //-------------------------------------------------------------------------------------------------------

    void ForwardShadingRenderer::Initialize( SystemRegistry* pSystemRegistry, RenderSettings const& renderSettings )
    {
        m_pRenderSystem = pSystemRegistry->GetSystem<RenderSystem>();
        m_pRenderGlobalSettings = &renderSettings;

        // Pipelines
        //---------------------------------------------------------------------------------------------------

        static StringID const s_InstanceCullingShaderID( "InstanceCulling" );
        static StringID const s_ClusterCullingShaderID( "ClusterCulling" );
        static StringID const s_BucketResolveShaderID( "BucketResolve" );
        static StringID const s_LightCullingShaderID( "LightCulling" );

        m_pInstanceCullingShader = m_pRenderSystem->FindComputeShader( s_InstanceCullingShaderID );
        m_pClusterCullingShader = m_pRenderSystem->FindComputeShader( s_ClusterCullingShaderID );
        m_pBucketResolveShader = m_pRenderSystem->FindComputeShader( s_BucketResolveShaderID );
        m_pLightCullingShader = m_pRenderSystem->FindComputeShader( s_LightCullingShaderID );

        #if EE_DEVELOPMENT_TOOLS
        static StringID const s_InstancePickingResolveShaderID( "InstancePickingResolve" );

        m_pInstancePickingResolveShader = m_pRenderSystem->FindComputeShader( s_InstancePickingResolveShaderID );
        #endif

        m_materialShaderPipelineBuckets = ForwardShadingPass::InitializeMaterialShaderBuckets( m_pRenderSystem );

        m_clusterCulling_ClusterBuffer.Initialize( m_pRenderSystem->GetContextRHI() );
        m_clusterCulling_ArgumentBuffer.Initialize( m_pRenderSystem->GetContextRHI() );

        // Render passes
        //---------------------------------------------------------------------------------------------------

        ForEachRenderPass( [this] ( auto& renderPass )
        {
            RenderPassContext context = { m_pRenderSystem, m_pRenderGlobalSettings, m_materialShaderPipelineBuckets, };
            renderPass.Initialize( context );
        } );

        #if EE_DEVELOPMENT_TOOLS
        m_debugDrawPass.SetDebugMeshRegistry( pSystemRegistry->GetSystem<DebugMeshRegistry>() );
        #endif
    }

    void ForwardShadingRenderer::Shutdown()
    {
        #if EE_DEVELOPMENT_TOOLS
        m_debugDrawPass.ClearDebugMeshRegistry();
        #endif

        for ( ForwardShadingMaterialShaderPipelineBucket& materialShaderPipelineBucket : m_materialShaderPipelineBuckets )
        {
            materialShaderPipelineBucket.Shutdown( m_pRenderSystem->GetContextRHI() );
        }
        m_materialShaderPipelineBuckets.clear();

        ForEachRenderPass( [this] ( auto& renderPass )
        {
            renderPass.Shutdown( m_pRenderSystem );
        } );

        m_renderPass_CascadedShadows.clear();

        m_clusterCulling_ClusterBuffer.Shutdown( m_pRenderSystem->GetContextRHI() );
        m_clusterCulling_ArgumentBuffer.Shutdown( m_pRenderSystem->GetContextRHI() );

        RHI::DestroyBuffer( m_pRenderSystem->GetContextRHI(), eastl::move( m_instanceCulling_CounterBuffer ) );
        RHI::DestroyBuffer( m_pRenderSystem->GetContextRHI(), eastl::move( m_clusterCulling_CounterBuffer ) );
    }

    void ForwardShadingRenderer::UpdateDeviceResources( UpdateContext const& ctx )
    {
        EE_PROFILE_FUNCTION_RENDER();

        uint32_t            frameIndex = m_pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = m_pRenderSystem->GetContextRHI();

        //---------------------------------------------------------------------------------------------------

        m_renderPass_GlobalEnvironmentMap.UpdateDeviceResources_DFG( pContextRHI );
    }

    void ForwardShadingRenderer::UpdateViewportDeviceResources( UpdateContext const& ctx, RenderViewport* pRenderViewport, EntityWorld* pWorld )
    {
        EE_PROFILE_FUNCTION_RENDER();

        uint32_t frameIndex = m_pRenderSystem->GetFrameIndex();
        RHI::Context* pContextRHI = m_pRenderSystem->GetContextRHI();
        RenderWorldSystem* pRenderWorldSystem = pWorld->GetWorldSystem<RenderWorldSystem>();

        //---------------------------------------------------------------------------------------------------

        bool const enableAsyncCompute = m_pRenderGlobalSettings->m_enableAsyncCompute;
        bool const enableSMAA = m_pRenderGlobalSettings->m_enableSMAA;
        bool const enableSSAO = m_pRenderGlobalSettings->m_enableSSAO;
        bool const enableSSAOLowResolution = m_pRenderGlobalSettings->m_enableSSAOLowResolution;
        bool const enableDepthDownsample = ( enableSSAO && enableSSAOLowResolution ) || false;
        bool const useAsyncCompute = enableAsyncCompute && ( enableSSAO || false );

        //---------------------------------------------------------------------------------------------------

        m_renderPass_ForwardShading.UpdateViewportDeviceResources( m_pRenderSystem, pRenderViewport );

        if ( enableSMAA )
        {
            m_renderPass_SMAA.UpdateViewportDeviceResources( m_pRenderSystem, pRenderViewport );
        }

        if ( enableDepthDownsample )
        {
            m_renderPass_DepthDownsample.UpdateViewportDeviceResources( m_pRenderSystem, pRenderViewport );
        }

        if ( enableSSAO )
        {
            m_renderPass_GTAO.m_enableLowResolution = enableSSAOLowResolution;
            m_renderPass_GTAO.UpdateViewportDeviceResources( m_pRenderSystem, pRenderViewport );
        }

        m_renderPass_PostProcess.UpdateViewportDeviceResources( m_pRenderSystem, pRenderViewport );

        #if EE_DEVELOPMENT_TOOLS
        m_debugDrawPass.UpdateViewportDeviceResources
        (
            m_pRenderSystem,
            pRenderWorldSystem->m_deviceRenderWorld,
            pWorld->GetDebugDrawSystem(),
            ctx.GetDeltaTime(),
            pRenderViewport
        );

        pRenderWorldSystem->UpdateViewportPickingData( pRenderViewport );
        #endif

        // Compute how much render views we need
        //---------------------------------------------------------------------------------------------------

        uint32_t currentRenderViewOffset = 0;

        pRenderViewport->m_numRenderViewBucketsPerView = uint32_t( m_materialShaderPipelineBuckets.size() * DeviceRenderView::s_NumRenderBucketsPerViewBucket );

        pRenderViewport->m_numGlobalEnvironmentMapRenderViews = GlobalEnvironmentMapPass::NumRenderViews;
        pRenderViewport->m_globalEnvironmentMapRenderViewsOffset = currentRenderViewOffset;
        currentRenderViewOffset += pRenderViewport->m_numGlobalEnvironmentMapRenderViews;

        pRenderViewport->m_numCascadedShadowRenderViews = pRenderWorldSystem->m_numShadowCastingDirectionalLights * CascadedShadowPass::NumShadowCascades;
        pRenderViewport->m_cascadedShadowRenderViewsOffset = currentRenderViewOffset;
        currentRenderViewOffset += pRenderViewport->m_numCascadedShadowRenderViews;

        pRenderViewport->m_numForwardShadingRenderViews = 1;
        pRenderViewport->m_forwardShadingRenderViewsOffset = currentRenderViewOffset;
        currentRenderViewOffset += pRenderViewport->m_numForwardShadingRenderViews;

        pRenderViewport->m_numRenderViews = pRenderViewport->m_numGlobalEnvironmentMapRenderViews + pRenderViewport->m_numCascadedShadowRenderViews + pRenderViewport->m_numForwardShadingRenderViews;
        pRenderViewport->m_numRenderBuckets = pRenderViewport->m_numRenderViews * pRenderViewport->m_numRenderViewBucketsPerView;

        EE_ASSERT( currentRenderViewOffset == pRenderViewport->m_numRenderViews );

        // Global Parameters Buffer
        //---------------------------------------------------------------------------------------------------

        if ( !pRenderViewport->m_globalParametersBuffers[frameIndex] )
        {
            RHI::BufferParameters globalParametersBufferParameters = {};
            globalParametersBufferParameters.m_bufferSize = sizeof( ShaderTypes::GlobalParameters );
            globalParametersBufferParameters.m_bufferStride = sizeof( ShaderTypes::GlobalParameters );
            globalParametersBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            globalParametersBufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::ConstantBuffer;
            globalParametersBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            globalParametersBufferParameters.m_debugName.sprintf( "Global Parameters Buffer %i", frameIndex );

            pRenderViewport->m_globalParametersBuffers[frameIndex] = RHI::CreateBuffer( pContextRHI, globalParametersBufferParameters );
        }

        // Render views
        //-----------------------------------------------------------------------------------------------

        size_t const renderViewBufferSize = pRenderViewport->m_numRenderViews * sizeof( ShaderTypes::RenderView );
        if ( !pRenderViewport->m_renderViewBuffers[frameIndex] || pRenderViewport->m_renderViewBuffers[frameIndex]->m_size < renderViewBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pRenderViewport->m_renderViewBuffers[frameIndex] ) );

            RHI::BufferParameters renderViewBufferParameters = {};
            renderViewBufferParameters.m_debugName.sprintf( "RenderView Buffer %i", frameIndex );
            renderViewBufferParameters.m_bufferSize = renderViewBufferSize;
            renderViewBufferParameters.m_bufferStride = sizeof( ShaderTypes::RenderView );
            renderViewBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            renderViewBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;

            pRenderViewport->m_renderViewBuffers[frameIndex] = RHI::CreateBuffer( pContextRHI, renderViewBufferParameters );
        }

        size_t const renderBucketBufferSize = pRenderViewport->m_numRenderBuckets * sizeof( ShaderTypes::RenderBucket );
        if ( !pRenderViewport->m_renderBucketBuffers[frameIndex] || pRenderViewport->m_renderBucketBuffers[frameIndex]->m_size < renderBucketBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pRenderViewport->m_renderBucketBuffers[frameIndex] ) );

            RHI::BufferParameters renderBucketBufferParameters = {};
            renderBucketBufferParameters.m_bufferSize = renderBucketBufferSize;
            renderBucketBufferParameters.m_bufferStride = sizeof( ShaderTypes::RenderBucket );
            renderBucketBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            renderBucketBufferParameters.m_debugName = "RenderViewport RenderBucket Buffer";
            renderBucketBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;

            pRenderViewport->m_renderBucketBuffers[frameIndex] = RHI::CreateBuffer( pContextRHI, renderBucketBufferParameters );
        }

        // Light culling
        //-----------------------------------------------------------------------------------------------
        uint32_t const numPointPages = pRenderWorldSystem->m_deviceRenderWorld.GetNumPointLightPages();
        uint32_t const numSpotPages = pRenderWorldSystem->m_deviceRenderWorld.GetNumSpotLightPages();
        uint32_t const maxLightPages = Math::Max( Math::Max( numPointPages, numSpotPages ), 1U );

        uint32_t lightCullingNumBuckets = maxLightPages * 2;
        uint32_t lightCullingTextureWidth = ( uint32_t( pRenderViewport->GetDimensions().m_x ) + m_pRenderGlobalSettings->m_lightCullingTileSize - 1 ) / m_pRenderGlobalSettings->m_lightCullingTileSize;
        uint32_t lightCullingTextureHeight = ( uint32_t( pRenderViewport->GetDimensions().m_y ) + m_pRenderGlobalSettings->m_lightCullingTileSize - 1 ) / m_pRenderGlobalSettings->m_lightCullingTileSize;

        lightCullingTextureWidth = Math::Max( lightCullingTextureWidth, 4U );
        lightCullingTextureHeight = Math::Max( lightCullingTextureWidth, 4U );
        lightCullingNumBuckets = Math::Max( lightCullingNumBuckets, 2U );

        if ( !pRenderViewport->m_LightCulling_BucketTexture ||
             pRenderViewport->m_LightCulling_BucketTexture->m_width != lightCullingTextureWidth ||
             pRenderViewport->m_LightCulling_BucketTexture->m_height != lightCullingTextureHeight ||
             pRenderViewport->m_LightCulling_BucketTexture->m_arrayLayers != lightCullingNumBuckets )
        {
            RHI::TextureParameters lightCullingTextureParameters = {};
            lightCullingTextureParameters.m_width = lightCullingTextureWidth;
            lightCullingTextureParameters.m_height = lightCullingTextureHeight;
            lightCullingTextureParameters.m_arrayLayers = lightCullingNumBuckets;
            lightCullingTextureParameters.m_format = RHI::DataFormat::RGBA32_UInt;
            lightCullingTextureParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Texture, RHI::DescriptorTypeFlags::RWTexture );
            lightCullingTextureParameters.m_debugName.sprintf( "RenderViewport LightCulling BucketTexture" );

            m_pRenderSystem->QueueResourceDelete( eastl::move( pRenderViewport->m_LightCulling_BucketTexture ) );
            pRenderViewport->m_LightCulling_BucketTexture = RHI::CreateTexture( pContextRHI, lightCullingTextureParameters );
        }

        // Cascaded shadows
        //-----------------------------------------------------------------------------------------------
        size_t const cascadedShadowsBufferSize = Math::Max( pRenderViewport->m_numCascadedShadowRenderViews, 1U ) * sizeof( ShaderTypes::CascadedShadow );
        if ( !pRenderViewport->m_cascadedShadowBuffers[frameIndex] || pRenderViewport->m_cascadedShadowBuffers[frameIndex]->m_size < cascadedShadowsBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pRenderViewport->m_cascadedShadowBuffers[frameIndex] ) );

            RHI::BufferParameters cascadedShadowBufferParameters = {};
            cascadedShadowBufferParameters.m_bufferSize = cascadedShadowsBufferSize;
            cascadedShadowBufferParameters.m_bufferStride = sizeof( ShaderTypes::CascadedShadow );
            cascadedShadowBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            cascadedShadowBufferParameters.m_debugName = "RenderViewport CascadedShadow Buffer";
            cascadedShadowBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;

            pRenderViewport->m_cascadedShadowBuffers[frameIndex] = RHI::CreateBuffer( pContextRHI, cascadedShadowBufferParameters );
        }

        // Initialize render buckets
        //---------------------------------------------------------------------------------------------------
        TArrayView<ShaderTypes::RenderBucket> renderBucketMemory_WriteCombined = TArrayView<ShaderTypes::RenderBucket>( static_cast<ShaderTypes::RenderBucket*>( pRenderViewport->m_renderBucketBuffers[frameIndex]->m_pMappedAddress_WriteCombined ), pRenderViewport->m_numRenderBuckets );

        uint32_t renderBucketIndex = 0;
        ForEachRenderBucket( pRenderWorldSystem->m_numShadowCastingDirectionalLights, [renderBucketMemory_WriteCombined, &renderBucketIndex] ( MaterialShaderRenderBucket& renderBucket )
        {
            ShaderTypes::RenderBucket deviceRenderBucket = {};
            deviceRenderBucket.m_clusterVisibleCounterBuffer = RHI::GetBufferHandle( renderBucket.m_clusterVisibleCounterBuffer, RHI::DescriptorTypeFlags::RWBuffer );
            deviceRenderBucket.m_clusterVisibleBuffer = RHI::GetBufferHandle( renderBucket.m_clusterVisibleBuffer.m_buffer, RHI::DescriptorTypeFlags::RWBuffer );
            deviceRenderBucket.m_drawCounterBuffer = RHI::GetBufferHandle( renderBucket.m_drawCounterBuffer, RHI::DescriptorTypeFlags::RWBuffer );
            deviceRenderBucket.m_drawArgumentBuffer = RHI::GetBufferHandle( renderBucket.m_drawArgumentBuffer.m_buffer, RHI::DescriptorTypeFlags::RWBuffer );

            renderBucketMemory_WriteCombined[renderBucketIndex] = deviceRenderBucket;
            renderBucketIndex++;
        } );

        EE_ASSERT( renderBucketIndex == pRenderViewport->m_numRenderBuckets );

        // Update constant buffers
        //---------------------------------------------------------------------------------------------------

        Math::ViewVolume const& mainCameraViewVolume = pRenderViewport->GetViewVolume();

        alignas( 32 ) ShaderTypes::GlobalParameters globalParameters = {};

        mainCameraViewVolume.GetViewPosition().StoreFloat3( globalParameters.m_cameraPosition );
        mainCameraViewVolume.GetViewForwardVector().StoreFloat3( globalParameters.m_cameraForwardDirection );
        mainCameraViewVolume.GetViewRightVector().StoreFloat3( globalParameters.m_cameraRightDirection );
        mainCameraViewVolume.GetViewUpVector().StoreFloat3( globalParameters.m_cameraUpDirection );

        globalParameters.m_rendererGlobalFlags = ShaderTypes::RENDERER_GLOBAL_FLAG_NONE;

        globalParameters.m_viewportSize[0] = float( pRenderViewport->GetSize().m_x );
        globalParameters.m_viewportSize[1] = float( pRenderViewport->GetSize().m_y );

        globalParameters.m_viewportSize[2] = 1.0F / globalParameters.m_viewportSize[0];
        globalParameters.m_viewportSize[3] = 1.0F / globalParameters.m_viewportSize[1];

        globalParameters.m_meshInstancePageBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetMeshInstancePageBufferHandle( frameIndex );
        globalParameters.m_meshInstanceBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetMeshInstanceBufferHandle();

        globalParameters.m_skinningTransformBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetSkinningTransformBufferHandle();
        globalParameters.m_renderBucketBuffer = RHI::GetBufferHandle( pRenderViewport->m_renderBucketBuffers[frameIndex], RHI::DescriptorTypeFlags::Buffer );

        globalParameters.m_shaderDataBuffer = m_pRenderSystem->GetShaderDataBufferHandle();
        globalParameters.m_renderViewBuffer = RHI::GetBufferHandle( pRenderViewport->m_renderViewBuffers[frameIndex], RHI::DescriptorTypeFlags::Buffer );

        globalParameters.m_meshBuffer = m_pRenderSystem->GetMeshBufferHandle();
        globalParameters.m_clusterBuffer = m_pRenderSystem->GetClusterBufferHandle();

        globalParameters.m_InstanceCulling_CounterBuffer = RHI::GetBufferHandle( m_instanceCulling_CounterBuffer, RHI::DescriptorTypeFlags::RWBuffer );
        globalParameters.m_ClusterCulling_ClusterBuffer = RHI::GetBufferHandle( m_clusterCulling_ClusterBuffer.m_buffer, RHI::DescriptorTypeFlags::RWBuffer );
        globalParameters.m_ClusterCulling_CounterBuffer = RHI::GetBufferHandle( m_clusterCulling_CounterBuffer, RHI::DescriptorTypeFlags::RWBuffer );
        globalParameters.m_ClusterCulling_ArgumentBuffer = RHI::GetBufferHandle( m_clusterCulling_ArgumentBuffer.m_buffer, RHI::DescriptorTypeFlags::RWBuffer );

        globalParameters.m_meshInstanceRootBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetMeshInstanceRootBufferHandle();
        globalParameters.m_cascadedShadowBuffer = RHI::GetBufferHandle( pRenderViewport->m_cascadedShadowBuffers[frameIndex], RHI::DescriptorTypeFlags::Buffer );
        if ( enableSSAO )
        {
            globalParameters.m_ssaoTexture = RHI::GetTextureHandle( pRenderViewport->m_GTAO_ResultTexture, RHI::DescriptorTypeFlags::Texture, 0 );
        }
        globalParameters.m_dfgTexture = m_renderPass_GlobalEnvironmentMap.GetDFGTextureHandle();
        globalParameters.m_radianceTexture = pRenderWorldSystem->GetRadianceTextureHandle();

        // Light culling
        globalParameters.m_lightCullingTextureSize[0] = float( lightCullingTextureWidth );
        globalParameters.m_lightCullingTextureSize[1] = float( lightCullingTextureHeight );
        globalParameters.m_lightCullingTextureSize[2] = 1.0F / float( lightCullingTextureWidth );
        globalParameters.m_lightCullingTextureSize[3] = 1.0F / float( lightCullingTextureHeight );

        // Directional lights
        globalParameters.m_directionalLightBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetDirectionalLightBufferHandle();
        globalParameters.m_directionalLightPageBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetDirectionalLightPageBufferHandle( frameIndex );
        globalParameters.m_numDirectionalLightPages = pRenderWorldSystem->m_deviceRenderWorld.GetNumDirectionalLightPages();

        // Point lights
        globalParameters.m_pointLightBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetPointLightBufferHandle();
        globalParameters.m_pointLightPageBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetPointLightPageBufferHandle( frameIndex );
        globalParameters.m_numPointLightPages = pRenderWorldSystem->m_deviceRenderWorld.GetNumPointLightPages();

        // Spot lights
        globalParameters.m_spotLightBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetSpotLightBufferHandle();
        globalParameters.m_spotLightPageBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetSpotLightPageBufferHandle( frameIndex );
        globalParameters.m_numSpotLightPages = pRenderWorldSystem->m_deviceRenderWorld.GetNumSpotLightPages();

        // Light culling buckets
        globalParameters.m_LightCulling_BucketTexture = RHI::GetTextureHandle( pRenderViewport->m_LightCulling_BucketTexture, RHI::DescriptorTypeFlags::Texture, 0 );
        globalParameters.m_lightCulling_NumBuckets = lightCullingNumBuckets;

        // Picking
        #if EE_DEVELOPMENT_TOOLS
        if ( pRenderViewport->IsPickingEnabled() )
        {
            auto UpdateInstancePickingDistancesBuffer = [pContextRHI, this] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
            {
                m_pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );

                RHI::BufferParameters instancePickingDistanceBufferParameters = {};
                instancePickingDistanceBufferParameters.m_bufferSize = newBufferSize;
                instancePickingDistanceBufferParameters.m_format = RHI::DataFormat::R32_SFloat;
                instancePickingDistanceBufferParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer );
                instancePickingDistanceBufferParameters.m_debugName.sprintf( "Instance Picking Distances Buffer" );

                return RHI::CreateBuffer( pContextRHI, instancePickingDistanceBufferParameters );
            };

            size_t instanceDistancesBufferSize = pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstancePages() * 64 * sizeof( float );
            pRenderViewport->m_instancePickingDistancesBuffer.UpdateDeviceResources( instanceDistancesBufferSize, UpdateInstancePickingDistancesBuffer );

            pRenderViewport->m_instancePickingResultsBuffer.UpdateBuffers( m_pRenderSystem, frameIndex, sizeof( ShaderTypes::PickingResult ), RHI::DescriptorTypeFlags::RWBuffer );

            globalParameters.m_instancePickingResultsBuffer = pRenderViewport->m_instancePickingResultsBuffer.GetAppendBufferHandle();
            globalParameters.m_instancePickingDistancesBuffer = RHI::GetBufferHandle( pRenderViewport->m_instancePickingDistancesBuffer.m_buffer, RHI::DescriptorTypeFlags::RWBuffer );

            Vector mouseClipSpace = pRenderViewport->ScreenSpaceToClipSpace( pRenderViewport->m_lastKnownPickingMousePosition );
            float pickingRadiusClipSpace = float( pRenderViewport->m_lastKnownPickingPixelRadius ) / pRenderViewport->GetDimensions().GetMax();

            globalParameters.m_pickingInput[0] = mouseClipSpace.GetX();
            globalParameters.m_pickingInput[1] = mouseClipSpace.GetY();
            globalParameters.m_pickingInput[2] = pickingRadiusClipSpace;
            globalParameters.m_pickingInput[3] = 0.0F;

            globalParameters.m_pickingEnabled = 1;
        }
        else
        {
            globalParameters.m_pickingEnabled = 0;
        }
        #else
        globalParameters.m_pickingEnabled = 0;
        #endif

        globalParameters.m_mainCameraRenderView = pRenderViewport->m_forwardShadingRenderViewsOffset;

        // Misc
        globalParameters.m_irradianceTexture = pRenderWorldSystem->GetIrradianceTextureHandle();
        globalParameters.m_radianceTextureMipLevels = pRenderWorldSystem->GetRadianceTextureMipLevels();
        globalParameters.m_deviceAddress = pRenderViewport->m_globalParametersBuffers[frameIndex]->m_deviceAddress;

        if ( !enableSSAO )
        {
            globalParameters.m_rendererGlobalFlags |= ShaderTypes::RENDERER_GLOBAL_FLAG_DISABLE_SSAO;
        }

        #if EE_DEVELOPMENT_TOOLS
        globalParameters.m_shaderDebugDrawBuffer = RHI::GetBufferHandle( pRenderViewport->m_shaderDebugDrawBuffers[frameIndex], RHI::DescriptorTypeFlags::Buffer );
        #endif

        #if EE_DEVELOPMENT_TOOLS
        auto const* pVisSettings = pRenderViewport->GetViewportSettings<RenderViewportSettings>();
        globalParameters.m_rendererDebugVisualizationMode = uint8_t( pVisSettings->m_visualizationMode );

        globalParameters.m_rendererDebugFlags = ShaderTypes::RENDERER_DEBUG_FLAG_NONE;
        if ( pVisSettings->m_showWireframe )
        {
            globalParameters.m_rendererDebugFlags |= ShaderTypes::RENDERER_DEBUG_FLAG_SHOW_WIREFRAME;
        }
        if ( pVisSettings->m_showShadowCascades )
        {
            globalParameters.m_rendererDebugFlags |= ShaderTypes::RENDERER_DEBUG_FLAG_SHOW_SHADOW_CASCADES;
        }
        #endif

        Memory::CopyToWriteCombined( pRenderViewport->m_globalParametersBuffers[frameIndex]->m_pMappedAddress_WriteCombined, &globalParameters, sizeof( globalParameters ) );
    }

    void ForwardShadingRenderer::UpdateWorldDeviceResources( UpdateContext const& ctx, EntityWorld* pWorld )
    {
        EE_PROFILE_FUNCTION_RENDER();

        uint32_t frameIndex = m_pRenderSystem->GetFrameIndex();
        RHI::Context* pContextRHI = m_pRenderSystem->GetContextRHI();
        RenderWorldSystem* pRenderWorldSystem = pWorld->GetWorldSystem<RenderWorldSystem>();
        MaterialShaderClusterCapacity&  materialShaderClusterCapacity = pRenderWorldSystem->m_materialShaderClusterCapacity;

        // Validate material cluster capacity, resizes internal buffers to match the amount of shaders
        //---------------------------------------------------------------------------------------------------

        materialShaderClusterCapacity.Validate( m_materialShaderPipelineBuckets.size() );

        // Update world
        //---------------------------------------------------------------------------------------------------

        pRenderWorldSystem->UpdateDeviceResources();

        // Update directional lights
        //---------------------------------------------------------------------------------------------------

        size_t numShadowCastingDirectionalLights = 0;
        for ( DirectionalLightComponent const* pDirectionalLightComponent : pRenderWorldSystem->m_directionalLightComponents )
        {
            if ( pDirectionalLightComponent->GetShadowed() )
            {
                CascadedShadowPass* pCascadedShadowPass = nullptr;
                if ( numShadowCastingDirectionalLights >= m_renderPass_CascadedShadows.size() )
                {
                    RenderPassContext context = { m_pRenderSystem, m_pRenderGlobalSettings, m_materialShaderPipelineBuckets, };

                    pCascadedShadowPass = &m_renderPass_CascadedShadows.emplace_back();
                    pCascadedShadowPass->Initialize( context );
                }
                else
                {
                    pCascadedShadowPass = &m_renderPass_CascadedShadows[numShadowCastingDirectionalLights];
                }

                numShadowCastingDirectionalLights++;
            }
        }
        EE_ASSERT( numShadowCastingDirectionalLights == pRenderWorldSystem->m_numShadowCastingDirectionalLights );

        // Update all render passes
        //---------------------------------------------------------------------------------------------------

        m_renderPass_GlobalEnvironmentMap.UpdateDeviceResources
        (
            m_pRenderSystem, m_materialShaderPipelineBuckets,
            materialShaderClusterCapacity.GetViewLayerClusterCapacity( ViewLayer::GlobalEnvironmentMap )
        );

        for ( size_t cascadedShadowPassIndex = 0; cascadedShadowPassIndex < numShadowCastingDirectionalLights; ++cascadedShadowPassIndex )
        {
            m_renderPass_CascadedShadows[cascadedShadowPassIndex].UpdateDeviceResources( m_pRenderSystem, materialShaderClusterCapacity.GetViewLayerClusterCapacity( ViewLayer::ShadowMap ) );
        }
        m_renderPass_ForwardShading.UpdateDeviceResources( m_pRenderSystem, m_materialShaderPipelineBuckets, materialShaderClusterCapacity.GetViewLayerClusterCapacity( ViewLayer::ForwardShading ) );

        #if EE_DEVELOPMENT_TOOLS
        m_debugDrawPass.UpdateDeviceResources( m_pRenderSystem );
        #endif

        // Create cluster culling buffers
        //---------------------------------------------------------------------------------------------------

        auto UpdateBuffer_ClusterCullingArgument = [this, pContextRHI] ( DeviceBufferState&& oldBuffer, size_t newBufferSize )
        {
            m_pRenderSystem->QueueResourceDelete( oldBuffer );

            RHI::BufferParameters clusterCulling_ArgumentBufferParameters = {};
            clusterCulling_ArgumentBufferParameters.m_alignment = RHI::IndirectCommandAlignment;
            clusterCulling_ArgumentBufferParameters.m_descriptorTypes = TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::IndirectArgumentBuffer, RHI::DescriptorTypeFlags::RWBuffer );
            clusterCulling_ArgumentBufferParameters.m_bufferSize = newBufferSize;
            clusterCulling_ArgumentBufferParameters.m_bufferStride = sizeof( ShaderTypes::ClusterCullingArgument );
            clusterCulling_ArgumentBufferParameters.m_debugName = "Renderer_ForwardShading ClusterCullingArgument Buffer";

            return RHI::CreateBuffer( pContextRHI, clusterCulling_ArgumentBufferParameters );
        };

        m_clusterCulling_ArgumentBuffer.UpdateDeviceResources
        (
            ( ( materialShaderClusterCapacity.GetGlobalClusterCapacity() + 63 ) / 64 ) * sizeof( ShaderTypes::ClusterCullingArgument ),
            UpdateBuffer_ClusterCullingArgument
        );

        //---------------------------------------------------------------------------------------------------

        auto UpdateBuffer_ClusterCulling = [this, pContextRHI] ( DeviceBufferState&& oldBuffer, size_t newBufferSize )
        {
            m_pRenderSystem->QueueResourceDelete( eastl::move( oldBuffer ) );

            RHI::BufferParameters clusterCulling_ClusterBufferParameters = {};
            clusterCulling_ClusterBufferParameters.m_bufferSize = newBufferSize;
            clusterCulling_ClusterBufferParameters.m_format = RHI::DataFormat::RG32_UInt;
            clusterCulling_ClusterBufferParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer );
            clusterCulling_ClusterBufferParameters.m_debugName = "Renderer_ForwardShading ClusterCulling Cluster Buffer";

            return RHI::CreateBuffer( pContextRHI, clusterCulling_ClusterBufferParameters );
        };

        m_clusterCulling_ClusterBuffer.UpdateDeviceResources
        (
            materialShaderClusterCapacity.GetGlobalClusterCapacity() * sizeof( uint2 ),
            UpdateBuffer_ClusterCulling
        );

        //---------------------------------------------------------------------------------------------------

        if ( !m_clusterCulling_CounterBuffer )
        {
            RHI::BufferParameters counterBufferParameters = {};
            counterBufferParameters.m_bufferSize = sizeof( uint32_t );
            counterBufferParameters.m_bufferStride = sizeof( uint32_t );
            counterBufferParameters.m_format = RHI::DataFormat::R32_UInt;
            counterBufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::RWBuffer;

            counterBufferParameters.m_debugName = "Renderer_ForwardShading ClusterCulling Counter Buffer";
            m_clusterCulling_CounterBuffer = RHI::CreateBuffer( pContextRHI, counterBufferParameters );

            counterBufferParameters.m_debugName = "Renderer_ForwardShading InstanceCulling Counter Buffer";
            m_instanceCulling_CounterBuffer = RHI::CreateBuffer( pContextRHI, counterBufferParameters );
        }
    }

    void ForwardShadingRenderer::DispatchWorld( UpdateContext const& updateContext, RenderViewport const* pRenderViewport, EntityWorld* pWorld )
    {
        EE_PROFILE_FUNCTION_RENDER();

        uint32_t const frameIndex = m_pRenderSystem->GetFrameIndex();
        RHI::CommandBuffer* pCommandBuffer = pRenderViewport->m_pWindow->GetActiveCommandBuffer( frameIndex );

        RenderWorldSystem* pRenderWorldSystem = pWorld->GetWorldSystem<RenderWorldSystem>();
        pRenderWorldSystem->m_deviceRenderWorld.DispatchWorldUpdate( m_pRenderSystem->GetMeshBufferHandle(), pCommandBuffer, frameIndex );
        pRenderWorldSystem->m_deviceRenderWorld.WaitForCopyTasks( m_pRenderSystem );
    }

    void ForwardShadingRenderer::DrawWorldToViewport( UpdateContext const& ctx, RenderViewport const* pRenderViewport, EntityWorld const* pWorld )
    {
        EE_ASSERT( pRenderViewport != nullptr );
        EE_ASSERT( pWorld != nullptr );
        EE_ASSERT( pRenderViewport->IsValid() );

        EE_PROFILE_FUNCTION_RENDER();

        RenderWorldSystem const* pRenderWorldSystem = pWorld->GetWorldSystem<RenderWorldSystem>();
        MaterialShaderClusterCapacity const& materialShaderClusterCapacity = pRenderWorldSystem->m_materialShaderClusterCapacity;

        //---------------------------------------------------------------------------------------------------

        uint32_t const frameIndex = m_pRenderSystem->GetFrameIndex();

        //---------------------------------------------------------------------------------------------------

        bool const enableAsyncCompute = m_pRenderGlobalSettings->m_enableAsyncCompute;

        bool const enableSMAA = m_pRenderGlobalSettings->m_enableSMAA;

        bool const enableSSAO = m_pRenderGlobalSettings->m_enableSSAO;
        bool const enableSSAOLowResolution = m_pRenderGlobalSettings->m_enableSSAOLowResolution;
        bool const enableDepthDownsample = ( enableSSAO && enableSSAOLowResolution ) || false;

        bool const useAsyncCompute = enableAsyncCompute && ( enableSSAO || false );

        //---------------------------------------------------------------------------------------------------

        TArrayView<ShaderTypes::RenderView> renderViews_WriteCombined = TArrayView<ShaderTypes::RenderView>( static_cast<ShaderTypes::RenderView*>( pRenderViewport->m_renderViewBuffers[frameIndex]->m_pMappedAddress_WriteCombined ), pRenderViewport->m_numRenderViews );

        TArrayView<ShaderTypes::RenderView> globalEnvironmentMapRenderViews_WriteCombined = renderViews_WriteCombined.subspan( pRenderViewport->m_globalEnvironmentMapRenderViewsOffset, pRenderViewport->m_numGlobalEnvironmentMapRenderViews );
        TArrayView<ShaderTypes::RenderView> cascadedShadowMapRenderViews_WriteCombined = renderViews_WriteCombined.subspan( pRenderViewport->m_cascadedShadowRenderViewsOffset, pRenderViewport->m_numCascadedShadowRenderViews );
        TArrayView<ShaderTypes::RenderView> forwardShadingRenderViews_WriteCombined = renderViews_WriteCombined.subspan( pRenderViewport->m_forwardShadingRenderViewsOffset, pRenderViewport->m_numForwardShadingRenderViews );

        //---------------------------------------------------------------------------------------------------

        RHI::BufferHandle   renderViewBufferHandle = RHI::GetBufferHandle( pRenderViewport->m_renderViewBuffers[frameIndex], RHI::DescriptorTypeFlags::Buffer );
        RHI::Buffer*        pGlobalParametersBuffer = pRenderViewport->m_globalParametersBuffers[frameIndex];

        // Start frame barriers (TODO: move them earlier?)
        //---------------------------------------------------------------------------------------------------

        RHI::CommandBuffer* pCommandBuffer_DepthPass = pRenderViewport->m_pWindow->GetActiveCommandBuffer( frameIndex );

        EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

        m_resourceStates.Writeable( m_instanceCulling_CounterBuffer, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess );
        m_resourceStates.Writeable( m_clusterCulling_CounterBuffer, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess );

        ForEachRenderBucket( pRenderWorldSystem->m_numShadowCastingDirectionalLights, [this] ( MaterialShaderRenderBucket& renderBucket )
        {
            m_resourceStates.Writeable( renderBucket.m_clusterVisibleCounterBuffer, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess );
            m_resourceStates.Writeable( renderBucket.m_drawCounterBuffer, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess );
        } );

        // Flush all barriers at start of the frame
        m_resourceStates.FlushBarriers( pCommandBuffer_DepthPass );

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_DepthPass, "Clear Buffers" );

            RHI::CmdClearBuffer( pCommandBuffer_DepthPass, m_instanceCulling_CounterBuffer, 0 );
            RHI::CmdClearBuffer( pCommandBuffer_DepthPass, m_clusterCulling_CounterBuffer, 0 );

            #if EE_DEVELOPMENT_TOOLS
            m_debugDrawPass.ClearBuffers( m_resourceStates, pCommandBuffer_DepthPass, frameIndex );
            #endif

            ForEachRenderBucket( pRenderWorldSystem->m_numShadowCastingDirectionalLights, [pCommandBuffer_DepthPass] ( MaterialShaderRenderBucket& renderBucket )
            {
                RHI::CmdClearBuffer( pCommandBuffer_DepthPass, renderBucket.m_clusterVisibleCounterBuffer, 0 );
                RHI::CmdClearBuffer( pCommandBuffer_DepthPass, renderBucket.m_drawCounterBuffer, 0 );
            } );
        }

        #if EE_DEVELOPMENT_TOOLS
        if ( pRenderViewport->IsPickingEnabled() )
        {
            RHI::CmdClearBuffer( pCommandBuffer_DepthPass, pRenderViewport->m_instancePickingDistancesBuffer.m_buffer, eastl::bit_cast<uint32_t>( 1.5E+10F ) );

            pRenderViewport->m_instancePickingResultsBuffer.Clear( m_resourceStates, pCommandBuffer_DepthPass, frameIndex );
        }
        #endif

        // Common root constants
        //---------------------------------------------------------------------------------------------------
        ShaderTypes::CommonRootConstants commonRootConstants = {};
        commonRootConstants.m_numRenderViewBucketsPerView = pRenderViewport->m_numRenderViewBucketsPerView;
        commonRootConstants.m_numRenderViews = pRenderViewport->m_numRenderViews;
        commonRootConstants.m_numRenderBuckets = pRenderViewport->m_numRenderBuckets;

        // Instance culling
        //---------------------------------------------------------------------------------------------------

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_DepthPass, "Instance Culling" );

            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );
            m_resourceStates.Writeable( m_clusterCulling_CounterBuffer, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess );
            m_resourceStates.Writeable( m_clusterCulling_ArgumentBuffer.m_buffer, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess );
            m_resourceStates.Writeable( m_clusterCulling_ClusterBuffer.m_buffer, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess );
            m_resourceStates.FlushBarriers( pCommandBuffer_DepthPass );

            ShaderTypes::InstanceCullingRootConstants instanceCullingRootConstants = {};
            instanceCullingRootConstants.m_commonRootConstants = commonRootConstants;

            RHI::CmdSetPipeline( pCommandBuffer_DepthPass, m_pInstanceCullingShader->m_pPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer_DepthPass, 0, &instanceCullingRootConstants, sizeof( instanceCullingRootConstants ) );
            RHI::CmdSetRootParameter( pCommandBuffer_DepthPass, 1, pGlobalParametersBuffer, 0 );
            RHI::CmdDispatchCompute( pCommandBuffer_DepthPass, pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstancePages(), 1, 1 );
        }

        // Cluster culling
        //---------------------------------------------------------------------------------------------------

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_DepthPass, "Cluster Culling" );

            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );
            m_resourceStates.ReadOnly( m_clusterCulling_CounterBuffer, RHI::PipelineStage::ExecuteIndirect, RHI::ResourceAccess::IndirectArgument );
            m_resourceStates.ReadOnly( m_clusterCulling_ArgumentBuffer.m_buffer, RHI::PipelineStage::ExecuteIndirect, RHI::ResourceAccess::IndirectArgument );
            m_resourceStates.ReadOnly( m_clusterCulling_ClusterBuffer.m_buffer, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::ShaderResource );

            ForEachRenderBucket( pRenderWorldSystem->m_numShadowCastingDirectionalLights, [this] ( MaterialShaderRenderBucket& renderBucket )
            {
                m_resourceStates.Writeable( renderBucket.m_clusterVisibleCounterBuffer, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess );
                m_resourceStates.Writeable( renderBucket.m_clusterVisibleBuffer.m_buffer, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess );
            } );
            m_resourceStates.FlushBarriers( pCommandBuffer_DepthPass );

            RHI::CmdSetPipeline( pCommandBuffer_DepthPass, m_pClusterCullingShader->m_pPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer_DepthPass, 0, nullptr, sizeof( ShaderTypes::ClusterCullingRootConstants ) );
            RHI::CmdExecuteIndirect
            (
                pCommandBuffer_DepthPass, m_pClusterCullingShader->m_pCommandSignature,
                ( materialShaderClusterCapacity.GetGlobalClusterCapacity() + 63 ) / 64,
                m_clusterCulling_ArgumentBuffer.m_buffer, 0,
                m_clusterCulling_CounterBuffer, 0
            );

            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );
            ForEachRenderBucket( pRenderWorldSystem->m_numShadowCastingDirectionalLights, [this] ( MaterialShaderRenderBucket& renderBucket )
            {
                m_resourceStates.ReadOnly( renderBucket.m_clusterVisibleCounterBuffer, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::ShaderResource );
                m_resourceStates.ReadOnly( renderBucket.m_clusterVisibleBuffer.m_buffer, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::ShaderResource );
            } );
            m_resourceStates.FlushBarriers( pCommandBuffer_DepthPass );

            ShaderTypes::BucketResolveRootConstants bucketResolveRootConstants = {};
            bucketResolveRootConstants.m_commonRootConstants = commonRootConstants;

            RHI::CmdSetPipeline( pCommandBuffer_DepthPass, m_pBucketResolveShader->m_pPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer_DepthPass, 0, &bucketResolveRootConstants, sizeof( bucketResolveRootConstants ) );
            RHI::CmdSetRootParameter( pCommandBuffer_DepthPass, 1, pGlobalParametersBuffer, 0 );
            RHI::CmdDispatchCompute( pCommandBuffer_DepthPass, ( pRenderViewport->m_numRenderBuckets + 63 ) / 64, 1, 1 );
        }

        // Light culling
        //---------------------------------------------------------------------------------------------------
        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_DepthPass, "Light Culling" );

            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );
            m_resourceStates.Writeable( pRenderViewport->m_LightCulling_BucketTexture, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::TextureState::UnorderedAccess );
            m_resourceStates.FlushBarriers( pCommandBuffer_DepthPass );

            // TODO: Try light culling in async?
            ShaderTypes::LightCullingRootConstants lightCullingRootConstants = {};
            lightCullingRootConstants.m_commonRootConstants = commonRootConstants;
            lightCullingRootConstants.m_mainCameraRenderView = pRenderViewport->m_forwardShadingRenderViewsOffset;
            lightCullingRootConstants.m_LightCulling_BucketTexture = RHI::GetTextureHandle( pRenderViewport->m_LightCulling_BucketTexture, RHI::DescriptorTypeFlags::RWTexture, 0 );

            RHI::CmdSetPipeline( pCommandBuffer_DepthPass, m_pLightCullingShader->m_pPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer_DepthPass, 0, &lightCullingRootConstants, sizeof( ShaderTypes::LightCullingRootConstants ) );
            RHI::CmdSetRootParameter( pCommandBuffer_DepthPass, 1, pGlobalParametersBuffer, 0 );
            RHI::CmdDispatchCompute( pCommandBuffer_DepthPass, pRenderViewport->m_LightCulling_BucketTexture->m_width, pRenderViewport->m_LightCulling_BucketTexture->m_height, pRenderViewport->m_LightCulling_BucketTexture->m_arrayLayers / 2 );
        }

        // Wait for all cluster and culling dispatches
        //---------------------------------------------------------------------------------------------------
        {
            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );
            m_resourceStates.ReadOnly( pRenderViewport->m_LightCulling_BucketTexture, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
            ForEachRenderBucket( pRenderWorldSystem->m_numShadowCastingDirectionalLights, [this] ( MaterialShaderRenderBucket& renderBucket )
            {
                m_resourceStates.ReadOnly( renderBucket.m_drawCounterBuffer, RHI::PipelineStage::ExecuteIndirect, RHI::ResourceAccess::IndirectArgument );
                m_resourceStates.ReadOnly( renderBucket.m_drawArgumentBuffer.m_buffer, RHI::PipelineStage::ExecuteIndirect, RHI::ResourceAccess::IndirectArgument );
            } );

            m_resourceStates.FlushBarriers( pCommandBuffer_DepthPass );
        }

        // Global environment map pass
        //---------------------------------------------------------------------------------------------------

        m_renderPass_GlobalEnvironmentMap.PrecomputeDFG( pCommandBuffer_DepthPass );
        if ( pRenderWorldSystem->m_needUpdateGlobalEnvironmentMap )
        {
            m_renderPass_GlobalEnvironmentMap.DrawAndFilterGlobalEnvironmentMap
            (
                m_materialShaderPipelineBuckets,
                materialShaderClusterCapacity.GetViewLayerClusterCapacity( ViewLayer::GlobalEnvironmentMap ),
                pGlobalParametersBuffer,
                pCommandBuffer_DepthPass,
                pRenderWorldSystem->m_pRadianceTexture,
                pRenderWorldSystem->m_pIrradianceTexture,
                renderViewBufferHandle,
                pRenderViewport->m_globalEnvironmentMapRenderViewsOffset,
                globalEnvironmentMapRenderViews_WriteCombined
            );

            // TODO: Implement the concept of device generated resources in the engine.
            // Until then we const_cast the system to update the dirty flag.
            RenderWorldSystem* pRenderWorldSystemMutable = const_cast<RenderWorldSystem*>( pRenderWorldSystem );
            pRenderWorldSystemMutable->m_needUpdateGlobalEnvironmentMap = false;
        }

        // Forward shading depth pass
        //---------------------------------------------------------------------------------------------------

        m_renderPass_ForwardShading.DepthOnlyPass
        (
            m_materialShaderPipelineBuckets,
            materialShaderClusterCapacity.GetViewLayerClusterCapacity( ViewLayer::ForwardShading ),
            pRenderViewport,
            m_resourceStates,
            pCommandBuffer_DepthPass,
            forwardShadingRenderViews_WriteCombined
        );

        if ( enableDepthDownsample )
        {
            m_renderPass_DepthDownsample.DrawToViewport( pRenderViewport, m_resourceStates, pCommandBuffer_DepthPass, pRenderViewport->m_ForwardShading_DepthTexture );
        }

        uint64_t signalSemaphore_DepthPass = 0;
        if ( useAsyncCompute )
        {
            if ( enableSSAO )
            {
                EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

                if ( enableSSAOLowResolution )
                {
                    m_resourceStates.ReadOnly( pRenderViewport->m_DepthDownsample4, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
                }
                else
                {
                    m_resourceStates.ReadOnly( pRenderViewport->m_ForwardShading_DepthTexture, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
                }

                m_resourceStates.Writeable( pRenderViewport->m_GTAO_PrefilterDepthTexture, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::TextureState::UnorderedAccess );
                m_resourceStates.Writeable( pRenderViewport->m_GTAO_ResultTextureNoisy0, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::TextureState::UnorderedAccess );
                m_resourceStates.Writeable( pRenderViewport->m_GTAO_ResultTextureNoisy1, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::TextureState::UnorderedAccess );
                m_resourceStates.Writeable( pRenderViewport->m_GTAO_ResultTexture, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::TextureState::UnorderedAccess );
                m_resourceStates.FlushBarriers( pCommandBuffer_DepthPass );
            }

            // Ensure all pending buffer writes are completed before the first submit happens.
            Memory::WriteCombinedBarrier();

            signalSemaphore_DepthPass = SubmitGraphicsCommandBuffer( eastl::move( pCommandBuffer_DepthPass ) );
        }

        // SSAO
        //---------------------------------------------------------------------------------------------------

        RHI::CommandBuffer* pCommandBuffer_GTAO = nullptr;
        uint64_t signalSemaphore_GTAO = 0;

        if ( enableSSAO )
        {
            pCommandBuffer_GTAO = pCommandBuffer_DepthPass;

            if ( useAsyncCompute )
            {
                pCommandBuffer_GTAO = pRenderViewport->m_pWindow->AcquireComputeCommandBuffer( m_pRenderSystem->GetContextRHI(), frameIndex );
            }

            {
                EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_GTAO, "XeGTAO" );

                m_renderPass_GTAO.PrefilterDepth( pRenderViewport, m_resourceStates, pCommandBuffer_GTAO, frameIndex );
                m_renderPass_GTAO.ComputeNoisyResult( pRenderViewport, m_resourceStates, pCommandBuffer_GTAO, renderViewBufferHandle, pRenderViewport->m_forwardShadingRenderViewsOffset, frameIndex );
                m_renderPass_GTAO.Denoise( pRenderViewport, m_resourceStates, pCommandBuffer_GTAO, useAsyncCompute, frameIndex );
            }

            if ( useAsyncCompute )
            {
                EE_ASSERT( !m_resourceStates.HasPendingBarriers() );
                if ( enableSSAOLowResolution )
                {
                    m_resourceStates.ReadOnly( pRenderViewport->m_GTAO_ResultTextureNoisy1, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
                }
                else
                {
                    m_resourceStates.ReadOnly( pRenderViewport->m_GTAO_ResultTexture, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
                }
                m_resourceStates.FlushBarriers( pCommandBuffer_GTAO );

                RHI::QueueDeviceWait( m_pRenderSystem->GetComputeQueue(), m_pRenderSystem->GetGraphicsQueue(), signalSemaphore_DepthPass );
                signalSemaphore_GTAO = SubmitComputeCommandBuffer( eastl::move( pCommandBuffer_GTAO ) );
            }
        }

        // Cascaded shadows and directional lights
        //---------------------------------------------------------------------------------------------------

        RHI::CommandBuffer* pCommandBuffer_CascadedShadows = pCommandBuffer_DepthPass;
        uint64_t signalSemaphore_CascadedShadows = 0;

        if ( useAsyncCompute )
        {
            pCommandBuffer_CascadedShadows = pRenderViewport->m_pWindow->AcquireGraphicsCommandBuffer( m_pRenderSystem->GetContextRHI(), frameIndex );
        }

        TArrayView<ShaderTypes::CascadedShadow> cascadedShadows_WriteCombined = TArrayView<ShaderTypes::CascadedShadow>( static_cast<ShaderTypes::CascadedShadow*>( pRenderViewport->m_cascadedShadowBuffers[frameIndex]->m_pMappedAddress_WriteCombined ), pRenderViewport->m_numCascadedShadowRenderViews );

        size_t numCascadedShadowPasses = 0;

        for ( DirectionalLightComponent const* pDirectionalLightComponent : pRenderWorldSystem->m_directionalLightComponents )
        {
            Float3 lightDirection = -pDirectionalLightComponent->GetLightDirection();

            if ( pDirectionalLightComponent->GetShadowed() )
            {
                CascadedShadowPass& cascadedShadowPass = m_renderPass_CascadedShadows[numCascadedShadowPasses];

                cascadedShadowPass.DrawShadowCascades
                (
                    m_materialShaderPipelineBuckets,
                    materialShaderClusterCapacity.GetViewLayerClusterCapacity( ViewLayer::ShadowMap ),
                    pRenderViewport,
                    lightDirection,
                    m_resourceStates,
                    pCommandBuffer_CascadedShadows,
                    &cascadedShadows_WriteCombined[numCascadedShadowPasses],
                    cascadedShadowMapRenderViews_WriteCombined.subspan( numCascadedShadowPasses * CascadedShadowPass::NumShadowCascades, CascadedShadowPass::NumShadowCascades )
                );

                numCascadedShadowPasses++;
            }
        }

        EE_ASSERT( numCascadedShadowPasses == pRenderWorldSystem->m_numShadowCastingDirectionalLights );

        if ( useAsyncCompute )
        {
            signalSemaphore_CascadedShadows = SubmitGraphicsCommandBuffer( eastl::move( pCommandBuffer_CascadedShadows ) );
        }

        // Forward shading pass
        //---------------------------------------------------------------------------------------------------

        RHI::CommandBuffer* pCommandBuffer_ShadingPass = pCommandBuffer_DepthPass;
        if ( useAsyncCompute )
        {
            pCommandBuffer_ShadingPass = pRenderViewport->m_pWindow->AcquireGraphicsCommandBuffer( m_pRenderSystem->GetContextRHI(), frameIndex );
        }

        if ( enableSSAO && enableSSAOLowResolution )
        {
            m_renderPass_GTAO.Upsample( pRenderViewport, m_resourceStates, pCommandBuffer_ShadingPass, frameIndex );
        }

        {
            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );
            m_resourceStates.ReadOnly( pRenderViewport->m_LightCulling_BucketTexture, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
            if ( enableSSAO )
            {
                m_resourceStates.ReadOnly( pRenderViewport->m_GTAO_ResultTexture, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
            }

            for ( size_t cascadedShadowPassIndex = 0; cascadedShadowPassIndex < numCascadedShadowPasses; ++cascadedShadowPassIndex )
            {
                m_resourceStates.ReadOnly( m_renderPass_CascadedShadows[cascadedShadowPassIndex].m_depthTargetArray, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
            }
            m_resourceStates.FlushBarriers( pCommandBuffer_ShadingPass );
        }

        m_renderPass_ForwardShading.ShadingPass
        (
            m_materialShaderPipelineBuckets,
            materialShaderClusterCapacity.GetViewLayerClusterCapacity( ViewLayer::ForwardShading ),
            pRenderViewport,
            m_resourceStates,
            pCommandBuffer_ShadingPass
        );

        // Resolve picking after shading pass
        #if EE_DEVELOPMENT_TOOLS
        if ( pRenderViewport->IsPickingEnabled() )
        {
            RHI::CmdBarrier( pCommandBuffer_ShadingPass, RHI::PipelineStage::AllShader, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::UnorderedAccess );

            RHI::CmdSetPipeline( pCommandBuffer_ShadingPass, m_pInstancePickingResolveShader->m_pPipeline );
            RHI::CmdSetRootParameter( pCommandBuffer_ShadingPass, 0, pGlobalParametersBuffer, 0 );
            RHI::CmdDispatchCompute( pCommandBuffer_ShadingPass, pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstancePages(), 1, 1 );

            pRenderViewport->m_instancePickingResultsBuffer.CopyResults( m_resourceStates, pCommandBuffer_ShadingPass, frameIndex );
            pRenderViewport->m_instancePickingResultsBuffer.Barrier( m_resourceStates, pCommandBuffer_ShadingPass, frameIndex );
        }
        #endif

        // Post processing
        //---------------------------------------------------------------------------------------------------

        if ( enableSMAA )
        {
            m_renderPass_SMAA.DrawToViewport
            (
                pRenderViewport,
                m_resourceStates,
                pCommandBuffer_ShadingPass,
                m_pRenderSystem->GetSMAAAreaTexture(),
                m_pRenderSystem->GetSMAASearchTexture()
            );
        }

        RHI::LoadAction postProcessLoadAction = {};
        postProcessLoadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;
        postProcessLoadAction.m_colorClearValues[0] = pRenderViewport->m_finalTexture->m_clearValue;

        EE_ASSERT( !m_resourceStates.HasPendingBarriers() );
        m_resourceStates.Writeable( pRenderViewport->m_finalTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
        m_resourceStates.FlushBarriers( pCommandBuffer_ShadingPass );

        RHI::CmdSetRenderTargets( pCommandBuffer_ShadingPass, { &pRenderViewport->m_finalTexture.m_pTexture, 1 }, nullptr, &postProcessLoadAction );

        m_renderPass_PostProcess.DrawToViewport
        (
            m_resourceStates,
            pCommandBuffer_ShadingPass,
            m_pRenderSystem->GetTonemapLUT(),
            enableSMAA ? pRenderViewport->m_SMAA_ResultTexture : pRenderViewport->m_ForwardShading_ColorTexture,
            pRenderViewport->m_ForwardShading_DepthTexture
        );

        // Debug draw
        //---------------------------------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_debugDrawPass.DrawToViewport
        (
            pRenderViewport,
            pRenderWorldSystem->m_deviceRenderWorld,
            pRenderViewport->m_finalTexture,
            m_pRenderSystem->GetClusterBufferHandle(),
            renderViewBufferHandle,
            pRenderViewport->m_forwardShadingRenderViewsOffset,
            m_resourceStates,
            pCommandBuffer_ShadingPass,
            frameIndex
        );

        if ( !pRenderViewport->IsStandalone() )
        {
            m_resourceStates.ReadOnly( pRenderViewport->m_finalTexture, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
        }
        #endif

        // End frame
        //---------------------------------------------------------------------------------------------------

        RHI::CmdSetRenderTargets( pCommandBuffer_ShadingPass, {}, nullptr );
        m_resourceStates.FlushBarriers( pCommandBuffer_ShadingPass );

        if ( useAsyncCompute )
        {
            // HACK: Reuse command buffer for next frame/world, improves GPU utilization dramatically (very noticeable).
            // Only works when there's a single window, multiple windows _need_ to submit and synchronize properly.
            if ( m_pRenderSystem->IsSingleRenderWindow() )
            {
                RHI::QueueDeviceWait( m_pRenderSystem->GetGraphicsQueue(), m_pRenderSystem->GetComputeQueue(), signalSemaphore_GTAO );
            }
            else
            {
                RHI::QueueDeviceWait( m_pRenderSystem->GetGraphicsQueue(), m_pRenderSystem->GetComputeQueue(), signalSemaphore_GTAO );
                SubmitGraphicsCommandBuffer( eastl::move( pCommandBuffer_ShadingPass ) );

                pRenderViewport->m_pWindow->AcquireGraphicsCommandBuffer( m_pRenderSystem->GetContextRHI(), frameIndex );
            }
        }
    }

    uint64_t ForwardShadingRenderer::SubmitGraphicsCommandBuffer( RHI::CommandBuffer*&& pCommandBuffer )
    {
        EE_PROFILE_FUNCTION_RENDER();

        //---------------------------------------------------------------------------------------------------

        RHI::EndCommandBuffer( pCommandBuffer );

        uint64_t semaphore = RHI::QueueSubmit( m_pRenderSystem->GetGraphicsQueue(), { &pCommandBuffer, 1 } );

        pCommandBuffer = nullptr;

        return semaphore;
    }

    uint64_t ForwardShadingRenderer::SubmitComputeCommandBuffer( RHI::CommandBuffer*&& pCommandBuffer )
    {
        EE_PROFILE_FUNCTION_RENDER();

        //---------------------------------------------------------------------------------------------------

        RHI::EndCommandBuffer( pCommandBuffer );

        uint64_t semaphore = RHI::QueueSubmit( m_pRenderSystem->GetComputeQueue(), { &pCommandBuffer, 1 } );

        pCommandBuffer = nullptr;

        return semaphore;
    }
}
