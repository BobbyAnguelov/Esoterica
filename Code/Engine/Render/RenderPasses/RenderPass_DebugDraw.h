#pragma once

#include "Base/Render/RHI.h"
#include "Base/Drawing/DebugDrawingSystem.h"
#include "Engine/Render/Device/DeviceAppendBuffer.h"
#include "Engine/Render/Shaders/EngineShader.h"
#include "Engine/Render/RenderPasses/RenderPass.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    class RenderSystem;
    class RenderViewport;
    class RenderSettings;
    class DebugMeshRegistry;
    class DeviceRenderWorld;

    //-------------------------------------------------------------------------

    struct DebugDrawRenderPass final
    {
    public:

        ~DebugDrawRenderPass() { EE_ASSERT( m_pDebugMeshRegistry == nullptr ); }

        //---------------------------------------------------------------------

        void Initialize( RenderPassContext const& context );
        void Shutdown( RenderSystem* pRenderSystem );

        void UpdateDeviceResources( RenderSystem* pRenderSystem );
        void UpdateViewportDeviceResources( RenderSystem* pRenderSystem,
                                            DeviceRenderWorld const& deviceRenderWorld,
                                            DebugDrawSystem* pDebugDrawingSystem, Seconds const deltaTime,
                                            RenderViewport* pRenderViewport );

        void ClearBuffers( DeviceResourceStates& resourceStates, RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex );

        void DrawToViewport( RenderViewport const*          pRenderViewport,
                             DeviceRenderWorld const&       deviceRenderWorld,
                             DeviceTextureState&            finalRenderTarget,
                             RHI::BufferHandle              clusterBuffer,
                             RHI::BufferHandle              renderViewBuffer,
                             uint32_t                       mainCameraViewIndex,
                             DeviceResourceStates&          resourceStates,
                             RHI::CommandBuffer*            pCommandBuffer,
                             uint32_t                       frameIndex );

        template <typename F>
        inline void ForEachBucket( F fn )
        {
            for ( DepthTestBucket& bucket : m_depthBuckets )
            {
                fn( bucket.m_argumentBuffer, bucket.m_commandsBuffer );
            }
        }

        inline void SetDebugMeshRegistry( DebugMeshRegistry* pDebugMeshRegistry )
        {
            EE_ASSERT( !m_pDebugMeshRegistry );
            m_pDebugMeshRegistry = pDebugMeshRegistry;
        }

        inline void ClearDebugMeshRegistry()
        {
            m_pDebugMeshRegistry = nullptr;
        }

    public:

        //---------------------------------------------------------------------

        enum DepthTestBucketID
        {
            DEPTH_TEST_ON_WRITE,
            DEPTH_TEST_ON,
            DEPTH_TEST_SEPARATE_WRITE,
        };

        static constexpr uint32_t NUM_DEPTH_TEST_BUCKETS = 3;

        struct DepthTestBucket
        {
            uint32_t                                    m_pickingSortPriority = 0;
            DeviceBufferState                           m_argumentBuffer = {};
            DeviceAppendBuffer<void>                    m_commandsBuffer = {};
        };

    private:

        //---------------------------------------------------------------------

        RenderSettings const*                           m_pRenderSettings = nullptr;
        DebugMeshRegistry*                              m_pDebugMeshRegistry = nullptr;

        SurfaceShader const*                            m_pDebugDrawShader = nullptr;
        SurfaceShader const*                            m_pDebugDrawMeshShader = nullptr;
        ComputeShader const*                            m_pDebugDrawResolveShader = nullptr;

        DebugDrawInternal::FrameCommandBuffer           m_frameCommandBuffer;

        RHI::Pipeline*                                  m_pTransparentDepthOnDepthPipeline = nullptr;
        RHI::Pipeline*                                  m_pTransparentDepthOnColorPipeline = nullptr;
        RHI::Pipeline*                                  m_pTransparentDepthOnNoWriteColorPipeline = nullptr;
        RHI::Pipeline*                                  m_pTransparentDepthOffPipeline = nullptr;

        RHI::Pipeline*                                  m_pTransparentDepthOnDepthPipeline_Mesh = nullptr;
        RHI::Pipeline*                                  m_pTransparentDepthOnColorPipeline_Mesh = nullptr;
        RHI::Pipeline*                                  m_pTransparentDepthOnNoWriteColorPipeline_Mesh = nullptr;
        RHI::Pipeline*                                  m_pTransparentDepthOffPipeline_Mesh = nullptr;

        TArray<RHI::Texture*, 2>                        m_fontCacheTextures = {};
        TArray<TVector<uint8_t>, 2>                     m_fontCaches = {};

        Float2                                          m_fontPixelSize = Float2::Zero;

        TArray<DepthTestBucket, NUM_DEPTH_TEST_BUCKETS> m_depthBuckets = {};
    };
}
#endif
