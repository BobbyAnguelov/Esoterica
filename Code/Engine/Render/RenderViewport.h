#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Viewport/Viewport.h"
#include "Engine/Viewport/ViewportPicking.h"
#include "Engine/Render/ActiveRenderView.h"
#include "Engine/Render/RenderProxies.h"
#include "Engine/Render/Device/DeviceResourceState.h"
#include "Engine/Render/Device/DeviceAppendBuffer.h"
#include "Engine/Render/Device/DeviceResizeBuffer.h"
#include "Base/Render/RHI.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Window;
    class DeviceRenderWorld;
    class RenderWorldSystem;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API RenderViewport final : public Viewport
    {

    public:

        virtual bool IsValid() const override;

        void Initialize( RenderSystem* pRenderSystem, RenderWorldSystem* pRenderWorldSystem, Render::Window* pWindow );
        void Shutdown( RenderSystem* pRenderSystem, RenderWorldSystem* pRenderWorldSystem );

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
        mutable DeviceTextureState                          m_forwardShading_depthTexture = {};
        mutable DeviceTextureState                          m_forwardShading_colorTexture = {};

        mutable DeviceTextureState                          m_depthDownsample2 = {};
        mutable DeviceTextureState                          m_depthDownsample4 = {};
        mutable DeviceTextureState                          m_depthDownsample8 = {};

        mutable DeviceTextureState                          m_SMAA_stencilTexture = {};
        mutable DeviceTextureState                          m_SMAA_edgesTexture = {};
        mutable DeviceTextureState                          m_SMAA_blendTexture = {};
        mutable DeviceTextureState                          m_SMAA_resultTexture = {};

        mutable DeviceTextureState                          m_GTAO_resultTextureNoisy0 = {};
        mutable DeviceTextureState                          m_GTAO_resultTextureNoisy1 = {};

        mutable DeviceTextureState                          m_GTAO_resultTextureHalfResolution = {};
        mutable DeviceTextureState                          m_GTAO_resultTexture = {};
        mutable DeviceTextureState                          m_GTAO_edgesTexture = {};
        mutable DeviceTextureState                          m_GTAO_prefilterDepthTexture = {};

        mutable DeviceTextureState                          m_finalTexture = {};

        #if EE_DEVELOPMENT_TOOLS
        mutable DeviceTextureState                          m_debugDraw_depthTexture = {};

        mutable DeviceTextureState                          m_editorOutline_depthTexture = {};
        mutable DeviceTextureState                          m_editorOutline_idTexture = {};
        mutable DeviceTextureState                          m_editorOutline_JFA_texture0 = {};
        mutable DeviceTextureState                          m_editorOutline_JFA_texture1 = {};
        #endif

        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_GTAO_parametersBuffers = {};
        uint32_t                                            m_GTAO_noiseIndex = 0;

        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_globalParametersBuffers = {};
        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_renderBucketBuffers = {};
        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_renderViewIndirectionBuffers = {};

        // TODO: Hacky, this should be owned by the world?
        RenderViewProxy                                     m_mainRenderViewProxy = {};
        RenderViewProxy                                     m_globalEnvironmentMapRenderViewProxy = {};

        TVector<ActiveRenderView>                           m_activeRenderViews;
        ActiveRenderViewList                                m_activeRenderViewList;
        ActiveRenderViewSelection                           m_activeRenderViewSelection;

        #if EE_DEVELOPMENT_TOOLS
        RenderViewProxy                                     m_editorOutlineRenderViewProxy = {};
        #endif

        #if EE_DEVELOPMENT_TOOLS
        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_shaderDebugDrawBuffers = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>   m_debugCommandsBuffers = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>   m_debugCommandsBuffersOutline = {};
        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_debugParametersBuffers = {};

        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_meshArgumentCounterBuffers = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>   m_meshArgumentBuffers = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>   m_meshParametersBuffers = {};

        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>   m_debugMeshArgumentBuffersOutline = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>   m_debugMeshParametersBuffersOutline = {};

        uint32_t                                            m_numCommands_transparentDepthOnWrite = 0;
        uint32_t                                            m_numCommands_transparentDepthOnNoWrite = 0;
        uint32_t                                            m_numCommands_transparentDepthSeparateWrite = 0;
        uint32_t                                            m_numCommands_outline = 0;
        uint32_t                                            m_numMeshCommands_outline = 0;

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
