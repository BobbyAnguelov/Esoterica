#include "RenderViewport.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Engine/Render/Systems/WorldSystem_Render.h"
#include "Base/Render/RHI.h"
#include "Base/Render/RenderWindow.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    bool RenderViewport::IsValid() const
    {
        if ( m_pWindow == nullptr )
        {
            return false;
        }

        return Viewport::IsValid();
    }

    void RenderViewport::Initialize( RenderSystem* pRenderSystem, RenderWorldSystem* pRenderWorldSystem, Render::Window* pWindow )
    {
        EE_ASSERT( pWindow != nullptr && pWindow->IsValid() );
        UpdateRenderWindow( pWindow );

        RHI::Context* pContextRHI = pRenderSystem->GetContextRHI();

        #if EE_DEVELOPMENT_TOOLS
        m_instancePickingDistancesBuffer.Initialize( pContextRHI, true );

        m_instancePickingResultsBuffer.Initialize( pContextRHI, "InstancePickingResults" );
        m_debugDrawPickingResultsBuffer.Initialize( pContextRHI, "DebugDrawPickingResults" );

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            m_debugCommandsBuffers[frameIndex].Initialize( pContextRHI, true );
            m_debugCommandsBuffersOutline[frameIndex].Initialize( pContextRHI, true );
            m_debugMeshArgumentBuffersOutline[frameIndex].Initialize( pContextRHI, true );
            m_debugMeshParametersBuffersOutline[frameIndex].Initialize( pContextRHI, true );
            m_meshParametersBuffers[frameIndex].Initialize( pContextRHI, true );
            m_meshArgumentBuffers[frameIndex].Initialize( pContextRHI, true );
        }
        #endif

        m_mainRenderViewProxy = pRenderWorldSystem->m_deviceRenderWorld.AllocateRenderViews( DeviceRenderViewType::Main, 1 );
        m_globalEnvironmentMapRenderViewProxy = pRenderWorldSystem->m_deviceRenderWorld.AllocateRenderViews( DeviceRenderViewType::GlobalEnvironmentMap, g_NumGlobalEnvironmentMapViews );

        #if EE_DEVELOPMENT_TOOLS
        m_editorOutlineRenderViewProxy = pRenderWorldSystem->m_deviceRenderWorld.AllocateRenderViews( DeviceRenderViewType::EditorOutline, 1 );
        #endif
    }

    void RenderViewport::Shutdown( RenderSystem* pRenderSystem, RenderWorldSystem* pRenderWorldSystem )
    {
        RHI::Context* const pContextRHI = pRenderSystem->GetContextRHI();

        RHI::DestroyTexture( pContextRHI, eastl::move( m_forwardShading_colorTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_forwardShading_depthTexture ) );

        RHI::DestroyTexture( pContextRHI, eastl::move( m_depthDownsample2 ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_depthDownsample4 ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_depthDownsample8 ) );

        RHI::DestroyTexture( pContextRHI, eastl::move( m_SMAA_stencilTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_SMAA_edgesTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_SMAA_blendTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_SMAA_resultTexture ) );

        RHI::DestroyTexture( pContextRHI, eastl::move( m_GTAO_resultTextureNoisy0 ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_GTAO_resultTextureNoisy1 ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_GTAO_resultTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_GTAO_resultTextureHalfResolution ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_GTAO_edgesTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_GTAO_prefilterDepthTexture ) );

        if ( !IsStandalone() )
        {
            RHI::DestroyTexture( pContextRHI, eastl::move( m_finalTexture ) );
        }

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_globalParametersBuffers[frameIndex] ) );
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_renderBucketBuffers[frameIndex] ) );
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_renderViewIndirectionBuffers[frameIndex] ) );

            RHI::DestroyBuffer( pContextRHI, eastl::move( m_GTAO_parametersBuffers[frameIndex] ) );
        }

        for ( ActiveRenderView& activeRenderView : m_activeRenderViews )
        {
            activeRenderView.Shutdown( pRenderSystem );
        }
        m_activeRenderViews.clear();

        #if EE_DEVELOPMENT_TOOLS
        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            m_debugCommandsBuffers[frameIndex].Shutdown( pContextRHI );
            m_debugCommandsBuffersOutline[frameIndex].Shutdown( pContextRHI );
            m_debugMeshArgumentBuffersOutline[frameIndex].Shutdown( pContextRHI );
            m_debugMeshParametersBuffersOutline[frameIndex].Shutdown( pContextRHI );
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_shaderDebugDrawBuffers[frameIndex] ) );
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_debugParametersBuffers[frameIndex] ) );

            m_meshParametersBuffers[frameIndex].Shutdown( pContextRHI );
            m_meshArgumentBuffers[frameIndex].Shutdown( pContextRHI );
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_meshArgumentCounterBuffers[frameIndex] ) );
        }

        m_instancePickingDistancesBuffer.Shutdown( pContextRHI );
        m_instancePickingResultsBuffer.Shutdown( pContextRHI );
        m_debugDrawPickingResultsBuffer.Shutdown( pContextRHI );

        RHI::DestroyTexture( pContextRHI, eastl::move( m_debugDraw_depthTexture ) );

        RHI::DestroyTexture( pContextRHI, eastl::move( m_editorOutline_depthTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_editorOutline_idTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_editorOutline_JFA_texture0 ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_editorOutline_JFA_texture1 ) );
        #endif

        pRenderWorldSystem->m_deviceRenderWorld.DeallocateRenderViews( eastl::move( m_mainRenderViewProxy ) );
        pRenderWorldSystem->m_deviceRenderWorld.DeallocateRenderViews( eastl::move( m_globalEnvironmentMapRenderViewProxy ) );

        #if EE_DEVELOPMENT_TOOLS
        pRenderWorldSystem->m_deviceRenderWorld.DeallocateRenderViews( eastl::move( m_editorOutlineRenderViewProxy ) );
        #endif
    }

    void RenderViewport::UpdateRenderWindow( Render::Window* pWindow )
    {
        if ( pWindow == m_pWindow )
        {
            return;
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( pWindow != nullptr && pWindow->IsValid() );
        m_pWindow = pWindow;

        Float2 const windowSize = Float2( m_pWindow->GetSwapchainSize() );
        EE_ASSERT( !windowSize.IsNearZero() );
        float const aspectRatio = windowSize.m_x / windowSize.m_y;

        m_topLeftPosition = Float2::Zero;
        m_size = windowSize;
        m_viewVolume = Math::ViewVolume::CreatePerspective( aspectRatio, FloatRange( 0.1f, 1000.0f ), 90.0f, Transform::Identity );
    }

}
