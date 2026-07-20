#include "RenderViewport.h"
#include "RenderViewport.h"
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

    void RenderViewport::Initialize( RHI::Context* pContextRHI, Render::Window* pWindow )
    {
        EE_ASSERT( pWindow != nullptr && pWindow->IsValid() );
        UpdateRenderWindow( pWindow );

        #if EE_DEVELOPMENT_TOOLS
        m_instancePickingDistancesBuffer.Initialize( pContextRHI );

        m_instancePickingResultsBuffer.Initialize( pContextRHI, "InstancePickingResults" );
        m_debugDrawPickingResultsBuffer.Initialize( pContextRHI, "DebugDrawPickingResults" );

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            m_debugCommandsBuffers[frameIndex].Shutdown( pContextRHI );
            m_meshParametersBuffers[frameIndex].Shutdown( pContextRHI );
            m_meshArgumentBuffers[frameIndex].Shutdown( pContextRHI );
        }
        #endif
    }

    void RenderViewport::Shutdown( RHI::Context* pContextRHI )
    {
        RHI::DestroyTexture( pContextRHI, eastl::move( m_ForwardShading_ColorTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_ForwardShading_DepthTexture ) );

        RHI::DestroyTexture( pContextRHI, eastl::move( m_DepthDownsample2 ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_DepthDownsample4 ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_DepthDownsample8 ) );

        RHI::DestroyTexture( pContextRHI, eastl::move( m_SMAA_StencilTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_SMAA_EdgesTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_SMAA_BlendTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_SMAA_ResultTexture ) );

        RHI::DestroyTexture( pContextRHI, eastl::move( m_GTAO_ResultTextureNoisy0 ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_GTAO_ResultTextureNoisy1 ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_GTAO_ResultTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_GTAO_ResultTextureHalfResolution ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_GTAO_EdgesTexture ) );
        RHI::DestroyTexture( pContextRHI, eastl::move( m_GTAO_PrefilterDepthTexture ) );

        RHI::DestroyTexture( pContextRHI, eastl::move( m_LightCulling_BucketTexture ) );

        if ( !IsStandalone() )
        {
            RHI::DestroyTexture( pContextRHI, eastl::move( m_finalTexture ) );
        }

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_globalParametersBuffers[frameIndex] ) );
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_renderViewBuffers[frameIndex] ) );
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_renderBucketBuffers[frameIndex] ) );
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_cascadedShadowBuffers[frameIndex] ) );

            RHI::DestroyBuffer( pContextRHI, eastl::move( m_GTAO_parametersBuffers[frameIndex] ) );
        }

        #if EE_DEVELOPMENT_TOOLS
        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            m_debugCommandsBuffers[frameIndex].Shutdown( pContextRHI );
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_shaderDebugDrawBuffers[frameIndex] ) );
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_debugParametersBuffers[frameIndex] ) );

            m_meshParametersBuffers[frameIndex].Shutdown( pContextRHI );
            m_meshArgumentBuffers[frameIndex].Shutdown( pContextRHI );
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_meshArgumentCounterBuffers[frameIndex] ) );
        }

        m_instancePickingDistancesBuffer.Shutdown( pContextRHI );
        m_instancePickingResultsBuffer.Shutdown( pContextRHI );
        m_debugDrawPickingResultsBuffer.Shutdown( pContextRHI );

        RHI::DestroyTexture( pContextRHI, eastl::move( m_DebugDraw_DepthTexture ) );
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
