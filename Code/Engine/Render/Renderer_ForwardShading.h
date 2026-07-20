
#pragma once

#include "Engine/Render/Device/DeviceResizeBuffer.h"
#include "Engine/Render/RenderPasses/RenderPass.h"
#include "Engine/Render/RenderPasses/RenderPass_GlobalEnvironmentMap.h"
#include "Engine/Render/RenderPasses/RenderPass_SMAA.h"
#include "Engine/Render/RenderPasses/RenderPass_GTAO.h"
#include "Engine/Render/RenderPasses/RenderPass_PostProcess.h"
#include "Engine/Render/RenderPasses/RenderPass_ForwardShading.h"
#include "Engine/Render/RenderPasses/RenderPass_CascadedShadow.h"
#include "Engine/Render/RenderPasses/RenderPass_DepthDownsample.h"
#include "Engine/Render/RenderPasses/RenderPass_DebugDraw.h"

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

    //-------------------------------------------------------------------------------------------------------

    class ForwardShadingRenderer final
    {
    public:

        void Initialize( SystemRegistry* pSystemRegistry, RenderSettings const& renderSettings );
        void Shutdown();

        void UpdateDeviceResources( UpdateContext const& updateContext );
        void UpdateViewportDeviceResources( UpdateContext const& updateContext, RenderViewport* pRenderViewport, EntityWorld* pWorld );
        void UpdateWorldDeviceResources( UpdateContext const& updateContext, EntityWorld* pWorld );

        void DispatchWorld( UpdateContext const& updateContext, RenderViewport const* pRenderViewport, EntityWorld* pWorld );
        void DrawWorldToViewport( UpdateContext const& updateContext, RenderViewport const* pRenderViewport, EntityWorld const* pWorld );

    private:

        //---------------------------------------------------------------------------------------------------
        uint64_t SubmitGraphicsCommandBuffer( RHI::CommandBuffer*&& pCommandBuffer );
        uint64_t SubmitComputeCommandBuffer( RHI::CommandBuffer*&& pCommandBuffer );

    private:

        //---------------------------------------------------------------------------------------------------

        template <typename F>
        void ForEachRenderBucket( uint32_t numCascadedShadowPasses, F fn );

        template <typename F>
        void ForEachRenderPass( F fn );

    private:

        //---------------------------------------------------------------------------------------------------

        RenderSystem*                                                       m_pRenderSystem = nullptr;

        RenderSettings const*                                               m_pRenderGlobalSettings = nullptr;

        TVector<ForwardShadingMaterialShaderPipelineBucket>                 m_materialShaderPipelineBuckets;

        ComputeShader const*                                                m_pInstanceCullingShader = nullptr;
        ComputeShader const*                                                m_pClusterCullingShader = nullptr;
        ComputeShader const*                                                m_pLightCullingShader = nullptr;
        ComputeShader const*                                                m_pBucketResolveShader = nullptr;

        DeviceBufferState                                                   m_instanceCulling_CounterBuffer = {};
        DeviceResizeBufferState                                             m_clusterCulling_ClusterBuffer = {};

        DeviceBufferState                                                   m_clusterCulling_CounterBuffer = {};
        DeviceResizeBufferState                                             m_clusterCulling_ArgumentBuffer = {};

        TVector<CascadedShadowPass>                                         m_renderPass_CascadedShadows;
        ForwardShadingPass                                                  m_renderPass_ForwardShading;
        GlobalEnvironmentMapPass                                            m_renderPass_GlobalEnvironmentMap;
        SMAAPass                                                            m_renderPass_SMAA;
        GTAOPass                                                            m_renderPass_GTAO;
        DepthDownsamplePass                                                 m_renderPass_DepthDownsample;
        PostProcessPass                                                     m_renderPass_PostProcess;

        DeviceResourceStates                                                m_resourceStates;

        #if EE_DEVELOPMENT_TOOLS
        ComputeShader const*                                                m_pInstancePickingResolveShader = nullptr;

        DebugDrawRenderPass                                                 m_debugDrawPass;
        #endif
    };
}
