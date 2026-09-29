
#pragma once

#include "Engine/Render/ActiveRenderView.h"
#include "Engine/Render/Device/DeviceResizeBuffer.h"
#include "Engine/Render/Device/SpatialHash.h"
#include "Engine/Render/RenderPasses/RenderPass.h"
#include "Engine/Render/RenderPasses/RenderPass_GlobalEnvironmentMap.h"
#include "Engine/Render/RenderPasses/RenderPass_SMAA.h"
#include "Engine/Render/RenderPasses/RenderPass_GTAO.h"
#include "Engine/Render/RenderPasses/RenderPass_PostProcess.h"
#include "Engine/Render/RenderPasses/RenderPass_ForwardShading.h"
#include "Engine/Render/RenderPasses/RenderPass_CascadedShadow.h"
#include "Engine/Render/RenderPasses/RenderPass_PunctualShadow.h"
#include "Engine/Render/RenderPasses/RenderPass_DepthDownsample.h"
#include "Engine/Render/RenderPasses/RenderPass_DebugDraw.h"
#include "Engine/Render/RenderPasses/RenderPass_EditorOutline.h"

namespace EE
{
    class UpdateContext;
    class EntityWorld;
    class SystemRegistry;
}

namespace EE::Render
{
    class Window;
    class RenderSystem;
    class RenderWorldSystem;
    class RenderWorldSettings;
    class RenderSettings;

    //-------------------------------------------------------------------------

    struct ShaderCullingBucket
    {
        void Initialize( RHI::Context* pContextRHI, char const* pShaderName );
        void Shutdown( RHI::Context* pContextRHI );

        //-------------------------------------------------------------------------

        DeviceResizeBuffer                                                  m_instanceVisibilityBuffer = {};
        DeviceResizeBuffer                                                  m_clusterCullingWorkBuffer = {};
        DeviceResizeBuffer                                                  m_cullingArgumentBuffer = {};
        DeviceResizeBuffer                                                  m_drawCompactionArgumentBuffer = {};
        DeviceResizeBuffer                                                  m_drawClusterBuffer = {};
        RHI::Buffer*                                                        m_pCullingCounterBuffer = nullptr;
        RHI::Buffer*                                                        m_pDrawClusterCountersBuffer = nullptr;
        RHI::Buffer*                                                        m_pDrawClusterScatterOffsetsBuffer = nullptr;
        RHI::Buffer*                                                        m_pDrawClusterBaseOffsetsBuffer = nullptr;
    };

    //-------------------------------------------------------------------------

    class ForwardShadingRenderer final
    {
    public:

        void Initialize( SystemRegistry* pSystemRegistry, RenderSettings const& renderSettings );
        void Shutdown();

        void UpdateDeviceResources( UpdateContext const& updateContext );
        void UpdateViewportDeviceResources( UpdateContext const& updateContext, RenderViewport* pRenderViewport, EntityWorld* pWorld );
        void UpdateWorldDeviceResources( UpdateContext const& updateContext, EntityWorld* pWorld );

        uint64_t DispatchWorld( UpdateContext const& updateContext, RenderViewport const* pRenderViewport, EntityWorld* pWorld, uint64_t waitSemaphore );
        uint64_t DrawWorldToViewport( UpdateContext const& updateContext, RenderViewport const* pRenderViewport, EntityWorld const* pWorld, uint64_t waitSemaphore );

    private:

        //-------------------------------------------------------------------------

        uint64_t SubmitGraphicsCommandBuffer( RHI::CommandBuffer*&& pCommandBuffer );
        uint64_t SubmitComputeCommandBuffer( RHI::CommandBuffer*&& pCommandBuffer );

        template <typename F>
        void ForEachRenderBucket( ActiveRenderViewList const& activeRenderViewList, F fn );

        template <typename F>
        void ForEachRenderPass( F fn );

        //-------------------------------------------------------------------------

        RenderSystem*                                                       m_pRenderSystem = nullptr;

        RenderSettings const*                                               m_pRenderGlobalSettings = nullptr;

        RHI::Buffer*                                                        m_pProbeTableBuffer = nullptr;
        RHI::Texture*                                                       m_pDFGTexture = nullptr;

        TVector<ForwardShadingMaterialShaderPipelineBucket>                 m_materialShaderPipelineBuckets;
        TVector<ShaderCullingBucket>                                        m_shaderCullingBuckets;

        //-------------------------------------------------------------------------

        ComputeShader const*                                                m_pInstanceCullingShader = nullptr;
        ComputeShader const*                                                m_pCullingCompactionShader = nullptr;
        ComputeShader const*                                                m_pCullingArgumentGenerationShader = nullptr;
        ComputeShader const*                                                m_pDrawCompactionShader = nullptr;
        ComputeShader const*                                                m_pClusterCullingShader = nullptr;
        ComputeShader const*                                                m_pDrawArgumentGenerationShader = nullptr;
        ComputeShader const*                                                m_pLightCulling_cullLightsShader = nullptr;

        DeviceSpatialHash                                                   m_lightCulling_spatialHash;

        //-------------------------------------------------------------------------

        CascadedShadowPass                                                  m_renderPass_cascadedShadows;
        PunctualShadowPass                                                  m_renderPass_punctualShadows;
        ForwardShadingPass                                                  m_renderPass_forwardShading;
        GlobalEnvironmentMapPass                                            m_renderPass_globalEnvironmentMap;
        SMAAPass                                                            m_renderPass_SMAA;
        GTAOPass                                                            m_renderPass_GTAO;
        DepthDownsamplePass                                                 m_renderPass_depthDownsample;
        PostProcessPass                                                     m_renderPass_postProcess;

        //-------------------------------------------------------------------------

        DeviceResourceStates                                                m_resourceStates;

        TArray<uint64_t, RHI::MaxPendingFrames>                             m_signalSemaphores_worldUpdate = {};
        TArray<uint64_t, RHI::MaxPendingFrames>                             m_signalSemaphores_shadingPass = {};

        #if EE_DEVELOPMENT_TOOLS
        ComputeShader const*                                                m_pInstancePickingResolveShader = nullptr;

        DebugDrawRenderPass                                                 m_renderPass_debugDraw;
        EditorOutlineRenderPass                                             m_renderPass_editorOutline;
        #endif
    };
}
