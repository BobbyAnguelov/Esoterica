#include "Animation_EditorGraph_Common.h"
#include "EngineTools/Core/VisualGraph/VisualGraph_DrawingContext.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    constexpr static float const g_playbackBarMinimumWidth = 120;
    constexpr static float const g_playbackBarRegionHeight = 16;
    constexpr static float const g_playbackBarHeight = 10;
    constexpr static float const g_playbackBarMarkerSize = 4;

    //-------------------------------------------------------------------------

    bool GraphNodeContext::IsNodeActive( int16_t nodeIdx ) const
    {
        return m_pGraphComponent->IsNodeActive( nodeIdx );
    }

    #if EE_DEVELOPMENT_TOOLS
    PoseNodeDebugInfo GraphNodeContext::GetPoseNodeDebugInfo( int16_t runtimeNodeIdx ) const
    {
        return m_pGraphComponent->GetPoseNodeDebugInfo( runtimeNodeIdx );
    }
    #endif

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void DrawPoseNodeDebugInfo( VisualGraph::DrawContext const& ctx, float width, PoseNodeDebugInfo const& debugInfo )
    {
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 2.0f );

        float const availableWidth = Math::Max( width, g_playbackBarMinimumWidth );
        ImVec2 const playbackBarSize = ImVec2( availableWidth, g_playbackBarHeight );
        ImVec2 const playbackBarTopLeft = ImGui::GetCursorScreenPos();
        ImVec2 const playbackBarBottomRight = playbackBarTopLeft + playbackBarSize;

        Percentage const percentageThroughTrack = debugInfo.m_currentTime.GetNormalizedTime();
        float const pixelOffsetForPercentageThrough = Math::Floor( playbackBarSize.x * percentageThroughTrack );

        //-------------------------------------------------------------------------

        // Draw spacer
        ImVec2 const playbackBarRegion = ImVec2( availableWidth, g_playbackBarRegionHeight );
        ImGui::InvisibleButton( "Spacer", playbackBarRegion );

        // Draw events
        bool useAlternateColor = false;
        ImVec2 eventTopLeft = playbackBarTopLeft;
        ImVec2 eventBottomRight = playbackBarBottomRight;
        for ( auto const& evt : debugInfo.m_pSyncTrack->GetEvents() )
        {
            eventBottomRight.x = eventTopLeft.x + Math::Floor( playbackBarSize.x * evt.m_duration );
            ctx.m_pDrawList->AddRectFilled( eventTopLeft, eventBottomRight, useAlternateColor ? ImGuiX::ConvertColor( Colors::White ) : ImGuiX::ConvertColor( Colors::DarkGray ) );
            eventTopLeft.x = eventBottomRight.x;
            useAlternateColor = !useAlternateColor;
        }

        // Draw progress bar
        ImVec2 progressBarTopLeft = playbackBarTopLeft;
        ImVec2 progressBarBottomRight = playbackBarTopLeft + ImVec2( pixelOffsetForPercentageThrough, playbackBarSize.y );
        ctx.m_pDrawList->AddRectFilled( progressBarTopLeft, progressBarBottomRight, ImGuiX::ConvertColor( Colors::LimeGreen.GetAlphaVersion( 0.65f ) ) );

        // Draw Marker
        ImVec2 t0( progressBarTopLeft.x + pixelOffsetForPercentageThrough, playbackBarBottomRight.y );
        ImVec2 t1( t0.x - g_playbackBarMarkerSize, playbackBarBottomRight.y + g_playbackBarMarkerSize );
        ImVec2 t2( t0.x + g_playbackBarMarkerSize, playbackBarBottomRight.y + g_playbackBarMarkerSize );
        ctx.m_pDrawList->AddLine( t0, t0 - ImVec2( 0, playbackBarSize.y ), ImGuiX::ConvertColor( Colors::LimeGreen ) );
        ctx.m_pDrawList->AddTriangleFilled( t0, t1, t2, ImGuiX::ConvertColor( Colors::LimeGreen ) );

        // Draw text info
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 2.0f );
        ImGui::Text( "Time: %.2f/%.2fs", debugInfo.m_currentTime.ToFloat() * debugInfo.m_duration, debugInfo.m_duration.ToFloat() );
        ImGui::Text( "Percent: %.1f%%", debugInfo.m_currentTime.ToFloat() * 100 );
        ImGui::Text( "Event: %d, %.1f%%", debugInfo.m_currentSyncTime.m_eventIdx, debugInfo.m_currentSyncTime.m_percentageThrough.ToFloat() * 100 );
    }

    void DrawEmptyPoseNodeDebugInfo( VisualGraph::DrawContext const& ctx, float width )
    {
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 4.0f );

        float const availableWidth = Math::Max( width, g_playbackBarMinimumWidth );
        ImVec2 const playbackBarSize = ImVec2( availableWidth, g_playbackBarHeight );
        ImVec2 const playbackBarTopLeft = ImGui::GetCursorScreenPos();
        ImVec2 const playbackBarBottomRight = playbackBarTopLeft + playbackBarSize;

        // Draw spacer
        ImVec2 const playbackBarRegion = ImVec2( availableWidth, g_playbackBarRegionHeight );
        ImGui::InvisibleButton( "Spacer", playbackBarRegion );

        // Draw empty playback visualization bar
        ctx.m_pDrawList->AddRectFilled( playbackBarTopLeft, playbackBarTopLeft + playbackBarSize, ImGuiX::ConvertColor( Colors::DarkGray ) );

        // Draw text placeholders
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 2.0f );
        ImGui::Text( "Time: N/A" );
        ImGui::Text( "Percent: N/A" );
        ImGui::Text( "Event: N/A" );
    }
    #endif
}