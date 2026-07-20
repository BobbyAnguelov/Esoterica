#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Viewport/Viewport.h"
#include "Engine/Viewport/ViewportPicking.h"
#include "Engine/Render/Device/DeviceResourceState.h"
#include "Engine/Render/Device/DeviceAppendBuffer.h"
#include "Engine/Render/Device/DeviceResizeBuffer.h"
#include "Base/Render/RHI.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Window;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API RenderViewport final : public Viewport
    {

    public:

        virtual bool IsValid() const override;

        void Initialize( RHI::Context* pContextRHI, Render::Window* pWindow );
        void Shutdown( RHI::Context* pContextRHI );

        void UpdateRenderWindow( Render::Window* pWindow );

        inline bool IsStandalone() const
        {
            #if EE_DEVELOPMENT_TOOLS
            return m_isStandalone;
            #else
            return true;
            #endif
        }

        #if EE_DEVELOPMENT_TOOLS
        inline void SetPickingEnabled( bool enabled ) { m_isPickingEnabled = enabled; }
        inline bool IsPickingEnabled() const { return m_isPickingEnabled; }
        #endif

        inline bool TextureNeedsResize( RHI::Texture* pTexture ) const
        {
            if ( !pTexture || pTexture->m_width != uint32_t( m_size.m_x ) || pTexture->m_height != uint32_t( m_size.m_y ) )
            {
                return true;
            }
            return false;
        }

    public:

        Render::Window*                                     m_pWindow = nullptr;

        // TODO: Bunch of mutable stuff here, we don't have/need multithreaded command buffer recording right now so it's a later problem.
        // Renderer is recording very small command buffers so it's not a performance issue, all culling work is done on the GPU.
        mutable DeviceTextureState                          m_ForwardShading_DepthTexture = {};
        mutable DeviceTextureState                          m_ForwardShading_ColorTexture = {};

        mutable DeviceTextureState                          m_DepthDownsample2 = {};
        mutable DeviceTextureState                          m_DepthDownsample4 = {};
        mutable DeviceTextureState                          m_DepthDownsample8 = {};

        mutable DeviceTextureState                          m_SMAA_StencilTexture = {};
        mutable DeviceTextureState                          m_SMAA_EdgesTexture = {};
        mutable DeviceTextureState                          m_SMAA_BlendTexture = {};
        mutable DeviceTextureState                          m_SMAA_ResultTexture = {};

        mutable DeviceTextureState                          m_GTAO_ResultTextureNoisy0 = {};
        mutable DeviceTextureState                          m_GTAO_ResultTextureNoisy1 = {};

        mutable DeviceTextureState                          m_GTAO_ResultTextureHalfResolution = {};
        mutable DeviceTextureState                          m_GTAO_ResultTexture = {};
        mutable DeviceTextureState                          m_GTAO_EdgesTexture = {};
        mutable DeviceTextureState                          m_GTAO_PrefilterDepthTexture = {};

        mutable DeviceTextureState                          m_LightCulling_BucketTexture = {};

        mutable DeviceTextureState                          m_finalTexture = {};

        #if EE_DEVELOPMENT_TOOLS
        mutable DeviceTextureState                          m_DebugDraw_DepthTexture = {};
        #endif

        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_GTAO_parametersBuffers = {};
        uint32_t                                            m_GTAO_noiseIndex = 0;

        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_globalParametersBuffers = {};
        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_renderViewBuffers = {};
        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_renderBucketBuffers = {};
        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_cascadedShadowBuffers = {};

        uint32_t                                            m_numGlobalEnvironmentMapRenderViews = 0;
        uint32_t                                            m_numCascadedShadowRenderViews = 0;
        uint32_t                                            m_numForwardShadingRenderViews = 0;

        uint32_t                                            m_globalEnvironmentMapRenderViewsOffset = 0;
        uint32_t                                            m_cascadedShadowRenderViewsOffset = 0;
        uint32_t                                            m_forwardShadingRenderViewsOffset = 0;

        uint32_t                                            m_numRenderViews = 0;
        uint32_t                                            m_numRenderBuckets = 0;
        uint32_t                                            m_numRenderViewBucketsPerView = 0;

        #if EE_DEVELOPMENT_TOOLS
        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_shaderDebugDrawBuffers = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>   m_debugCommandsBuffers = {};
        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_debugParametersBuffers = {};

        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_meshArgumentCounterBuffers = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>   m_meshArgumentBuffers = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>   m_meshParametersBuffers = {};

        uint32_t                                            m_numCommands_TransparentDepthOnWrite = 0;
        uint32_t                                            m_numCommands_TransparentDepthOnNoWrite = 0;
        uint32_t                                            m_numCommands_TransparentDepthSeparateWrite = 0;

        DeviceAppendBuffer<PickingResult>                   m_instancePickingResultsBuffer;
        DeviceResizeBuffer                                  m_instancePickingDistancesBuffer;
        DeviceAppendBuffer<PickingResult>                   m_debugDrawPickingResultsBuffer;

        Float2                                              m_lastKnownPickingMousePosition = Float2::Zero;
        uint32_t                                            m_lastKnownPickingPixelRadius = 2;

        bool                                                m_isStandalone = true;
        bool                                                m_isPickingEnabled = false;
        #endif
    };
}
