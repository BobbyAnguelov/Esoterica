#include "VisualGraph_View.h"
#include "System/Imgui/ImguiStyle.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    constexpr static float const g_transitionArrowWidth = 3.0f;
    constexpr static float const g_transitionArrowOffset = 4.0f;
    constexpr static float const g_spacingBetweenTitleAndNodeContents = 6.0f;
    constexpr static float const g_pinRadius = 5.0f;
    constexpr static float const g_pinSelectionExtraRadius = 10.0f;
    constexpr static float const g_spacingBetweenInputOutputPins = 16.0f;

    //-------------------------------------------------------------------------

    static void GetConnectionPointsBetweenStateMachineNodes( ImRect const& startNodeRect, ImRect const& endNodeRect, ImVec2& startPoint, ImVec2& endPoint )
    {
        startPoint = startNodeRect.GetCenter();
        endPoint = endNodeRect.GetCenter();
        ImVec2 const midPoint = startPoint + ( ( endPoint - startPoint ) / 2 );

        startPoint = ImGuiX::GetClosestPointOnRectBorder( startNodeRect, midPoint );
        endPoint = ImGuiX::GetClosestPointOnRectBorder( endNodeRect, midPoint );
    }

    static bool IsHoveredOverCurve( ImVec2 const& p1, ImVec2 const& p2, ImVec2 const& p3, ImVec2 const& p4, ImVec2 const& mousePosition, float hoverThreshold )
    {
        ImVec2 const min = ImVec2( ImMin( p1.x, p4.x ), ImMin( p1.y, p4.y ) );
        ImVec2 const max = ImVec2( ImMax( p1.x, p4.x ), ImMax( p1.y, p4.y ) );

        ImRect rect( min, max );
        rect.Add( p2 );
        rect.Add( p3 );
        rect.Expand( ImVec2( hoverThreshold, hoverThreshold ) );

        if ( rect.Contains( mousePosition ) )
        {
            ImVec2 const closetPointToCurve = ImBezierCubicClosestPointCasteljau( p1, p2, p3, p4, mousePosition, ImGui::GetStyle().CurveTessellationTol );
            if ( Vector( closetPointToCurve ).GetDistance2( mousePosition ) < hoverThreshold )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    void GraphView::SetGraphToView( UserContext* pUserContext, BaseGraph* pGraph, bool tryMaintainSelection )
    {
        if ( m_pGraph == pGraph )
        {
            return;
        }

        TVector<SelectedNode> oldSelection;
        oldSelection.swap( m_selectedNodes );

        ResetInternalState();
        m_pGraph = pGraph;
        if ( m_pGraph != nullptr )
        {
            m_pViewOffset = &m_pGraph->m_viewOffset;
            m_pGraph->OnShowGraph( pUserContext );
        }

        if ( tryMaintainSelection )
        {
            for ( auto const& oldSelectedNode : oldSelection )
            {
                auto pFoundNode = GetViewedGraph()->FindNode( oldSelectedNode.m_nodeID );
                if ( pFoundNode != nullptr )
                {
                    AddToSelection( pFoundNode );
                }
            }
        }
    }

    void GraphView::ResetInternalState()
    {
        m_pViewOffset = nullptr;
        m_hasFocus = false;
        m_selectionChanged = false;
        m_pHoveredNode = nullptr;
        m_pHoveredPin = nullptr;
        m_contextMenuState.Reset();
        m_dragState.Reset();
        ClearSelection();
    }

    //-------------------------------------------------------------------------

    bool GraphView::BeginDrawCanvas( float childHeightOverride )
    {
        m_selectionChanged = false;
        m_isViewHovered = false;
        m_canvasSize = ImVec2( 0, 0 );

        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        ImGui::PushStyleColor( ImGuiCol_ChildBg, ImGuiX::Style::s_gridBackgroundColor.Value );
        bool const childVisible = ImGui::BeginChild( "GraphCanvas", ImVec2( 0.f, childHeightOverride ), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse );
        if ( childVisible )
        {
            auto pWindow = ImGui::GetCurrentWindow();
            auto pDrawList = ImGui::GetWindowDrawList();

            m_hasFocus = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows );
            m_isViewHovered = ImGui::IsWindowHovered();
            m_canvasSize = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();
            pDrawList->ChannelsSplit( 4 );

            // Background
            //-------------------------------------------------------------------------

            pDrawList->ChannelsSetCurrent( (uint8_t) DrawChannel::Background );

            ImRect const windowRect = pWindow->Rect();
            ImVec2 const windowTL = windowRect.GetTL();
            float const canvasWidth = windowRect.GetWidth();
            float const canvasHeight = windowRect.GetHeight();

            // Draw Grid
            for ( float x = Math::FModF( 0, VisualSettings::s_gridSpacing ); x < canvasWidth; x += VisualSettings::s_gridSpacing )
            {
                pDrawList->AddLine( windowTL + ImVec2( x, 0.0f ), windowTL + ImVec2( x, canvasHeight ), ImGuiX::Style::s_gridLineColor );
            }

            for ( float y = Math::FModF( 0, VisualSettings::s_gridSpacing ); y < canvasHeight; y += VisualSettings::s_gridSpacing )
            {
                pDrawList->AddLine( windowTL + ImVec2( 0.0f, y ), windowTL + ImVec2( canvasWidth, y ), ImGuiX::Style::s_gridLineColor );
            }

            // Draw title
            auto pViewedGraph = GetViewedGraph();
            if ( pViewedGraph != nullptr )
            {
                ImGuiX::ScopedFont font( ImGuiX::Font::LargeBold );
                pDrawList->AddText( windowRect.Min + VisualSettings::s_graphTitleMargin, VisualSettings::s_graphTitleColor, pViewedGraph->GetTitle() );
            }
            else
            {
                ImGuiX::ScopedFont font( ImGuiX::Font::LargeBold );
                pDrawList->AddText( windowRect.Min + VisualSettings::s_graphTitleMargin, ImGuiX::Style::s_colorTextDisabled, "Nothing to Show" );
            }
        }

        return childVisible;
    }

    void GraphView::EndDrawCanvas()
    {
        ImGui::GetWindowDrawList()->ChannelsMerge();

        //-------------------------------------------------------------------------

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopID();
    }

    void GraphView::DrawStateMachineNodeTitle( DrawContext const& ctx, SM::Node* pNode, ImVec2& newNodeSize )
    {
        EE_ASSERT( pNode != nullptr );

        ImGui::BeginGroup();
        ImGuiX::ScopedFont fontOverride( ImGuiX::Font::Tiny );
        if ( pNode->GetIcon() != nullptr )
        {
            ImGui::Text( pNode->GetIcon() );
            ImGui::SameLine( 0, 2 );
        }
        ImGui::Text( pNode->GetName() );
        ImGui::EndGroup();

        newNodeSize = pNode->m_titleRectSize = ImGui::GetItemRectSize();

        ImGui::SetCursorPosY( ImGui::GetCursorPos().y + g_spacingBetweenTitleAndNodeContents );
        newNodeSize.y += g_spacingBetweenTitleAndNodeContents;
    }

    void GraphView::DrawStateMachineNodeBackground( DrawContext const& ctx, UserContext* pUserContext, SM::Node* pNode, ImVec2& newNodeSize )
    {
        EE_ASSERT( pNode != nullptr );

        ImVec2 const windowNodePosition = ctx.m_windowRect.Min - ctx.m_viewOffset + pNode->m_canvasPosition;
        ImVec2 const nodeMargin = pNode->GetNodeMargin();
        ImVec2 const rectMin = windowNodePosition - nodeMargin;
        ImVec2 const rectMax = windowNodePosition + pNode->m_size + nodeMargin;
        ImVec2 const rectTitleBarMax = windowNodePosition + ImVec2( newNodeSize.x, pNode->m_titleRectSize.m_y ) + nodeMargin;

        // Colors
        //-------------------------------------------------------------------------

        ImColor const nodeBackgroundColor( VisualSettings::s_genericNodeBackgroundColor );
        ImColor nodeTitleColor( pNode->GetTitleBarColor() );

        auto pStateMachineGraph = GetStateMachineGraph();
        if ( pNode->m_ID == pStateMachineGraph->m_entryStateID )
        {
            nodeTitleColor = ImGuiX::ConvertColor( Colors::Green );
        }

        NodeVisualState visualState = pNode->IsActive( pUserContext ) ? NodeVisualState::Active : NodeVisualState::None;
        if ( IsNodeSelected( pNode ) )
        {
            visualState = NodeVisualState::Selected;
        }
        else if ( pNode->m_isHovered )
        {
            visualState = NodeVisualState::Hovered;
        }

        // Draw
        //-------------------------------------------------------------------------

        if ( IsOfType<SM::State>( pNode ) )
        {
            ctx.m_pDrawList->AddRectFilled( rectMin, rectMax, nodeBackgroundColor, 3, ImDrawFlags_RoundCornersAll );
            ctx.m_pDrawList->AddRectFilled( rectMin, rectTitleBarMax, nodeTitleColor, 3, ImDrawFlags_RoundCornersTop );
            ctx.m_pDrawList->AddRect( rectMin, rectMax, pNode->GetNodeBorderColor( ctx, pUserContext, visualState ), 3, ImDrawFlags_RoundCornersAll, VisualSettings::s_nodeSelectionBorder );
        }
        else // Non-state node
        {
            ctx.m_pDrawList->AddRectFilled( rectMin, rectMax, nodeBackgroundColor, 3 );
            ctx.m_pDrawList->AddRect( rectMin, rectMax, pNode->GetNodeBorderColor( ctx, pUserContext, visualState ), 3, ImDrawFlags_RoundCornersAll, VisualSettings::s_nodeSelectionBorder );
        }
    }

    void GraphView::DrawStateMachineNode( DrawContext const& ctx, UserContext* pUserContext, SM::Node* pNode )
    {
        EE_ASSERT( pNode != nullptr );

        if ( !pNode->IsVisible() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        ImGui::PushID( pNode );

        //-------------------------------------------------------------------------
        // Draw contents
        //-------------------------------------------------------------------------

        ctx.m_pDrawList->ChannelsSetCurrent( 2 );

        ImVec2 newNodeSize( 0, 0 );
        ImGui::SetCursorPos( ImVec2( pNode->m_canvasPosition ) - ctx.m_viewOffset );
        ImGui::BeginGroup();
        {
            DrawStateMachineNodeTitle( ctx, pNode, newNodeSize );

            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
                ImGui::BeginGroup();
                pNode->DrawExtraControls( ctx, pUserContext );
                ImGui::EndGroup();
            }

            ImVec2 const extraControlsRectSize = ImGui::GetItemRectSize();
            newNodeSize.x = Math::Max( newNodeSize.x, extraControlsRectSize.x );
            newNodeSize.y += extraControlsRectSize.y;
        }
        ImGui::EndGroup();

        // Draw background
        //-------------------------------------------------------------------------

        ctx.m_pDrawList->ChannelsSetCurrent( 1 );
        DrawStateMachineNodeBackground( ctx, pUserContext, pNode, newNodeSize );
        pNode->m_size = newNodeSize;

        //-------------------------------------------------------------------------

        ImGui::PopID();

        //-------------------------------------------------------------------------

        pNode->m_isHovered = m_isViewHovered && pNode->GetCanvasRect().Contains( ctx.m_mouseCanvasPos );
    }

    void GraphView::DrawStateMachineTransitionConduit( DrawContext const& ctx, UserContext* pUserContext, SM::TransitionConduit* pTransition )
    {
        EE_ASSERT( pTransition != nullptr );
        EE_ASSERT( pTransition->m_startStateID.IsValid() && pTransition->m_endStateID.IsValid() );

        auto pStartState = Cast<SM::State>( m_pGraph->FindNode( pTransition->m_startStateID ) );
        auto pEndState = Cast<SM::State>( m_pGraph->FindNode( pTransition->m_endStateID ) );

        auto const startNodeRect = pStartState->GetCanvasRect();
        auto const endNodeRect = pEndState->GetCanvasRect();

        // Calculate transition arrow start and end points
        //-------------------------------------------------------------------------

        ImVec2 startPoint, endPoint;
        GetConnectionPointsBetweenStateMachineNodes( startNodeRect, endNodeRect, startPoint, endPoint );

        Vector const arrowDir = Vector( endPoint - startPoint ).GetNormalized2();
        Vector const orthogonalDir = arrowDir.Orthogonal2D();
        ImVec2 const offset = ( orthogonalDir * g_transitionArrowOffset ).ToFloat2();

        startPoint += offset;
        endPoint += offset;

        // Update hover state and visual state
        //-------------------------------------------------------------------------

        pTransition->m_isHovered = false;
        NodeVisualState visualState = pTransition->IsActive( pUserContext ) ? NodeVisualState::Active : NodeVisualState::None;

        ImVec2 const closestPointOnTransitionToMouse = ImLineClosestPoint( startPoint, endPoint, ctx.m_mouseCanvasPos );
        if ( m_isViewHovered && ImLengthSqr( ctx.m_mouseCanvasPos - closestPointOnTransitionToMouse ) < Math::Pow( VisualSettings::s_connectionSelectionExtraRadius, 2 ) )
        {
            visualState = NodeVisualState::Hovered;
            pTransition->m_isHovered = true;
        }

        if ( IsNodeSelected( pTransition ) )
        {
            visualState = NodeVisualState::Selected;
        }

        // Draw
        //-------------------------------------------------------------------------

        ImColor const transitionColor = pTransition->GetNodeBorderColor( ctx, pUserContext, visualState );
        ImGuiX::DrawArrow( ctx.m_pDrawList, ctx.CanvasPositionToScreenPosition( startPoint ), ctx.CanvasPositionToScreenPosition( endPoint ), transitionColor, g_transitionArrowWidth );

        // Update transition position and size
        //-------------------------------------------------------------------------

        ImVec2 const min( Math::Min( startPoint.x, endPoint.x ), Math::Min( startPoint.y, endPoint.y ) );
        ImVec2 const max( Math::Max( startPoint.x, endPoint.x ), Math::Max( startPoint.y, endPoint.y ) );
        pTransition->m_canvasPosition = min;
        pTransition->m_size = max - min;
    }

    //-------------------------------------------------------------------------

    void GraphView::DrawFlowNodeTitle( DrawContext const& ctx, Flow::Node* pNode, ImVec2& newNodeSize )
    {
        EE_ASSERT( pNode != nullptr );

        ImGui::BeginGroup();
        ImGuiX::ScopedFont fontOverride( ImGuiX::Font::Tiny, ImColor( VisualSettings::s_genericNodeTitleTextColor ) );
        if ( pNode->GetIcon() != nullptr )
        {
            ImGui::Text( pNode->GetIcon() );
            ImGui::SameLine( 0, 2 );
        }
        ImGui::Text( pNode->GetName() );
        ImGui::EndGroup();

        newNodeSize = pNode->m_titleRectSize = ImGui::GetItemRectSize();

        ImGui::SetCursorPosY( ImGui::GetCursorPos().y + g_spacingBetweenTitleAndNodeContents );
        newNodeSize.y += g_spacingBetweenTitleAndNodeContents;
    }

    void GraphView::DrawFlowNodePins( DrawContext const& ctx, UserContext* pUserContext, Flow::Node* pNode, ImVec2& newNodeSize )
    {
        EE_ASSERT( pNode != nullptr );

        pNode->m_pHoveredPin = nullptr;

        //-------------------------------------------------------------------------

        ImVec2 const nodeMargin = pNode->GetNodeMargin();

        //-------------------------------------------------------------------------

        ImVec2 pinRectSize( 0, 0 );
        int32_t const numPinRows = Math::Max( pNode->GetNumInputPins(), pNode->GetNumOutputPins() );
        for ( auto i = 0; i < numPinRows; i++ )
        {
            bool const hasInputPin = i < pNode->m_inputPins.size();
            bool const hasOutputPin = i < pNode->m_outputPins.size();
            float estimatedSpacingBetweenPins = 0.0f;

            // Input Pin
            //-------------------------------------------------------------------------

            if ( hasInputPin )
            {
                ImGui::BeginGroup();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( pNode->m_inputPins[i].m_name.c_str() );
                if ( pNode->DrawPinControls( pUserContext, pNode->m_inputPins[i] ) )
                {
                    ImGui::SameLine( 0, 0 );
                }
                ImGui::EndGroup();

                // Record estimate pin label rect width and update node size
                pNode->m_inputPins[i].m_size = ImGui::GetItemRectSize();

                // Check hover state
                ImRect const rect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
                ImColor pinColor = pNode->GetPinColor( pNode->m_inputPins[i] );
                pNode->m_inputPins[i].m_screenPosition = rect.Min + ImVec2( -nodeMargin.x, rect.GetHeight() / 2 );
                bool const isPinHovered = Vector( pNode->m_inputPins[i].m_screenPosition ).GetDistance2( ImGui::GetMousePos() ) < ( g_pinRadius + g_pinSelectionExtraRadius );
                if ( isPinHovered )
                {
                    pNode->m_pHoveredPin = &pNode->m_inputPins[i];
                    pinColor = ImGuiX::AdjustColorBrightness( pinColor, 1.55f );
                }

                // Draw pin
                ctx.m_pDrawList->AddCircleFilled( pNode->m_inputPins[i].m_screenPosition, g_pinRadius, pinColor );
            }

            // Update Cursor Position
            //-------------------------------------------------------------------------

            if ( hasOutputPin )
            {
                // Ensure that we correctly move the cursor to the next line for the output pins
                if ( !hasInputPin )
                {
                    ImGui::NewLine();
                }

                float const inputPinWidth = hasInputPin ? pNode->m_inputPins[i].GetWidth() : 0;
                estimatedSpacingBetweenPins = pNode->GetWidth() - inputPinWidth - pNode->m_outputPins[i].GetWidth();
                estimatedSpacingBetweenPins = Math::Max( estimatedSpacingBetweenPins, g_spacingBetweenInputOutputPins );
                ImGui::SameLine( 0, estimatedSpacingBetweenPins );
            }

            // Output Pin
            //-------------------------------------------------------------------------

            if ( hasOutputPin )
            {
                ImGui::BeginGroup();
                ImGui::AlignTextToFramePadding();
                if ( pNode->DrawPinControls( pUserContext, pNode->m_outputPins[i] ) )
                {
                    ImGui::SameLine( 0, 0 );
                }
                ImGui::Text( pNode->m_outputPins[i].m_name.c_str() );
                ImGui::EndGroup();

                // Get the size of the above group
                pNode->m_outputPins[i].m_size = ImGui::GetItemRectSize();

                // Check hover state
                ImRect const rect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
                ImColor pinColor = pNode->GetPinColor( pNode->m_outputPins[i] );
                pNode->m_outputPins[i].m_screenPosition = rect.Max + ImVec2( nodeMargin.x, -rect.GetHeight() / 2 );
                bool const isPinHovered = Vector( pNode->m_outputPins[i].m_screenPosition ).GetDistance2( ImGui::GetMousePos() ) < ( g_pinRadius + g_pinSelectionExtraRadius );
                if ( isPinHovered )
                {
                    pNode->m_pHoveredPin = &pNode->m_outputPins[i];
                    pinColor = ImGuiX::AdjustColorBrightness( pinColor, 1.25f );
                }

                ctx.m_pDrawList->AddCircleFilled( pNode->m_outputPins[i].m_screenPosition, g_pinRadius, pinColor );
            }

            // Update Rect Size
            //-------------------------------------------------------------------------

            ImVec2 pinRowRectSize( 0, 0 );

            if ( hasInputPin )
            {
                pinRowRectSize.x += pNode->m_inputPins[i].m_size.m_x;
                pinRowRectSize.y = pNode->m_inputPins[i].m_size.m_y;
            }

            if ( hasOutputPin )
            {
                pinRowRectSize.x += pNode->m_outputPins[i].m_size.m_x;
                pinRowRectSize.y = Math::Max( pinRowRectSize.y, pNode->m_outputPins[i].m_size.m_y );
            }

            pinRowRectSize.x += estimatedSpacingBetweenPins;
            pinRectSize.x = Math::Max( pinRectSize.x, pinRowRectSize.x );
            pinRectSize.y += pinRowRectSize.y;
        }

        // Update node size with the max width of all output pins
        //-------------------------------------------------------------------------

        newNodeSize.x = Math::Max( newNodeSize.x, pinRectSize.x );
        newNodeSize.y += pinRectSize.y;
    }

    void GraphView::DrawFlowNodeBackground( DrawContext const& ctx, UserContext* pUserContext, Flow::Node* pNode, ImVec2& newNodeSize )
    {
        EE_ASSERT( pNode != nullptr );

        ImVec2 const windowNodePosition = ctx.m_windowRect.Min - ctx.m_viewOffset + pNode->m_canvasPosition;
        ImVec2 const nodeMargin = pNode->GetNodeMargin();
        ImVec2 const rectMin = windowNodePosition - nodeMargin;
        ImVec2 const rectMax = windowNodePosition + newNodeSize + nodeMargin;
        ImVec2 const rectTitleBarMax = windowNodePosition + ImVec2( newNodeSize.x, pNode->m_titleRectSize.m_y ) + nodeMargin;

        // Draw
        //-------------------------------------------------------------------------

        ctx.m_pDrawList->AddRectFilled( rectMin, rectMax, VisualSettings::s_genericNodeBackgroundColor, 3, ImDrawFlags_RoundCornersAll );
        ctx.m_pDrawList->AddRectFilled( rectMin, rectTitleBarMax, pNode->GetTitleBarColor(), 3, ImDrawFlags_RoundCornersTop );

        NodeVisualState visualState = pNode->IsActive( pUserContext ) ? NodeVisualState::Active : NodeVisualState::None;

        if ( IsNodeSelected( pNode ) )
        {
            visualState = NodeVisualState::Selected;
        }
        else if ( pNode->m_isHovered && pNode->m_pHoveredPin == nullptr )
        {
            visualState = NodeVisualState::Hovered;
        }

        ctx.m_pDrawList->AddRect( rectMin, rectMax, pNode->GetNodeBorderColor( ctx, pUserContext, visualState ), 3, ImDrawFlags_RoundCornersAll, VisualSettings::s_nodeSelectionBorder );
    }

    void GraphView::DrawFlowNode( DrawContext const& ctx, UserContext* pUserContext, Flow::Node* pNode )
    {
        EE_ASSERT( pNode != nullptr );

        if ( !pNode->IsVisible() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        ImGui::PushID( pNode );

        //-------------------------------------------------------------------------
        // Draw Node
        //-------------------------------------------------------------------------

        ctx.m_pDrawList->ChannelsSetCurrent( (uint8_t) DrawChannel::NodeForeground );

        ImVec2 newNodeSize( 0, 0 );
        ImGui::SetCursorPos( ImVec2( pNode->m_canvasPosition ) - ctx.m_viewOffset );
        ImGui::BeginGroup();
        {
            DrawFlowNodeTitle( ctx, pNode, newNodeSize );

            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );

                DrawFlowNodePins( ctx, pUserContext, pNode, newNodeSize );

                //-------------------------------------------------------------------------

                ImGui::BeginGroup();
                pNode->DrawExtraControls( ctx, pUserContext );
                ImGui::EndGroup();
            }

            ImVec2 const extraControlsRectSize = ImGui::GetItemRectSize();
            newNodeSize.x = Math::Max( newNodeSize.x, extraControlsRectSize.x );
            newNodeSize.y += extraControlsRectSize.y;
        }
        ImGui::EndGroup();

        // Draw background
        //-------------------------------------------------------------------------

        ctx.m_pDrawList->ChannelsSetCurrent( (uint8_t) DrawChannel::NodeBackground );
        DrawFlowNodeBackground( ctx, pUserContext, pNode, newNodeSize );
        pNode->m_size = newNodeSize;

        //-------------------------------------------------------------------------

        ImGui::PopID();

        //-------------------------------------------------------------------------

        pNode->m_isHovered = m_isViewHovered && pNode->GetCanvasRect().Contains( ctx.m_mouseCanvasPos ) || pNode->m_pHoveredPin != nullptr;
    }

    //-------------------------------------------------------------------------

    void GraphView::UpdateAndDraw( TypeSystem::TypeRegistry const& typeRegistry, UserContext* pUserContext, float childHeightOverride )
    {
        EE_ASSERT( pUserContext != nullptr );

        DrawContext drawingContext;

        if ( BeginDrawCanvas( childHeightOverride ) && m_pGraph != nullptr )
        {
            auto pWindow = ImGui::GetCurrentWindow();
            ImVec2 const mousePos = ImGui::GetMousePos();

            drawingContext.m_pDrawList = ImGui::GetWindowDrawList();
            drawingContext.m_viewOffset = *m_pViewOffset;
            drawingContext.m_windowRect = pWindow->Rect();
            drawingContext.m_canvasVisibleRect = ImRect( *m_pViewOffset, ImVec2( *m_pViewOffset ) + drawingContext.m_windowRect.GetSize());
            drawingContext.m_mouseScreenPos = ImGui::GetMousePos();
            drawingContext.m_mouseCanvasPos = drawingContext.ScreenPositionToCanvasPosition( drawingContext.m_mouseScreenPos );

            //-------------------------------------------------------------------------
            // Draw Graph
            //-------------------------------------------------------------------------

            m_pHoveredNode = nullptr;
            m_pHoveredPin = nullptr;

            // State Machine Graph
            if ( IsViewingStateMachineGraph() )
            {
                for ( auto pNode : m_pGraph->m_nodes )
                {
                    // If we have a rect width, perform culling
                    auto pStateMachineNode = Cast<SM::Node>( pNode );

                    if ( auto pTransition = TryCast<SM::TransitionConduit>( pNode ) )
                    {
                        DrawStateMachineTransitionConduit( drawingContext, pUserContext, pTransition );
                    }
                    else
                    {
                        DrawStateMachineNode( drawingContext, pUserContext, pStateMachineNode );
                    }

                    if ( pStateMachineNode->m_isHovered )
                    {
                        m_pHoveredNode = pStateMachineNode;
                    }
                }
            }
            else // Flow Graph
            {
                auto pFlowGraph = GetFlowGraph();

                // Draw Nodes
                //-------------------------------------------------------------------------

                for ( auto pNode : m_pGraph->m_nodes )
                {
                    auto pFlowNode = Cast<Flow::Node>( pNode );
                    DrawFlowNode( drawingContext, pUserContext, pFlowNode );

                    if ( pFlowNode->m_isHovered )
                    {
                        m_pHoveredNode = pFlowNode;
                        m_pHoveredPin = pFlowNode->m_pHoveredPin;
                    }
                }

                // Draw connections
                //-------------------------------------------------------------------------

                drawingContext.m_pDrawList->ChannelsSetCurrent( (uint8_t) DrawChannel::Connections );

                m_hoveredConnectionID.Clear();
                for ( auto const& connection : pFlowGraph->m_connections )
                {
                    auto pStartPin = connection.m_pStartNode->GetOutputPin( connection.m_startPinID );
                    auto pEndPin = connection.m_pEndNode->GetInputPin( connection.m_endPinID );

                    ImColor connectionColor = connection.m_pStartNode->GetPinColor( *pStartPin );

                    bool const invertOrder = pStartPin->m_screenPosition.m_x > pEndPin->m_screenPosition.m_x;
                    ImVec2 const& p1 = invertOrder ? pEndPin->m_screenPosition : pStartPin->m_screenPosition;
                    ImVec2 const& p4 = invertOrder ? pStartPin->m_screenPosition : pEndPin->m_screenPosition;
                    ImVec2 const p2 = p1 + ImVec2( 50, 0 );
                    ImVec2 const p3 = p4 + ImVec2( -50, 0 );

                    if ( m_hasFocus && IsHoveredOverCurve( p1, p2, p3, p4, drawingContext.m_mouseScreenPos, VisualSettings::s_connectionSelectionExtraRadius ) )
                    {
                        m_hoveredConnectionID = connection.m_ID;
                        connectionColor = ImColor( VisualSettings::s_connectionColorHovered );
                    }

                    drawingContext.m_pDrawList->AddBezierCubic( p1, p2, p3, p4, connectionColor, 3.0f );
                }
            }

            drawingContext.m_pDrawList->ChannelsMerge();

            //-------------------------------------------------------------------------
            // Extra
            //-------------------------------------------------------------------------

            m_pGraph->DrawExtraInformation( drawingContext, pUserContext );

            //-------------------------------------------------------------------------

            HandleContextMenu( drawingContext, pUserContext );
            HandleInput( typeRegistry, drawingContext, pUserContext );
            DrawDialogs();
        }

        EndDrawCanvas();

        //-------------------------------------------------------------------------

        if ( m_pGraph != nullptr )
        {
            if ( ImGui::BeginDragDropTarget() )
            {
                m_pGraph->HandleDragAndDrop( pUserContext, drawingContext.m_mouseCanvasPos );
                ImGui::EndDragDropTarget();
            }
        }
    }

    void GraphView::ResetView()
    {
        if ( m_pGraph == nullptr || m_pGraph->m_nodes.empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto pMostSignificantNode = m_pGraph->GetMostSignificantNode();
        if ( pMostSignificantNode != nullptr )
        {
            CenterView( pMostSignificantNode );
        }
        else
        {
            int32_t const numNodes = (int32_t) m_pGraph->m_nodes.size();
            ImRect totalRect = ImRect( m_pGraph->m_nodes[0]->GetCanvasPosition(), ImVec2( m_pGraph->m_nodes[0]->GetCanvasPosition() ) + m_pGraph->m_nodes[0]->GetSize() );
            for ( int32_t i = 1; i < numNodes; i++ )
            {
                auto pNode = m_pGraph->m_nodes[i];
                ImRect const nodeRect( pNode->GetCanvasPosition(), ImVec2( pNode->GetCanvasPosition() ) + pNode->GetSize() );
                totalRect.Add( nodeRect );
            }

            *m_pViewOffset = totalRect.GetCenter() - ( m_canvasSize / 2 );
        }
    }

    void GraphView::CenterView( BaseNode const* pNode )
    {
        EE_ASSERT( m_pGraph != nullptr );
        EE_ASSERT( pNode != nullptr && m_pGraph->FindNode( pNode->GetID() ) != nullptr );

        ImVec2 const nodeHalfSize = ( pNode->GetSize() / 2 );
        ImVec2 const nodeCenter = ImVec2( pNode->GetCanvasPosition() ) + nodeHalfSize;
        *m_pViewOffset = nodeCenter - ( m_canvasSize / 2 );
    }

    //-------------------------------------------------------------------------

    void GraphView::SelectNode( BaseNode const* pNode )
    {
        EE_ASSERT( GetViewedGraph()->FindNode( pNode->GetID() ) != nullptr );
        ClearSelection();
        AddToSelection( const_cast<BaseNode*>( pNode ) );
    }

    void GraphView::ClearSelection()
    {
        TVector<SelectedNode> oldSelection;
        oldSelection.swap( m_selectedNodes );
        OnSelectionChangedInternal( oldSelection, m_selectedNodes );
    }

    void GraphView::UpdateSelection( BaseNode* pNewSelectedNode )
    {
        EE_ASSERT( pNewSelectedNode != nullptr );

        // Avoid calling the notification if the selection hasn't changed
        if ( m_selectedNodes.size() == 1 && m_selectedNodes[0].m_pNode == pNewSelectedNode )
        {
            return;
        }

        //-------------------------------------------------------------------------

        TVector<SelectedNode> oldSelection;
        oldSelection.swap( m_selectedNodes );
        m_selectedNodes.emplace_back( pNewSelectedNode );
        OnSelectionChangedInternal( oldSelection, m_selectedNodes );
    }

    void GraphView::UpdateSelection( TVector<SelectedNode>&& newSelection )
    {
        for ( auto& selectedNode : newSelection )
        {
            EE_ASSERT( selectedNode.m_pNode != nullptr );
        }

        TVector<SelectedNode> oldSelection;
        oldSelection.swap( m_selectedNodes );
        m_selectedNodes.swap( newSelection );
        OnSelectionChangedInternal( oldSelection, m_selectedNodes );
    }

    void GraphView::AddToSelection( BaseNode* pNodeToAdd )
    {
        EE_ASSERT( pNodeToAdd != nullptr );
        EE_ASSERT( !IsNodeSelected( pNodeToAdd ) );

        TVector<SelectedNode> oldSelection;
        oldSelection = m_selectedNodes;
        m_selectedNodes.emplace_back( pNodeToAdd );
        OnSelectionChangedInternal( oldSelection, m_selectedNodes );
    }

    void GraphView::RemoveFromSelection( BaseNode* pNodeToRemove )
    {
        EE_ASSERT( pNodeToRemove != nullptr );
        EE_ASSERT( IsNodeSelected( pNodeToRemove ) );

        TVector<SelectedNode> oldSelection = m_selectedNodes;
        m_selectedNodes.erase_first( pNodeToRemove );
        OnSelectionChangedInternal( oldSelection, m_selectedNodes );
    }

    void GraphView::DestroySelectedNodes()
    {
        auto pGraph = GetViewedGraph();

        ScopedGraphModification sgm( pGraph );

        // Exclude any state machine transitions, as we will end up double deleting them since they are removed if the state is removed
        if ( IsViewingStateMachineGraph() )
        {
            for ( int32_t i = (int32_t) m_selectedNodes.size() - 1; i >= 0; i-- )
            {
                if ( auto pConduit = TryCast<SM::TransitionConduit>( m_selectedNodes[i].m_pNode ) )
                {
                    m_selectedNodes.erase_unsorted( m_selectedNodes.begin() + i );
                }
            }
        }

        // Delete selected nodes
        for ( auto const& selectedNode : m_selectedNodes )
        {
            if ( selectedNode.m_pNode->IsDestroyable() )
            {
                pGraph->DestroyNode( selectedNode.m_nodeID );
            }
        }

        ClearSelection();
    }

    void GraphView::CopySelectedNodes( TypeSystem::TypeRegistry const& typeRegistry )
    {
        if ( m_selectedNodes.empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        Serialization::JsonArchiveWriter archive;
        auto pWriter = archive.GetWriter();

        pWriter->StartObject();

        // Copy Nodes
        //-------------------------------------------------------------------------

        pWriter->Key( s_copiedNodesKey );
        pWriter->StartArray();

        TInlineVector<UUID, 10> copiedNodes;
        for ( auto const& selectedNode : m_selectedNodes )
        {
            // Do not copy any selected conduits, as conduits are automatically copied!!!
            if ( auto pConduit = TryCast<SM::TransitionConduit>( selectedNode.m_pNode ) )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            if ( selectedNode.m_pNode->IsUserCreatable() )
            {
                selectedNode.m_pNode->Serialize( typeRegistry, *pWriter );
                copiedNodes.emplace_back( selectedNode.m_pNode->GetID() );
            }
        }

        // Ensure that all transitions between copied states are also copied
        if ( IsViewingStateMachineGraph() )
        {
            for ( auto pNode : m_pGraph->m_nodes )
            {
                if ( auto pConduit = TryCast<SM::TransitionConduit>( pNode ) )
                {
                    // If the conduit is already copied, then do nothing
                    if ( VectorContains( copiedNodes, pConduit->GetID() ) )
                    {
                        continue;
                    }

                    // If there exists a non-copied conduit between two copied states, then serialize it
                    if ( VectorContains( copiedNodes, pConduit->GetStartStateID() ) && VectorContains( copiedNodes, pConduit->GetEndStateID() ) )
                    {
                        pConduit->Serialize( typeRegistry, *pWriter );
                    }
                }
            }
        }

        pWriter->EndArray();

        // Serialize node connections
        //-------------------------------------------------------------------------

        if ( IsViewingFlowGraph() )
        {
            auto pFlowGraph = GetFlowGraph();

            pWriter->Key( s_copiedConnectionsKey );
            pWriter->StartArray();

            for ( auto const& connection : pFlowGraph->m_connections )
            {
                if ( VectorContains( copiedNodes, connection.m_pStartNode->GetID() ) && VectorContains( copiedNodes, connection.m_pEndNode->GetID() ) )
                {
                    pFlowGraph->SerializeConnection( *pWriter, connection );
                }
            }

            pWriter->EndArray();
        }

        //-------------------------------------------------------------------------

        pWriter->EndObject();

        //-------------------------------------------------------------------------

        ImGui::SetClipboardText( archive.GetStringBuffer().GetString() );
    }

    void GraphView::PasteNodes( TypeSystem::TypeRegistry const& typeRegistry, ImVec2 const& canvasPastePosition )
    {
        Serialization::JsonArchiveReader archive;
        if ( !archive.ReadFromString( ImGui::GetClipboardText() ) )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto& document = archive.GetDocument();

        auto copiedNodesArrayIter = document.FindMember( s_copiedNodesKey );
        if ( copiedNodesArrayIter == document.MemberEnd() )
        {
            return;
        }

        // Deserialize pasted nodes and regenerated IDs
        //-------------------------------------------------------------------------

        THashMap<UUID, UUID> IDMapping;
        TInlineVector<BaseNode*, 20> pastedNodes;
        for ( auto& nodeObjectValue : copiedNodesArrayIter->value.GetArray() )
        {
            auto pPastedNode = BaseNode::TryCreateNodeFromSerializedData( typeRegistry, nodeObjectValue, m_pGraph );

            if ( m_pGraph->CanCreateNode( pPastedNode->GetTypeInfo() ) )
            {
                pPastedNode->RegenerateIDs( IDMapping );
                pPastedNode->PostPaste();
                pastedNodes.emplace_back( pPastedNode );
            }
            else
            {
                pPastedNode->Shutdown();
                EE::Delete( pPastedNode );
            }
        }

        if ( pastedNodes.empty() )
        {
            return;
        }

        // Add nodes to the graph
        //-------------------------------------------------------------------------

        m_pGraph->BeginModification();

        for ( auto pPastedNode : pastedNodes )
        {
            m_pGraph->AddNode( pPastedNode );
        }

        // Serialize and fix connections
        //-------------------------------------------------------------------------

        if ( IsViewingFlowGraph() )
        {
            auto pFlowGraph = GetFlowGraph();
            auto copiedConnectionsArrayIter = document.FindMember( s_copiedConnectionsKey );
            if ( copiedConnectionsArrayIter != document.MemberEnd() )
            {
                for ( auto& connectionObjectValue : copiedConnectionsArrayIter->value.GetArray() )
                {
                    UUID startNodeID = UUID( connectionObjectValue[Flow::Connection::s_startNodeKey].GetString() );
                    UUID endNodeID = UUID( connectionObjectValue[Flow::Connection::s_endNodeKey].GetString() );
                    UUID startPinID = UUID( connectionObjectValue[Flow::Connection::s_startPinKey].GetString() );
                    UUID endPinID = UUID( connectionObjectValue[Flow::Connection::s_endPinKey].GetString() );

                    startNodeID = IDMapping.at( startNodeID );
                    endNodeID = IDMapping.at( endNodeID );
                    startPinID = IDMapping.at( startPinID );
                    endPinID = IDMapping.at( endPinID );

                    Flow::Connection connection;
                    connection.m_pStartNode = pFlowGraph->GetNode( startNodeID );
                    connection.m_pEndNode = pFlowGraph->GetNode( endNodeID );
                    connection.m_startPinID = startPinID;
                    connection.m_endPinID = endPinID;

                    // Only add valid connections (some nodes may have been excluded during the paste)
                    if ( connection.m_pStartNode != nullptr && connection.m_pEndNode != nullptr )
                    {
                        pFlowGraph->m_connections.emplace_back( connection );
                    }
                }
            }
        }
        else // State Machine
        {
            for ( auto pPastedNode : pastedNodes )
            {
                if ( auto pConduit = TryCast<SM::TransitionConduit>( pPastedNode ) )
                {
                    pConduit->m_startStateID = IDMapping.at( pConduit->m_startStateID );
                    pConduit->m_endStateID = IDMapping.at( pConduit->m_endStateID );
                }
            }
        }

        // Updated pasted node positions
        //-------------------------------------------------------------------------

        Float2 leftMostNodePosition( FLT_MAX, FLT_MAX );
        int32_t const numPastedNodes = (int32_t) pastedNodes.size();
        for ( int32_t i = 0; i < numPastedNodes; i++ )
        {
            if ( pastedNodes[i]->GetCanvasPosition().m_x < leftMostNodePosition.m_x )
            {
                leftMostNodePosition = pastedNodes[i]->GetCanvasPosition();
            }
        }

        for ( int32_t i = 0; i < numPastedNodes; i++ )
        {
            pastedNodes[i]->SetCanvasPosition( pastedNodes[i]->GetCanvasPosition() - leftMostNodePosition + Float2( canvasPastePosition ) );
        }

        // Notify graph that nodes were pasted
        //-------------------------------------------------------------------------

        m_pGraph->PostPasteNodes( pastedNodes );
        m_pGraph->EndModification();
    }

    void GraphView::BeginRenameNode( BaseNode* pNode )
    {
        EE_ASSERT( pNode != nullptr && pNode->IsRenameable() );
        EE::Printf( m_renameBuffer, 255, pNode->GetName() );
        m_pNodeBeingRenamed = pNode;
    }

    void GraphView::EndRenameNode( bool shouldUpdateNode )
    {
        EE_ASSERT( m_pNodeBeingRenamed != nullptr && m_pNodeBeingRenamed->IsRenameable() );

        if ( shouldUpdateNode )
        {
            // Set new name
            //-------------------------------------------------------------------------

            ScopedNodeModification const snm( m_pNodeBeingRenamed );
            String const uniqueName = m_pNodeBeingRenamed->GetParentGraph()->GetUniqueNameForRenameableNode( m_renameBuffer, m_pNodeBeingRenamed );
            m_pNodeBeingRenamed->SetName( uniqueName );
        }

        m_pNodeBeingRenamed = nullptr;
    }

    //-------------------------------------------------------------------------

    void GraphView::StartDraggingView( DrawContext const& ctx )
    {
        EE_ASSERT( m_dragState.m_mode == DragMode::None );
        m_dragState.m_mode = DragMode::View;
        m_dragState.m_startValue = *m_pViewOffset;
    }

    void GraphView::OnDragView( DrawContext const& ctx )
    {
        EE_ASSERT( m_dragState.m_mode == DragMode::View);

        if ( !ImGui::IsMouseDown( ImGuiMouseButton_Middle ) )
        {
            StopDraggingView( ctx );
            return;
        }

        //-------------------------------------------------------------------------

        ImVec2 const mouseDragDelta = ImGui::GetMouseDragDelta( ImGuiMouseButton_Middle );
        *m_pViewOffset = m_dragState.m_startValue - mouseDragDelta;
    }

    void GraphView::StopDraggingView( DrawContext const& ctx )
    {
        m_dragState.Reset();
    }

    //-------------------------------------------------------------------------

    void GraphView::StartDraggingSelection( DrawContext const& ctx )
    {
        EE_ASSERT( m_dragState.m_mode == DragMode::None );
        m_dragState.m_mode = DragMode::Selection;
        m_dragState.m_startValue = ImGui::GetMousePos();
    }

    void GraphView::OnDragSelection( DrawContext const& ctx )
    {
        if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
        {
            StopDraggingSelection( ctx );
            return;
        }

        ctx.m_pDrawList->AddRectFilled( m_dragState.m_startValue, ImGui::GetMousePos(), ImGuiX::Style::s_selectionBoxFillColor );
        ctx.m_pDrawList->AddRect( m_dragState.m_startValue, ImGui::GetMousePos(), ImGuiX::Style::s_selectionBoxOutlineColor );
    }

    void GraphView::StopDraggingSelection( DrawContext const& ctx )
    {
        ImVec2 const mousePos = ImGui::GetMousePos();
        ImVec2 const min( Math::Min( m_dragState.m_startValue.x, mousePos.x ), Math::Min( m_dragState.m_startValue.y, mousePos.y ) );
        ImVec2 const max( Math::Max( m_dragState.m_startValue.x, mousePos.x ), Math::Max( m_dragState.m_startValue.y, mousePos.y ) );
        ImRect const selectionWindowRect( min - ctx.m_windowRect.Min, max - ctx.m_windowRect.Min );

        TVector<SelectedNode> newSelection;
        for ( auto pNode : GetViewedGraph()->m_nodes )
        {
            ImRect const nodeWindowRect = pNode->GetWindowRect( *m_pViewOffset );
            if ( selectionWindowRect.Overlaps( nodeWindowRect ) )
            {
                newSelection.emplace_back( pNode );
            }
        }

        UpdateSelection( eastl::move( newSelection ) );
        m_dragState.Reset();
    }

    //-------------------------------------------------------------------------

    void GraphView::StartDraggingNode( DrawContext const& ctx )
    {
        EE_ASSERT( m_dragState.m_mode == DragMode::None );
        EE_ASSERT( m_pHoveredNode != nullptr );
        m_dragState.m_mode = DragMode::Node;
        m_dragState.m_pNode = m_pHoveredNode;
        m_dragState.m_startValue = m_pHoveredNode->GetCanvasPosition();

        GetViewedGraph()->BeginModification();
    }

    void GraphView::OnDragNode( DrawContext const& ctx )
    {
        EE_ASSERT( m_dragState.m_mode == DragMode::Node );

        if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
        {
            StopDraggingNode( ctx );
            return;
        }

        //-------------------------------------------------------------------------

        ImVec2 const mouseDragDelta = ImGui::GetMouseDragDelta( ImGuiMouseButton_Left );
        Float2 const frameDragDelta = mouseDragDelta - m_dragState.m_lastFrameDragDelta;
        m_dragState.m_lastFrameDragDelta = mouseDragDelta;

        for ( auto const& selectedNode : m_selectedNodes )
        {
            auto pNode = selectedNode.m_pNode;
            pNode->SetCanvasPosition( pNode->GetCanvasPosition() + frameDragDelta );
        }
    }

    void GraphView::StopDraggingNode( DrawContext const& ctx )
    {
        m_dragState.Reset();
        GetViewedGraph()->EndModification();
    }

    //-------------------------------------------------------------------------

    void GraphView::StartDraggingConnection( DrawContext const& ctx )
    {
        EE_ASSERT( m_dragState.m_mode == DragMode::None );
        m_dragState.m_mode = DragMode::Connection;
        m_dragState.m_pNode = m_pHoveredNode;

        if ( IsViewingStateMachineGraph() )
        {
            m_dragState.m_startValue = ctx.CanvasPositionToScreenPosition( m_pHoveredNode->GetCanvasPosition() );
        }
        else
        {
            m_dragState.m_startValue = m_pHoveredPin->m_screenPosition;
            m_dragState.m_pPin = m_pHoveredPin;
        }
    }

    void GraphView::OnDragConnection( DrawContext const& ctx )
    {
        EE_ASSERT( m_dragState.m_mode == DragMode::Connection );

        if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
        {
            StopDraggingConnection( ctx );
            return;
        }

        //-------------------------------------------------------------------------

        if ( IsViewingStateMachineGraph() )
        {
            auto pStateMachineGraph = GetStateMachineGraph();
            auto pEndState = TryCast<SM::State>( m_pHoveredNode );

            bool const isValidConnection = pEndState != nullptr && pStateMachineGraph->CanCreateTransitionConduit( Cast<SM::State>( m_dragState.m_pNode ), pEndState );
            ImColor const connectionColor = isValidConnection ? VisualSettings::s_connectionColorValid : VisualSettings::s_connectionColorInvalid;
            ImGuiX::DrawArrow( ctx.m_pDrawList, ctx.CanvasPositionToScreenPosition( m_dragState.m_pNode->GetCanvasRect().GetCenter() ), ctx.m_mouseScreenPos, connectionColor, g_transitionArrowWidth );
        }
        else
        {
            auto pFlowGraph = GetFlowGraph();
            auto pDraggedFlowNode = m_dragState.GetAsFlowNode();
            ImColor connectionColor = pDraggedFlowNode->GetPinColor( *m_dragState.m_pPin );

            if ( m_pHoveredPin != nullptr )
            {
                auto pHoveredFlowNode = Cast<Flow::Node>( m_pHoveredNode );

                // Trying to make an invalid connection to a pin with the same direction or the same node
                if ( m_pHoveredPin->m_direction == m_dragState.m_pPin->m_direction || m_pHoveredNode == m_dragState.m_pNode )
                {
                    connectionColor = VisualSettings::s_connectionColorInvalid;
                }
                else // Check connection validity
                {
                    if ( m_dragState.m_pPin->IsOutputPin() )
                    {
                        if ( !pFlowGraph->IsValidConnection( pDraggedFlowNode, m_dragState.m_pPin, pHoveredFlowNode, m_pHoveredPin ) )
                        {
                            connectionColor = VisualSettings::s_connectionColorInvalid;
                        }
                    }
                    else // The hovered pin is the output pin
                    {
                        if ( !pFlowGraph->IsValidConnection( pHoveredFlowNode, m_pHoveredPin, pDraggedFlowNode, m_dragState.m_pPin ) )
                        {
                            connectionColor = VisualSettings::s_connectionColorInvalid;
                        }
                    }
                }
            }

            //-------------------------------------------------------------------------

            bool const invertOrder = m_dragState.m_pPin->m_screenPosition.m_x > ctx.m_mouseScreenPos.x;
            auto const& p1 = invertOrder ? ctx.m_mouseScreenPos : m_dragState.m_pPin->m_screenPosition;
            auto const& p2 = invertOrder ? m_dragState.m_pPin->m_screenPosition : ctx.m_mouseScreenPos;
            ctx.m_pDrawList->AddBezierCubic( p1, p1 + Float2( +50, 0 ), p2 + Float2( -50, 0 ), p2, connectionColor, 3.0f );
        }
    }

    void GraphView::StopDraggingConnection( DrawContext const& ctx )
    {
        if ( IsViewingStateMachineGraph() )
        {
            auto pStateMachineGraph = GetStateMachineGraph();
            auto pStartState = Cast<SM::State>( m_dragState.m_pNode );
            auto pEndState = TryCast<SM::State>( m_pHoveredNode );
            if ( pEndState != nullptr && pStateMachineGraph->CanCreateTransitionConduit( pStartState, pEndState ) )
            {
                pStateMachineGraph->CreateTransitionConduit( pStartState, pEndState );
            }
        }
        else
        {
            auto pFlowGraph = GetFlowGraph();

            if( m_pHoveredPin != nullptr && m_pHoveredPin->m_direction != m_dragState.m_pPin->m_direction )
            {
                auto pHoveredFlowNode = Cast<Flow::Node>( m_pHoveredNode );

                if ( m_dragState.m_pPin->IsOutputPin() )
                {
                    pFlowGraph->TryMakeConnection( m_dragState.GetAsFlowNode(), m_dragState.m_pPin, pHoveredFlowNode, m_pHoveredPin );
                }
                else // The hovered pin is the output pin
                {
                    pFlowGraph->TryMakeConnection( pHoveredFlowNode, m_pHoveredPin, m_dragState.GetAsFlowNode(), m_dragState.m_pPin );
                }
            }
            else
            {
                pFlowGraph->BreakAnyConnectionsForPin( m_dragState.m_pPin->m_ID );
            }
        }

        //-------------------------------------------------------------------------

        m_dragState.Reset();
    }

    //-------------------------------------------------------------------------

    void GraphView::HandleInput( TypeSystem::TypeRegistry const& typeRegistry, DrawContext const& ctx, UserContext* pUserContext )
    {
        // Allow selection without focus
        //-------------------------------------------------------------------------

        if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
        {
            if ( ImGui::GetIO().KeyCtrl )
            {
                if ( m_pHoveredNode != nullptr )
                {
                    if ( IsNodeSelected( m_pHoveredNode ) )
                    {
                        RemoveFromSelection( m_pHoveredNode );
                    }
                    else
                    {
                        AddToSelection( m_pHoveredNode );
                    }
                }
            }
            else if ( ImGui::GetIO().KeyAlt )
            {
                if ( IsViewingFlowGraph() )
                {
                    auto pFlowGraph = GetFlowGraph();

                    if ( m_hoveredConnectionID.IsValid() )
                    {
                        pFlowGraph->BreakConnection( m_hoveredConnectionID );
                    }
                    else if ( m_pHoveredPin != nullptr )
                    {
                        pFlowGraph->BreakAnyConnectionsForPin( m_pHoveredPin->m_ID );
                    }
                }
                else // State Machine
                {
                    if ( m_pHoveredNode != nullptr && IsOfType<SM::TransitionConduit>( m_pHoveredNode ) )
                    {
                        ClearSelection();
                        m_pGraph->DestroyNode( m_pHoveredNode->GetID() );
                        m_pHoveredNode = nullptr;
                    }
                }
            }
            else // No modifier
            {
                if ( m_pHoveredNode != nullptr )
                {
                    if ( !IsNodeSelected( m_pHoveredNode ) )
                    {
                        UpdateSelection( m_pHoveredNode );
                    }
                }
                else if ( m_isViewHovered )
                {
                    ClearSelection();
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
        {
            if ( m_pHoveredNode != nullptr )
            {
                m_pHoveredNode->OnDoubleClick( pUserContext );
            }
            else if ( m_isViewHovered )
            {
                m_pGraph->OnDoubleClick( pUserContext );
            }
        }

        //-------------------------------------------------------------------------

        // Detect clicked that start within the view window
        if ( m_isViewHovered )
        {
            // Track left mouse
            if ( m_isViewHovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
            {
                m_dragState.m_leftMouseClickDetected = true;
            }
            else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
            {
                m_dragState.m_leftMouseClickDetected = false;
            }

            // Track middle mouse
            if ( m_isViewHovered && ImGui::IsMouseClicked( ImGuiMouseButton_Middle ) )
            {
                m_dragState.m_middleMouseClickDetected = true;
            }
            else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Middle ) )
            {
                m_dragState.m_middleMouseClickDetected = false;
            }
        }

        // Handle dragging logic
        switch ( m_dragState.m_mode )
        {
            // Should we start dragging?
            case DragMode::None:
            {
                if ( m_dragState.m_leftMouseClickDetected && ImGui::IsMouseDragging( ImGuiMouseButton_Left, 3 ) )
                {
                    if ( ImGui::IsWindowHovered() )
                    {
                        if ( IsViewingFlowGraph() )
                        {
                            if ( m_pHoveredNode != nullptr )
                            {
                                if ( m_pHoveredPin != nullptr )
                                {
                                    StartDraggingConnection( ctx );
                                }
                                else
                                {
                                    StartDraggingNode( ctx );
                                }
                            }
                            else
                            {
                                StartDraggingSelection( ctx );
                            }
                        }
                        else // State Machine
                        {
                            if ( m_pHoveredNode != nullptr )
                            {
                                if ( ImGui::GetIO().KeyAlt && IsOfType<SM::State>( m_pHoveredNode ) )
                                {
                                    StartDraggingConnection( ctx );
                                }
                                else if ( !IsOfType<SM::TransitionConduit>( m_pHoveredNode ) )
                                {
                                    StartDraggingNode( ctx );
                                }
                            }
                            else
                            {
                                StartDraggingSelection( ctx );
                            }
                        }
                    }
                }
                else if ( m_dragState.m_middleMouseClickDetected && ImGui::IsMouseDragging( ImGuiMouseButton_Middle, 3 ) )
                {
                    StartDraggingView( ctx );
                }
            }
            break;

            case DragMode::Node:
            {
                OnDragNode( ctx );
            }
            break;

            case DragMode::Connection:
            {
                OnDragConnection( ctx );
            }
            break;

            case DragMode::Selection:
            {
                OnDragSelection( ctx );
            }
            break;

            case DragMode::View:
            {
                OnDragView( ctx );
            }
            break;
        }

        // Keyboard
        //-------------------------------------------------------------------------

        // These operation require the graph view to be focused!
        if ( m_hasFocus )
        {
            if ( ImGui::IsKeyPressed( ImGuiKey_F2 ) )
            {
                if ( m_selectedNodes.size() == 1 && m_selectedNodes[0].m_pNode->IsRenameable() )
                {
                    BeginRenameNode( m_selectedNodes[0].m_pNode );
                }
            }
            else if ( ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_C ) )
            {
                CopySelectedNodes( typeRegistry );
            }
            else if ( ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_X ) )
            {
                CopySelectedNodes( typeRegistry );
                DestroySelectedNodes();
            }
            else if ( ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_V ) )
            {
                ImVec2 pasteLocation( 0.0f, 0.0f );

                if ( m_isViewHovered )
                {
                    pasteLocation = ctx.m_mouseCanvasPos;
                }
                else
                {
                    pasteLocation = ctx.m_canvasVisibleRect.GetCenter();
                }

                PasteNodes( typeRegistry, pasteLocation );
            }

            if ( !m_selectedNodes.empty() && ImGui::IsKeyPressed( ImGuiKey_Delete ) )
            {
                DestroySelectedNodes();
            }

            if ( ImGui::IsKeyPressed( ImGuiKey_Home ) )
            {
                ResetView();
            }
        }
    }

    void GraphView::HandleContextMenu( DrawContext const& ctx, UserContext* pUserContext )
    {
        if ( m_isViewHovered && ImGui::IsMouseReleased( ImGuiMouseButton_Right ) )
        {
            m_contextMenuState.m_mouseCanvasPos = ctx.m_mouseCanvasPos;
            m_contextMenuState.m_menuOpened = true;
            m_contextMenuState.m_pNode = m_pHoveredNode;
            m_contextMenuState.m_pPin = m_pHoveredPin;

            // if we are opening a context menu for a node, set that node as selected
            if ( m_contextMenuState.m_pNode != nullptr )
            {
                UpdateSelection( m_contextMenuState.m_pNode );
            }

            ImGui::OpenPopupEx( GImGui->CurrentWindow->GetID( "GraphContextMenu" ) );
        }

        //-------------------------------------------------------------------------

        if ( IsContextMenuOpen() )
        {
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4, 4 ) );
            if ( ImGui::BeginPopupContextItem( "GraphContextMenu" ) )
            {
                if ( m_contextMenuState.m_pNode != nullptr )
                {
                    if ( IsViewingFlowGraph() )
                    {
                        auto pFlowGraph = GetFlowGraph();
                        auto pFlowNode = m_contextMenuState.GetAsFlowNode();

                        // Dynamic Pins
                        if ( pFlowNode->SupportsDynamicInputPins() )
                        {
                            if ( ImGui::MenuItem( EE_ICON_PLUS_CIRCLE_OUTLINE" Add Input" ) )
                            {
                                pFlowGraph->CreateDynamicPin( pFlowNode->GetID() );
                            }

                            if ( m_contextMenuState.m_pPin != nullptr && m_contextMenuState.m_pPin->IsDynamicPin() )
                            {
                                if ( ImGui::MenuItem( EE_ICON_CLOSE_CIRCLE_OUTLINE" Remove Input" ) )
                                {
                                    pFlowGraph->DestroyDynamicPin( pFlowNode->GetID(), m_contextMenuState.m_pPin->m_ID );
                                }
                            }
                        }

                        // Connections
                        if ( ImGui::MenuItem( EE_ICON_PIPE_DISCONNECTED" Break All Connections" ) )
                        {
                            pFlowGraph->BreakAllConnectionsForNode( pFlowNode );
                        }

                        // User Options
                        pFlowNode->DrawContextMenuOptions( ctx, pUserContext, m_contextMenuState.m_mouseCanvasPos, m_contextMenuState.m_pPin );
                    }
                    else
                    {
                        if ( ImGui::MenuItem( EE_ICON_STAR" Make Default Entry State" ) )
                        {
                            auto pParentStateMachineGraph = Cast<VisualGraph::StateMachineGraph>( m_contextMenuState.m_pNode->GetParentGraph() );
                            pParentStateMachineGraph->SetDefaultEntryState( m_contextMenuState.m_pNode->GetID() );
                        }

                        ImGui::Separator();

                        auto pStateMachineNode = m_contextMenuState.GetAsStateMachineNode();
                        pStateMachineNode->DrawContextMenuOptions( ctx, pUserContext, m_contextMenuState.m_mouseCanvasPos );
                    }

                    //-------------------------------------------------------------------------

                    if ( m_contextMenuState.m_pNode->IsDestroyable() && m_pGraph->CanDeleteNode( m_contextMenuState.m_pNode ) )
                    {
                        ImGui::Separator();

                        if ( m_contextMenuState.m_pNode->IsRenameable() )
                        {
                            if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename Node" ) )
                            {
                                BeginRenameNode( m_contextMenuState.m_pNode );
                            }
                        }

                        if ( ImGui::MenuItem( EE_ICON_DELETE" Delete Node" ) )
                        {
                            ClearSelection();
                            m_contextMenuState.m_pNode->Destroy();
                            m_contextMenuState.Reset();
                        }
                    }
                }
                else // Graph Context Menu
                {
                    if ( ImGui::MenuItem( EE_ICON_IMAGE_FILTER_CENTER_FOCUS" Reset View" ) )
                    {
                        ResetView();
                    }

                    m_pGraph->DrawContextMenuOptions( ctx, pUserContext, m_contextMenuState.m_mouseCanvasPos );
                }

                ImGui::EndPopup();
            }
            else
            {
                ImGui::SetWindowFocus();
                m_contextMenuState.Reset();
            }
            ImGui::PopStyleVar( 2 );
        }
    }

    static int FilterNodeNameChars( ImGuiInputTextCallbackData* data )
    {
        if ( isalnum( data->EventChar ) || data->EventChar == '_' )
        {
            return 0;
        }
        return 1;
    }

    void GraphView::DrawDialogs()
    {
        if ( m_pNodeBeingRenamed != nullptr )
        {
            ImGui::OpenPopup( s_dialogID_Rename );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 6, 6 ) );
            ImGui::SetNextWindowSize( ImVec2( 400, -1 ) );
            if ( ImGui::BeginPopupModal( s_dialogID_Rename, nullptr, ImGuiWindowFlags_NoSavedSettings ) )
            {
                if ( m_selectedNodes.size() != 1 )
                {
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    auto pNode = m_selectedNodes.back().m_pNode;
                    EE_ASSERT( pNode->IsRenameable() );
                    bool updateConfirmed = false;

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( "Name: " );
                    ImGui::SameLine();

                    ImGui::SetNextItemWidth( -1 );

                    if ( ImGui::IsWindowAppearing() )
                    {
                        ImGui::SetKeyboardFocusHere();
                    }

                    if ( ImGui::InputText( "##NodeName", m_renameBuffer, 255, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter, FilterNodeNameChars ) )
                    {
                        if ( m_renameBuffer[0] != 0 )
                        {
                            updateConfirmed = true;
                        }
                    }

                    ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 4 );
                    ImGui::NewLine();

                    float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
                    ImGui::SameLine( 0, dialogWidth - 104 );

                    ImGui::BeginDisabled( m_renameBuffer[0] == 0 );
                    if ( ImGui::Button( "Ok", ImVec2( 50, 0 ) ) || updateConfirmed )
                    {
                        EndRenameNode( true );
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndDisabled();

                    ImGui::SameLine( 0, 4 );

                    if ( ImGui::Button( "Cancel", ImVec2( 50, 0 ) ) )
                    {
                        EndRenameNode( false );
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::EndPopup();
            }
            ImGui::PopStyleVar();
        }
    }
}