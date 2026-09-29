#include "Renderer_ForwardShading.h"
#include "Engine/Render/RenderViewport.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/Systems/WorldSystem_Render.h"
#include "Engine/Render/Settings/ViewportSettings_Render.h"
#include "Engine/Render/Components/Component_Lights.h" // TODO: Move light components to DeviceRenderWorld
#include "Engine/Render/DebugMesh/DebugMeshRegistry.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/Profiling.h"
#include "Base/Memory/Memory.h"
#include "Base/Render/Settings/Settings_Render.h"
#include "Base/Render/RenderWindow.h"
#include "Base/Render/RHI.h"
#include "Base/Math/ViewVolume.h"
#include "EASTL/bit.h"

#include "Engine/Render/Shaders/Renderer/RendererTypes.esh"
#include "Engine/Render/Shaders/Picking/Picking.esh"

#include "Engine/Render/ProbeTables/DFGTable_Esoterica.h"
#include "Engine/Render/ProbeTables/ProbeTable_GGX_Esoterica_Cube.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    template<typename F>
    inline void ForwardShadingRenderer::ForEachRenderBucket( ActiveRenderViewList const& activeRenderViewList, F fn )
    {
        for ( ActiveRenderView const& activeRenderView : activeRenderViewList.m_activeRenderViews )
        {
            activeRenderView.ForEachRenderBucket( fn );
        }
    }

    template <typename F>
    inline void ForwardShadingRenderer::ForEachRenderPass( F fn )
    {
        fn( m_renderPass_forwardShading );
        fn( m_renderPass_cascadedShadows );
        fn( m_renderPass_punctualShadows );
        fn( m_renderPass_depthDownsample );
        fn( m_renderPass_globalEnvironmentMap );
        fn( m_renderPass_SMAA );
        fn( m_renderPass_GTAO );
        fn( m_renderPass_postProcess );

        #if EE_DEVELOPMENT_TOOLS
        fn( m_renderPass_debugDraw );
        fn( m_renderPass_editorOutline );
        #endif
    }

    //-------------------------------------------------------------------------

    void ShaderCullingBucket::Initialize( RHI::Context* pContextRHI, char const* pShaderName )
    {
        m_instanceVisibilityBuffer.Initialize( pContextRHI, false );
        m_clusterCullingWorkBuffer.Initialize( pContextRHI, false );
        m_cullingArgumentBuffer.Initialize( pContextRHI, false );
        m_drawCompactionArgumentBuffer.Initialize( pContextRHI, false );
        m_drawClusterBuffer.Initialize( pContextRHI, false );

        RHI::BufferParameters cullingCounterBufferParameters = {};
        cullingCounterBufferParameters.m_bufferSize = sizeof( uint32_t );
        cullingCounterBufferParameters.m_bufferStride = sizeof( uint32_t );
        cullingCounterBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer, RHI::DescriptorTypeFlags::Raw };
        cullingCounterBufferParameters.m_debugName.sprintf( "%s Culling Work Counter Buffer", pShaderName );

        m_pCullingCounterBuffer = RHI::CreateBuffer( pContextRHI, cullingCounterBufferParameters );

        uint32_t const numDrawClusterBufferEntries = uint32_t( EE_MAX_CULLING_VIEWS * ActiveRenderView::s_NumRenderBucketsPerViewBucket );

        RHI::BufferParameters drawClusterCountersBufferParameters = {};
        drawClusterCountersBufferParameters.m_bufferSize = sizeof( uint32_t ) * numDrawClusterBufferEntries;
        drawClusterCountersBufferParameters.m_bufferStride = sizeof( uint32_t );
        drawClusterCountersBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer, RHI::DescriptorTypeFlags::Raw };
        drawClusterCountersBufferParameters.m_debugName.sprintf( "%s Draw Cluster Counters Buffer", pShaderName );

        m_pDrawClusterCountersBuffer = RHI::CreateBuffer( pContextRHI, drawClusterCountersBufferParameters );

        RHI::BufferParameters drawClusterScatterOffsetsBufferParameters = {};
        drawClusterScatterOffsetsBufferParameters.m_bufferSize = sizeof( uint32_t ) * numDrawClusterBufferEntries;
        drawClusterScatterOffsetsBufferParameters.m_bufferStride = sizeof( uint32_t );
        drawClusterScatterOffsetsBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer, RHI::DescriptorTypeFlags::Raw };
        drawClusterScatterOffsetsBufferParameters.m_debugName.sprintf( "%s Draw Cluster Scatter Offsets Buffer", pShaderName );

        m_pDrawClusterScatterOffsetsBuffer = RHI::CreateBuffer( pContextRHI, drawClusterScatterOffsetsBufferParameters );

        RHI::BufferParameters drawClusterBaseOffsetsBufferParameters = {};
        drawClusterBaseOffsetsBufferParameters.m_bufferSize = sizeof( uint32_t ) * numDrawClusterBufferEntries;
        drawClusterBaseOffsetsBufferParameters.m_bufferStride = sizeof( uint32_t );
        drawClusterBaseOffsetsBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer };
        drawClusterBaseOffsetsBufferParameters.m_debugName.sprintf( "%s Draw Cluster Base Offsets Buffer", pShaderName );

        m_pDrawClusterBaseOffsetsBuffer = RHI::CreateBuffer( pContextRHI, drawClusterBaseOffsetsBufferParameters );
    }

    void ShaderCullingBucket::Shutdown( RHI::Context* pContextRHI )
    {
        m_instanceVisibilityBuffer.Shutdown( pContextRHI );
        m_clusterCullingWorkBuffer.Shutdown( pContextRHI );
        m_cullingArgumentBuffer.Shutdown( pContextRHI );
        m_drawCompactionArgumentBuffer.Shutdown( pContextRHI );
        m_drawClusterBuffer.Shutdown( pContextRHI );

        RHI::DestroyBuffer( pContextRHI, eastl::move( m_pCullingCounterBuffer ) );
        RHI::DestroyBuffer( pContextRHI, eastl::move( m_pDrawClusterCountersBuffer ) );
        RHI::DestroyBuffer( pContextRHI, eastl::move( m_pDrawClusterScatterOffsetsBuffer ) );
        RHI::DestroyBuffer( pContextRHI, eastl::move( m_pDrawClusterBaseOffsetsBuffer ) );
    }

    //-------------------------------------------------------------------------

    void ForwardShadingRenderer::Initialize( SystemRegistry* pSystemRegistry, RenderSettings const& renderSettings )
    {
        m_pRenderSystem = pSystemRegistry->GetSystem<RenderSystem>();
        m_pRenderGlobalSettings = &renderSettings;

        // Pipelines
        //-------------------------------------------------------------------------

        static StringID const s_InstanceCullingShaderID( "InstanceCulling" );
        static StringID const s_CullingCompactionShaderID( "CullingCompaction" );
        static StringID const s_CullingArgumentGenerationShaderID( "CullingArgumentGeneration" );
        static StringID const s_DrawCompactionShaderID( "DrawCompaction" );
        static StringID const s_ClusterCullingShaderID( "ClusterCulling" );
        static StringID const s_DrawArgumentGenerationShaderID( "DrawArgumentGeneration" );
        static StringID const s_LightCulling_CullLightsShaderID( "LightCulling_CullLights" );

        m_pInstanceCullingShader = m_pRenderSystem->FindComputeShader( s_InstanceCullingShaderID );
        m_pCullingCompactionShader = m_pRenderSystem->FindComputeShader( s_CullingCompactionShaderID );
        m_pCullingArgumentGenerationShader = m_pRenderSystem->FindComputeShader( s_CullingArgumentGenerationShaderID );
        m_pDrawCompactionShader = m_pRenderSystem->FindComputeShader( s_DrawCompactionShaderID );
        m_pClusterCullingShader = m_pRenderSystem->FindComputeShader( s_ClusterCullingShaderID );
        m_pDrawArgumentGenerationShader = m_pRenderSystem->FindComputeShader( s_DrawArgumentGenerationShaderID );
        m_pLightCulling_cullLightsShader = m_pRenderSystem->FindComputeShader( s_LightCulling_CullLightsShaderID );

        #if EE_DEVELOPMENT_TOOLS
        static StringID const s_InstancePickingResolveShaderID( "InstancePickingResolve" );

        m_pInstancePickingResolveShader = m_pRenderSystem->FindComputeShader( s_InstancePickingResolveShaderID );
        #endif

        m_materialShaderPipelineBuckets = ForwardShadingPass::InitializeMaterialShaderBuckets( m_pRenderSystem );

        // Culling buckets - one per material shader, shared by all viewports
        //-------------------------------------------------------------------------

        m_shaderCullingBuckets.resize( m_materialShaderPipelineBuckets.size() );
        for ( size_t shaderIndex = 0; shaderIndex < m_shaderCullingBuckets.size(); ++shaderIndex )
        {
            m_shaderCullingBuckets[shaderIndex].Initialize( m_pRenderSystem->GetContextRHI(), m_materialShaderPipelineBuckets[shaderIndex].m_shaderName.data() );
        }

        m_lightCulling_spatialHash.Initialize( m_pRenderSystem->GetContextRHI(), "LightCulling", m_pRenderGlobalSettings->m_spatialHashTableSize, 0 );

        // Render passes
        //-------------------------------------------------------------------------

        ForEachRenderPass( [this] ( auto& renderPass )
        {
            RenderPassContext context = { m_pRenderSystem, m_pRenderGlobalSettings, m_materialShaderPipelineBuckets, };
            renderPass.Initialize( context );
        } );

        #if EE_DEVELOPMENT_TOOLS
        m_renderPass_debugDraw.SetDebugMeshRegistry( pSystemRegistry->GetSystem<DebugMeshRegistry>() );
        #endif
    }

    void ForwardShadingRenderer::Shutdown()
    {
        #if EE_DEVELOPMENT_TOOLS
        m_renderPass_debugDraw.ClearDebugMeshRegistry();
        #endif

        for ( ForwardShadingMaterialShaderPipelineBucket& materialShaderPipelineBucket : m_materialShaderPipelineBuckets )
        {
            materialShaderPipelineBucket.Shutdown( m_pRenderSystem->GetContextRHI() );
        }
        m_materialShaderPipelineBuckets.clear();

        for ( ShaderCullingBucket& shaderCullingBucket : m_shaderCullingBuckets )
        {
            shaderCullingBucket.Shutdown( m_pRenderSystem->GetContextRHI() );
        }
        m_shaderCullingBuckets.clear();

        ForEachRenderPass( [this] ( auto& renderPass )
        {
            renderPass.Shutdown( m_pRenderSystem );
        } );

        m_lightCulling_spatialHash.Shutdown( m_pRenderSystem->GetContextRHI() );

        RHI::DestroyBuffer( m_pRenderSystem->GetContextRHI(), eastl::move( m_pProbeTableBuffer ) );
        RHI::DestroyTexture( m_pRenderSystem->GetContextRHI(), eastl::move( m_pDFGTexture ) );
    }

    void ForwardShadingRenderer::UpdateDeviceResources( UpdateContext const& ctx )
    {
        EE_PROFILE_FUNCTION_RENDER();

        uint32_t            frameIndex = m_pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = m_pRenderSystem->GetContextRHI();

        // Probe table
        //-------------------------------------------------------------------------

        if ( !m_pProbeTableBuffer )
        {
            struct ProbeTableHeader
            {
                uint32_t    m_magic;
                uint32_t    m_version;
                uint32_t    m_numLevels;
                uint32_t    m_numParameters;
                uint32_t    m_numActiveCoefficients;
                uint32_t    m_numTapsPerAxis;
                uint32_t    m_projection;
                uint32_t    m_numAxes;
                uint32_t    m_numIndices;
                uint32_t    m_numFloat4;
                uint32_t    m_payloadOffset;
                uint32_t    m_payloadBytes;

                char        m_shapeName[16];
                char        m_profileName[48];
                float       m_widths[7];

                uint32_t    m_reserved[1];
            };
            static_assert( sizeof( ProbeTableHeader ) == 144, "the probe table header is a file layout and must not gain padding" );

            Blob const probeTable = Embed::ProbeTable_GGX_Esoterica_Cube::GetFileData();
            EE_ASSERT( !probeTable.empty() );
            EE_ASSERT( probeTable.size() >= sizeof( ProbeTableHeader ) );

            ProbeTableHeader const* pHeader = reinterpret_cast<ProbeTableHeader const*>( probeTable.data() );

            EE_ASSERT( pHeader->m_magic == 0x4C425446 );                        // 'FTBL'
            EE_ASSERT( pHeader->m_version == 4 );
            EE_ASSERT( pHeader->m_numLevels == ( RHI::ComputeUncompressedMipLevels( 128, 128, 1 ) - 1 ) );
            EE_ASSERT( pHeader->m_numParameters == 5 );
            EE_ASSERT( pHeader->m_numActiveCoefficients == 1 );
            EE_ASSERT( pHeader->m_numTapsPerAxis == 8 );
            EE_ASSERT( pHeader->m_projection == 0 );                            // cubemap
            EE_ASSERT( pHeader->m_numAxes == 3 );
            EE_ASSERT( ( pHeader->m_payloadOffset + pHeader->m_payloadBytes ) == probeTable.size() );

            uint8_t const* const pPayload = reinterpret_cast<uint8_t const*>( probeTable.data() ) + pHeader->m_payloadOffset;

            auto CopyProbeTable = [pPayload] ( void* pDstMemory_WriteCombined, size_t size )
            {
                //Memory::CopyToWriteCombined( pDstMemory_WriteCombined, pPayload, size );
                memcpy( pDstMemory_WriteCombined, pPayload, size ); // payload not aligned
            };

            RHI::BufferParameters probeTableParameters = {};
            probeTableParameters.m_bufferSize = pHeader->m_payloadBytes;
            probeTableParameters.m_bufferStride = sizeof( Float4 );
            probeTableParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::Buffer;
            probeTableParameters.m_debugName = "Probe Table";

            m_pProbeTableBuffer = m_pRenderSystem->QueueBufferCreate( CopyProbeTable, probeTableParameters );
        }

        // DFG texture
        //-------------------------------------------------------------------------

        if ( !m_pDFGTexture )
        {
            constexpr uint32_t DFGResolution = 128;

            struct DFGTableHeader
            {
                uint32_t    m_magic;
                uint32_t    m_version;
                uint32_t    m_resolution;
                uint32_t    m_sampleCount;
                uint32_t    m_valuesPerTexel;
                uint32_t    m_valueFormat;
                uint32_t    m_payloadOffset;
                uint32_t    m_payloadBytes;

                char        m_name[24];
                uint32_t    m_reserved[2];
            };
            static_assert( sizeof( DFGTableHeader ) == 64, "the DFG table header is a file layout and must not gain padding" );

            Blob const dfgTable = Embed::DFGTable_Esoterica::GetFileData();

            EE_ASSERT( RHI::ComputeFormatRowStride( RHI::DataFormat::RG16_SFloat, DFGResolution ) == ( DFGResolution * 4U ) );
            EE_ASSERT( dfgTable.size() >= sizeof( DFGTableHeader ) );

            if ( dfgTable.size() < sizeof( DFGTableHeader ) )
            {
                return;
            }

            DFGTableHeader const* pHeader = reinterpret_cast<DFGTableHeader const*>( dfgTable.data() );

            bool const isDFGTable = ( pHeader->m_magic == 0x46444746 )                       // 'FGDF'
                && ( pHeader->m_version == 2 )
                && ( pHeader->m_resolution == DFGResolution )
                && ( pHeader->m_sampleCount != 0 )
                && ( pHeader->m_valuesPerTexel == 2 )
                && ( pHeader->m_valueFormat == 0 )                                          // two binary16
                && ( pHeader->m_payloadBytes == ( DFGResolution * DFGResolution * 4U ) )
                && ( ( uint64_t( pHeader->m_payloadOffset ) + uint64_t( pHeader->m_payloadBytes ) ) == dfgTable.size() );

            EE_ASSERT( isDFGTable );

            if ( !isDFGTable )
            {
                return;
            }

            uint8_t const* const pPayload = reinterpret_cast<uint8_t const*>( dfgTable.data() ) + pHeader->m_payloadOffset;

            auto CopyDFGTable = [pPayload] ( uint8_t* pDstMemory_WriteCombined, size_t srcOffset, uint32_t srcRowStride, uint32_t row )
            {
                (void) row;

                //Memory::CopyToWriteCombined( pDstMemory_WriteCombined, pPayload + srcOffset, srcRowStride );
                memcpy( pDstMemory_WriteCombined, pPayload + srcOffset, srcRowStride ); // payload not aligned
            };

            RHI::TextureParameters dfgTextureParameters = {};
            dfgTextureParameters.m_width = DFGResolution;
            dfgTextureParameters.m_height = DFGResolution;
            dfgTextureParameters.m_format = RHI::DataFormat::RG16_SFloat;
            dfgTextureParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::Texture;
            dfgTextureParameters.m_debugName = "Precomputed DFG";

            m_pDFGTexture = m_pRenderSystem->QueueTextureCreate( CopyDFGTable, dfgTextureParameters );
        }

        #if EE_DEVELOPMENT_TOOLS
        m_renderPass_debugDraw.UpdateDeviceResources( m_pRenderSystem );
        #endif
    }

    void ForwardShadingRenderer::UpdateViewportDeviceResources( UpdateContext const& ctx, RenderViewport* pRenderViewport, EntityWorld* pWorld )
    {
        EE_PROFILE_FUNCTION_RENDER();

        uint32_t frameIndex = m_pRenderSystem->GetFrameIndex();
        RHI::Context* pContextRHI = m_pRenderSystem->GetContextRHI();
        RenderWorldSystem* pRenderWorldSystem = pWorld->GetWorldSystem<RenderWorldSystem>();

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        pRenderWorldSystem->UpdateViewportPickingData( pRenderViewport );
        #endif

        bool const enableAsyncCompute = m_pRenderGlobalSettings->m_enableAsyncCompute;
        bool const enableSMAA = m_pRenderGlobalSettings->m_enableSMAA;
        bool const enableSSAO = m_pRenderGlobalSettings->m_enableSSAO;
        bool const enableSSAOLowResolution = m_pRenderGlobalSettings->m_enableSSAOLowResolution;

        #if EE_DEVELOPMENT_TOOLS
        bool const enableEditorOutline = pRenderViewport->IsPickingEnabled();
        #else
        bool const enableEditorOutline = false;
        #endif

        bool const enableDepthDownsample = ( enableSSAO && enableSSAOLowResolution );

        //-------------------------------------------------------------------------

        m_renderPass_forwardShading.UpdateViewportDeviceResources( m_pRenderSystem, pRenderViewport );

        if ( enableSMAA )
        {
            m_renderPass_SMAA.UpdateViewportDeviceResources( m_pRenderSystem, pRenderViewport );
        }

        if ( enableDepthDownsample )
        {
            m_renderPass_depthDownsample.UpdateViewportDeviceResources( m_pRenderSystem, pRenderViewport );
        }

        if ( enableSSAO )
        {
            m_renderPass_GTAO.m_enableLowResolution = enableSSAOLowResolution;
            m_renderPass_GTAO.UpdateViewportDeviceResources( m_pRenderSystem, pRenderViewport );
        }

        m_renderPass_postProcess.UpdateViewportDeviceResources( m_pRenderSystem, pRenderViewport );
        m_renderPass_globalEnvironmentMap.UpdateViewportDeviceResources( m_pRenderSystem, pRenderViewport );

        #if EE_DEVELOPMENT_TOOLS
        m_renderPass_debugDraw.UpdateViewportDeviceResources
        (
            m_pRenderSystem,
            pRenderWorldSystem->m_deviceRenderWorld,
            pWorld->GetDebugDrawSystem(),
            ctx.GetDeltaTime(),
            pRenderViewport
        );

        if ( enableEditorOutline )
        {
            m_renderPass_editorOutline.UpdateViewportDeviceResources( m_pRenderSystem, pRenderViewport );
        }
        #endif

        // Light culling spatial hash
        //-------------------------------------------------------------------------

        uint32_t const spatialHashPayloadStride = DeviceSpatialHash::ComputePayloadStride
        (
            pRenderWorldSystem->m_deviceRenderWorld.GetNumPointLightPages(),
            pRenderWorldSystem->m_deviceRenderWorld.GetNumSpotLightPages()
        );

        m_lightCulling_spatialHash.UpdateBuffers( m_pRenderSystem, frameIndex, spatialHashPayloadStride );

        m_lightCulling_spatialHash.m_numLODs = Math::Clamp( m_pRenderGlobalSettings->m_spatialHashNumLODs, 1u, DeviceSpatialHash::s_maxLODs );

        m_lightCulling_spatialHash.m_borderCells[1] = m_pRenderGlobalSettings->m_spatialHashBorderLOD1;
        m_lightCulling_spatialHash.m_borderCells[2] = m_pRenderGlobalSettings->m_spatialHashBorderLOD2;
        m_lightCulling_spatialHash.m_borderCells[3] = m_pRenderGlobalSettings->m_spatialHashBorderLOD3;
        m_lightCulling_spatialHash.m_borderCells[4] = m_pRenderGlobalSettings->m_spatialHashBorderLOD4;
        m_lightCulling_spatialHash.m_borderCells[5] = m_pRenderGlobalSettings->m_spatialHashBorderLOD5;

        // Build active render view list
        //-------------------------------------------------------------------------

        DeviceRenderWorld& deviceRenderWorld = pRenderWorldSystem->m_deviceRenderWorld;

        uint32_t editorOutlineRenderViewIndex = ActiveRenderViewList::s_InvalidActiveRenderViewIndex;

        #if EE_DEVELOPMENT_TOOLS
        if ( enableEditorOutline )
        {
            editorOutlineRenderViewIndex = pRenderViewport->m_editorOutlineRenderViewProxy.GetBaseRenderViewIndex();
        }
        #endif

        pRenderViewport->m_activeRenderViewSelection.SelectActiveRenderViews
        (
            deviceRenderWorld,
            pRenderViewport->m_activeRenderViewList,
            pRenderViewport->m_mainRenderViewProxy.GetBaseRenderViewIndex(),
            editorOutlineRenderViewIndex
        );

        uint32_t const numActiveRenderViews = pRenderViewport->m_activeRenderViewList.m_numActiveRenderViews;
        EE_ASSERT( numActiveRenderViews <= EE_MAX_CULLING_VIEWS );

        pRenderViewport->m_activeRenderViewList.m_numRenderViewBucketsPerView = uint32_t( m_materialShaderPipelineBuckets.size() * ActiveRenderView::s_NumRenderBucketsPerViewBucket );
        pRenderViewport->m_activeRenderViewList.m_numRenderBuckets = numActiveRenderViews * pRenderViewport->m_activeRenderViewList.m_numRenderViewBucketsPerView;

        // Resize the per active view culling state
        //-------------------------------------------------------------------------

        if ( pRenderViewport->m_activeRenderViews.size() < numActiveRenderViews )
        {
            size_t const numExistingActiveRenderViews = pRenderViewport->m_activeRenderViews.size();

            pRenderViewport->m_activeRenderViews.resize( numActiveRenderViews );
            for ( size_t activeRenderViewIndex = numExistingActiveRenderViews; activeRenderViewIndex < pRenderViewport->m_activeRenderViews.size(); ++activeRenderViewIndex )
            {
                pRenderViewport->m_activeRenderViews[activeRenderViewIndex].Initialize( pContextRHI, m_materialShaderPipelineBuckets.size() );
            }
        }

        //-------------------------------------------------------------------------

        pRenderViewport->m_activeRenderViewList.m_activeRenderViews = { pRenderViewport->m_activeRenderViews.data(), numActiveRenderViews };

        for ( uint32_t activeRenderViewIndex = 0; activeRenderViewIndex < numActiveRenderViews; ++activeRenderViewIndex )
        {
            pRenderViewport->m_activeRenderViews[activeRenderViewIndex].UpdateDeviceResources( m_pRenderSystem, deviceRenderWorld );
        }

        // RenderView indirection tables
        //-------------------------------------------------------------------------

        if ( !pRenderViewport->m_renderViewIndirectionBuffers[frameIndex] || pRenderViewport->m_renderViewIndirectionBuffers[frameIndex]->m_size < ( EE_MAX_CULLING_VIEWS * sizeof( uint16_t ) ) )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pRenderViewport->m_renderViewIndirectionBuffers[frameIndex] ) );

            RHI::BufferParameters indirectionBufferParameters = {};
            indirectionBufferParameters.m_bufferSize = EE_MAX_CULLING_VIEWS * sizeof( uint16_t );
            indirectionBufferParameters.m_bufferStride = sizeof( uint16_t );
            indirectionBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            indirectionBufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::Buffer;
            indirectionBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            indirectionBufferParameters.m_debugName.sprintf( "RenderView Indirection Buffer %i", frameIndex );

            pRenderViewport->m_renderViewIndirectionBuffers[frameIndex] = RHI::CreateBuffer( pContextRHI, indirectionBufferParameters );
        }

        {
            uint16_t* pIndirectionTable = static_cast<uint16_t*>( pRenderViewport->m_renderViewIndirectionBuffers[frameIndex]->m_pMappedAddress_WriteCombined );

            for ( uint32_t activeRenderViewIndex = 0; activeRenderViewIndex < EE_MAX_CULLING_VIEWS; ++activeRenderViewIndex )
            {
                pIndirectionTable[activeRenderViewIndex] = pRenderViewport->m_activeRenderViewList.m_deviceRenderViewIndicesPerActiveRenderView[activeRenderViewIndex];
            }
        }

        pRenderViewport->m_activeRenderViewList.m_activeRenderViewIndicesByDeviceRenderView.clear();
        pRenderViewport->m_activeRenderViewList.m_activeRenderViewIndicesByDeviceRenderView.resize( deviceRenderWorld.GetRenderViewCapacity(), ActiveRenderViewList::s_InvalidActiveRenderViewIndex );

        for ( uint32_t activeRenderViewIndex = 0; activeRenderViewIndex < numActiveRenderViews; ++activeRenderViewIndex )
        {
            uint16_t const deviceRenderViewIndex = pRenderViewport->m_activeRenderViewList.m_deviceRenderViewIndicesPerActiveRenderView[activeRenderViewIndex];
            pRenderViewport->m_activeRenderViewList.m_activeRenderViewIndicesByDeviceRenderView[deviceRenderViewIndex] = uint16_t( activeRenderViewIndex );
        }

        //-------------------------------------------------------------------------

        for ( uint32_t shaderIndex = 0; shaderIndex < m_materialShaderPipelineBuckets.size(); ++shaderIndex )
        {
            uint32_t const clusterCapacity = pRenderWorldSystem->m_deviceRenderWorld.GetClusterCapacity( shaderIndex );

            auto UpdateBuffer_DrawClusterBuffer = [this, shaderIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
            {
                m_pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );

                RHI::BufferParameters drawClusterBufferParameters = {};
                drawClusterBufferParameters.m_bufferSize = newBufferSize;
                drawClusterBufferParameters.m_bufferStride = sizeof( uint32_t );
                drawClusterBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer };
                drawClusterBufferParameters.m_debugName.sprintf( "Shader Culling Bucket Draw Cluster Buffer %u", shaderIndex );

                return RHI::CreateBuffer( m_pRenderSystem->GetContextRHI(), drawClusterBufferParameters );
            };

            m_shaderCullingBuckets[shaderIndex].m_drawClusterBuffer.UpdateDeviceResources
            (
                Math::Max( 1ULL, size_t( clusterCapacity ) * pRenderViewport->m_activeRenderViewList.m_numActiveRenderViews * sizeof( uint32_t ) ),
                UpdateBuffer_DrawClusterBuffer
            );
        }

        // Global Parameters Buffer
        //-------------------------------------------------------------------------

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
        //-------------------------------------------------------------------------

        size_t const renderBucketBufferSize = pRenderViewport->m_activeRenderViewList.m_numRenderBuckets * sizeof( ShaderTypes::RenderBucket );
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

        // Render buckets
        //-------------------------------------------------------------------------
        TArrayView<ShaderTypes::RenderBucket> renderBucketMemory_WriteCombined = TArrayView<ShaderTypes::RenderBucket>( static_cast<ShaderTypes::RenderBucket*>( pRenderViewport->m_renderBucketBuffers[frameIndex]->m_pMappedAddress_WriteCombined ), pRenderViewport->m_activeRenderViewList.m_numRenderBuckets );

        uint32_t renderBucketIndex = 0;
        ForEachRenderBucket( pRenderViewport->m_activeRenderViewList, [renderBucketMemory_WriteCombined, &renderBucketIndex] ( ActiveRenderViewMaterialShaderBucket const& renderBucket )
        {
            ShaderTypes::RenderBucket deviceRenderBucket = {};
            deviceRenderBucket.m_drawCounterBuffer = RHI::GetBufferHandle( renderBucket.m_pDrawCounterBuffer, RHI::DescriptorTypeFlags::RWBuffer );
            deviceRenderBucket.m_drawArgumentBuffer = RHI::GetBufferHandle( renderBucket.m_drawArgumentBuffer.m_pBuffer, RHI::DescriptorTypeFlags::RWBuffer );

            renderBucketMemory_WriteCombined[renderBucketIndex] = deviceRenderBucket;
            renderBucketIndex++;
        } );

        EE_ASSERT( renderBucketIndex == pRenderViewport->m_activeRenderViewList.m_numRenderBuckets );

        // Update constant buffers
        //-------------------------------------------------------------------------

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

        globalParameters.m_skinningTransformBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetSkinningTransformBufferHandle();
        globalParameters.m_renderBucketBuffer = RHI::GetBufferHandle( pRenderViewport->m_renderBucketBuffers[frameIndex], RHI::DescriptorTypeFlags::Buffer );

        globalParameters.m_shaderDataBuffer = m_pRenderSystem->GetShaderDataBufferHandle();
        globalParameters.m_renderViewBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetRenderViewBufferHandle();
        globalParameters.m_renderViewIndirectionBuffer = RHI::GetBufferHandle( pRenderViewport->m_renderViewIndirectionBuffers[frameIndex], RHI::DescriptorTypeFlags::Buffer );

        globalParameters.m_meshInstanceRootBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetMeshInstanceRootBufferHandle();
        globalParameters.m_meshInstanceRootPageBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetMeshInstanceRootPageBufferHandle( frameIndex );
        globalParameters.m_pointShadowResolution = m_pRenderGlobalSettings->m_pointShadowResolution;
        globalParameters.m_spotShadowResolution = m_pRenderGlobalSettings->m_spotShadowResolution;

        globalParameters.m_directionalShadowNormalOffsetScale = m_pRenderGlobalSettings->m_directionalShadowNormalOffsetScale;
        globalParameters.m_spotShadowNormalOffsetScale = m_pRenderGlobalSettings->m_spotShadowNormalOffsetScale;
        globalParameters.m_pointShadowNormalOffsetScale = m_pRenderGlobalSettings->m_pointShadowNormalOffsetScale;
        globalParameters.m_directionalShadowDepthBias = m_pRenderGlobalSettings->m_directionalShadowDepthBias;

        // Punctual biases are authored in world space shadow texels and are applied to the distance along the
        // projection axis by the shader. The directional bias stays in the linear orthographic range of a cascade.
        globalParameters.m_spotShadowDepthBias = m_pRenderGlobalSettings->m_spotShadowDepthBias;
        globalParameters.m_pointShadowDepthBias = m_pRenderGlobalSettings->m_pointShadowDepthBias;

        DeviceRenderView const* const pGlobalEnvironmentMapView = &pRenderViewport->m_globalEnvironmentMapRenderViewProxy.m_renderViewHandle.m_data[0];

        if ( enableSSAO )
        {
            globalParameters.m_ssaoTexture = RHI::GetTextureHandle( pRenderViewport->m_GTAO_resultTexture, RHI::DescriptorTypeFlags::Texture, 0 );
        }
        globalParameters.m_dfgTexture = RHI::GetTextureHandle( m_pDFGTexture, RHI::DescriptorTypeFlags::Texture, 0 );
        globalParameters.m_radianceTexture = RHI::GetTextureHandle( pGlobalEnvironmentMapView->m_pEnvironmentMapRadianceTexture, RHI::DescriptorTypeFlags::TextureCube, 0 );

        globalParameters.m_directionalLightBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetDirectionalLightBufferHandle();
        globalParameters.m_directionalLightPageBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetDirectionalLightPageBufferHandle( frameIndex );
        globalParameters.m_numDirectionalLightPages = pRenderWorldSystem->m_deviceRenderWorld.GetNumDirectionalLightPages();

        globalParameters.m_pointLightBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetPointLightBufferHandle();
        globalParameters.m_pointLightPageBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetPointLightPageBufferHandle( frameIndex );
        globalParameters.m_numPointLightPages = pRenderWorldSystem->m_deviceRenderWorld.GetNumPointLightPages();

        globalParameters.m_spotLightBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetSpotLightBufferHandle();
        globalParameters.m_spotLightPageBuffer = pRenderWorldSystem->m_deviceRenderWorld.GetSpotLightPageBufferHandle( frameIndex );
        globalParameters.m_numSpotLightPages = pRenderWorldSystem->m_deviceRenderWorld.GetNumSpotLightPages();

        globalParameters.m_lightCulling_MinCellSize = m_pRenderGlobalSettings->m_lightCullingMinCellSize;
        globalParameters.m_lightCulling_SpatialHashLow = m_lightCulling_spatialHash.GetPackedHandleLow();
        globalParameters.m_lightCulling_SpatialHashHigh = m_lightCulling_spatialHash.GetPackedHandleHigh( m_pRenderGlobalSettings->m_lightCullingMinCellSize );

        // Picking
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( pRenderViewport->IsPickingEnabled() )
        {
            auto UpdateInstancePickingDistancesBuffer = [pContextRHI, this] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
            {
                m_pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );

                RHI::BufferParameters instancePickingDistanceBufferParameters = {};
                instancePickingDistanceBufferParameters.m_bufferSize = newBufferSize;
                instancePickingDistanceBufferParameters.m_bufferStride = sizeof( uint32_t );
                instancePickingDistanceBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer, RHI::DescriptorTypeFlags::Raw };
                instancePickingDistanceBufferParameters.m_debugName.sprintf( "Instance Root Picking Distances Buffer" );

                return RHI::CreateBuffer( pContextRHI, instancePickingDistanceBufferParameters );
            };

            size_t instanceDistancesBufferSize = size_t( pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstanceRootPages() ) * 64 * sizeof( float );
            pRenderViewport->m_instancePickingDistancesBuffer.UpdateDeviceResources( instanceDistancesBufferSize, UpdateInstancePickingDistancesBuffer );

            pRenderViewport->m_instancePickingResultsBuffer.UpdateBuffers( m_pRenderSystem, frameIndex, sizeof( ShaderTypes::PickingResult ), RHI::DescriptorTypeFlags::RWBuffer );

            globalParameters.m_instancePickingResultsBuffer = pRenderViewport->m_instancePickingResultsBuffer.GetAppendBufferHandle();
            globalParameters.m_instancePickingDistancesBuffer = RHI::GetBufferHandle( pRenderViewport->m_instancePickingDistancesBuffer.m_pBuffer, RHI::DescriptorTypeFlags::RWBuffer );

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

        globalParameters.m_mainCameraRenderView = pRenderViewport->m_activeRenderViewList.m_mainRenderViewActiveIndex;
        globalParameters.m_numRenderViewBucketsPerView = pRenderViewport->m_activeRenderViewList.m_numRenderViewBucketsPerView;
        globalParameters.m_numActiveRenderViews = pRenderViewport->m_activeRenderViewList.m_numActiveRenderViews;
        globalParameters.m_numRenderBuckets = pRenderViewport->m_activeRenderViewList.m_numRenderBuckets;

        // Editor selection outline
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        globalParameters.m_meshInstanceRootOutlineBuffer = pRenderWorldSystem->GetMeshInstanceRootOutlineBufferHandle();
        globalParameters.m_editorOutlineRenderViewIndex = pRenderViewport->m_activeRenderViewList.m_editorOutlineRenderViewActiveIndex;
        globalParameters.m_editorOutlineEnabled = enableEditorOutline ? 1 : 0;
        #else
        globalParameters.m_editorOutlineEnabled = 0;
        #endif

        // Misc
        //-------------------------------------------------------------------------

        globalParameters.m_shCoefficientsBuffer = RHI::GetBufferHandle( pGlobalEnvironmentMapView->m_pSHCoefficientsBuffer, RHI::DescriptorTypeFlags::Buffer );
        globalParameters.m_radianceTextureMipLevels = float( pGlobalEnvironmentMapView->m_pEnvironmentMapRadianceTexture->m_mipLevels );
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

        RenderWorldSystem* pRenderWorldSystem = pWorld->GetWorldSystem<RenderWorldSystem>();

        // Fit cascaded shadows for the main viewport
        //-------------------------------------------------------------------------

        RenderViewport* pWorldMainViewport = static_cast<RenderViewport*>( pWorld->GetMainViewport() );
        pRenderWorldSystem->UpdateDirectionalLightShadows( pWorldMainViewport->GetViewVolume() );

        // Viewport owned render views
        //-------------------------------------------------------------------------

        m_renderPass_forwardShading.UpdateWorldDeviceResources( pWorld );
        m_renderPass_globalEnvironmentMap.UpdateWorldDeviceResources( pWorld );

        #if EE_DEVELOPMENT_TOOLS
        m_renderPass_editorOutline.UpdateWorldDeviceResources( pWorld );
        #endif

        // Update world resources
        //-------------------------------------------------------------------------

        pRenderWorldSystem->UpdateDeviceResources();

        // Per-shader culling buffers
        //-------------------------------------------------------------------------

        EE_ASSERT( m_materialShaderPipelineBuckets.size() == pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstanceShaderPools() );

        RenderSystem* pRenderSystem = m_pRenderSystem;

        for ( uint32_t shaderIndex = 0; shaderIndex < m_materialShaderPipelineBuckets.size(); ++shaderIndex )
        {
            uint32_t const instanceCapacity = pRenderWorldSystem->m_deviceRenderWorld.GetMeshInstanceCapacity( shaderIndex );
            uint32_t const clusterCapacity = pRenderWorldSystem->m_deviceRenderWorld.GetClusterCapacity( shaderIndex );

            //-------------------------------------------------------------------------

            auto UpdateBuffer_InstanceVisibility = [pRenderSystem, shaderIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
            {
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );

                RHI::BufferParameters instanceVisibilityBufferParameters = {};
                instanceVisibilityBufferParameters.m_bufferSize = newBufferSize;
                instanceVisibilityBufferParameters.m_bufferStride = sizeof( uint64_t );
                instanceVisibilityBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer };
                instanceVisibilityBufferParameters.m_debugName.sprintf( "MaterialShaderBucket Instance Visibility Buffer %u", shaderIndex );

                return RHI::CreateBuffer( pRenderSystem->GetContextRHI(), instanceVisibilityBufferParameters );
            };

            m_shaderCullingBuckets[shaderIndex].m_instanceVisibilityBuffer.UpdateDeviceResources
            (
                Math::Max( 1ULL, size_t( instanceCapacity ) * sizeof( uint64_t ) ),
                UpdateBuffer_InstanceVisibility
            );

            //-------------------------------------------------------------------------

            auto UpdateBuffer_CullingArgument = [pRenderSystem, shaderIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
            {
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );

                RHI::BufferParameters cullingArgumentBufferParameters = {};
                cullingArgumentBufferParameters.m_alignment = RHI::g_indirectCommandAlignment;
                cullingArgumentBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::IndirectArgumentBuffer, RHI::DescriptorTypeFlags::RWBuffer };
                cullingArgumentBufferParameters.m_bufferSize = newBufferSize;
                cullingArgumentBufferParameters.m_bufferStride = sizeof( ShaderTypes::ClusterCullingArgument );
                cullingArgumentBufferParameters.m_debugName.sprintf( "MaterialShaderBucket ClusterCullingArgument Buffer %u", shaderIndex );

                return RHI::CreateBuffer( pRenderSystem->GetContextRHI(), cullingArgumentBufferParameters );
            };

            size_t const maxNumCullingArguments = size_t( clusterCapacity ) / ( size_t( RHI::Limits::MaxDispatchSize ) * 128 ) + 1;

            m_shaderCullingBuckets[shaderIndex].m_cullingArgumentBuffer.UpdateDeviceResources
            (
                Math::Max( 1ULL, maxNumCullingArguments * sizeof( ShaderTypes::ClusterCullingArgument ) ),
                UpdateBuffer_CullingArgument
            );

            //-------------------------------------------------------------------------

            auto UpdateBuffer_ClusterCullingWorkBuffer = [pRenderSystem, shaderIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
            {
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );

                RHI::BufferParameters cullingWorkBufferParameters = {};
                cullingWorkBufferParameters.m_bufferSize = newBufferSize;
                cullingWorkBufferParameters.m_bufferStride = sizeof( ShaderTypes::ClusterCullingWorkEntry );
                cullingWorkBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer };
                cullingWorkBufferParameters.m_debugName.sprintf( "Shader Culling Bucket Cluster Culling Work Buffer %u", shaderIndex );

                return RHI::CreateBuffer( pRenderSystem->GetContextRHI(), cullingWorkBufferParameters );
            };

            m_shaderCullingBuckets[shaderIndex].m_clusterCullingWorkBuffer.UpdateDeviceResources
            (
                Math::Max( 1ULL, size_t( clusterCapacity ) * sizeof( ShaderTypes::ClusterCullingWorkEntry ) ),
                UpdateBuffer_ClusterCullingWorkBuffer
            );

            //-------------------------------------------------------------------------

            auto UpdateBuffer_DrawCompactionArgument = [pRenderSystem, shaderIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
            {
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );

                RHI::BufferParameters drawCompactionArgumentBufferParameters = {};
                drawCompactionArgumentBufferParameters.m_alignment = RHI::g_indirectCommandAlignment;
                drawCompactionArgumentBufferParameters.m_bufferSize = newBufferSize;
                drawCompactionArgumentBufferParameters.m_bufferStride = sizeof( ShaderTypes::DrawCompactionArgument );
                drawCompactionArgumentBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::IndirectArgumentBuffer, RHI::DescriptorTypeFlags::RWBuffer };
                drawCompactionArgumentBufferParameters.m_debugName.sprintf( "MaterialShaderBucket DrawCompactionArgument Buffer %u", shaderIndex );

                return RHI::CreateBuffer( pRenderSystem->GetContextRHI(), drawCompactionArgumentBufferParameters );
            };

            m_shaderCullingBuckets[shaderIndex].m_drawCompactionArgumentBuffer.UpdateDeviceResources
            (
                Math::Max( 1ULL, maxNumCullingArguments * sizeof( ShaderTypes::DrawCompactionArgument ) ),
                UpdateBuffer_DrawCompactionArgument
            );
        }
    }

    uint64_t ForwardShadingRenderer::DispatchWorld( UpdateContext const& updateContext, RenderViewport const* pRenderViewport, EntityWorld* pWorld, uint64_t waitSemaphore )
    {
        EE_PROFILE_FUNCTION_RENDER();

        //-------------------------------------------------------------------------

        bool const enableAsyncCompute = m_pRenderGlobalSettings->m_enableAsyncCompute;

        bool const enableSMAA = m_pRenderGlobalSettings->m_enableSMAA;

        bool const enableSSAO = m_pRenderGlobalSettings->m_enableSSAO;
        bool const enableSSAOLowResolution = m_pRenderGlobalSettings->m_enableSSAOLowResolution;
        bool const enableDepthDownsample = ( enableSSAO && enableSSAOLowResolution ) || false;

        uint32_t const frameIndex = m_pRenderSystem->GetFrameIndex();
        RHI::CommandBuffer* pCommandBuffer = pRenderViewport->m_pWindow->GetActiveCommandBuffer( frameIndex );

        if ( enableAsyncCompute )
        {
            pCommandBuffer = pRenderViewport->m_pWindow->AcquireComputeCommandBuffer( m_pRenderSystem->GetContextRHI(), frameIndex );
        }

        RenderWorldSystem* pRenderWorldSystem = pWorld->GetWorldSystem<RenderWorldSystem>();
        pRenderWorldSystem->m_deviceRenderWorld.DispatchWorldUpdate( pCommandBuffer, frameIndex );
        pRenderWorldSystem->m_deviceRenderWorld.WaitForCopyTasks( m_pRenderSystem );

        if ( enableAsyncCompute )
        {
            // Wait for the PREVIOUS frame's shading to finish, allows to overlap with workload after previous frame shading
            RHI::QueueDeviceWait( m_pRenderSystem->GetComputeQueue(), m_pRenderSystem->GetGraphicsQueue(), m_signalSemaphores_shadingPass[( frameIndex + RHI::MaxPendingFrames - 1 ) % RHI::MaxPendingFrames] );

            // Wait for the previous world rendering to be completed when we have multiple viewports
            RHI::QueueDeviceWait( m_pRenderSystem->GetComputeQueue(), m_pRenderSystem->GetGraphicsQueue(), waitSemaphore );

            m_signalSemaphores_worldUpdate[frameIndex] = SubmitComputeCommandBuffer( eastl::move( pCommandBuffer ) );

            return m_signalSemaphores_worldUpdate[frameIndex];
        }

        return 0;
    }

    uint64_t ForwardShadingRenderer::DrawWorldToViewport( UpdateContext const& ctx, RenderViewport const* pRenderViewport, EntityWorld const* pWorld, uint64_t waitSemaphore )
    {
        EE_ASSERT( pRenderViewport != nullptr );
        EE_ASSERT( pWorld != nullptr );
        EE_ASSERT( pRenderViewport->IsValid() );

        EE_PROFILE_FUNCTION_RENDER();

        RenderWorldSystem const* pRenderWorldSystem = pWorld->GetWorldSystem<RenderWorldSystem>();

        //-------------------------------------------------------------------------

        uint32_t const frameIndex = m_pRenderSystem->GetFrameIndex();

        //-------------------------------------------------------------------------

        bool const enableAsyncCompute = m_pRenderGlobalSettings->m_enableAsyncCompute;

        bool const enableSMAA = m_pRenderGlobalSettings->m_enableSMAA;

        bool const enableSSAO = m_pRenderGlobalSettings->m_enableSSAO;
        bool const enableSSAOLowResolution = m_pRenderGlobalSettings->m_enableSSAOLowResolution;

        bool const enableDepthDownsample = ( enableSSAO && enableSSAOLowResolution );

        #if EE_DEVELOPMENT_TOOLS
        bool const enableEditorOutline = pRenderViewport->IsPickingEnabled();
        #else
        bool const enableEditorOutline = false;
        #endif

        //-------------------------------------------------------------------------

        DeviceRenderWorld const& deviceRenderWorld = pRenderWorldSystem->m_deviceRenderWorld;
        ActiveRenderViewList const& activeRenderViewList = pRenderViewport->m_activeRenderViewList;

        RHI::BufferHandle   renderViewBufferHandle = deviceRenderWorld.GetRenderViewBufferHandle();
        RHI::Buffer*        pGlobalParametersBuffer = pRenderViewport->m_globalParametersBuffers[frameIndex];

        //-------------------------------------------------------------------------

        Memory::WriteCombinedBarrier();

        RHI::CommandBuffer* pCommandBuffer_GeometryCulling = pRenderViewport->m_pWindow->GetActiveCommandBuffer( frameIndex );
        uint64_t signalSemaphore_GeometryCulling = 0;

        if ( enableAsyncCompute )
        {
            pCommandBuffer_GeometryCulling = pRenderViewport->m_pWindow->AcquireComputeCommandBuffer( m_pRenderSystem->GetContextRHI(), frameIndex );
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_GeometryCulling, "Clear Buffers" );

            #if EE_DEVELOPMENT_TOOLS
            m_renderPass_debugDraw.ClearBuffers( pCommandBuffer_GeometryCulling, frameIndex );
            #endif

            for ( uint32_t shaderIndex = 0; shaderIndex < pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstanceShaderPools(); ++shaderIndex )
            {
                RHI::CmdClearBuffer( pCommandBuffer_GeometryCulling, m_shaderCullingBuckets[shaderIndex].m_pCullingCounterBuffer, 0 );
                RHI::CmdClearBuffer( pCommandBuffer_GeometryCulling, m_shaderCullingBuckets[shaderIndex].m_pDrawClusterCountersBuffer, 0 );
                RHI::CmdClearBuffer( pCommandBuffer_GeometryCulling, m_shaderCullingBuckets[shaderIndex].m_pDrawClusterScatterOffsetsBuffer, 0 );
            }

            ForEachRenderBucket( activeRenderViewList, [pCommandBuffer_GeometryCulling] ( ActiveRenderViewMaterialShaderBucket const& renderBucket )
            {
                RHI::CmdClearBuffer( pCommandBuffer_GeometryCulling, renderBucket.m_pDrawCounterBuffer, 0 );
            } );

            RHI::CmdBarrier( pCommandBuffer_GeometryCulling, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::AllShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::ShaderResource );
        }

        //-------------------------------------------------------------------------

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_GeometryCulling, "Instance Culling" );

            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

            RHI::CmdSetPipeline( pCommandBuffer_GeometryCulling, m_pInstanceCullingShader->m_pPipeline );

            for ( uint32_t shaderIndex = 0; shaderIndex < pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstanceShaderPools(); ++shaderIndex )
            {
                uint32_t const instanceCapacity = pRenderWorldSystem->m_deviceRenderWorld.GetMeshInstanceCapacity( shaderIndex );
                EE_ASSERT( instanceCapacity > 0 );

                ShaderTypes::InstanceCullingRootConstants instanceCullingRootConstants = {};
                instanceCullingRootConstants.m_instanceCapacity = instanceCapacity;
                instanceCullingRootConstants.m_instanceBuffer = RHI::GetBufferHandle( pRenderWorldSystem->m_deviceRenderWorld.GetMeshInstanceBuffer( shaderIndex ), RHI::DescriptorTypeFlags::Buffer );
                instanceCullingRootConstants.m_instancePageBuffer = RHI::GetBufferHandle( pRenderWorldSystem->m_deviceRenderWorld.GetMeshInstancePageBuffer( shaderIndex, frameIndex ), RHI::DescriptorTypeFlags::Buffer );
                instanceCullingRootConstants.m_instanceVisibilityBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_instanceVisibilityBuffer.m_pBuffer, RHI::DescriptorTypeFlags::RWBuffer );

                uint32_t const numInstanceGroups = ( instanceCapacity + 127 ) / 128;
                uint32_t const numDispatchSplits = ( numInstanceGroups + RHI::Limits::MaxDispatchSize - 1 ) / RHI::Limits::MaxDispatchSize;
                for ( uint32_t dispatchIndex = 0; dispatchIndex < numDispatchSplits; ++dispatchIndex )
                {
                    uint32_t const groupOffset = dispatchIndex * RHI::Limits::MaxDispatchSize;

                    instanceCullingRootConstants.m_instanceOffset = groupOffset * 128;

                    RHI::CmdSetRootConstants( pCommandBuffer_GeometryCulling, 0, &instanceCullingRootConstants, sizeof( instanceCullingRootConstants ) );
                    RHI::CmdSetRootParameter( pCommandBuffer_GeometryCulling, 1, pGlobalParametersBuffer, 0 );
                    RHI::CmdDispatchCompute( pCommandBuffer_GeometryCulling, Math::Min( numInstanceGroups - groupOffset, uint32_t( RHI::Limits::MaxDispatchSize ) ), 1, 1 );
                }
            }
        }

        RHI::CmdBarrier( pCommandBuffer_GeometryCulling, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::ShaderResource );

        //-------------------------------------------------------------------------

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_GeometryCulling, "Instance Compaction" );

            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

            RHI::CmdSetPipeline( pCommandBuffer_GeometryCulling, m_pCullingCompactionShader->m_pPipeline );

            for ( uint32_t shaderIndex = 0; shaderIndex < pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstanceShaderPools(); ++shaderIndex )
            {
                uint32_t const instanceCapacity = pRenderWorldSystem->m_deviceRenderWorld.GetMeshInstanceCapacity( shaderIndex );
                EE_ASSERT( instanceCapacity > 0 );

                ShaderTypes::CullingCompactionRootConstants compactionRootConstants = {};
                compactionRootConstants.m_instanceCapacity = instanceCapacity;
                compactionRootConstants.m_instanceBuffer = RHI::GetBufferHandle( pRenderWorldSystem->m_deviceRenderWorld.GetMeshInstanceBuffer( shaderIndex ), RHI::DescriptorTypeFlags::Buffer );
                compactionRootConstants.m_instanceVisibilityBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_instanceVisibilityBuffer.m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
                compactionRootConstants.m_cullingWorkBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_clusterCullingWorkBuffer.m_pBuffer, RHI::DescriptorTypeFlags::RWBuffer );
                compactionRootConstants.m_workCounterBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_pCullingCounterBuffer, RHI::DescriptorTypeFlags::RWBuffer );

                uint32_t const numInstanceGroups = ( instanceCapacity + 127 ) / 128;
                uint32_t const numDispatches = ( numInstanceGroups + RHI::Limits::MaxDispatchSize - 1 ) / RHI::Limits::MaxDispatchSize;
                for ( uint32_t dispatchIndex = 0; dispatchIndex < numDispatches; ++dispatchIndex )
                {
                    uint32_t const groupOffset = dispatchIndex * RHI::Limits::MaxDispatchSize;

                    compactionRootConstants.m_instanceOffset = groupOffset * 128;

                    RHI::CmdSetRootConstants( pCommandBuffer_GeometryCulling, 0, &compactionRootConstants, sizeof( compactionRootConstants ) );
                    RHI::CmdDispatchCompute( pCommandBuffer_GeometryCulling, Math::Min( numInstanceGroups - groupOffset, uint32_t( RHI::Limits::MaxDispatchSize ) ), 1, 1 );
                }
            }

            RHI::CmdBarrier( pCommandBuffer_GeometryCulling, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::ShaderResource );
        }

        //-------------------------------------------------------------------------

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_GeometryCulling, "Culling Argument Generation" );

            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

            RHI::CmdSetPipeline( pCommandBuffer_GeometryCulling, m_pCullingArgumentGenerationShader->m_pPipeline );

            for ( uint32_t shaderIndex = 0; shaderIndex < pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstanceShaderPools(); ++shaderIndex )
            {
                uint32_t const clusterCapacity = pRenderWorldSystem->m_deviceRenderWorld.GetClusterCapacity( shaderIndex );
                EE_ASSERT( clusterCapacity > 0 );

                size_t const maxNumCullingArguments = size_t( clusterCapacity ) / ( size_t( RHI::MaxDispatchSize ) * 128 ) + 1;

                ShaderTypes::CullingArgumentGenerationRootConstants argumentGenerationRootConstants = {};
                argumentGenerationRootConstants.m_instanceBuffer = RHI::GetBufferHandle( pRenderWorldSystem->m_deviceRenderWorld.GetMeshInstanceBuffer( shaderIndex ), RHI::DescriptorTypeFlags::Buffer );
                argumentGenerationRootConstants.m_instanceVisibilityBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_instanceVisibilityBuffer.m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
                argumentGenerationRootConstants.m_cullingWorkBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_clusterCullingWorkBuffer.m_pBuffer, RHI::DescriptorTypeFlags::RWBuffer );
                argumentGenerationRootConstants.m_workCounterBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_pCullingCounterBuffer, RHI::DescriptorTypeFlags::Buffer );
                argumentGenerationRootConstants.m_clusterToInstanceBuffer = RHI::GetBufferHandle( pRenderWorldSystem->m_deviceRenderWorld.GetClusterToInstanceBuffer( shaderIndex ), RHI::DescriptorTypeFlags::Buffer );
                argumentGenerationRootConstants.m_cullingArgumentBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_cullingArgumentBuffer.m_pBuffer, RHI::DescriptorTypeFlags::RWBuffer );
                argumentGenerationRootConstants.m_drawCompactionArgumentBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_drawCompactionArgumentBuffer.m_pBuffer, RHI::DescriptorTypeFlags::RWBuffer );
                argumentGenerationRootConstants.m_drawClusterBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_drawClusterBuffer.m_pBuffer, RHI::DescriptorTypeFlags::RWBuffer );
                argumentGenerationRootConstants.m_drawClusterBaseOffsetsBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_pDrawClusterBaseOffsetsBuffer, RHI::DescriptorTypeFlags::Buffer );
                argumentGenerationRootConstants.m_drawClusterCountersBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_pDrawClusterCountersBuffer, RHI::DescriptorTypeFlags::RWBuffer );
                argumentGenerationRootConstants.m_drawClusterScatterOffsetsBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_pDrawClusterScatterOffsetsBuffer, RHI::DescriptorTypeFlags::RWBuffer );
                argumentGenerationRootConstants.m_shaderIndex = shaderIndex;
                argumentGenerationRootConstants.m_numArguments = uint32_t( maxNumCullingArguments );

                RHI::CmdSetRootConstants( pCommandBuffer_GeometryCulling, 0, &argumentGenerationRootConstants, sizeof( argumentGenerationRootConstants ) );
                RHI::CmdSetRootParameter( pCommandBuffer_GeometryCulling, 1, pGlobalParametersBuffer, 0 );
                RHI::CmdDispatchCompute( pCommandBuffer_GeometryCulling, uint32_t( ( maxNumCullingArguments + 63 ) / 64 ), 1, 1 );
            }

            RHI::CmdBarrier( pCommandBuffer_GeometryCulling, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::ShaderResource );
        }

        //-------------------------------------------------------------------------

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_GeometryCulling, "Cluster Culling" );

            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

            RHI::CmdSetPipeline( pCommandBuffer_GeometryCulling, m_pClusterCullingShader->m_pPipeline );

            for ( uint32_t shaderIndex = 0; shaderIndex < pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstanceShaderPools(); ++shaderIndex )
            {
                RHI::Buffer* const pCullingArgumentBuffer = m_shaderCullingBuckets[shaderIndex].m_cullingArgumentBuffer.m_pBuffer;
                RHI::CmdBarrier( pCommandBuffer_GeometryCulling, pCullingArgumentBuffer, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ExecuteIndirect, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::IndirectArgument );
            }

            for ( uint32_t shaderIndex = 0; shaderIndex < pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstanceShaderPools(); ++shaderIndex )
            {
                uint32_t const clusterCapacity = pRenderWorldSystem->m_deviceRenderWorld.GetClusterCapacity( shaderIndex );
                EE_ASSERT( clusterCapacity > 0 );

                size_t const maxNumCullingArguments = size_t( clusterCapacity ) / ( size_t( RHI::MaxDispatchSize ) * 128 ) + 1;

                RHI::CmdSetRootConstants( pCommandBuffer_GeometryCulling, 0, nullptr, sizeof( ShaderTypes::ClusterCullingRootConstants ) );
                RHI::CmdExecuteIndirect
                (
                    pCommandBuffer_GeometryCulling, m_pClusterCullingShader->m_pCommandSignature,
                    uint32_t( maxNumCullingArguments ),
                    m_shaderCullingBuckets[shaderIndex].m_cullingArgumentBuffer.m_pBuffer, 0,
                    nullptr, 0
                );
            }

            RHI::CmdBarrier( pCommandBuffer_GeometryCulling, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::ShaderResource );
        }

        //-------------------------------------------------------------------------

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_GeometryCulling, "Draw Argument Generation" );

            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

            RHI::CmdSetPipeline( pCommandBuffer_GeometryCulling, m_pDrawArgumentGenerationShader->m_pPipeline );

            for ( uint32_t shaderIndex = 0; shaderIndex < pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstanceShaderPools(); ++shaderIndex )
            {
                uint32_t const clusterCapacity = pRenderWorldSystem->m_deviceRenderWorld.GetClusterCapacity( shaderIndex );
                EE_ASSERT( clusterCapacity > 0 );

                ShaderTypes::DrawArgumentGenerationRootConstants drawArgumentGenerationRootConstants = {};
                drawArgumentGenerationRootConstants.m_shaderIndex = shaderIndex;
                drawArgumentGenerationRootConstants.m_drawClusterCountersBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_pDrawClusterCountersBuffer, RHI::DescriptorTypeFlags::Buffer );
                drawArgumentGenerationRootConstants.m_drawClusterBaseOffsetsBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_pDrawClusterBaseOffsetsBuffer, RHI::DescriptorTypeFlags::RWBuffer );
                drawArgumentGenerationRootConstants.m_drawClusterBuffer = RHI::GetBufferHandle( m_shaderCullingBuckets[shaderIndex].m_drawClusterBuffer.m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
                drawArgumentGenerationRootConstants.m_clusterToInstanceBuffer = RHI::GetBufferHandle( pRenderWorldSystem->m_deviceRenderWorld.GetClusterToInstanceBuffer( shaderIndex ), RHI::DescriptorTypeFlags::Buffer );
                drawArgumentGenerationRootConstants.m_instanceBuffer = RHI::GetBufferHandle( pRenderWorldSystem->m_deviceRenderWorld.GetMeshInstanceBuffer( shaderIndex ), RHI::DescriptorTypeFlags::Buffer );

                RHI::CmdSetRootConstants( pCommandBuffer_GeometryCulling, 0, &drawArgumentGenerationRootConstants, sizeof( drawArgumentGenerationRootConstants ) );
                RHI::CmdSetRootParameter( pCommandBuffer_GeometryCulling, 1, pGlobalParametersBuffer, 0 );
                RHI::CmdDispatchCompute( pCommandBuffer_GeometryCulling, 1, 1, 1 );
            }

            RHI::CmdBarrier( pCommandBuffer_GeometryCulling, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::ShaderResource );
        }

        //-------------------------------------------------------------------------

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_GeometryCulling, "Draw Compaction" );

            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

            RHI::CmdSetPipeline( pCommandBuffer_GeometryCulling, m_pDrawCompactionShader->m_pPipeline );

            for ( uint32_t shaderIndex = 0; shaderIndex < pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstanceShaderPools(); ++shaderIndex )
            {
                RHI::Buffer* const pDrawCompactionArgumentBuffer = m_shaderCullingBuckets[shaderIndex].m_drawCompactionArgumentBuffer.m_pBuffer;
                RHI::CmdBarrier( pCommandBuffer_GeometryCulling, pDrawCompactionArgumentBuffer, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ExecuteIndirect, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::IndirectArgument );
            }

            for ( uint32_t shaderIndex = 0; shaderIndex < pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstanceShaderPools(); ++shaderIndex )
            {
                uint32_t const clusterCapacity = pRenderWorldSystem->m_deviceRenderWorld.GetClusterCapacity( shaderIndex );
                EE_ASSERT( clusterCapacity > 0 );

                size_t const maxNumCullingArguments = size_t( clusterCapacity ) / ( size_t( RHI::MaxDispatchSize ) * 128 ) + 1;

                RHI::CmdSetRootConstants( pCommandBuffer_GeometryCulling, 0, nullptr, sizeof( ShaderTypes::DrawCompactionRootConstants ) );
                RHI::CmdExecuteIndirect
                (
                    pCommandBuffer_GeometryCulling, m_pDrawCompactionShader->m_pCommandSignature,
                    uint32_t( maxNumCullingArguments ),
                    m_shaderCullingBuckets[shaderIndex].m_drawCompactionArgumentBuffer.m_pBuffer, 0,
                    nullptr, 0
                );
            }

            RHI::CmdBarrier( pCommandBuffer_GeometryCulling, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::UnorderedAccess );
        }

        if ( enableAsyncCompute )
        {
            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

            // Wait for the PREVIOUS frame's shading to finish, allows to overlap with workload after previous frame shading
            RHI::QueueDeviceWait( m_pRenderSystem->GetComputeQueue(), m_pRenderSystem->GetGraphicsQueue(), m_signalSemaphores_shadingPass[( frameIndex + RHI::MaxPendingFrames - 1 ) % RHI::MaxPendingFrames] );

            // Wait for the previous world rendering to be completed when we have multiple viewports
            RHI::QueueDeviceWait( m_pRenderSystem->GetComputeQueue(), m_pRenderSystem->GetGraphicsQueue(), waitSemaphore );

            signalSemaphore_GeometryCulling = SubmitComputeCommandBuffer( eastl::move( pCommandBuffer_GeometryCulling ) );
        }

        // Spatial hash light culling - overlap with depth prepass
        //-------------------------------------------------------------------------

        RHI::CommandBuffer* pCommandBuffer_LightCulling = pRenderViewport->m_pWindow->GetActiveCommandBuffer( frameIndex );
        uint64_t signalSemaphore_LightCulling = 0;

        if ( enableAsyncCompute )
        {
            pCommandBuffer_LightCulling = pRenderViewport->m_pWindow->AcquireComputeCommandBuffer( m_pRenderSystem->GetContextRHI(), frameIndex );
        }

        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_LightCulling, "Light Culling" );

            m_lightCulling_spatialHash.Clear( pCommandBuffer_LightCulling, frameIndex );

            RHI::CmdBarrier( pCommandBuffer_LightCulling, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::UnorderedAccess );

            DeviceSpatialHashDispatchParameters dispatchParameters[DeviceSpatialHash::s_maxLODs] = {};
            DeviceSpatialHash::ComputeDispatchParameters
            (
                m_pRenderGlobalSettings->m_lightCullingInitialDispatchX,
                m_pRenderGlobalSettings->m_lightCullingInitialDispatchY,
                m_pRenderGlobalSettings->m_lightCullingInitialDispatchZ,
                m_lightCulling_spatialHash.m_numLODs,
                m_lightCulling_spatialHash.m_borderCells,
                dispatchParameters
            );

            for ( int lod = int( m_lightCulling_spatialHash.m_numLODs - 1 ); lod >= 0; --lod )
            {
                ShaderTypes::LightCulling_CullLightsRootConstants cullLightsRootConstants = {};
                cullLightsRootConstants.m_cellLevel = uint32_t( lod );
                cullLightsRootConstants.m_cellOffsetX = dispatchParameters[lod].m_dispatchOffset[0];
                cullLightsRootConstants.m_cellOffsetY = dispatchParameters[lod].m_dispatchOffset[1];
                cullLightsRootConstants.m_cellOffsetZ = dispatchParameters[lod].m_dispatchOffset[2];
                cullLightsRootConstants.m_numLODs = m_lightCulling_spatialHash.m_numLODs;

                RHI::CmdSetPipeline( pCommandBuffer_LightCulling, m_pLightCulling_cullLightsShader->m_pPipeline );
                RHI::CmdSetRootConstants( pCommandBuffer_LightCulling, 0, &cullLightsRootConstants, sizeof( cullLightsRootConstants ) );
                RHI::CmdSetRootParameter( pCommandBuffer_LightCulling, 1, pGlobalParametersBuffer, 0 );
                RHI::CmdDispatchCompute( pCommandBuffer_LightCulling, dispatchParameters[lod].m_dispatchSize[0], dispatchParameters[lod].m_dispatchSize[1], dispatchParameters[lod].m_dispatchSize[2] );

                if ( lod > 0 )
                {
                    RHI::CmdBarrier( pCommandBuffer_LightCulling, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::UnorderedAccess );
                }
            }

            RHI::CmdBarrier( pCommandBuffer_LightCulling, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::AllShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::UnorderedAccess );
        }

        if ( enableAsyncCompute )
        {
            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

            signalSemaphore_LightCulling = SubmitComputeCommandBuffer( eastl::move( pCommandBuffer_LightCulling ) );
        }


        // Wait for all cluster and culling dispatches
        //-------------------------------------------------------------------------

        RHI::CommandBuffer* pCommandBuffer_DepthPass = pRenderViewport->m_pWindow->GetActiveCommandBuffer( frameIndex );

        {
            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

            // Clear picking buffers
            #if EE_DEVELOPMENT_TOOLS
            if ( pRenderViewport->IsPickingEnabled() )
            {
                RHI::CmdClearBuffer( pCommandBuffer_DepthPass, pRenderViewport->m_instancePickingDistancesBuffer.m_pBuffer, eastl::bit_cast<uint32_t>( 1.5E+10F ) );

                pRenderViewport->m_instancePickingResultsBuffer.Clear( pCommandBuffer_DepthPass, frameIndex );
            }
            #endif

            ForEachRenderBucket( activeRenderViewList, [pCommandBuffer_DepthPass] ( ActiveRenderViewMaterialShaderBucket const& renderBucket )
            {
                RHI::CmdBarrier( pCommandBuffer_DepthPass, renderBucket.m_pDrawCounterBuffer, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ExecuteIndirect, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::IndirectArgument );
                RHI::CmdBarrier( pCommandBuffer_DepthPass, renderBucket.m_drawArgumentBuffer.m_pBuffer, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ExecuteIndirect, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::IndirectArgument );
            } );
            RHI::CmdBarrier( pCommandBuffer_DepthPass, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::AllShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::ShaderResource );
        }

        // Global environment map capture
        //-------------------------------------------------------------------------

        bool const captureGlobalEnvironmentMap = activeRenderViewList.IsActive( pRenderViewport->m_globalEnvironmentMapRenderViewProxy.GetBaseRenderViewIndex() );
        if ( captureGlobalEnvironmentMap )
        {
            m_renderPass_globalEnvironmentMap.CaptureGlobalEnvironmentMap
            (
                m_materialShaderPipelineBuckets,
                activeRenderViewList,
                deviceRenderWorld,
                pRenderViewport,
                pGlobalParametersBuffer,
                pCommandBuffer_DepthPass
            );
        }

        // Forward shading depth pass
        //-------------------------------------------------------------------------

        m_renderPass_forwardShading.DepthOnlyPass
        (
            m_materialShaderPipelineBuckets,
            activeRenderViewList.GetActiveRenderView( activeRenderViewList.m_mainRenderViewActiveIndex ),
            pRenderViewport,
            m_resourceStates,
            pCommandBuffer_DepthPass
        );

        if ( enableDepthDownsample )
        {
            m_renderPass_depthDownsample.DrawToViewport( pRenderViewport, m_resourceStates, pCommandBuffer_DepthPass, pRenderViewport->m_forwardShading_depthTexture );
        }

        #if EE_DEVELOPMENT_TOOLS
        if ( enableEditorOutline )
        {
            m_renderPass_editorOutline.DrawToViewport
            (
                m_materialShaderPipelineBuckets,
                activeRenderViewList.GetActiveRenderView( activeRenderViewList.m_editorOutlineRenderViewActiveIndex ),
                pRenderViewport,
                m_resourceStates,
                pCommandBuffer_DepthPass
            );

            m_renderPass_debugDraw.DrawOutlineToViewport
            (
                pRenderViewport,
                renderViewBufferHandle,
                pRenderViewport->m_mainRenderViewProxy.GetBaseRenderViewIndex(),
                pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstanceRootPages() * 64,
                m_resourceStates,
                pCommandBuffer_DepthPass,
                frameIndex
            );
        }
        #endif

        uint64_t signalSemaphore_DepthPass = 0;
        if ( enableAsyncCompute )
        {
            if ( enableSSAO )
            {
                EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

                if ( enableSSAOLowResolution )
                {
                    m_resourceStates.ReadOnly( pRenderViewport->m_depthDownsample4, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
                }
                else
                {
                    m_resourceStates.ReadOnly( pRenderViewport->m_forwardShading_depthTexture, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
                }

                m_resourceStates.Writeable( pRenderViewport->m_GTAO_prefilterDepthTexture, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::TextureState::UnorderedAccess );
                m_resourceStates.Writeable( pRenderViewport->m_GTAO_resultTextureNoisy0, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::TextureState::UnorderedAccess );
                m_resourceStates.Writeable( pRenderViewport->m_GTAO_resultTextureNoisy1, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::TextureState::UnorderedAccess );
                m_resourceStates.Writeable( pRenderViewport->m_GTAO_resultTexture, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::TextureState::UnorderedAccess );
                m_resourceStates.FlushBarriers( pCommandBuffer_DepthPass );
            }

            RHI::QueueDeviceWait( m_pRenderSystem->GetGraphicsQueue(), m_pRenderSystem->GetComputeQueue(), m_signalSemaphores_worldUpdate[frameIndex] );
            RHI::QueueDeviceWait( m_pRenderSystem->GetGraphicsQueue(), m_pRenderSystem->GetComputeQueue(), signalSemaphore_GeometryCulling );
            signalSemaphore_DepthPass = SubmitGraphicsCommandBuffer( eastl::move( pCommandBuffer_DepthPass ) );
        }

        // GTAO + environment map filtering.
        //-------------------------------------------------------------------------

        RHI::CommandBuffer* pCommandBuffer_GTAO = pCommandBuffer_DepthPass;
        uint64_t signalSemaphore_GTAO = 0;

        if ( enableAsyncCompute )
        {
            pCommandBuffer_GTAO = pRenderViewport->m_pWindow->AcquireComputeCommandBuffer( m_pRenderSystem->GetContextRHI(), frameIndex );
        }

        if ( captureGlobalEnvironmentMap )
        {
            m_renderPass_globalEnvironmentMap.FilterEnvironmentMap
            (
                deviceRenderWorld,
                pRenderViewport,
                pCommandBuffer_GTAO,
                m_pProbeTableBuffer
            );
        }

        if ( enableSSAO )
        {
            EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer_GTAO, "XeGTAO" );

            m_renderPass_GTAO.PrefilterDepth( pRenderViewport, m_resourceStates, pCommandBuffer_GTAO, frameIndex );
            m_renderPass_GTAO.ComputeNoisyResult( pRenderViewport, m_resourceStates, pCommandBuffer_GTAO, renderViewBufferHandle, pRenderViewport->m_mainRenderViewProxy.GetBaseRenderViewIndex(), frameIndex );
            m_renderPass_GTAO.Denoise( pRenderViewport, m_resourceStates, pCommandBuffer_GTAO, enableAsyncCompute, frameIndex );
        }

        if ( enableAsyncCompute )
        {
            if ( enableSSAO )
            {
                EE_ASSERT( !m_resourceStates.HasPendingBarriers() );
                if ( enableSSAOLowResolution )
                {
                    m_resourceStates.ReadOnly( pRenderViewport->m_GTAO_resultTextureNoisy1, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
                }
                else
                {
                    m_resourceStates.ReadOnly( pRenderViewport->m_GTAO_resultTexture, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
                }
                m_resourceStates.FlushBarriers( pCommandBuffer_GTAO );
            }

            RHI::QueueDeviceWait( m_pRenderSystem->GetComputeQueue(), m_pRenderSystem->GetGraphicsQueue(), signalSemaphore_DepthPass );
            signalSemaphore_GTAO = SubmitComputeCommandBuffer( eastl::move( pCommandBuffer_GTAO ) );
        }

        // Shadow maps
        //-------------------------------------------------------------------------

        RHI::CommandBuffer* pCommandBuffer_CascadedShadows = pCommandBuffer_DepthPass;
        uint64_t signalSemaphore_CascadedShadows = 0;

        if ( enableAsyncCompute )
        {
            pCommandBuffer_CascadedShadows = pRenderViewport->m_pWindow->AcquireGraphicsCommandBuffer( m_pRenderSystem->GetContextRHI(), frameIndex );
        }

        m_renderPass_cascadedShadows.BarrierWriteable( deviceRenderWorld, activeRenderViewList, m_resourceStates );
        m_renderPass_punctualShadows.BarrierWriteable( deviceRenderWorld, activeRenderViewList, m_resourceStates );
        m_resourceStates.FlushBarriers( pCommandBuffer_CascadedShadows );

        m_renderPass_cascadedShadows.DrawShadowCascades
        (
            m_materialShaderPipelineBuckets,
            activeRenderViewList,
            deviceRenderWorld,
            m_resourceStates,
            pCommandBuffer_CascadedShadows
        );

        m_renderPass_punctualShadows.DrawPunctualShadows
        (
            m_materialShaderPipelineBuckets,
            activeRenderViewList,
            deviceRenderWorld,
            m_resourceStates,
            pCommandBuffer_CascadedShadows
        );

        if ( enableAsyncCompute )
        {
            signalSemaphore_CascadedShadows = SubmitGraphicsCommandBuffer( eastl::move( pCommandBuffer_CascadedShadows ) );
        }

        // Forward shading pass
        //-------------------------------------------------------------------------

        RHI::CommandBuffer* pCommandBuffer_ShadingPass = pCommandBuffer_DepthPass;
        if ( enableAsyncCompute )
        {
            pCommandBuffer_ShadingPass = pRenderViewport->m_pWindow->AcquireGraphicsCommandBuffer( m_pRenderSystem->GetContextRHI(), frameIndex );
        }

        if ( enableSSAO && enableSSAOLowResolution )
        {
            m_renderPass_GTAO.Upsample( pRenderViewport, m_resourceStates, pCommandBuffer_ShadingPass, frameIndex );
        }

        // Barriers
        //-------------------------------------------------------------------------
        EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

        if ( enableSSAO )
        {
            m_resourceStates.ReadOnly( pRenderViewport->m_GTAO_resultTexture, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
        }

        m_renderPass_cascadedShadows.BarrierReadOnly( deviceRenderWorld, activeRenderViewList, m_resourceStates );
        m_renderPass_punctualShadows.BarrierReadOnly( deviceRenderWorld, activeRenderViewList, m_resourceStates );
        m_resourceStates.FlushBarriers( pCommandBuffer_ShadingPass );

        m_renderPass_forwardShading.ShadingPass
        (
            m_materialShaderPipelineBuckets,
            activeRenderViewList.GetActiveRenderView( activeRenderViewList.m_mainRenderViewActiveIndex ),
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
            RHI::CmdDispatchCompute( pCommandBuffer_ShadingPass, pRenderWorldSystem->m_deviceRenderWorld.GetNumMeshInstanceRootPages(), 1, 1 );

            pRenderViewport->m_instancePickingResultsBuffer.CopyResults( pCommandBuffer_ShadingPass, frameIndex );
            pRenderViewport->m_instancePickingResultsBuffer.Barrier( pCommandBuffer_ShadingPass, frameIndex );
        }
        #endif

        if ( enableAsyncCompute )
        {
            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

            RHI::CmdSetRenderTargets( pCommandBuffer_ShadingPass, {}, nullptr );

            RHI::QueueDeviceWait( m_pRenderSystem->GetGraphicsQueue(), m_pRenderSystem->GetComputeQueue(), signalSemaphore_LightCulling );
            RHI::QueueDeviceWait( m_pRenderSystem->GetGraphicsQueue(), m_pRenderSystem->GetComputeQueue(), signalSemaphore_GTAO );

            m_signalSemaphores_shadingPass[frameIndex] = SubmitGraphicsCommandBuffer( eastl::move( pCommandBuffer_ShadingPass ) );
        }

        // Post processing
        //-------------------------------------------------------------------------

        RHI::CommandBuffer* pCommandBuffer_PostProcessing = pCommandBuffer_ShadingPass;

        if ( enableAsyncCompute )
        {
            EE_ASSERT( !m_resourceStates.HasPendingBarriers() );

            pCommandBuffer_PostProcessing = pRenderViewport->m_pWindow->AcquireGraphicsCommandBuffer( m_pRenderSystem->GetContextRHI(), frameIndex );

            Float2 const viewportSize = pRenderViewport->GetSize();
            RHI::CmdSetViewport( pCommandBuffer_PostProcessing, 0.0F, 0.0F, viewportSize.m_x, viewportSize.m_y, 0.0F, 1.0F );
            RHI::CmdSetScissor( pCommandBuffer_PostProcessing, 0, 0, uint32_t( viewportSize.m_x ), uint32_t( viewportSize.m_y ) );
        }

        if ( enableSMAA )
        {
            m_renderPass_SMAA.DrawToViewport
            (
                pRenderViewport,
                m_resourceStates,
                pCommandBuffer_PostProcessing,
                m_pRenderSystem->GetSMAAAreaTexture(),
                m_pRenderSystem->GetSMAASearchTexture()
            );
        }

        RHI::LoadAction postProcessLoadAction = {};
        postProcessLoadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;
        postProcessLoadAction.m_colorClearValues[0] = pRenderViewport->m_finalTexture->m_clearValue;

        EE_ASSERT( !m_resourceStates.HasPendingBarriers() );
        m_resourceStates.Writeable( pRenderViewport->m_finalTexture, RHI::PipelineStage::Draw, RHI::ResourceAccess::RenderTarget, RHI::TextureState::RenderTarget );
        m_resourceStates.FlushBarriers( pCommandBuffer_PostProcessing );

        RHI::CmdSetRenderTargets( pCommandBuffer_PostProcessing, { &pRenderViewport->m_finalTexture.m_pTexture, 1 }, nullptr, &postProcessLoadAction );

        m_renderPass_postProcess.DrawToViewport
        (
            m_resourceStates,
            pCommandBuffer_PostProcessing,
            m_pRenderSystem->GetTonemapLUT(),
            enableSMAA ? pRenderViewport->m_SMAA_resultTexture : pRenderViewport->m_forwardShading_colorTexture,
            pRenderViewport->m_forwardShading_depthTexture
        );

        // Debug draw and outlines
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_renderPass_debugDraw.DrawToViewport
        (
            pRenderViewport,
            pRenderWorldSystem->m_deviceRenderWorld,
            pRenderViewport->m_finalTexture,
            renderViewBufferHandle,
            pRenderViewport->m_mainRenderViewProxy.GetBaseRenderViewIndex(),
            m_resourceStates,
            pCommandBuffer_PostProcessing,
            frameIndex
        );

        if ( enableEditorOutline )
        {
            m_renderPass_editorOutline.ResolveToViewport( pRenderViewport, m_resourceStates, pCommandBuffer_PostProcessing );
        }

        if ( !pRenderViewport->IsStandalone() )
        {
            m_resourceStates.ReadOnly( pRenderViewport->m_finalTexture, RHI::PipelineStage::PixelShader, RHI::ResourceAccess::ShaderResource, RHI::TextureState::ShaderResource );
        }
        #endif

        // End frame
        //-------------------------------------------------------------------------

        RHI::CmdSetRenderTargets( pCommandBuffer_PostProcessing, {}, nullptr );
        m_resourceStates.FlushBarriers( pCommandBuffer_PostProcessing );

        if ( enableAsyncCompute )
        {
            SubmitGraphicsCommandBuffer( eastl::move( pCommandBuffer_PostProcessing ) );

            pRenderViewport->m_pWindow->AcquireGraphicsCommandBuffer( m_pRenderSystem->GetContextRHI(), frameIndex );

            return m_signalSemaphores_shadingPass[frameIndex];
        }

        return 0;
    }

    uint64_t ForwardShadingRenderer::SubmitGraphicsCommandBuffer( RHI::CommandBuffer*&& pCommandBuffer )
    {
        EE_PROFILE_FUNCTION_RENDER();

        //-------------------------------------------------------------------------

        RHI::EndCommandBuffer( pCommandBuffer );

        uint64_t semaphore = RHI::QueueSubmit( m_pRenderSystem->GetContextRHI(), m_pRenderSystem->GetGraphicsQueue(), { &pCommandBuffer, 1 } );

        pCommandBuffer = nullptr;

        return semaphore;
    }

    uint64_t ForwardShadingRenderer::SubmitComputeCommandBuffer( RHI::CommandBuffer*&& pCommandBuffer )
    {
        EE_PROFILE_FUNCTION_RENDER();

        //-------------------------------------------------------------------------

        RHI::EndCommandBuffer( pCommandBuffer );

        uint64_t semaphore = RHI::QueueSubmit( m_pRenderSystem->GetContextRHI(), m_pRenderSystem->GetComputeQueue(), { &pCommandBuffer, 1 } );

        pCommandBuffer = nullptr;

        return semaphore;
    }
}
