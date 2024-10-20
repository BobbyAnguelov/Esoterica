#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    constexpr static float const g_playbackBarMinimumWidth = 120;
    constexpr static float const g_playbackBarHeight = 12;
    constexpr static float const g_playbackBarMarkerSize = 4;
    constexpr static float const g_playbackBarRegionHeight = g_playbackBarHeight + g_playbackBarMarkerSize;

    //-------------------------------------------------------------------------

    void DrawPoseNodeDebugInfo( NodeGraph::DrawContext const& ctx, float canvasWidth, PoseNodeDebugInfo const* pDebugInfo )
    {
        float const availableCanvasWidth = Math::Max( canvasWidth, g_playbackBarMinimumWidth );
        ImVec2 const playbackBarSize = ctx.CanvasToWindow( ImVec2( availableCanvasWidth, g_playbackBarHeight ) );
        ImVec2 const playbackBarTopLeft = ImGui::GetCursorScreenPos();
        ImVec2 const playbackBarBottomRight = playbackBarTopLeft + playbackBarSize;

        // Draw spacer
        //-------------------------------------------------------------------------

        ImGui::Dummy( playbackBarSize );

        // Draw Info
        //-------------------------------------------------------------------------

        if ( pDebugInfo != nullptr )
        {
            Percentage const percentageThroughTrack = pDebugInfo->m_currentTime.GetNormalizedTime();
            float const pixelOffsetForPercentageThrough = Math::Floor( playbackBarSize.x * percentageThroughTrack );

            // Draw events
            auto GetIntervalColor = [&] ( int32_t intervalIdx ) { return Math::IsEven( intervalIdx ) ? Colors::White : Colors::DarkGray; };

            TInlineVector<SyncTrack::Event, 10> const& events = pDebugInfo->m_pSyncTrack->GetEvents();
            int32_t const numEvents = (int32_t) events.size();
            for ( int32_t i = 0; i < numEvents; i++ )
            {
                ImVec2 const eventTopLeft( Math::Round( playbackBarTopLeft.x + playbackBarSize.x * events[i].m_startTime ), playbackBarTopLeft.y );
                ImVec2 const eventBottomRight( Math::Min( playbackBarBottomRight.x, eventTopLeft.x + Math::Round( playbackBarSize.x * events[i].m_duration ) ), playbackBarBottomRight.y );

                // Draw start line (and overflow event)
                if( i == 0 && events[i].m_startTime != 0.0f )
                {
                    ImVec2 const eventBottomLeft( eventTopLeft.x, eventBottomRight.y );

                    ctx.m_pDrawList->AddRectFilled( playbackBarTopLeft, eventBottomLeft, GetIntervalColor( numEvents - 1 ) );
                    ctx.m_pDrawList->AddLine( eventTopLeft, eventBottomLeft, Colors::HotPink, 2.0f * ctx.m_viewScaleFactor );
                }

                // Draw interval
                ctx.m_pDrawList->AddRectFilled( eventTopLeft, eventBottomRight, GetIntervalColor( i ) );
            }

            // Draw progress bar
            ImVec2 progressBarTopLeft = playbackBarTopLeft;
            ImVec2 progressBarBottomRight = playbackBarTopLeft + ImVec2( pixelOffsetForPercentageThrough, playbackBarSize.y );
            ctx.m_pDrawList->AddRectFilled( progressBarTopLeft, progressBarBottomRight, Colors::LimeGreen.GetAlphaVersion( 0.65f ) );

            // Draw Marker
            float const scaledMarkerSize = ctx.CanvasToWindow( g_playbackBarMarkerSize );
            ImVec2 t0( progressBarTopLeft.x + pixelOffsetForPercentageThrough, playbackBarBottomRight.y );
            ImVec2 t1( t0.x - scaledMarkerSize, playbackBarBottomRight.y + scaledMarkerSize );
            ImVec2 t2( t0.x + scaledMarkerSize, playbackBarBottomRight.y + scaledMarkerSize );
            ctx.m_pDrawList->AddLine( t0, t0 - ImVec2( 0, playbackBarSize.y ), Colors::LimeGreen );
            ctx.m_pDrawList->AddTriangleFilled( t0, t1, t2, Colors::LimeGreen );

            // Draw text info
            ImGui::Text( "Time: %.2f/%.2fs", pDebugInfo->m_currentTime.ToFloat() * pDebugInfo->m_duration, pDebugInfo->m_duration.ToFloat() );
            ImGui::Text( "Percent: %.1f%%", pDebugInfo->m_currentTime.ToFloat() * 100 );
            ImGui::Text( "Event: %d, %.1f%%", pDebugInfo->m_currentSyncTime.m_eventIdx, pDebugInfo->m_currentSyncTime.m_percentageThrough.ToFloat() * 100 );
            StringID const eventID = pDebugInfo->m_pSyncTrack->GetEventID( pDebugInfo->m_currentSyncTime.m_eventIdx );
            ImGui::Text( "Event ID: %s", eventID.IsValid() ? eventID.c_str() : "No ID");
        }
        else
        {
            // Draw empty playback visualization bar
            ctx.m_pDrawList->AddRectFilled( playbackBarTopLeft, playbackBarTopLeft + playbackBarSize, Colors::DarkGray );

            // Draw text placeholders
            ImGui::Text( "Time: N/A" );
            ImGui::Text( "Percent: N/A" );
            ImGui::Text( "Event: N/A" );
            ImGui::Text( "Event ID: N/A" );
        }
    }

    void DrawRuntimeNodeIndex( NodeGraph::DrawContext const& ctx, ToolsGraphUserContext* pGraphNodeContext, NodeGraph::BaseNode* pNode, int16_t runtimeNodeIdx )
    {
        ImGuiStyle const& style = ImGui::GetStyle();

        //-------------------------------------------------------------------------

        InlineString const idxStr( InlineString::CtorSprintf(), "%d", runtimeNodeIdx );

        ImVec2 const textSize = ImGui::CalcTextSize( idxStr.c_str() );
        float const scaledBubblePadding = ctx.CanvasToWindow( 4.0f );
        float const scaledVerticalOffset = ctx.CanvasToWindow( pNode->GetNodeMargin() ).y + style.ItemSpacing.y;
        float const scaledBubbleWidth = textSize.x + style.ItemSpacing.x + ( scaledBubblePadding * 2 );
        float const scaledBubbleHeight = ImGui::GetFrameHeight();

        //-------------------------------------------------------------------------

        Float2 const nodeWindowPosition = ctx.CanvasToWindowPosition( pNode->GetPosition() );
        ImVec2 const startRect = ctx.WindowToScreenPosition( ImVec2( nodeWindowPosition.m_x, nodeWindowPosition.m_y - scaledBubbleHeight - scaledVerticalOffset ) );
        ImVec2 const endRect = ImVec2( startRect.x + scaledBubbleWidth, startRect.y + scaledBubbleHeight );
        float const scaledRounding = ctx.CanvasToWindow( 3.0f * ctx.m_viewScaleFactor );
        ctx.m_pDrawList->AddRectFilled( startRect, endRect, Colors::MediumRed, scaledRounding );

        //-------------------------------------------------------------------------

        auto pFont = ImGuiX::GetFont( ImGuiX::Font::Medium );
        Float2 const scaledTextOffset( scaledBubblePadding, scaledBubblePadding );
        ctx.m_pDrawList->AddText( pFont, pFont->FontSize * ctx.m_viewScaleFactor, startRect + scaledTextOffset, Colors::White, idxStr.c_str() );
    }

    void DrawVectorInfoText( NodeGraph::DrawContext const& ctx, Float3 const& value )
    {
        ImGui::Text( "X: %.2f, Y: %.2f, Z: %.2f", value.m_x, value.m_y, value.m_z );
    }

    void DrawTargetInfoText( NodeGraph::DrawContext const& ctx, Target const& value )
    {
        if ( value.IsTargetSet() )
        {
            if ( value.IsBoneTarget() )
            {
                if ( value.GetBoneID().IsValid() )
                {
                    ImGui::Text( "Value: %s", value.GetBoneID().c_str() );
                }
                else
                {
                    ImGui::Text( "Value: Invalid" );
                }
            }
            else
            {
                Transform const& transform = value.GetTransform();
                Vector const& translation = transform.GetTranslation();
                EulerAngles const angles = transform.GetRotation().ToEulerAngles();

                ImGui::Text( "Rot: X: %.3f, Y: %.3f, Z: %.3f", angles.m_x.ToDegrees().ToFloat(), angles.m_y.ToDegrees().ToFloat(), angles.m_z.ToDegrees().ToFloat() );
                ImGui::Text( "Trans: X: %.3f, Y: %.3f, Z: %.3f", translation.GetX(), translation.GetY(), translation.GetZ() );
                ImGui::Text( "Scl: %.3f", transform.GetScale() );
            }
        }
        else
        {
            ImGui::Text( "Not Set" );
        }
    }

    void DrawValueDisplayText( NodeGraph::DrawContext const& ctx, ToolsGraphUserContext* pGraphNodeContext, int16_t runtimeNodeIdx, GraphValueType valueType )
    {
        EE_ASSERT( pGraphNodeContext != nullptr );
        EE_ASSERT( pGraphNodeContext->HasDebugData() );
        EE_ASSERT( runtimeNodeIdx != InvalidIndex );

        //-------------------------------------------------------------------------

        switch ( valueType )
        {
            case GraphValueType::Bool:
            {
                auto const value = pGraphNodeContext->GetNodeValue<bool>( runtimeNodeIdx );
                ImGui::Text( "Value: " );
                ImGui::SameLine();
                ImGui::TextColored( ( value ? Colors::LimeGreen : Colors::IndianRed ).ToFloat4(), value ? "True" : "False" );
            }
            break;

            case GraphValueType::ID:
            {
                auto const value = pGraphNodeContext->GetNodeValue<StringID>( runtimeNodeIdx );
                if ( value.IsValid() )
                {
                    ImGui::Text( "Value: %s", value.c_str() );
                }
                else
                {
                    ImGui::Text( "Value: Invalid" );
                }
            }
            break;

            case GraphValueType::Float:
            {
                auto const value = pGraphNodeContext->GetNodeValue<float>( runtimeNodeIdx );
                ImGui::Text( "Value: %.3f", value );
            }
            break;

            case GraphValueType::Vector:
            {
                auto const value = pGraphNodeContext->GetNodeValue<Float3>( runtimeNodeIdx );
                DrawVectorInfoText( ctx, value );
            }
            break;

            case GraphValueType::Target:
            {
                auto const value = pGraphNodeContext->GetNodeValue<Target>( runtimeNodeIdx );
                DrawTargetInfoText( ctx, value );
            }
            break;

            default:
            break;
        }
    }

    static void TraverseHierarchy( NodeGraph::BaseNode const* pNode, TVector<NodeGraph::BaseNode const*>& nodePath )
    {
        EE_ASSERT( pNode != nullptr );
        nodePath.emplace_back( pNode );

        if ( pNode->HasParentGraph() && !pNode->GetParentGraph()->IsRootGraph() )
        {
            TraverseHierarchy( pNode->GetParentGraph()->GetParentNode(), nodePath );
        }
    }

    //-------------------------------------------------------------------------

    void FlowToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );

        bool const isPreviewing = pGraphNodeContext->HasDebugData();
        int16_t const runtimeNodeIdx = isPreviewing ? pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() ) : InvalidIndex;
        bool const isPreviewingAndValidRuntimeNodeIdx = isPreviewing && ( runtimeNodeIdx != InvalidIndex );

        GraphValueType const nodeValueType = GetOutputValueType();

        //-------------------------------------------------------------------------
        // Runtime Index Info
        //-------------------------------------------------------------------------

        if ( pGraphNodeContext->m_showRuntimeIndices && isPreviewingAndValidRuntimeNodeIdx )
        {
            DrawRuntimeNodeIndex( ctx, pGraphNodeContext, this, runtimeNodeIdx );
        }

        //-------------------------------------------------------------------------
        // Draw Pose Node
        //-------------------------------------------------------------------------

        if ( nodeValueType == GraphValueType::Pose )
        {
            if ( isPreviewingAndValidRuntimeNodeIdx && pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) )
            {
                PoseNodeDebugInfo const debugInfo = pGraphNodeContext->GetPoseNodeDebugInfo( runtimeNodeIdx );
                DrawPoseNodeDebugInfo( ctx, GetWidth(), &debugInfo );
            }
            else
            {
                DrawPoseNodeDebugInfo( ctx, GetWidth(), nullptr );
            }

            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y );

            DrawInfoText( ctx );
        }

        //-------------------------------------------------------------------------
        // Draw Value Node
        //-------------------------------------------------------------------------

        else
        {
            DrawInfoText( ctx );

            if ( nodeValueType != GraphValueType::Unknown && nodeValueType != GraphValueType::BoneMask && nodeValueType != GraphValueType::Pose && nodeValueType != GraphValueType::Special )
            {
                DrawInternalSeparator( ctx );

                BeginDrawInternalRegion( ctx );

                if ( isPreviewingAndValidRuntimeNodeIdx && pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) && HasOutputPin() )
                {
                    DrawValueDisplayText( ctx, pGraphNodeContext, runtimeNodeIdx, nodeValueType );
                }
                else
                {
                    ImGui::NewLine();
                }

                EndDrawInternalRegion( ctx );
            }
        }
    }

    bool FlowToolsNode::IsActive( NodeGraph::UserContext* pUserContext ) const
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

    Color FlowToolsNode::GetTitleBarColor() const
    {
        return GetColorForValueType( GetOutputValueType() );
    }

    Color FlowToolsNode::GetPinColor( NodeGraph::Pin const& pin ) const
    {
        return GetColorForValueType( GetValueTypeForPinType( pin.m_type ) );
    }

    void FlowToolsNode::DrawContextMenuOptions( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, NodeGraph::Pin* pPin )
    {
        // Draw runtime node index
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            if ( ImGui::BeginMenu( EE_ICON_INFORMATION_OUTLINE" Runtime Node ID" ) )
            {
                int16_t runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
                if ( runtimeNodeIdx != InvalidIndex )
                {
                    InlineString const label( InlineString::CtorSprintf(), "Runtime Index: %d", runtimeNodeIdx );
                    if ( ImGui::MenuItem( label.c_str() ) )
                    {
                        InlineString const value( InlineString::CtorSprintf(), "%d", runtimeNodeIdx );
                        ImGui::SetClipboardText( value.c_str() );
                    }
                }

                ImGui::EndMenu();
            }
        }
    }
}