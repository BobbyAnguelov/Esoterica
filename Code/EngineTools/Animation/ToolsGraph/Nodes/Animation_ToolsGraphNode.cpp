#include "Animation_ToolsGraphNode.h"
#include "System/Imgui/ImguiStyle.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    constexpr static float const g_playbackBarMinimumWidth = 120;
    constexpr static float const g_playbackBarRegionHeight = 16;
    constexpr static float const g_playbackBarHeight = 10;
    constexpr static float const g_playbackBarMarkerSize = 4;

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

    static void TraverseHierarchy( VisualGraph::BaseNode const* pNode, TVector<VisualGraph::BaseNode const*>& nodePath )
    {
        EE_ASSERT( pNode != nullptr );
        nodePath.emplace_back( pNode );

        if ( pNode->HasParentGraph() && !pNode->GetParentGraph()->IsRootGraph() )
        {
            TraverseHierarchy( pNode->GetParentGraph()->GetParentNode(), nodePath );
        }
    }

    //-------------------------------------------------------------------------

    void FlowToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        bool const isPreviewing = pGraphNodeContext->HasDebugData();
        int16_t const runtimeNodeIdx = isPreviewing ? pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() ) : InvalidIndex;
        bool const isPreviewingAndValidRuntimeNodeIdx = isPreviewing && ( runtimeNodeIdx != InvalidIndex );

        //-------------------------------------------------------------------------
        // Draw Pose Node
        //-------------------------------------------------------------------------

        if ( GetValueType() == GraphValueType::Pose )
        {
            if ( isPreviewingAndValidRuntimeNodeIdx && pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) )
            {
                PoseNodeDebugInfo const debugInfo = pGraphNodeContext->GetPoseNodeDebugInfo( runtimeNodeIdx );
                DrawPoseNodeDebugInfo( ctx, GetWidth(), debugInfo );
            }
            else
            {
                DrawEmptyPoseNodeDebugInfo( ctx, GetWidth() );
            }

            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 4 );
        }

        //-------------------------------------------------------------------------
        // Draw Value Node
        //-------------------------------------------------------------------------

        else
        {
            if ( isPreviewingAndValidRuntimeNodeIdx && pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) )
            {
                if ( HasOutputPin() )
                {
                    GraphValueType const valueType = GetValueType();
                    switch ( valueType )
                    {
                        case GraphValueType::Bool:
                        {
                            auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<bool>( runtimeNodeIdx );
                            ImGui::Text( value ? "True" : "False" );
                        }
                        break;

                        case GraphValueType::ID:
                        {
                            auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<StringID>( runtimeNodeIdx );
                            if ( value.IsValid() )
                            {
                                ImGui::Text( value.c_str() );
                            }
                            else
                            {
                                ImGui::Text( "Invalid" );
                            }
                        }
                        break;

                        case GraphValueType::Int:
                        {
                            auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<int32_t>( runtimeNodeIdx );
                            ImGui::Text( "%d", value );
                        }
                        break;

                        case GraphValueType::Float:
                        {
                            auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<float>( runtimeNodeIdx );
                            ImGui::Text( "%.3f", value );
                        }
                        break;

                        case GraphValueType::Vector:
                        {
                            auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<Vector>( runtimeNodeIdx );
                            ImGuiX::DisplayVector4( value );
                        }
                        break;

                        case GraphValueType::Target:
                        {
                            auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<Target>( runtimeNodeIdx );
                            if ( value.IsBoneTarget() )
                            {
                                if ( value.GetBoneID().IsValid() )
                                {
                                    ImGui::Text( value.GetBoneID().c_str() );
                                }
                                else
                                {
                                    ImGui::Text( "Invalid" );
                                }
                            }
                            else
                            {
                                ImGui::Text( "Target TODO" );
                            }
                        }
                        break;

                        case GraphValueType::BoneMask:
                        case GraphValueType::Pose:
                        default:
                        break;
                    }
                }
            }
        }

        //-------------------------------------------------------------------------

        DrawInfoText( ctx );
    }

    bool FlowToolsNode::IsActive( VisualGraph::UserContext* pUserContext ) const
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            // Some nodes dont have runtime representations
            auto const runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
            if ( runtimeNodeIdx != InvalidIndex )
            {
                return pGraphNodeContext->IsNodeActive( runtimeNodeIdx );
            }
        }

        return false;
    }

    ImColor FlowToolsNode::GetTitleBarColor() const
    {
        return ImGuiX::ConvertColor( GetColorForValueType( GetValueType() ) );
    }

    ImColor FlowToolsNode::GetPinColor( VisualGraph::Flow::Pin const& pin ) const
    {
        return ImGuiX::ConvertColor( GetColorForValueType( (GraphValueType) pin.m_type ) );
    }

    void FlowToolsNode::DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, VisualGraph::Flow::Pin* pPin )
    {
        ImGui::Separator();

        if ( ImGui::BeginMenu( EE_ICON_INFORMATION_OUTLINE" Node Info" ) )
        {
            // UUID
            auto IDStr = GetID().ToString();
            InlineString label = InlineString( InlineString::CtorSprintf(), "UUID: %s", IDStr.c_str() );
            if ( ImGui::MenuItem( label.c_str() ) )
            {
                ImGui::SetClipboardText( IDStr.c_str() );
            }

            // Draw runtime node index
            auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
            if ( pGraphNodeContext->HasDebugData() )
            {
                int16_t runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
                if ( runtimeNodeIdx != InvalidIndex )
                {
                    label = InlineString( InlineString::CtorSprintf(), "Runtime Index: %d", runtimeNodeIdx );
                    if ( ImGui::MenuItem( label.c_str() ) )
                    {
                        InlineString const value( InlineString::CtorSprintf(), "%d", runtimeNodeIdx );
                        ImGui::SetClipboardText( value.c_str() );
                    }
                }
            }

            ImGui::EndMenu();
        }
    }
}