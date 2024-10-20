#include "NodeGraph_View.h"
#include "NodeGraph_Comment.h"
#include "Base/Math/Lerp.h"
#include "Base/Serialization/TypeSerialization.h"
#include "NodeGraph_Style.h"

//-------------------------------------------------------------------------

namespace EE::NodeGraph
{
    static ImVec2 const                 g_graphTitleMargin( 16, 10 );
    constexpr static float const        g_gridSpacing = 30;
    constexpr static float const        g_nodeSelectionBorderThickness = 2.0f;
    constexpr static float const        g_connectionSelectionExtraRadius = 5.0f;
    constexpr static float const        g_transitionArrowWidth = 3.0f;
    constexpr static float const        g_transitionArrowOffset = 8.0f;
    constexpr static float const        g_titleBarColorItemWidth = 8.0f;
    constexpr static float const        g_spacingBetweenTitleAndNodeContents = 6.0f;
    constexpr static float const        g_pinRadius = 6.0f;
    constexpr static float const        g_spacingBetweenInputOutputPins = 16.0f;
    constexpr static float const        g_autoConnectMaxDistanceThreshold = 200.0f;

    static Color const                  g_gridBackgroundColor( 40, 40, 40, 200 );
    static Color const                  g_gridLineColor( 200, 200, 200, 40 );
    static Color const                  g_graphTitleColor( 255, 255, 255, 255 );
    static Color const                  g_graphTitleReadOnlyColor( 196, 196, 196, 255 );
    static Color const                  g_selectionBoxOutlineColor( 61, 224, 133, 150 );
    static Color const                  g_selectionBoxFillColor( 61, 224, 133, 30 );
    static Color const                  g_readOnlyCanvasBorderColor = Colors::LightCoral;

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

    static void GetNodeBackgroundAndBorderColors( Color titleBarColor, Color baseColor, TBitFlags<NodeVisualState> visualState, Color& outTitleBarColor, Color& outBackgroundColor, Color& outBorderColor )
    {
        if ( visualState.IsFlagSet( NodeVisualState::Selected ) && visualState.IsFlagSet( NodeVisualState::Hovered ) )
        {
            outTitleBarColor = titleBarColor.GetScaledColor( 1.6f );
            outBackgroundColor = baseColor.GetScaledColor( 1.6f );
            outBorderColor = Color( Style::s_nodeSelectedBorderColor ).GetScaledColor( 1.5f );
        }
        else if ( visualState.IsFlagSet( NodeVisualState::Selected ) )
        {
            outTitleBarColor = titleBarColor.GetScaledColor( 1.45f );
            outBackgroundColor = baseColor.GetScaledColor( 1.45f );
            outBorderColor = Style::s_nodeSelectedBorderColor;
        }
        else if ( visualState.IsFlagSet( NodeVisualState::Hovered ) )
        {
            outTitleBarColor = titleBarColor.GetScaledColor( 1.15f );
            outBackgroundColor = baseColor.GetScaledColor( 1.15f );
            outBorderColor = Colors::Transparent;
        }
        else
        {
            outTitleBarColor = titleBarColor;
            outBackgroundColor = baseColor;
            outBorderColor = Colors::Transparent;
        }
    }

    //-------------------------------------------------------------------------

    GraphView::GraphView( UserContext* m_pUserContext )
        : m_pUserContext( m_pUserContext )
    {
        EE_ASSERT( m_pUserContext != nullptr );
        m_graphEndModificationBindingID = BaseGraph::OnEndRootGraphModification().Bind( [this] ( BaseGraph* pGraph ) { OnGraphModified( pGraph ); } );
    }

    GraphView::~GraphView()
    {
        BaseGraph::OnEndRootGraphModification().Unbind( m_graphEndModificationBindingID );
    }

    void GraphView::OnGraphModified( BaseGraph* pModifiedGraph )
    {
        // If this is the currently viewed graph, trigger the show graph flow since we might have added or removed nodes
        if ( pModifiedGraph == m_pGraph )
        {
            m_pGraph->OnShowGraph();
        }
    }

    //-------------------------------------------------------------------------

    void GraphView::SetGraphToView( BaseGraph* pGraph, bool tryMaintainSelection )
    {
        if ( m_pGraph == pGraph )
        {
            return;
        }

        // Record selection
        //-------------------------------------------------------------------------

        TVector<SelectedNode> oldSelection;
        oldSelection.swap( m_selectedNodes );

        // Switch graph
        //-------------------------------------------------------------------------

        ResetInternalState();

        m_pGraph = pGraph;
        if ( m_pGraph != nullptr )
        {
            m_pViewOffset = &m_pGraph->m_viewOffset;

            DrawContext drawingContext;
            drawingContext.SetViewScaleFactor( m_pGraph->m_viewScaleFactor );

            // Ensure that the view overlaps some section of the nodes in the graph
            // We dont want to show an empty area of graph
            ImRect canvasAreaWithNodes;
            for ( TTypeInstance<BaseNode> const& nodeInstance : m_pGraph->m_nodes )
            {
                canvasAreaWithNodes.Add( nodeInstance->GetRect() );
            }

            ImRect visibleCanvasRect( *m_pViewOffset, ImVec2( *m_pViewOffset ) + m_canvasSize );
            if ( !visibleCanvasRect.Overlaps( canvasAreaWithNodes ) )
            {
                *m_pViewOffset = canvasAreaWithNodes.Min;
            }

            // Notify graph it is being viewed
            m_pGraph->OnShowGraph();
        }
        else
        {
            m_defaultViewOffset = Float2::Zero;
            m_pViewOffset = &m_defaultViewOffset;
        }

        RefreshNodeSizes();

        // Restore old selection
        //-------------------------------------------------------------------------

        if ( tryMaintainSelection )
        {
            TVector<SelectedNode> newSelection;
            for ( auto const& oldSelectedNode : oldSelection )
            {
                auto pFoundNode = GetViewedGraph()->FindNode( oldSelectedNode.m_nodeID );
                if ( pFoundNode != nullptr )
                {
                    newSelection.emplace_back( oldSelectedNode );
                }
            }

            UpdateSelection( newSelection );
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
        m_dragAndDropState.Reset();
        ClearSelection();
    }

    //-------------------------------------------------------------------------

    bool GraphView::BeginDrawCanvas( float childHeightOverride )
    {
        m_selectionChanged = false;
        m_isViewHovered = false;
        m_canvasSize = ImVec2( 0, 0 );

        // Create child window
        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        ImGui::PushStyleColor( ImGuiCol_ChildBg, g_gridBackgroundColor );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4, 4 ) );

        if ( m_requestFocus )
        {
            ImGui::SetNextWindowFocus();
            m_requestFocus = false;
        }

        bool const childVisible = ImGui::BeginChild( "GraphCanvas", ImVec2( 0.f, childHeightOverride ), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse );
        if ( childVisible )
        {
            auto pWindow = ImGui::GetCurrentWindow();
            auto pDrawList = ImGui::GetWindowDrawList();

            m_hasFocus = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_NoPopupHierarchy );
            m_isViewHovered = ImGui::IsWindowHovered();
            m_canvasSize = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ) * ( 1.0f / GetViewScaleFactor() );

            // Background
            //-------------------------------------------------------------------------

            ImRect const windowRect = pWindow->Rect();
            ImVec2 const windowTL = windowRect.GetTL();
            float const canvasWidth = windowRect.GetWidth();
            float const canvasHeight = windowRect.GetHeight();

            Color const gridLineColor = IsReadOnly() ? g_readOnlyCanvasBorderColor.GetScaledColor( 0.4f ) : g_gridLineColor;

            // Draw Grid
            for ( float x = Math::FModF( 0, g_gridSpacing ); x < canvasWidth; x += g_gridSpacing )
            {
                pDrawList->AddLine( windowTL + ImVec2( x, 0.0f ), windowTL + ImVec2( x, canvasHeight ), gridLineColor );
            }

            for ( float y = Math::FModF( 0, g_gridSpacing ); y < canvasHeight; y += g_gridSpacing )
            {
                pDrawList->AddLine( windowTL + ImVec2( 0.0f, y ), windowTL + ImVec2( canvasWidth, y ), gridLineColor );
            }
        }

        return childVisible;
    }

    void GraphView::EndDrawCanvas( DrawContext const& ctx )
    {
        // Is the canvas visible?
        if ( ctx.m_pDrawList != nullptr )
        {
            // Border
            //-------------------------------------------------------------------------

            if ( IsReadOnly() )
            {
                static constexpr float const rectThickness = 8.0f;
                static constexpr float const rectMargin = 2.0f;
                static ImVec2 const offset( ( rectThickness / 2.f ) + rectMargin, ( rectThickness / 2.f ) + rectMargin );
                ctx.m_pDrawList->AddRect( ctx.m_windowRect.Min + offset, ctx.m_windowRect.Max - offset, g_readOnlyCanvasBorderColor, 8.0f, 0, rectThickness );
            }

            // Title Block
            //-------------------------------------------------------------------------

            ImVec2 textPosition = ctx.m_windowRect.Min + g_graphTitleMargin;
            {
                ImGuiX::ScopedFont font( ImGuiX::Font::LargeBold );
                auto pViewedGraph = GetViewedGraph();
                if ( pViewedGraph != nullptr )
                {
                    // Draw title text
                    //-------------------------------------------------------------------------

                    ImFont* pTitleFont = ImGuiX::GetFont( ImGuiX::Font::LargeBold );
                    InlineString const title( InlineString::CtorSprintf(), "%s%s", pViewedGraph->GetName(), IsReadOnly() ? " (Read-Only)" : "" );
                    ctx.m_pDrawList->AddText( pTitleFont, pTitleFont->FontSize, textPosition, IsReadOnly() ? ImGuiX::Style::s_colorTextDisabled : g_graphTitleColor, title.c_str() );
                    textPosition += ImVec2( 0, pTitleFont->FontSize );

                    // Draw extra info
                    //-------------------------------------------------------------------------

                    ImFont* pMediumFont = ImGuiX::GetFont( ImGuiX::Font::Medium );
                    if ( !m_pUserContext->GetExtraGraphTitleInfoText().empty() )
                    {
                        ctx.m_pDrawList->AddText( pMediumFont, pMediumFont->FontSize, textPosition, m_pUserContext->GetExtraTitleInfoTextColor(), m_pUserContext->GetExtraGraphTitleInfoText().c_str() );
                    }
                }
                else
                {
                    ctx.m_pDrawList->AddText( textPosition, ImGuiX::Style::s_colorTextDisabled, m_isReadOnly ? "Nothing to Show (Read-Only)" : "Nothing to Show" );
                }
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::PopID();
    }

    //-------------------------------------------------------------------------

    void GraphView::ChangeViewScale( DrawContext const& ctx, float newViewScale )
    {
        float const deltaScale = newViewScale - m_pGraph->m_viewScaleFactor;
        if ( !Math::IsNearZero( deltaScale ) )
        {
            m_pGraph->m_viewOffset += ( ( ctx.m_mouseCanvasPos - m_pGraph->m_viewOffset ) * deltaScale ) / newViewScale;
            m_pGraph->m_viewScaleFactor = newViewScale;

            for ( TTypeInstance<BaseNode>& nodeInstance : m_pGraph->m_nodes )
            {
                nodeInstance->ResetCalculatedNodeSizes();
            }
        }
    }

    void GraphView::DrawStateMachineNode( DrawContext const& ctx, StateMachineNode* pNode )
    {
        EE_ASSERT( pNode != nullptr );

        if ( !pNode->IsVisible() )
        {
            return;
        }

        //-------------------------------------------------------------------------
        // Split Channels
        //-------------------------------------------------------------------------

        ImGui::PushID( pNode );

        ctx.SplitDrawChannels();

        //-------------------------------------------------------------------------
        // Draw contents
        //-------------------------------------------------------------------------

        ctx.SetDrawChannel( (uint8_t) DrawChannel::Foreground );

        ImVec2 newNodeWindowSize( 0, 0 );
        ImVec2 const windowPosition = ctx.CanvasToWindowPosition( pNode->GetPosition() );
        ImVec2 const scaledNodeMargin = ctx.CanvasToWindow( pNode->GetNodeMargin() );
        ImVec2 const scaledColorItemSpacing( ( g_titleBarColorItemWidth * ctx.m_viewScaleFactor ) - scaledNodeMargin.x, 0 );

        ImGui::SetCursorPos( windowPosition );
        ImGui::BeginGroup();
        {
            // Draw Title
            //-------------------------------------------------------------------------

            ImGuiX::ScopedFont fontOverride( ImGuiX::Font::Medium, Colors::White );
            ImGui::BeginGroup();
            ImGui::Dummy( scaledColorItemSpacing );
            ImGui::SameLine();
            if ( pNode->GetIcon() != nullptr )
            {
                ImGui::Text( pNode->GetIcon() );
                ImGui::SameLine();
            }
            ImGui::Text( pNode->GetName() );
            ImGui::EndGroup();

            newNodeWindowSize = ImGui::GetItemRectSize();
            pNode->m_titleRectSize = ctx.WindowToCanvas( newNodeWindowSize );

            float const scaledSpacing = ctx.CanvasToWindow( g_spacingBetweenTitleAndNodeContents );
            ImGui::SetCursorPosY( ImGui::GetCursorPos().y + scaledSpacing );
            newNodeWindowSize.y += scaledSpacing;

            // Draw Extra Controls
            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );

                auto const cursorStartPos = ImGui::GetCursorPos();
                ImGui::BeginGroup();

                pNode->DrawExtraControls( ctx, m_pUserContext );

                auto const cursorEndPos = ImGui::GetCursorPos();
                ImGui::SetCursorPos( cursorStartPos );
                ImGui::Dummy( cursorEndPos - cursorStartPos );
                ImGui::EndGroup();
            }

            ImVec2 const extraControlsRectSize = ImGui::GetItemRectSize();
            newNodeWindowSize.x = Math::Max( newNodeWindowSize.x, extraControlsRectSize.x );
            newNodeWindowSize.y += extraControlsRectSize.y;
        }
        ImGui::EndGroup();

        pNode->m_size = ctx.WindowToCanvas( newNodeWindowSize );

        //-------------------------------------------------------------------------
        // Draw background
        //-------------------------------------------------------------------------

        TBitFlags<NodeVisualState> visualState;
        visualState.SetFlag( NodeVisualState::Active, pNode->IsActive( m_pUserContext ) );
        visualState.SetFlag( NodeVisualState::Selected, IsNodeSelected( pNode ) );
        visualState.SetFlag( NodeVisualState::Hovered, pNode->m_isHovered );

        //-------------------------------------------------------------------------

        Color nodeTitleBarColor, nodeBackgroundColor, nodeBorderColor;
        GetNodeBackgroundAndBorderColors( Style::s_defaultTitleColor, Style::s_nodeBackgroundColor, visualState, nodeTitleBarColor, nodeBackgroundColor, nodeBorderColor );

        //-------------------------------------------------------------------------

        ImVec2 const backgroundRectMin = ctx.WindowToScreenPosition( windowPosition - scaledNodeMargin );
        ImVec2 const backgroundRectMax = ctx.WindowToScreenPosition( windowPosition + newNodeWindowSize + scaledNodeMargin );
        ImVec2 const rectTitleBarMax = ctx.WindowToScreenPosition( windowPosition + ImVec2( newNodeWindowSize.x, ctx.CanvasToWindow( pNode->m_titleRectSize.m_y ) ) + scaledNodeMargin );
        ImVec2 const rectTitleBarColorItemMax = ImVec2( backgroundRectMin.x + ( ctx.m_viewScaleFactor * g_titleBarColorItemWidth ), rectTitleBarMax.y );
        float const scaledCornerRounding = 2 * ctx.m_viewScaleFactor;
        float const scaledBorderThickness = g_nodeSelectionBorderThickness * ctx.m_viewScaleFactor;

        ctx.SetDrawChannel( (uint8_t) DrawChannel::Background );

        if ( IsOfType<StateNode>( pNode ) )
        {
            if ( visualState.IsFlagSet( NodeVisualState::Active ) )
            {
                ImVec2 const activeBorderPadding( Style::s_activeBorderIndicatorPadding, Style::s_activeBorderIndicatorPadding );
                ctx.m_pDrawList->AddRect( backgroundRectMin - activeBorderPadding, backgroundRectMax + activeBorderPadding, Style::s_activeIndicatorBorderColor, scaledCornerRounding, ImDrawFlags_RoundCornersAll, Style::s_activeBorderIndicatorThickness );
            }

            ctx.m_pDrawList->AddRectFilled( backgroundRectMin, backgroundRectMax, nodeBackgroundColor, scaledCornerRounding, ImDrawFlags_RoundCornersAll );
            ctx.m_pDrawList->AddRectFilled( backgroundRectMin, rectTitleBarMax, Style::s_defaultTitleColor, scaledCornerRounding, ImDrawFlags_RoundCornersTop );
            ctx.m_pDrawList->AddRectFilled( backgroundRectMin, rectTitleBarColorItemMax, pNode->GetTitleBarColor(), scaledCornerRounding, ImDrawFlags_RoundCornersTopLeft );
            ctx.m_pDrawList->AddRect( backgroundRectMin, backgroundRectMax, nodeBorderColor, scaledCornerRounding, ImDrawFlags_RoundCornersAll, scaledBorderThickness );
        }
        else // Non-state node
        {
            ctx.m_pDrawList->AddRectFilled( backgroundRectMin, backgroundRectMax, nodeBackgroundColor, scaledCornerRounding );
            ctx.m_pDrawList->AddRect( backgroundRectMin, backgroundRectMax, nodeBorderColor, scaledCornerRounding, ImDrawFlags_RoundCornersAll, scaledBorderThickness );
        }

        //-------------------------------------------------------------------------
        // Merge Channels
        //-------------------------------------------------------------------------

        ctx.MergeDrawChannels();
        ImGui::PopID();

        //-------------------------------------------------------------------------

        ImRect const nodeRect = pNode->GetRect();
        pNode->m_isHovered = m_isViewHovered && nodeRect.Contains( ctx.m_mouseCanvasPos );
    }

    void GraphView::DrawStateMachineTransitionConduit( DrawContext const& ctx, TransitionConduitNode* pTransitionConduit )
    {
        EE_ASSERT( pTransitionConduit != nullptr );
        EE_ASSERT( pTransitionConduit->m_startStateID.IsValid() && pTransitionConduit->m_endStateID.IsValid() );

        auto pStartState = Cast<StateNode>( m_pGraph->FindNode( pTransitionConduit->m_startStateID ) );
        auto pEndState = Cast<StateNode>( m_pGraph->FindNode( pTransitionConduit->m_endStateID ) );

        // Calculate transition arrow start and end points
        //-------------------------------------------------------------------------

        ImRect const startNodeRect = pStartState->GetRect();
        ImRect const scaledEndNodeRect = pEndState->GetRect();

        ImVec2 startPoint, endPoint;
        GetConnectionPointsBetweenStateMachineNodes( startNodeRect, scaledEndNodeRect, startPoint, endPoint );

        Vector const arrowDir = Vector( endPoint - startPoint ).GetNormalized2();
        Vector const orthogonalDir = arrowDir.Orthogonal2D();
        ImVec2 const offset = ( orthogonalDir * g_transitionArrowOffset ).ToFloat2();

        startPoint += offset;
        endPoint += offset;

        // Update hover state and visual state
        //-------------------------------------------------------------------------

        float const scaledSelectionExtraRadius = ctx.WindowToCanvas( g_connectionSelectionExtraRadius );
        ImVec2 const closestPointOnTransitionToMouse = ImLineClosestPoint( startPoint, endPoint, ctx.m_mouseCanvasPos );
        if ( m_isViewHovered && ImLengthSqr( ctx.m_mouseCanvasPos - closestPointOnTransitionToMouse ) < Math::Pow( scaledSelectionExtraRadius, 2 ) )
        {
            pTransitionConduit->m_isHovered = true;
        }
        else
        {
            pTransitionConduit->m_isHovered = false;
        }

        TBitFlags<NodeVisualState> visualState;
        visualState.SetFlag( NodeVisualState::Active, pTransitionConduit->IsActive( m_pUserContext ) );
        visualState.SetFlag( NodeVisualState::Selected, IsNodeSelected( pTransitionConduit ) );
        visualState.SetFlag( NodeVisualState::Hovered, pTransitionConduit->m_isHovered );

        // Draw Extra Controls
        //-------------------------------------------------------------------------

        pTransitionConduit->DrawExtraControls( ctx, m_pUserContext, startPoint, endPoint );

        // Draw Arrows
        //-------------------------------------------------------------------------

        float const scaledArrowHeadWidth = ctx.CanvasToWindow( 5.0f );
        float const scaledArrowWidth = ctx.CanvasToWindow( g_transitionArrowWidth );
        float const transitionProgress = pTransitionConduit->m_transitionProgress.GetNormalizedTime().ToFloat();
        bool const hasTransitionProgress = transitionProgress > 0.0f;
        Color const transitionColor = pTransitionConduit->GetConduitColor( ctx, m_pUserContext, visualState );

        if ( hasTransitionProgress )
        {
            ImGuiX::DrawArrow( ctx.m_pDrawList, ctx.CanvasToScreenPosition( startPoint ), ctx.CanvasToScreenPosition( endPoint ), Colors::DimGray, scaledArrowWidth, scaledArrowHeadWidth );
            ImVec2 const progressEndPoint = Math::Lerp( startPoint, endPoint, transitionProgress );
            ImGuiX::DrawArrow( ctx.m_pDrawList, ctx.CanvasToScreenPosition( startPoint ), ctx.CanvasToScreenPosition( progressEndPoint ), transitionColor, scaledArrowWidth, scaledArrowHeadWidth );
        }
        else
        {
            if ( visualState.IsFlagSet( NodeVisualState::Selected ) )
            {
                ImGuiX::DrawArrow( ctx.m_pDrawList, ctx.CanvasToScreenPosition( startPoint ), ctx.CanvasToScreenPosition( endPoint ), transitionColor, scaledArrowWidth * 1.5f, scaledArrowHeadWidth * 1.5f );
            }
            else
            {
                ImGuiX::DrawArrow( ctx.m_pDrawList, ctx.CanvasToScreenPosition( startPoint ), ctx.CanvasToScreenPosition( endPoint ), transitionColor, scaledArrowWidth, scaledArrowHeadWidth );
            }
        }

        // Update transition position and size
        //-------------------------------------------------------------------------

        ImVec2 const min( Math::Min( startPoint.x, endPoint.x ), Math::Min( startPoint.y, endPoint.y ) );
        ImVec2 const max( Math::Max( startPoint.x, endPoint.x ), Math::Max( startPoint.y, endPoint.y ) );
        pTransitionConduit->m_canvasPosition = min;
        pTransitionConduit->m_size = max - min;
    }

    void GraphView::DrawFlowNode( DrawContext const& ctx, FlowNode* pNode )
    {
        EE_ASSERT( pNode != nullptr );

        if ( !pNode->IsVisible() )
        {
            return;
        }

        //-------------------------------------------------------------------------
        // Split Channels
        //-------------------------------------------------------------------------

        ImGui::PushID( pNode );

        ctx.SplitDrawChannels();

        //-------------------------------------------------------------------------
        // Draw Node Contents
        //-------------------------------------------------------------------------

        ctx.SetDrawChannel( (uint8_t) DrawChannel::Foreground );

        ImVec2 newNodeWindowSize( 0, 0 );
        ImVec2 const windowPosition = ctx.CanvasToWindowPosition( pNode->GetPosition() );
        ImVec2 const scaledNodeMargin = ctx.CanvasToWindow( pNode->GetNodeMargin() );
        ImVec2 const scaledColorItemSpacing( ( g_titleBarColorItemWidth * ctx.m_viewScaleFactor ) - scaledNodeMargin.x, 0 );

        ImGui::SetCursorPos( windowPosition );
        ImGui::BeginGroup();
        {

            // Draw Title
            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont fontOverride( ImGuiX::Font::MediumBold, Colors::White );
                ImGui::BeginGroup();
                ImGui::Dummy( scaledColorItemSpacing );
                ImGui::SameLine();
                if ( pNode->GetIcon() != nullptr )
                {
                    ImGui::Text( pNode->GetIcon() );
                    ImGui::SameLine();
                }
                ImGui::Text( pNode->GetName() );
                ImGui::EndGroup();

                newNodeWindowSize = ImGui::GetItemRectSize();
                pNode->m_titleRectSize = ctx.WindowToCanvas( newNodeWindowSize );

                float const scaledSpacing = ctx.CanvasToWindow( g_spacingBetweenTitleAndNodeContents );
                ImGui::SetCursorPosY( ImGui::GetCursorPos().y + scaledSpacing );
                newNodeWindowSize.y += scaledSpacing;
            }

            // Draw pins
            //-------------------------------------------------------------------------

            bool hasPinControlsOnLastRow = false;

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );

                ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 2 * ctx.m_viewScaleFactor ) );
               
                pNode->m_pHoveredPin = nullptr;

                //-------------------------------------------------------------------------

                ImVec2 pinRectSize( 0, 0 );
                int32_t const numPinRows = Math::Max( pNode->GetNumInputPins(), pNode->GetNumOutputPins() );
                for ( auto i = 0; i < numPinRows; i++ )
                {
                    ImVec2 pinWindowSize( 0, 0 );
                    bool const hasInputPin = i < pNode->m_inputPins.size();
                    bool const hasOutputPin = i < pNode->m_outputPins.size();
                    float estimatedSpacingBetweenPins = 0.0f;

                    // Draw Pin
                    //-------------------------------------------------------------------------

                    auto DrawPin = [&] ( Pin& pin, bool isInputPin )
                    {
                        ImGui::BeginGroup();
                        ImGui::AlignTextToFramePadding();

                        if ( isInputPin )
                        {
                            ImGui::Text( pin.m_name.c_str() );
                        }

                        if ( pNode->DrawPinControls( ctx, m_pUserContext, pin ) )
                        {
                            hasPinControlsOnLastRow = ( i == ( numPinRows - 1 ) );
                        }

                        if ( !isInputPin )
                        {
                            ImGui::Text( pin.m_name.c_str() );
                        }
                        ImGui::EndGroup();

                        // Set pin rendered size
                        ImRect const pinRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
                        pin.m_size = ImGui::GetItemRectSize();

                        float const pinOffsetY = ImGui::GetFrameHeightWithSpacing() / 2;

                        if ( isInputPin )
                        {
                            pin.m_position = ImVec2( pinRect.Min.x - scaledNodeMargin.x, pinRect.Min.y + pinOffsetY );
                        }
                        else
                        {
                            pin.m_position = ImVec2( pinRect.Max.x + scaledNodeMargin.x, pinRect.Min.y + pinOffsetY );
                        }

                        // Check hover state - do it in window space since pin position/size are in window space
                        Color pinColor = pNode->GetPinColor( pin );
                        bool const isPinHovered = Vector( pin.m_position ).GetDistance2( ImGui::GetMousePos() ) < ctx.CanvasToWindow( g_pinRadius + FlowNode::s_pinSelectionExtraRadius );
                        if ( isPinHovered )
                        {
                            pNode->m_pHoveredPin = &pin;
                            pinColor.ScaleColor( 1.55f );
                        }

                        // Draw pin
                        float const scaledPinRadius = ctx.CanvasToWindow( g_pinRadius );
                        ctx.m_pDrawList->AddCircleFilled( pin.m_position, scaledPinRadius, pinColor );
                    };

                    // Input Pin
                    //-------------------------------------------------------------------------

                    float const pinRowCursorStartY = ImGui::GetCursorPosY();

                    if ( hasInputPin )
                    {
                        DrawPin( pNode->m_inputPins[i], true );
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
                        float const scaledSpacingBetweenPins = ctx.CanvasToWindow( g_spacingBetweenInputOutputPins );
                        estimatedSpacingBetweenPins = ctx.CanvasToWindow( pNode->GetWidth() ) - inputPinWidth - pNode->m_outputPins[i].GetWidth();
                        estimatedSpacingBetweenPins = Math::Max( estimatedSpacingBetweenPins, scaledSpacingBetweenPins );
                        ImGui::SameLine( 0, estimatedSpacingBetweenPins );
                    }

                    // Output Pin
                    //-------------------------------------------------------------------------

                    if ( hasOutputPin )
                    {
                        ImGui::SetCursorPosY( pinRowCursorStartY );
                        DrawPin( pNode->m_outputPins[i], false );
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
                    pinRectSize.y += pinRowRectSize.y + ImGui::GetStyle().ItemSpacing.y;
                }

                // Update node size with the max width of all output pins
                //-------------------------------------------------------------------------

                newNodeWindowSize.x = Math::Max( newNodeWindowSize.x, pinRectSize.x );
                newNodeWindowSize.y += pinRectSize.y;

                ImGui::PopStyleVar();
            }

            // Draw extra controls
            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );

                float const offsetY = ( hasPinControlsOnLastRow ? ImGui::GetStyle().ItemSpacing.y : 0.0f );

                ImVec2 const cursorStartPos = ImGui::GetCursorPos();
                ImGui::BeginGroup();
                ImGui::SetCursorPos( cursorStartPos + ImVec2( 0, offsetY ) );

                pNode->DrawExtraControls( ctx, m_pUserContext );

                ImVec2 const cursorEndPos = ImGui::GetCursorPos();
                ImGui::SetCursorPos( cursorStartPos );
                ImGui::Dummy( cursorEndPos - cursorStartPos );
                ImGui::EndGroup();
            }

            ImVec2 const extraControlsRectSize = ImGui::GetItemRectSize();
            newNodeWindowSize.x = Math::Max( newNodeWindowSize.x, extraControlsRectSize.x );
            newNodeWindowSize.y += extraControlsRectSize.y;
        }
        ImGui::EndGroup();

        pNode->m_size = ctx.WindowToCanvas( newNodeWindowSize );

        //-------------------------------------------------------------------------
        // Draw background
        //-------------------------------------------------------------------------

        TBitFlags<NodeVisualState> visualState;
        visualState.SetFlag( NodeVisualState::Active, pNode->IsActive( m_pUserContext ) );
        visualState.SetFlag( NodeVisualState::Selected, IsNodeSelected( pNode ) );
        visualState.SetFlag( NodeVisualState::Hovered, pNode->m_isHovered && pNode->m_pHoveredPin == nullptr );

        Color nodeTitleBarColor, nodeBackgroundColor, nodeBorderColor;
        GetNodeBackgroundAndBorderColors( Style::s_defaultTitleColor, Style::s_nodeBackgroundColor, visualState, nodeTitleBarColor, nodeBackgroundColor, nodeBorderColor );

        //-------------------------------------------------------------------------

        ImVec2 const backgroundRectMin = ctx.WindowToScreenPosition( windowPosition - scaledNodeMargin );
        ImVec2 const backgroundRectMax = ctx.WindowToScreenPosition( windowPosition + newNodeWindowSize + scaledNodeMargin );
        ImVec2 const rectTitleBarMax = ctx.WindowToScreenPosition( windowPosition + ImVec2( newNodeWindowSize.x, ctx.CanvasToWindow( pNode->m_titleRectSize.m_y ) ) + scaledNodeMargin );
        ImVec2 const rectTitleBarColorItemMax = ImVec2( backgroundRectMin.x + ( ctx.m_viewScaleFactor * g_titleBarColorItemWidth ), rectTitleBarMax.y );
        float const scaledCornerRounding = 8 * ctx.m_viewScaleFactor;
        float const scaledBorderThickness = g_nodeSelectionBorderThickness * ctx.m_viewScaleFactor;

        ctx.SetDrawChannel( (uint8_t) DrawChannel::Background );

        if ( visualState.IsFlagSet( NodeVisualState::Active ) )
        {
            ImVec2 const activeBorderPadding( Style::s_activeBorderIndicatorPadding, Style::s_activeBorderIndicatorPadding );
            ctx.m_pDrawList->AddRect( backgroundRectMin - activeBorderPadding, backgroundRectMax + activeBorderPadding, Style::s_activeIndicatorBorderColor, scaledCornerRounding, ImDrawFlags_RoundCornersAll, Style::s_activeBorderIndicatorThickness );
        }

        ctx.m_pDrawList->AddRectFilled( backgroundRectMin, backgroundRectMax, nodeBackgroundColor, scaledCornerRounding, ImDrawFlags_RoundCornersAll );
        ctx.m_pDrawList->AddRectFilled( backgroundRectMin, rectTitleBarMax, nodeTitleBarColor, scaledCornerRounding, ImDrawFlags_RoundCornersTop);
        ctx.m_pDrawList->AddRectFilled( backgroundRectMin, rectTitleBarColorItemMax, pNode->GetTitleBarColor(), scaledCornerRounding, ImDrawFlags_RoundCornersTopLeft );
        ctx.m_pDrawList->AddRect( backgroundRectMin, backgroundRectMax, nodeBorderColor, scaledCornerRounding, ImDrawFlags_RoundCornersAll, scaledBorderThickness );

        //-------------------------------------------------------------------------
        // Merge Channels
        //-------------------------------------------------------------------------

        ctx.MergeDrawChannels();
        ImGui::PopID();

        //-------------------------------------------------------------------------

        ImRect const nodeRect = pNode->GetRect();
        pNode->m_isHovered = m_isViewHovered && nodeRect.Contains( ctx.m_mouseCanvasPos ) || pNode->m_pHoveredPin != nullptr;
    }

    void GraphView::DrawCommentNode( DrawContext const& ctx, CommentNode* pNode )
    {
        EE_ASSERT( pNode != nullptr );

        if ( !pNode->IsVisible() )
        {
            return;
        }

        //-------------------------------------------------------------------------
        // Split Channels
        //-------------------------------------------------------------------------

        ImGui::PushID( pNode );

        ctx.SplitDrawChannels();

        //-------------------------------------------------------------------------
        // Draw contents
        //-------------------------------------------------------------------------

        ImVec2 const windowPosition = ctx.CanvasToWindowPosition( pNode->GetPosition() );
        ImVec2 const textOffset = ctx.CanvasToWindow( ImGui::GetStyle().WindowPadding );
        ImVec2 const textCursorStartPos = windowPosition + textOffset;

        ctx.SetDrawChannel( (uint8_t) DrawChannel::Foreground );
        ImGui::SetCursorPos( textCursorStartPos );
        ImGui::BeginGroup();
        {
            ImGuiX::ScopedFont fontOverride( ImGuiX::Font::MediumBold );

            float const nodeWindowWidth = ctx.CanvasToWindow( pNode->m_commentBoxSize.m_x );
            ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + nodeWindowWidth );
            ImGui::Text( "%s  %s", EE_ICON_COMMENT, pNode->GetName());
            ImGuiX::TextTooltip( pNode->GetName() );
            ImGui::PopTextWrapPos();

            pNode->m_titleRectSize = ctx.WindowToCanvas( ImGui::GetItemRectSize() );
            pNode->m_size = pNode->GetCommentBoxSize();
        }
        ImGui::EndGroup();

        //-------------------------------------------------------------------------
        // Draw background
        //-------------------------------------------------------------------------

        TBitFlags<NodeVisualState> visualState;
        visualState.SetFlag( NodeVisualState::Active, pNode->IsActive( m_pUserContext ) );
        visualState.SetFlag( NodeVisualState::Selected, IsNodeSelected( pNode ) );
        visualState.SetFlag( NodeVisualState::Hovered, pNode->m_isHovered );

        //-------------------------------------------------------------------------

        ImVec2 const windowNodeMargin = ctx.CanvasToWindow( pNode->GetNodeMargin() );
        ImVec2 const rectMin = ctx.WindowToScreenPosition( windowPosition - windowNodeMargin );
        ImVec2 const rectMax = ctx.WindowToScreenPosition( windowPosition + ctx.CanvasToWindow( pNode->GetCommentBoxSize() ) + windowNodeMargin );
        ImVec2 const titleRectMax( rectMax.x, ctx.WindowToScreenPosition( textCursorStartPos ).y + ctx.CanvasToWindow( pNode->m_titleRectSize.m_y ) + windowNodeMargin.y );

        float const scaledBorderThickness = ctx.CanvasToWindow( g_nodeSelectionBorderThickness );

        Color nodeTitleBarColor, nodeBackgroundColor, nodeBorderColor;
        GetNodeBackgroundAndBorderColors( pNode->GetCommentBoxColor(), pNode->GetCommentBoxColor(), visualState, nodeTitleBarColor, nodeBackgroundColor, nodeBorderColor );

        ctx.SetDrawChannel( (uint8_t) DrawChannel::Background );
        ctx.m_pDrawList->AddRectFilled( rectMin, rectMax, nodeBackgroundColor.GetAlphaVersion( 0.1f ), 0, ImDrawFlags_RoundCornersNone );
        ctx.m_pDrawList->AddRectFilled( rectMin, titleRectMax, nodeBackgroundColor.GetAlphaVersion( 0.3f ), 0, ImDrawFlags_RoundCornersNone );
        ctx.m_pDrawList->AddRect( rectMin, rectMax, nodeBackgroundColor, 0, ImDrawFlags_RoundCornersNone, scaledBorderThickness );
        ctx.m_pDrawList->AddRect( rectMin - ImVec2( scaledBorderThickness, scaledBorderThickness ), rectMax + ImVec2( scaledBorderThickness, scaledBorderThickness ), nodeBorderColor, 0, ImDrawFlags_RoundCornersNone, scaledBorderThickness );

        //-------------------------------------------------------------------------
        // Merge Channels
        //-------------------------------------------------------------------------

        ctx.MergeDrawChannels();
        ImGui::PopID();

        //-------------------------------------------------------------------------

        pNode->m_isHovered = false;

        if ( m_isViewHovered )
        {
            float const dilationRadius = ctx.WindowToCanvas( CommentNode::s_resizeSelectionRadius / 2 );

            // Broad hit-test
            ImRect commentNodeRect = pNode->GetRect();
            commentNodeRect.Expand( dilationRadius );
            if ( commentNodeRect.Contains( ctx.m_mouseCanvasPos ) )
            {
                // Test title region
                ImRect commentNodeTitleRect( pNode->GetRect().Min, ctx.ScreenToCanvasPosition( titleRectMax ) );
                commentNodeTitleRect.Expand( dilationRadius );
                if ( commentNodeTitleRect.Contains( ctx.m_mouseCanvasPos ) )
                {
                    pNode->m_isHovered = true;
                }
                // Test Edges
                else if ( pNode->GetHoveredResizeHandle( ctx ) != ResizeHandle::None )
                {
                    pNode->m_isHovered = true;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    void GraphView::UpdateAndDraw( TypeSystem::TypeRegistry const& typeRegistry, float childHeightOverride )
    {
        EE_ASSERT( m_pUserContext != nullptr );

        // Update context
        //-------------------------------------------------------------------------

        m_pUserContext->m_isAltDown = ImGui::GetIO().KeyAlt;
        m_pUserContext->m_isCtrlDown = ImGui::GetIO().KeyCtrl;
        m_pUserContext->m_isShiftDown = ImGui::GetIO().KeyShift;

        // Node visual update
        //-------------------------------------------------------------------------

        if ( m_pGraph != nullptr )
        {
            for ( TTypeInstance<BaseNode>& nodeInstance : m_pGraph->m_nodes )
            {
                nodeInstance->PreDrawUpdate( m_pUserContext );
            }
        }

        // Draw Graph
        //-------------------------------------------------------------------------

        DrawContext drawingContext;
        drawingContext.m_isReadOnly = m_isReadOnly;

        if ( BeginDrawCanvas( childHeightOverride ) )
        {
            auto pWindow = ImGui::GetCurrentWindow();
            ImVec2 const mousePos = ImGui::GetMousePos();

            drawingContext.SetViewScaleFactor( m_pGraph != nullptr ? m_pGraph->m_viewScaleFactor : 1.0f );
            drawingContext.m_pDrawList = ImGui::GetWindowDrawList();
            drawingContext.m_viewOffset = *m_pViewOffset;
            drawingContext.m_windowRect = pWindow->Rect();
            drawingContext.m_canvasVisibleRect = ImRect( drawingContext.m_viewOffset, drawingContext.m_viewOffset + drawingContext.m_windowRect.GetSize());
            drawingContext.m_mouseScreenPos = ImGui::GetMousePos();
            drawingContext.m_mouseCanvasPos = drawingContext.ScreenToCanvasPosition( drawingContext.m_mouseScreenPos );

            // Apply imgui scale
            //-------------------------------------------------------------------------

            ImGuiStyle unscaledStyle = ImGui::GetStyle();
            ImGui::SetWindowFontScale( drawingContext.m_viewScaleFactor );
            ImGui::GetStyle().ScaleAllSizes( drawingContext.m_viewScaleFactor );

            // Draw nodes
            //-------------------------------------------------------------------------

            if ( m_pGraph != nullptr )
            {
                m_pHoveredNode = nullptr;
                m_pHoveredPin = nullptr;

                // Comment nodes
                for ( TTypeInstance<BaseNode>& nodeInstance : m_pGraph->m_nodes )
                {
                    if ( auto pCommentNode = nodeInstance.TryGetAs<CommentNode>() )
                    {
                        DrawCommentNode( drawingContext, pCommentNode );

                        if ( pCommentNode->m_isHovered )
                        {
                            m_pHoveredNode = pCommentNode;
                        }
                    }
                }

                // State Machine Graph
                if ( IsViewingStateMachineGraph() )
                {
                    for ( TTypeInstance<BaseNode>& nodeInstance : m_pGraph->m_nodes )
                    {
                        // Ignore comment nodes
                        if ( auto pCommentNode = nodeInstance.TryGetAs<CommentNode>() )
                        {
                            continue;
                        }

                        if ( auto pConduitNode = nodeInstance.TryGetAs<TransitionConduitNode>() )
                        {
                            DrawStateMachineTransitionConduit( drawingContext, pConduitNode );
                        }
                        else
                        {
                            auto pStateMachineNode = nodeInstance.GetAs<StateMachineNode>();
                            DrawStateMachineNode( drawingContext, pStateMachineNode );
                        }

                        if ( nodeInstance->m_isHovered )
                        {
                            m_pHoveredNode = nodeInstance.Get();
                        }
                    }
                }
                else // Flow Graph
                {
                    auto pFlowGraph = GetFlowGraph();

                    // Draw Nodes
                    //-------------------------------------------------------------------------

                    for ( TTypeInstance<BaseNode>& nodeInstance : m_pGraph->m_nodes )
                    {
                        // Ignore comment nodes
                        if ( auto pCommentNode = nodeInstance.TryGetAs<CommentNode>() )
                        {
                            continue;
                        }

                        auto pFlowNode = nodeInstance.GetAs<FlowNode>();
                        DrawFlowNode( drawingContext, pFlowNode );

                        if ( pFlowNode->m_isHovered )
                        {
                            m_pHoveredNode = pFlowNode;
                            m_pHoveredPin = pFlowNode->m_pHoveredPin;
                        }
                    }

                    // Draw connections
                    //-------------------------------------------------------------------------

                    m_hoveredConnectionID.Clear();
                    for ( FlowGraph::Connection const& connection : pFlowGraph->m_connections )
                    {
                        FlowNode* pFromNode = Cast<FlowNode>( pFlowGraph->GetNode( connection.m_fromNodeID ) );
                        Pin* pStartPin = pFromNode->GetOutputPin( connection.m_outputPinID );
                        FlowNode* pToNode = Cast<FlowNode>( pFlowGraph->GetNode( connection.m_toNodeID ) );
                        Pin* pEndPin = pToNode->GetInputPin( connection.m_inputPinID );

                        bool const invertOrder = pStartPin->m_position.m_x > pEndPin->m_position.m_x;
                        ImVec2 const& p1 = invertOrder ? pEndPin->m_position : pStartPin->m_position;
                        ImVec2 const& p4 = invertOrder ? pStartPin->m_position : pEndPin->m_position;
                        ImVec2 const p2 = p1 + ImVec2( 50, 0 );
                        ImVec2 const p3 = p4 + ImVec2( -50, 0 );

                        Color connectionColor = pFromNode->GetPinColor( *pStartPin );
                        if ( m_hasFocus && IsHoveredOverCurve( p1, p2, p3, p4, drawingContext.m_mouseScreenPos, g_connectionSelectionExtraRadius ) )
                        {
                            m_hoveredConnectionID = connection.m_ID;
                            connectionColor = Color( Style::s_connectionColorHovered );
                        }

                        drawingContext.m_pDrawList->AddBezierCubic( p1, p2, p3, p4, connectionColor, Math::Max( 1.0f, 3.0f * drawingContext.m_viewScaleFactor ) );
                    }
                }

                // Extra
                //-------------------------------------------------------------------------

                m_pGraph->DrawExtraInformation( drawingContext, m_pUserContext );
            }

            // Restore original scale value
            //-------------------------------------------------------------------------

            ImGui::GetStyle() = unscaledStyle;
        }

        EndDrawCanvas( drawingContext );

        //-------------------------------------------------------------------------

        HandleInput( typeRegistry, drawingContext );
        HandleContextMenu( drawingContext );
        DrawDialogs();

        // Handle drag and drop
        //-------------------------------------------------------------------------

        auto ResetDragAndDropState = [this] { if ( m_dragAndDropState.m_isActiveDragAndDrop ) m_dragAndDropState.Reset(); };

        if ( m_pGraph != nullptr )
        {
            if ( m_isReadOnly )
            {
                ResetDragAndDropState();
            }
            else // Drag and drop allowed
            {
                if ( m_dragAndDropState.m_isActiveDragAndDrop )
                {
                    if ( m_pGraph->HandleDragAndDrop( m_pUserContext, m_dragAndDropState ) )
                    {
                        ResetDragAndDropState();
                    }
                }
                else // Check for drag and drop
                {
                    if ( ImGui::BeginDragDropTarget() )
                    {
                        TInlineVector<char const*, 10> supportedPayloadIDs;
                        m_pGraph->GetSupportedDragAndDropPayloadIDs( supportedPayloadIDs );

                        for ( auto pPayloadID : supportedPayloadIDs )
                        {
                            if ( m_dragAndDropState.TryAcceptDragAndDrop( pPayloadID ) )
                            {
                                m_dragAndDropState.m_mouseCanvasPos = drawingContext.m_mouseCanvasPos;
                                m_dragAndDropState.m_mouseScreenPos = drawingContext.m_mouseScreenPos;
                                m_dragAndDropState.m_isActiveDragAndDrop = true;
                                break;
                            }
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
            }
        }
        else
        {
            ResetDragAndDropState();
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
            ImRect totalRect = ImRect( m_pGraph->m_nodes[0]->GetPosition(), ImVec2( m_pGraph->m_nodes[0]->GetPosition() ) + m_pGraph->m_nodes[0]->GetSize() );
            for ( int32_t i = 1; i < numNodes; i++ )
            {
                TTypeInstance<BaseNode>& nodeInstance = m_pGraph->m_nodes[i];
                if ( nodeInstance->IsVisible() )
                {
                    ImRect const nodeRect( nodeInstance->GetPosition(), ImVec2( nodeInstance->GetPosition() ) + nodeInstance->GetSize() );
                    totalRect.Add( nodeRect );
                }
            }

            *m_pViewOffset = totalRect.GetCenter() - ( m_canvasSize / 2 );
        }
    }

    void GraphView::CenterView( BaseNode const* pNode )
    {
        EE_ASSERT( m_pGraph != nullptr );
        EE_ASSERT( pNode != nullptr && m_pGraph->FindNode( pNode->GetID() ) != nullptr );

        ImVec2 const nodeHalfSize = ( pNode->GetSize() / 2 );
        ImVec2 const nodeCenter = ImVec2( pNode->GetPosition() ) + nodeHalfSize;
        *m_pViewOffset = nodeCenter - ( m_canvasSize / 2 );
    }

    void GraphView::RefreshNodeSizes()
    {
        if ( m_pGraph != nullptr )
        {
            for ( TTypeInstance<BaseNode>& nodeInstance : m_pGraph->m_nodes )
            {
                nodeInstance->m_size = ImVec2( 0, 0 );
            }
        }
    }

    //-------------------------------------------------------------------------

    void GraphView::SelectNode( BaseNode const* pNode )
    {
        EE_ASSERT( GetViewedGraph()->FindNode( pNode->GetID() ) != nullptr );
        ClearSelection();
        AddToSelection( const_cast<BaseNode*>( pNode ) );
    }

    void GraphView::SelectNodes( TVector<BaseNode const*> pNodes )
    {
        ClearSelection();

        for ( auto pNode : pNodes )
        {
            EE_ASSERT( GetViewedGraph()->FindNode( pNode->GetID() ) != nullptr );
            AddToSelection( const_cast<BaseNode*>( pNode ) );
        }
    }

    void GraphView::ClearSelection()
    {
        TVector<SelectedNode> oldSelection;
        oldSelection.swap( m_selectedNodes );
        m_selectionChanged = true;
        m_pUserContext->NotifySelectionChanged( oldSelection, m_selectedNodes );
    }

    void GraphView::UpdateSelection( BaseNode* pNewSelectedNode )
    {
        EE_ASSERT( pNewSelectedNode != nullptr );

        TVector<SelectedNode> oldSelection;
        oldSelection.swap( m_selectedNodes );
        m_selectedNodes.emplace_back( pNewSelectedNode );
        m_selectionChanged = true;
        m_pUserContext->NotifySelectionChanged( oldSelection, m_selectedNodes );
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
        m_selectionChanged = true;
        m_pUserContext->NotifySelectionChanged( oldSelection, m_selectedNodes );
    }

    void GraphView::UpdateSelection( TVector<SelectedNode> const& newSelection )
    {
        for ( auto& selectedNode : newSelection )
        {
            EE_ASSERT( selectedNode.m_pNode != nullptr );
        }

        TVector<SelectedNode> oldSelection;
        oldSelection.swap( m_selectedNodes );
        m_selectedNodes = newSelection;
        m_selectionChanged = true;
        m_pUserContext->NotifySelectionChanged( oldSelection, m_selectedNodes );
    }

    void GraphView::AddToSelection( BaseNode* pNodeToAdd )
    {
        EE_ASSERT( pNodeToAdd != nullptr );
        EE_ASSERT( !IsNodeSelected( pNodeToAdd ) );

        TVector<SelectedNode> oldSelection;
        oldSelection = m_selectedNodes;
        m_selectedNodes.emplace_back( pNodeToAdd );
        m_selectionChanged = true;
        m_pUserContext->NotifySelectionChanged( oldSelection, m_selectedNodes );
    }

    void GraphView::RemoveFromSelection( BaseNode* pNodeToRemove )
    {
        EE_ASSERT( pNodeToRemove != nullptr );
        EE_ASSERT( IsNodeSelected( pNodeToRemove ) );

        TVector<SelectedNode> oldSelection = m_selectedNodes;
        m_selectedNodes.erase_first( pNodeToRemove );
        m_selectionChanged = true;
        m_pUserContext->NotifySelectionChanged( oldSelection, m_selectedNodes );
    }

    void GraphView::DestroySelectedNodes()
    {
        auto pGraph = GetViewedGraph();

        EE_ASSERT( !m_isReadOnly );
        ScopedGraphModification sgm( pGraph );

        // Exclude any state machine transitions, as we will end up double deleting them since they are removed if the state is removed
        if ( IsViewingStateMachineGraph() )
        {
            for ( int32_t i = (int32_t) m_selectedNodes.size() - 1; i >= 0; i-- )
            {
                if ( auto pConduit = TryCast<TransitionConduitNode>( m_selectedNodes[i].m_pNode ) )
                {
                    m_selectedNodes.erase_unsorted( m_selectedNodes.begin() + i );
                }
            }
        }

        // Delete selected nodes
        for ( SelectedNode const& selectedNode : m_selectedNodes )
        {
            if ( pGraph->CanDestroyNode( selectedNode.m_pNode ) && selectedNode.m_pNode->IsDestroyable() )
            {
                pGraph->DestroyNode( selectedNode.m_nodeID );
            }
        }

        ClearSelection();
    }

    void GraphView::CreateCommentAroundSelectedNodes()
    {
        if ( m_selectedNodes.empty() )
        {
            return;
        }

        // Calculate comment size
        //-------------------------------------------------------------------------

        int32_t const numNodes = (int32_t) m_selectedNodes.size();
        ImRect selectedNodesRect = ImRect( m_selectedNodes[0].m_pNode->GetPosition(), ImVec2( m_selectedNodes[0].m_pNode->GetPosition() ) + m_selectedNodes[0].m_pNode->GetSize() );
        for ( int32_t i = 1; i < numNodes; i++ )
        {
            auto pNode = m_selectedNodes[i].m_pNode;
            if ( pNode->IsVisible() )
            {
                ImRect const nodeRect( pNode->GetPosition(), ImVec2( pNode->GetPosition() ) + pNode->GetSize() );
                selectedNodesRect.Add( nodeRect );
            }
        }

        Float2 const commentPadding( 60, 60 );
        Float2 const canvasPos = selectedNodesRect.Min - commentPadding;
        Float2 const canvasBoxSize = selectedNodesRect.GetSize() + ( commentPadding * 2 );

        // Create comment
        //-------------------------------------------------------------------------

        ScopedGraphModification sgm( m_pGraph );
        auto pCommentNode = m_pGraph->CreateNode<CommentNode>();
        pCommentNode->m_name = "Comment";
        pCommentNode->m_canvasPosition = canvasPos;
        pCommentNode->m_commentBoxSize = canvasBoxSize;
    }

    void GraphView::CopySelectedNodes( TypeSystem::TypeRegistry const& typeRegistry )
    {
        if ( m_selectedNodes.empty() )
        {
            return;
        }

        // Prepare list of nodes to copy
        //-------------------------------------------------------------------------

        TInlineVector<BaseNode*, 10> nodesToCopy;
        for ( auto const& selectedNode : m_selectedNodes )
        {
            // Do not copy any selected conduits, as conduits are automatically copied!!!
            if ( auto pConduit = TryCast<TransitionConduitNode>( selectedNode.m_pNode ) )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            if ( selectedNode.m_pNode->IsUserCreatable() )
            {
                nodesToCopy.emplace_back( selectedNode.m_pNode );
            }
        }

        // Ensure that all transitions between copied states are also copied
        if ( IsViewingStateMachineGraph() )
        {
            for ( TTypeInstance<BaseNode>& nodeInstance : m_pGraph->m_nodes )
            {
                if ( auto pConduit = nodeInstance.TryGetAs<TransitionConduitNode>() )
                {
                    // If the conduit is already copied, then do nothing
                    if ( VectorContains( nodesToCopy, pConduit ) )
                    {
                        continue;
                    }

                    // If there exists a conduit between two copied states, then serialize it
                    auto SearchPredicate = [] ( BaseNode const* pNode, UUID const& ID ) { return pNode->GetID() == ID; };
                    bool const wasStartStateCopied = VectorContains( nodesToCopy, pConduit->GetStartStateID(), SearchPredicate );
                    bool const wasEndStateCopied = VectorContains( nodesToCopy, pConduit->GetEndStateID(), SearchPredicate );
                    if ( wasStartStateCopied && wasEndStateCopied )
                    {
                        nodesToCopy.emplace_back( pConduit );
                    }
                }
            }
        }

        // Serialize nodes and connections
        //-------------------------------------------------------------------------

        xml_document copiedData;
        xml_node dataNode = copiedData.append_child( s_graphCopiedDataNodeName );

        // Serialize nodes
        {
            xml_node copiedNodesNode = dataNode.append_child( s_copiedNodesNodeName );

            for ( auto pNode : nodesToCopy )
            {
                pNode->PreCopy();
                Serialization::WriteType( typeRegistry, pNode, copiedNodesNode );
            }
        }

        // Serialize node connections
        if ( IsViewingFlowGraph() )
        {
            auto pFlowGraph = GetFlowGraph();

            xml_node copiedConnectionsNode = dataNode.append_child( s_copiedConnectionsNodeName );

            for ( auto const& connection : pFlowGraph->m_connections )
            {
                auto SearchPredicate = [] ( BaseNode const* pNode, UUID const& nodeID ) { return pNode->GetID() == nodeID; };
                bool const wasFromNodeCopied = VectorContains( nodesToCopy, connection.m_fromNodeID, SearchPredicate );
                bool const wasToNodeCopied = VectorContains( nodesToCopy, connection.m_toNodeID, SearchPredicate );
                if ( wasFromNodeCopied && wasToNodeCopied )
                {
                    Serialization::WriteType( typeRegistry, &connection, copiedConnectionsNode );
                }
            }
        }

        //-------------------------------------------------------------------------

        String xmlStr;
        Serialization::WriteXmlToString( copiedData, xmlStr );
        ImGui::SetClipboardText( xmlStr.c_str() );
    }

    void GraphView::PasteNodes( TypeSystem::TypeRegistry const& typeRegistry, ImVec2 const& canvasPastePosition )
    {
        EE_ASSERT( m_pGraph != nullptr );

        if ( m_isReadOnly )
        {
            return;
        }

        // Get pasted data
        //-------------------------------------------------------------------------

        char const* pClipboardText = ImGui::GetClipboardText();
        if ( pClipboardText == nullptr )
        {
            return;
        }

        xml_document document;
        if ( !Serialization::ReadXmlFromString( pClipboardText, document ) )
        {
            return;
        }

        xml_node copiedDataNode = document.child( s_graphCopiedDataNodeName );
        if ( copiedDataNode.empty() )
        {
            return;
        }

        // Deserialize pasted nodes and regenerated IDs
        //-------------------------------------------------------------------------

        xml_node copiedNodesNode = copiedDataNode.child( s_copiedNodesNodeName );
        TInlineVector<xml_node, 10> graphNodeNodes;
        Serialization::GetAllChildNodes( copiedNodesNode, Serialization::g_typeNodeName, graphNodeNodes );

        if ( graphNodeNodes.empty() )
        {
            return;
        }

        THashMap<UUID, UUID> IDMapping;
        TInlineVector<BaseNode*, 20> pastedNodes;
        for ( xml_node& graphNodeNode : copiedNodesNode )
        {
            auto pPastedNode = Cast<BaseNode>( Serialization::TryCreateAndReadType( typeRegistry, graphNodeNode ) );

            if ( m_pGraph->CanCreateNode( pPastedNode->GetTypeInfo() ) )
            {
                // Set parent graph ptr
                pPastedNode->m_pParentGraph = m_pGraph;

                // Set child graph parent ptrs
                if ( pPastedNode->HasChildGraph() )
                {
                    pPastedNode->GetChildGraph()->m_pParentNode = pPastedNode;
                }

                // Set secondary graph parent ptrs
                if ( pPastedNode->HasSecondaryGraph() )
                {
                    pPastedNode->GetSecondaryGraph()->m_pParentNode = pPastedNode;
                }

                pPastedNode->RegenerateIDs( IDMapping );
                pPastedNode->PostPaste();
                pastedNodes.emplace_back( pPastedNode );
            }
            else
            {
                pPastedNode->m_pParentGraph = nullptr;
                pPastedNode->m_ID.Clear();
                EE::Delete( pPastedNode );
            }
        }

        if ( pastedNodes.empty() )
        {
            return;
        }

        // Add nodes to the graph
        //-------------------------------------------------------------------------

        EE_ASSERT( !m_isReadOnly );
        m_pGraph->BeginModification();

        for ( BaseNode* pPastedNode : pastedNodes )
        {
            EE_ASSERT( pPastedNode->m_pParentGraph == m_pGraph );
            if ( pPastedNode->IsRenameable() )
            {
                pPastedNode->Rename( pPastedNode->GetName() ); // Ensures a unique name if needed
            }
            m_pGraph->m_nodes.emplace_back( pPastedNode );
            m_pGraph->OnNodeAdded( pPastedNode );
        }

        // Serialize and fix connections
        //-------------------------------------------------------------------------

        if ( IsViewingFlowGraph() )
        {
            FlowGraph* pFlowGraph = GetFlowGraph();

            xml_node copiedConnectionsNode = copiedDataNode.child( s_copiedConnectionsNodeName );
            TInlineVector<xml_node, 10> connectionNodes;
            Serialization::GetAllChildNodes( copiedConnectionsNode, Serialization::g_typeNodeName, connectionNodes );

            for ( xml_node connectionNode : connectionNodes )
            {
                FlowGraph::Connection* pPastedConnection = Cast<FlowGraph::Connection>( Serialization::TryCreateAndReadType( typeRegistry, connectionNode ) );

                pPastedConnection->m_fromNodeID = IDMapping.at( pPastedConnection->m_fromNodeID );
                pPastedConnection->m_outputPinID = IDMapping.at( pPastedConnection->m_outputPinID );
                pPastedConnection->m_toNodeID = IDMapping.at( pPastedConnection->m_toNodeID );
                pPastedConnection->m_inputPinID = IDMapping.at( pPastedConnection->m_inputPinID );

                if ( pFlowGraph->IsValidConnection( pPastedConnection->m_fromNodeID, pPastedConnection->m_outputPinID, pPastedConnection->m_toNodeID, pPastedConnection->m_inputPinID ) )
                {
                    pFlowGraph->m_connections.emplace_back( *pPastedConnection );
                }

                EE::Delete( pPastedConnection );
            }
        }
        else // State Machine
        {
            for ( auto pPastedNode : pastedNodes )
            {
                if ( auto pConduit = TryCast<TransitionConduitNode>( pPastedNode ) )
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
            if ( pastedNodes[i]->GetPosition().m_x < leftMostNodePosition.m_x )
            {
                leftMostNodePosition = pastedNodes[i]->GetPosition();
            }
        }

        for ( int32_t i = 0; i < numPastedNodes; i++ )
        {
            pastedNodes[i]->SetPosition( pastedNodes[i]->GetPosition() - leftMostNodePosition + Float2( canvasPastePosition ) );
        }

        // Notify graph that nodes were pasted
        //-------------------------------------------------------------------------

        m_pGraph->PostPasteNodes( pastedNodes );
        m_pUserContext->NotifyNodesPasted( pastedNodes );
        m_pGraph->EndModification();
    }

    void GraphView::BeginRenameNode( BaseNode* pNode )
    {
        EE_ASSERT( pNode != nullptr && pNode->IsRenameable() );
        EE::Printf( m_textBuffer, 255, pNode->GetName() );
        m_pNodeBeingOperatedOn = pNode;
    }

    void GraphView::EndRenameNode( bool shouldUpdateNode )
    {
        EE_ASSERT( m_pNodeBeingOperatedOn != nullptr && m_pNodeBeingOperatedOn->IsRenameable() );

        if ( shouldUpdateNode )
        {
            // Set new name
            //-------------------------------------------------------------------------

            EE_ASSERT( !m_isReadOnly );
            ScopedNodeModification const snm( m_pNodeBeingOperatedOn );
            auto pParentGraph = m_pNodeBeingOperatedOn->GetParentGraph();
            m_pNodeBeingOperatedOn->Rename( m_textBuffer );

            if ( IsOfType<CommentNode>( m_pNodeBeingOperatedOn ) )
            {
                
            }
            else
            {
                String const uniqueName = pParentGraph->GetUniqueNameForRenameableNode( m_textBuffer, m_pNodeBeingOperatedOn );
                m_pNodeBeingOperatedOn->Rename( uniqueName );
            }
        }

        m_pNodeBeingOperatedOn = nullptr;
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

        //-------------------------------------------------------------------------

        ImVec2 mouseDragDelta = ImVec2( 0, 0 );

        if ( ImGui::IsMouseDown( ImGuiMouseButton_Right ) )
        {
            mouseDragDelta = ImGui::GetMouseDragDelta( ImGuiMouseButton_Right );
        }
        else if ( ImGui::IsMouseDown( ImGuiMouseButton_Middle ) )
        {
            mouseDragDelta = ImGui::GetMouseDragDelta( ImGuiMouseButton_Middle );
        }
        else
        {
            StopDraggingView( ctx );
            return;
        }

        m_dragState.m_lastFrameDragDelta = mouseDragDelta;

        //-------------------------------------------------------------------------

        *m_pViewOffset =  m_dragState.m_startValue - ctx.WindowToCanvas( mouseDragDelta );
    }

    void GraphView::StopDraggingView( DrawContext const& ctx )
    {
        m_dragState.Reset();
    }

    //-------------------------------------------------------------------------

    void GraphView::StartDraggingSelection( DrawContext const& ctx )
    {
        EE_ASSERT( m_pGraph != nullptr );
        EE_ASSERT( m_dragState.m_mode == DragMode::None );
        m_dragState.m_mode = DragMode::Selection;
        m_dragState.m_startValue = ImGui::GetMousePos();
    }

    void GraphView::OnDragSelection( DrawContext const& ctx )
    {
        EE_ASSERT( m_pGraph != nullptr );

        if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
        {
            StopDraggingSelection( ctx );
            return;
        }

        ctx.m_pDrawList->AddRectFilled( m_dragState.m_startValue, ImGui::GetMousePos(), g_selectionBoxFillColor );
        ctx.m_pDrawList->AddRect( m_dragState.m_startValue, ImGui::GetMousePos(), g_selectionBoxOutlineColor );
    }

    void GraphView::StopDraggingSelection( DrawContext const& ctx )
    {
        EE_ASSERT( m_pGraph != nullptr );

        ImVec2 const mousePos = ImGui::GetMousePos();
        ImVec2 const min( Math::Min( m_dragState.m_startValue.x, mousePos.x ), Math::Min( m_dragState.m_startValue.y, mousePos.y ) );
        ImVec2 const max( Math::Max( m_dragState.m_startValue.x, mousePos.x ), Math::Max( m_dragState.m_startValue.y, mousePos.y ) );
        ImRect const selectionWindowRect( min - ctx.m_windowRect.Min, max - ctx.m_windowRect.Min );

        TVector<SelectedNode> newSelection;
        for ( TTypeInstance<BaseNode>& nodeInstance : GetViewedGraph()->m_nodes )
        {
            ImRect const nodeWindowRect = ctx.CanvasToWindow( nodeInstance->GetRect() );
            if ( !nodeWindowRect.Contains( selectionWindowRect ) )
            {
                if ( selectionWindowRect.Overlaps( nodeWindowRect ) )
                {
                    newSelection.emplace_back( nodeInstance.Get() );
                }
            }
        }

        UpdateSelection( eastl::move( newSelection ) );
        m_dragState.Reset();
    }

    //-------------------------------------------------------------------------

    void GraphView::StartDraggingNode( DrawContext const& ctx )
    {
        EE_ASSERT( !m_isReadOnly );
        EE_ASSERT( m_dragState.m_mode == DragMode::None );
        EE_ASSERT( m_dragState.m_pNode != nullptr );

        m_dragState.m_mode = DragMode::Node;
        m_dragState.m_startValue = m_dragState.m_pNode->GetPosition();

        //-------------------------------------------------------------------------

        auto ProcessCommentNode = [this] ( CommentNode* pCommentNode )
        {
            ImRect const& unscaledCommentRect = pCommentNode->GetRect();
            for ( TTypeInstance<BaseNode>& nodeInstance : m_pGraph->m_nodes )
            {
                if ( unscaledCommentRect.Contains( nodeInstance->GetRect() ) )
                {
                    VectorEmplaceBackUnique( m_dragState.m_draggedNodes, nodeInstance.Get() );
                }
            }
        };

        // Create the dragged set of nodes
        for ( auto const& selectedNode : m_selectedNodes )
        {
            VectorEmplaceBackUnique( m_dragState.m_draggedNodes, selectedNode.m_pNode );

            // Add all enclosed nodes to the dragged set
            if ( auto pCommentNode = TryCast<CommentNode>( m_dragState.m_draggedNodes.back() ) )
            {
                ProcessCommentNode( pCommentNode );
            }
        }

        // Add any child comments nodes as well
        for ( int32_t i = 0; i < m_dragState.m_draggedNodes.size(); i++ )
        {
            if ( auto pCommentNode = TryCast<CommentNode>( m_dragState.m_draggedNodes[i] ) )
            {
                ProcessCommentNode( pCommentNode );
            }
        }

        GetViewedGraph()->BeginModification();
    }

    void GraphView::OnDragNode( DrawContext const& ctx )
    {
        EE_ASSERT( !m_isReadOnly );
        EE_ASSERT( m_dragState.m_mode == DragMode::Node );

        if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
        {
            StopDraggingNode( ctx );
            return;
        }

        //-------------------------------------------------------------------------

        ImVec2 const mouseDragDelta = ImGui::GetMouseDragDelta( ImGuiMouseButton_Left );
        EE_ASSERT( ctx.m_viewScaleFactor != 0.0f );
        Float2 const canvasFrameDragDelta = ctx.WindowToCanvas( mouseDragDelta - m_dragState.m_lastFrameDragDelta );
        m_dragState.m_lastFrameDragDelta = mouseDragDelta;

        for ( auto const& pDraggedNode : m_dragState.m_draggedNodes )
        {
            pDraggedNode->SetPosition( pDraggedNode->GetPosition() + canvasFrameDragDelta );
        }
    }

    void GraphView::StopDraggingNode( DrawContext const& ctx )
    {
        EE_ASSERT( !m_isReadOnly );
        m_dragState.Reset();
        GetViewedGraph()->EndModification();
    }

    //-------------------------------------------------------------------------

    void GraphView::TryGetAutoConnectionNodeAndPin( DrawContext const& ctx, FlowNode*& pOutNode, Pin*& pOutPin ) const
    {
        auto pFlowGraph = GetFlowGraph();
        auto pDraggedFlowNode = m_dragState.GetAsFlowNode();
        float const autoConnectThreshold = ( ctx.m_viewScaleFactor * g_autoConnectMaxDistanceThreshold );

        //-------------------------------------------------------------------------

        pOutNode = nullptr;
        pOutPin = nullptr;

        // Find nodes within detection distance
        //-------------------------------------------------------------------------

        TInlineVector<FlowNode*, 10> options;

        for ( TTypeInstance<BaseNode>& pNode : pFlowGraph->m_nodes )
        {
            if ( m_dragState.m_pNode == pNode.Get() )
            {
                continue;
            }

            ImVec2 const closestPointToRect = ImGuiX::GetClosestPointOnRectBorder( pNode->GetRect(), ctx.m_mouseCanvasPos );
            float const distanceToNodeRect = Vector( closestPointToRect ).Distance2( ctx.m_mouseCanvasPos ).ToFloat();
            if ( distanceToNodeRect < autoConnectThreshold )
            {
                options.emplace_back( pNode.GetAs<FlowNode>() );
            }
        }

        // Find closest pin on across all options
        //-------------------------------------------------------------------------

        if ( !options.empty() )
        {
            float closestValidPinDistance = FLT_MAX;

            auto EvaluatePinOption = [&] ( FlowNode* pNode, Pin const& pin )
            {
                float distanceToPin = Vector( pin.m_position ).Distance2( ctx.m_mouseScreenPos ).ToFloat();
                if ( distanceToPin < autoConnectThreshold && distanceToPin < closestValidPinDistance )
                {
                    closestValidPinDistance = distanceToPin;
                    pOutNode = pNode;
                    pOutPin = const_cast<Pin*>( &pin );
                }
            };

            for ( FlowNode* pOptionNode : options )
            {
                Pin::Direction const draggedPinDirection = Cast<FlowNode>( m_dragState.m_pNode )->GetPinDirection( m_dragState.m_pPin );
                if ( draggedPinDirection == Pin::Direction::Output )
                {
                    for ( auto& pin : pOptionNode->GetInputPins() )
                    {
                        if ( pFlowGraph->IsValidConnection( pDraggedFlowNode, m_dragState.m_pPin, pOptionNode, &pin ) )
                        {
                            EvaluatePinOption( pOptionNode, pin );
                        }
                    }
                }
                else // Dragging an input pin
                {
                    for ( auto& pin : pOptionNode->GetOutputPins() )
                    {
                        if ( pFlowGraph->IsValidConnection( pOptionNode, &pin, pDraggedFlowNode, m_dragState.m_pPin ) )
                        {
                            EvaluatePinOption( pOptionNode, pin );
                        }
                    }
                }
            }
        }
    }

    void GraphView::StartDraggingConnection( DrawContext const& ctx )
    {
        EE_ASSERT( !m_isReadOnly );
        EE_ASSERT( m_dragState.m_mode == DragMode::None );
        EE_ASSERT( m_dragState.m_pNode != nullptr );

        m_dragState.m_mode = DragMode::Connection;

        if ( IsViewingStateMachineGraph() )
        {
            m_dragState.m_startValue = ctx.CanvasToScreenPosition( m_dragState.m_pNode->GetPosition() );
        }
        else
        {
            m_dragState.m_startValue = m_dragState.m_pPin->m_position;
        }
    }

    void GraphView::OnDragConnection( DrawContext const& ctx )
    {
        EE_ASSERT( !m_isReadOnly );
        EE_ASSERT( m_dragState.m_mode == DragMode::Connection );

        auto const& IO = ImGui::GetIO();

        if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
        {
            StopDraggingConnection( ctx );
            return;
        }

        //-------------------------------------------------------------------------

        if ( IsViewingStateMachineGraph() )
        {
            auto pStateMachineGraph = GetStateMachineGraph();
            auto pEndState = TryCast<StateNode>( m_pHoveredNode );

            bool const isValidConnection = pEndState != nullptr && pStateMachineGraph->CanCreateTransitionConduit( Cast<StateNode>( m_dragState.m_pNode ), pEndState );
            Color const connectionColor = isValidConnection ? Style::s_connectionColorValid : Style::s_connectionColorInvalid;
            ImGuiX::DrawArrow( ctx.m_pDrawList, ctx.CanvasToScreenPosition( m_dragState.m_pNode->GetRect().GetCenter() ), ctx.m_mouseScreenPos, connectionColor, g_transitionArrowWidth );
        }
        else
        {
            auto pFlowGraph = GetFlowGraph();
            auto pDraggedFlowNode = m_dragState.GetAsFlowNode();
            Color connectionColor = pDraggedFlowNode->GetPinColor( *m_dragState.m_pPin );

            // Auto connection
            //-------------------------------------------------------------------------

            if ( IO.KeyAlt )
            {
                FlowNode* pAutoConnectNode = nullptr;
                Pin* pAutoConnectPin = nullptr;
                TryGetAutoConnectionNodeAndPin( ctx, pAutoConnectNode, pAutoConnectPin );

                if ( pAutoConnectPin != nullptr )
                {
                    ctx.m_pDrawList->AddLine( ctx.m_mouseScreenPos, pAutoConnectPin->m_position, Colors::GreenYellow.GetAlphaVersion( 0.85f ), 4.0f * ctx.m_viewScaleFactor );
                }
            }

            // Validity check for hovered pin
            //-------------------------------------------------------------------------

            if ( m_pHoveredPin != nullptr )
            {
                auto pHoveredFlowNode = Cast<FlowNode>( m_pHoveredNode );
                Pin::Direction const hoveredPinDirection = pHoveredFlowNode->GetPinDirection( m_pHoveredPin );
                Pin::Direction const draggedPinDirection = Cast<FlowNode>( m_dragState.m_pNode )->GetPinDirection( m_dragState.m_pPin );

                // Trying to make an invalid connection to a pin with the same direction or the same node
                if ( hoveredPinDirection == draggedPinDirection || m_pHoveredNode == m_dragState.m_pNode )
                {
                    connectionColor = Style::s_connectionColorInvalid;
                }
                else // Check connection validity
                {
                    if ( draggedPinDirection == Pin::Direction::Output )
                    {
                        if ( !pFlowGraph->IsValidConnection( pDraggedFlowNode, m_dragState.m_pPin, pHoveredFlowNode, m_pHoveredPin ) )
                        {
                            connectionColor = Style::s_connectionColorInvalid;
                        }
                    }
                    else // The hovered pin is the output pin
                    {
                        if ( !pFlowGraph->IsValidConnection( pHoveredFlowNode, m_pHoveredPin, pDraggedFlowNode, m_dragState.m_pPin ) )
                        {
                            connectionColor = Style::s_connectionColorInvalid;
                        }
                    }
                }
            }

            //-------------------------------------------------------------------------

            bool const invertOrder = m_dragState.m_pPin->m_position.m_x > ctx.m_mouseScreenPos.x;
            ImVec2 const& p1 = invertOrder ? ctx.m_mouseScreenPos : ImVec2( m_dragState.m_pPin->m_position );
            ImVec2 const& p2 = invertOrder ? ImVec2( m_dragState.m_pPin->m_position ) : ctx.m_mouseScreenPos;
            ctx.m_pDrawList->AddBezierCubic( p1, p1 + Float2( +50, 0 ), p2 + Float2( -50, 0 ), p2, connectionColor, 3.0f * ctx.m_viewScaleFactor );
        }
    }

    void GraphView::StopDraggingConnection( DrawContext const& ctx )
    {
        EE_ASSERT( !m_isReadOnly );

        if ( IsViewingStateMachineGraph() )
        {
            StateMachineGraph* pStateMachineGraph = GetStateMachineGraph();
            auto pStartState = Cast<StateNode>( m_dragState.m_pNode );
            auto pEndState = TryCast<StateNode>( m_pHoveredNode );
            if ( pEndState != nullptr && pStateMachineGraph->CanCreateTransitionConduit( pStartState, pEndState ) )
            {
                ScopedGraphModification sgm( pStateMachineGraph );
                pStateMachineGraph->CreateTransitionConduit( pStartState, pEndState );
            }
        }
        else
        {
            FlowGraph* pFlowGraph = GetFlowGraph();

            if( m_pHoveredPin != nullptr )
            {
                auto pHoveredFlowNode = Cast<FlowNode>( m_pHoveredNode );
                Pin::Direction const hoveredPinDirection = pHoveredFlowNode->GetPinDirection( m_pHoveredPin );
                Pin::Direction const draggedPinDirection = Cast<FlowNode>( m_dragState.m_pNode )->GetPinDirection( m_dragState.m_pPin );

                if ( hoveredPinDirection != draggedPinDirection )
                {
                    if ( draggedPinDirection == Pin::Direction::Output )
                    {
                        pFlowGraph->TryMakeConnection( m_dragState.GetAsFlowNode(), m_dragState.m_pPin, pHoveredFlowNode, m_pHoveredPin );
                    }
                    else // The hovered pin is the output pin
                    {
                        pFlowGraph->TryMakeConnection( pHoveredFlowNode, m_pHoveredPin, m_dragState.GetAsFlowNode(), m_dragState.m_pPin );
                    }
                }
            }
            else
            {
                // Auto connection
                //-------------------------------------------------------------------------

                bool showNodeCreationMenu = true;
                auto const& IO = ImGui::GetIO();
                if ( IO.KeyAlt )
                {
                    FlowNode* pAutoConnectNode = nullptr;
                    Pin* pAutoConnectPin = nullptr;
                    TryGetAutoConnectionNodeAndPin( ctx, pAutoConnectNode, pAutoConnectPin );

                    if ( pAutoConnectPin != nullptr )
                    {
                        Pin::Direction const draggedPinDirection = Cast<FlowNode>( m_dragState.m_pNode )->GetPinDirection( m_dragState.m_pPin );
                        if ( draggedPinDirection == Pin::Direction::Output )
                        {
                            pFlowGraph->TryMakeConnection( m_dragState.GetAsFlowNode(), m_dragState.m_pPin, pAutoConnectNode, pAutoConnectPin );
                        }
                        else // The auto-connect pin is the output pin
                        {
                            pFlowGraph->TryMakeConnection( pAutoConnectNode, pAutoConnectPin, m_dragState.GetAsFlowNode(), m_dragState.m_pPin );
                        }

                        showNodeCreationMenu = false;
                    }
                }

                // Node Creation
                //-------------------------------------------------------------------------

                if ( showNodeCreationMenu && pFlowGraph->SupportsNodeCreationFromConnection() )
                {
                    m_contextMenuState.m_pNode = m_dragState.m_pNode;
                    m_contextMenuState.m_pPin = m_dragState.m_pPin;
                    m_contextMenuState.m_requestOpenMenu = true;
                    m_contextMenuState.m_isAutoConnectMenu = true;
                }
            }
        }

        //-------------------------------------------------------------------------

        m_dragState.Reset();
    }

    //-------------------------------------------------------------------------

    void GraphView::StartResizingCommentBox( DrawContext const& ctx )
    {
        EE_ASSERT( !m_isReadOnly );
        EE_ASSERT( m_dragState.m_mode == DragMode::None );
        EE_ASSERT( m_dragState.m_pNode != nullptr );
        m_dragState.m_mode = DragMode::ResizeComment;

        GetViewedGraph()->BeginModification();
    }

    void GraphView::OnResizeCommentBox( DrawContext const& ctx )
    {
        if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
        {
            StopResizingCommentBox( ctx );
            return;
        }

        EE_ASSERT( m_dragState.m_mode == DragMode::ResizeComment );
        EE_ASSERT( m_dragState.m_pNode != nullptr );
        EE_ASSERT( m_dragState.m_resizeHandle != ResizeHandle::None );

        //-------------------------------------------------------------------------

        auto pCommentNode = Cast<CommentNode>( m_dragState.m_pNode );
        pCommentNode->AdjustSizeBasedOnMousePosition( ctx, m_dragState.m_resizeHandle );
    }

    void GraphView::StopResizingCommentBox( DrawContext const& ctx )
    {
        EE_ASSERT( !m_isReadOnly );
        m_dragState.Reset();
        GetViewedGraph()->EndModification();
    }

    //-------------------------------------------------------------------------

    void GraphView::HandleInput( TypeSystem::TypeRegistry const& typeRegistry, DrawContext const& ctx )
    {
        ImGuiIO const& IO = ImGui::GetIO();

        if ( m_pGraph == nullptr )
        {
            return;
        }

        // Zoom
        //-------------------------------------------------------------------------

        if ( m_isViewHovered && IO.MouseWheel != 0 )
        {
            float const desiredViewScale = Math::Clamp( m_pGraph->m_viewScaleFactor + IO.MouseWheel * 0.05f, 0.2f, 1.4f );
            ChangeViewScale( ctx, desiredViewScale );
        }

        // Allow selection without focus
        //-------------------------------------------------------------------------

        if ( m_isViewHovered )
        {
            if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
            {
                if ( IO.KeyCtrl || IO.KeyShift )
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
                else if ( IO.KeyAlt )
                {
                    if ( !m_isReadOnly )
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
                            if ( m_pHoveredNode != nullptr && IsOfType<TransitionConduitNode>( m_pHoveredNode ) )
                            {
                                ClearSelection();
                                m_pGraph->DestroyNode( m_pHoveredNode->GetID() );
                                m_pHoveredNode = nullptr;
                            }
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

                m_requestFocus = true;
            }
        }

        // Double Clicks
        //-------------------------------------------------------------------------

        if ( m_isViewHovered )
        {
            if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
            {
                if ( m_pHoveredNode != nullptr )
                {
                    if ( IsOfType<CommentNode>( m_pHoveredNode ) )
                    {
                        BeginRenameNode( m_pHoveredNode );
                    }
                    else
                    {
                        auto pNavChildGraph = m_pHoveredNode->GetNavigationTarget();
                        if ( pNavChildGraph != nullptr )
                        {
                            m_pUserContext->NavigateTo( pNavChildGraph );
                        }
                        else
                        {
                            m_pUserContext->DoubleClick( m_pHoveredNode );
                        }
                    }
                }
                else
                {
                    if ( m_pGraph != nullptr )
                    {
                        auto pNavParentGraph = m_pGraph->GetNavigationTarget();
                        if ( pNavParentGraph != nullptr )
                        {
                            m_pUserContext->NavigateTo( pNavParentGraph );
                        }
                        else
                        {
                            m_pUserContext->DoubleClick( m_pGraph );
                        }
                    }
                }

                m_requestFocus = true;
            }
        }

        // Dragging
        //-------------------------------------------------------------------------

        switch ( m_dragState.m_mode )
        {
            case DragMode::None:
            {
                if ( m_isViewHovered )
                {
                    // Update drag state resize handle
                    //-------------------------------------------------------------------------

                    if ( !m_dragState.m_dragReadyToStart )
                    {
                        auto pCommentNode = TryCast<CommentNode>( m_pHoveredNode );
                        if ( pCommentNode != nullptr )
                        {
                            m_dragState.m_resizeHandle = pCommentNode->GetHoveredResizeHandle( ctx );
                        }
                        else
                        {
                            m_dragState.m_resizeHandle = ResizeHandle::None;
                        }
                    }

                    // Prepare drag state - this needs to be done in advance since we only start the drag operation once we detect a mouse drag and at that point we might not be hovered over the node anymore
                    //-------------------------------------------------------------------------

                    if ( m_dragState.m_dragReadyToStart )
                    {
                        if ( !( ImGui::IsMouseDown( ImGuiMouseButton_Left ) || ImGui::IsMouseDown( ImGuiMouseButton_Right ) || ImGui::IsMouseDown( ImGuiMouseButton_Middle ) ) )
                        {
                            m_dragState.m_pNode = nullptr;
                            m_dragState.m_pPin = nullptr;
                            m_dragState.m_dragReadyToStart = false;
                        }
                    }
                    else
                    {
                        if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseClicked( ImGuiMouseButton_Right ) || ImGui::IsMouseClicked( ImGuiMouseButton_Middle ) )
                        {
                            m_dragState.m_pNode = m_pHoveredNode;
                            m_dragState.m_pPin = m_pHoveredPin;
                            m_dragState.m_dragReadyToStart = true;
                            m_requestFocus = true;
                        }
                    }

                    // Check for drag inputs
                    //-------------------------------------------------------------------------

                    if ( m_dragState.m_dragReadyToStart )
                    {
                        bool const nodeDragInput = ImGui::IsMouseDragging( ImGuiMouseButton_Left, 3 );
                        bool const viewDragInput = ImGui::IsMouseDragging( ImGuiMouseButton_Right, 3 ) || ImGui::IsMouseDragging( ImGuiMouseButton_Middle, 3 );

                        if ( nodeDragInput )
                        {
                            if ( !IsReadOnly() )
                            {
                                auto pCommentNode = TryCast<CommentNode>( m_dragState.m_pNode );
                                if ( pCommentNode != nullptr )
                                {
                                    if ( m_dragState.m_resizeHandle == ResizeHandle::None )
                                    {
                                        StartDraggingNode( ctx );
                                    }
                                    else
                                    {
                                        StartResizingCommentBox( ctx );
                                    }
                                }
                                else if ( IsViewingFlowGraph() )
                                {
                                    if ( m_dragState.m_pNode != nullptr )
                                    {
                                        if ( m_dragState.m_pPin != nullptr )
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
                                        if ( m_pGraph != nullptr )
                                        {
                                            StartDraggingSelection( ctx );
                                        }
                                    }
                                }
                                else // State Machine
                                {
                                    if ( m_dragState.m_pNode != nullptr )
                                    {
                                        if ( IO.KeyAlt && IsOfType<StateNode>( m_dragState.m_pNode ) )
                                        {
                                            StartDraggingConnection( ctx );
                                        }
                                        else if ( !IsOfType<TransitionConduitNode>( m_dragState.m_pNode ) )
                                        {
                                            StartDraggingNode( ctx );
                                        }
                                    }
                                    else
                                    {
                                        if ( m_pGraph != nullptr )
                                        {
                                            StartDraggingSelection( ctx );
                                        }
                                    }
                                }
                            }
                        }
                        else if ( viewDragInput )
                        {
                            StartDraggingView( ctx );
                        }
                        else if ( ImGui::IsMouseReleased( ImGuiMouseButton_Right ) )
                        {
                            m_contextMenuState.m_requestOpenMenu = true;
                        }

                        m_requestFocus = true;
                    }
                    else if ( ImGui::IsMouseReleased( ImGuiMouseButton_Right ) )
                    {
                        m_contextMenuState.m_requestOpenMenu = true;
                    }
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

            case DragMode::ResizeComment:
            {
                OnResizeCommentBox( ctx );
            }
            break;
        }

        // Mouse Cursor
        //-------------------------------------------------------------------------

        if ( IsDraggingView() )
        {
            ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
        }
        else // Change cursor to match current resize state
        {
            switch ( m_dragState.m_resizeHandle )
            {
                case ResizeHandle::N:
                ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeNS );
                break;

                case ResizeHandle::NW:
                ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeNWSE );
                break;

                case ResizeHandle::W:
                ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeEW );
                break;

                case ResizeHandle::SW:
                ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeNESW );
                break;

                case ResizeHandle::S:
                ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeNS );
                break;

                case ResizeHandle::SE:
                ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeNWSE );
                break;

                case ResizeHandle::E:
                ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeEW );
                break;

                case ResizeHandle::NE:
                ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeNESW );
                break;

                default:
                break;
            }
        }

        // Keyboard
        //-------------------------------------------------------------------------

        // These operation require the graph view to be focused!
        if ( m_hasFocus )
        {
            // General operations
            //-------------------------------------------------------------------------

            if ( IO.KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_C ) )
            {
                CopySelectedNodes( typeRegistry );
            }

            // Operations that modify the graph
            //-------------------------------------------------------------------------

            if( !m_isReadOnly )
            {
                if ( ImGui::IsKeyPressed( ImGuiKey_F2 ) )
                {
                    if ( m_selectedNodes.size() == 1 && m_selectedNodes[0].m_pNode->IsRenameable() )
                    {
                        BeginRenameNode( m_selectedNodes[0].m_pNode );
                    }
                }
                else if ( IO.KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_X ) )
                {
                    CopySelectedNodes( typeRegistry );
                    DestroySelectedNodes();
                }
                else if ( IO.KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_V ) )
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

                if ( !m_selectedNodes.empty() )
                {
                    if ( !ImGui::IsAnyItemActive() && ImGui::IsKeyPressed( ImGuiKey_Delete ) )
                    {
                        DestroySelectedNodes();
                    }

                    if ( IO.KeyShift && ImGui::IsKeyPressed( ImGuiKey_C ) )
                    {
                        CreateCommentAroundSelectedNodes();
                    }
                }
            }
        }
    }

    void GraphView::HandleContextMenu( DrawContext const& ctx )
    {
        if ( m_pGraph == nullptr )
        {
            return;
        }

        if( m_isViewHovered && !m_contextMenuState.m_menuOpened && m_contextMenuState.m_requestOpenMenu )
        {
            m_contextMenuState.m_mouseCanvasPos = ctx.m_mouseCanvasPos;
            m_contextMenuState.m_menuOpened = true;

            // If this isnt a auto-connect request, then use the hovered data
            if ( !m_contextMenuState.m_isAutoConnectMenu )
            {
                m_contextMenuState.m_pNode = m_pHoveredNode;
                m_contextMenuState.m_pPin = m_pHoveredPin;
            }

            // if we are opening a context menu for a node, set that node as selected
            if ( m_contextMenuState.m_pNode != nullptr )
            {
                UpdateSelection( m_contextMenuState.m_pNode );
            }

            ImGui::OpenPopupEx( GImGui->CurrentWindow->GetID( "GraphContextMenu" ) );
        }

        m_contextMenuState.m_requestOpenMenu = false;

        //-------------------------------------------------------------------------

        if ( IsContextMenuOpen() )
        {
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 8, 8 ) );
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4, 8 ) );
            if ( ImGui::BeginPopupContextItem( "GraphContextMenu" ) )
            {
                if ( IsOfType<CommentNode>( m_contextMenuState.m_pNode ) )
                {
                    DrawCommentContextMenu( ctx );
                }
                else if ( IsViewingFlowGraph() )
                {
                    DrawFlowGraphContextMenu( ctx );
                }
                else if ( IsViewingStateMachineGraph() )
                {
                    DrawStateMachineContextMenu( ctx );
                }

                ImGui::EndPopup();
            }
            else // Close menu
            {
                ImGui::SetWindowFocus();
                m_contextMenuState.Reset();
            }
            ImGui::PopStyleVar( 2 );
        }
    }

    void GraphView::DrawSharedContextMenuOptions( DrawContext const& ctx )
    {
        // View
        if ( ImGui::MenuItem( EE_ICON_FIT_TO_PAGE_OUTLINE" Reset View" ) )
        {
            ResetView();
        }

        // Zoom
        if ( m_pGraph != nullptr )
        {
            if ( m_pGraph->m_viewScaleFactor != 1.0f )
            {
                if ( ImGui::MenuItem( EE_ICON_MAGNIFY" Reset Zoom" ) )
                {
                    ChangeViewScale( ctx, 1.0f );
                }
            }
        }

        // Add comment node
        if ( m_pGraph->SupportsComments() )
        {
            if ( ImGui::MenuItem( EE_ICON_COMMENT" Add Comment" ) )
            {
                auto pCommentNode = m_pGraph->CreateNode<CommentNode>();
                pCommentNode->m_canvasPosition = m_contextMenuState.m_mouseCanvasPos;
                pCommentNode->m_commentBoxSize = ImVec2( 100, 100 );
            }
        }
    }

    void GraphView::DrawCommentContextMenu( DrawContext const& ctx )
    {
        auto pCommentNode = Cast<CommentNode>( m_contextMenuState.m_pNode );

        if ( ImGui::MenuItem( EE_ICON_COMMENT" Edit Comment" ) )
        {
            BeginRenameNode( m_selectedNodes[0].m_pNode );
        }

        if ( pCommentNode->DrawContextMenuOptions( ctx, m_pUserContext, m_contextMenuState.m_mouseCanvasPos ) )
        {
            m_contextMenuState.Reset();
            ImGui::CloseCurrentPopup();
        }
    }

    void GraphView::DrawFlowGraphContextMenu( DrawContext const& ctx )
    {
        EE_ASSERT( IsViewingFlowGraph() );

        auto pFlowGraph = GetFlowGraph();

        // Node Menu
        if ( !m_contextMenuState.m_isAutoConnectMenu && m_contextMenuState.m_pNode != nullptr )
        {
            EE_ASSERT( !IsOfType<CommentNode>( m_contextMenuState.m_pNode ) );

            auto pFlowNode = m_contextMenuState.GetAsFlowNode();

            // Default node Menu
            pFlowNode->DrawContextMenuOptions( ctx, m_pUserContext, m_contextMenuState.m_mouseCanvasPos, m_contextMenuState.m_pPin );

            //-------------------------------------------------------------------------

            if ( !m_isReadOnly )
            {
                ImGui::SeparatorText( "Connections" );

                // Dynamic Pins
                if ( pFlowNode->SupportsUserEditableDynamicInputPins() )
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
            }

            //-------------------------------------------------------------------------

            if ( !m_isReadOnly )
            {
                ImGui::SeparatorText( "Node" );

                if ( ImGui::BeginMenu( EE_ICON_IDENTIFIER" Node ID" ) )
                {
                    // UUID
                    auto IDStr = m_contextMenuState.m_pNode->GetID().ToString();
                    InlineString const label( InlineString::CtorSprintf(), "%s", IDStr.c_str() );
                    if ( ImGui::MenuItem( label.c_str() ) )
                    {
                        ImGui::SetClipboardText( IDStr.c_str() );
                    }

                    ImGui::EndMenu();
                }

                // Renameable Nodes
                if ( m_contextMenuState.m_pNode->IsRenameable() )
                {
                    if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename Node" ) )
                    {
                        BeginRenameNode( m_contextMenuState.m_pNode );
                    }
                }

                // Destroyable Nodes
                if ( m_contextMenuState.m_pNode->IsDestroyable() && m_pGraph->CanDestroyNode( m_contextMenuState.m_pNode ) )
                {
                    if ( ImGui::MenuItem( EE_ICON_DELETE" Delete Node" ) )
                    {
                        ClearSelection();
                        m_contextMenuState.m_pNode->Destroy();
                        m_contextMenuState.Reset();
                    }
                }
            }
        }
        else // Graph Menu
        {
            if ( pFlowGraph->HasContextMenuFilter() )
            {
                m_contextMenuState.m_filterWidget.UpdateAndDraw( -1, ImGuiX::FilterWidget::TakeInitialFocus );
            }

            //-------------------------------------------------------------------------

            DrawSharedContextMenuOptions( ctx );

            //-------------------------------------------------------------------------

            if ( pFlowGraph->DrawContextMenuOptions( ctx, m_pUserContext, m_contextMenuState.m_mouseCanvasPos, m_contextMenuState.m_filterWidget.GetFilterTokens(), TryCast<FlowNode>( m_contextMenuState.m_pNode ), m_contextMenuState.m_pPin ) )
            {
                m_contextMenuState.Reset();
                ImGui::CloseCurrentPopup();
            }
        }
    }

    void GraphView::DrawStateMachineContextMenu( DrawContext const& ctx )
    {
        EE_ASSERT( IsViewingStateMachineGraph() );

        auto pStateMachineGraph = GetStateMachineGraph();

        // Node Menu
        if ( m_contextMenuState.m_pNode != nullptr )
        {
            EE_ASSERT( !IsOfType<CommentNode>( m_contextMenuState.m_pNode ) );

            auto pStateMachineNode = m_contextMenuState.GetAsStateMachineNode();

            //-------------------------------------------------------------------------

            if ( !m_isReadOnly )
            {
                if ( ImGui::MenuItem( EE_ICON_STAR" Make Default Entry State" ) )
                {
                    auto pParentStateMachineGraph = Cast<StateMachineGraph>( m_contextMenuState.m_pNode->GetParentGraph() );
                    pParentStateMachineGraph->SetDefaultEntryState( m_contextMenuState.m_pNode->GetID() );
                }
            }

            //-------------------------------------------------------------------------

            // Default node menu
            pStateMachineNode->DrawContextMenuOptions( ctx, m_pUserContext, m_contextMenuState.m_mouseCanvasPos );

            //-------------------------------------------------------------------------

            if ( !m_isReadOnly )
            {
                // Renameable Nodes
                if ( m_contextMenuState.m_pNode->IsRenameable() )
                {
                    if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename Node" ) )
                    {
                        BeginRenameNode( m_contextMenuState.m_pNode );
                    }
                }

                // Destroyable Nodes
                bool nodeDestroyed = false;
                if ( m_contextMenuState.m_pNode->IsDestroyable() && m_pGraph->CanDestroyNode( m_contextMenuState.m_pNode ) )
                {
                    if ( ImGui::MenuItem( EE_ICON_DELETE" Delete Node" ) )
                    {
                        ClearSelection();
                        m_contextMenuState.m_pNode->Destroy();
                        m_contextMenuState.Reset();
                        nodeDestroyed = true;
                    }
                }
            }
        }
        else // Graph Menu
        {
            if ( pStateMachineGraph->HasContextMenuFilter() )
            {
                m_contextMenuState.m_filterWidget.UpdateAndDraw();
            }

            DrawSharedContextMenuOptions( ctx );

            if( pStateMachineGraph->DrawContextMenuOptions( ctx, m_pUserContext, m_contextMenuState.m_mouseCanvasPos, m_contextMenuState.m_filterWidget.GetFilterTokens() ) )
            {
                m_contextMenuState.Reset();
                ImGui::CloseCurrentPopup();
            }
        }
    }

    static int32_t FilterNodeNameChars( ImGuiInputTextCallbackData* data )
    {
        if ( isalnum( data->EventChar ) || data->EventChar == '_' )
        {
            return 0;
        }

        return 1;
    }

    void GraphView::DrawDialogs()
    {
        if ( m_pNodeBeingOperatedOn == nullptr )
        {
            return;
        }

        ImGuiStyle const& style = ImGui::GetStyle();
        constexpr static float const buttonWidth = 75;
        float const buttonSectionWidth = ( buttonWidth * 2 ) + style.ItemSpacing.x;

        // Edit Comment
        //-------------------------------------------------------------------------

        bool const isCommentNode = IsOfType<CommentNode>( m_pNodeBeingOperatedOn );
        if ( isCommentNode )
        {
            ImGui::OpenPopup( s_dialogID_Comment );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 6, 6 ) );
            ImGui::SetNextWindowSize( ImVec2( 600, -1 ) );
            if ( ImGui::BeginPopupModal( s_dialogID_Comment, nullptr, ImGuiWindowFlags_NoSavedSettings ) )
            {
                if ( ImGui::IsKeyPressed( ImGuiKey_Escape ) || m_selectedNodes.size() != 1 )
                {
                    EndRenameNode( false );
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    auto pNode = m_selectedNodes.back().m_pNode;
                    EE_ASSERT( pNode->IsRenameable() );
                    bool updateConfirmed = false;

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( "Comment: " );
                    ImGui::SameLine();

                    ImGui::SetNextItemWidth( -1 );

                    if ( ImGui::IsWindowAppearing() )
                    {
                        ImGui::SetKeyboardFocusHere();
                    }

                    if ( ImGui::InputTextMultiline( "##NodeName", m_textBuffer, 255, ImVec2( 0, ImGui::GetFrameHeightWithSpacing() * 5 ), ImGuiInputTextFlags_EnterReturnsTrue ) )
                    {
                        if ( m_textBuffer[0] != 0 )
                        {
                            updateConfirmed = true;
                        }
                    }

                    ImGui::NewLine();

                    float const dialogWidth = ImGui::GetContentRegionAvail().x;
                    ImGui::SameLine( 0, dialogWidth - buttonSectionWidth );

                    if ( ImGui::Button( "Ok", ImVec2( buttonWidth, 0 ) ) || updateConfirmed )
                    {
                        EndRenameNode( true );
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::SameLine( 0, 4 );

                    if ( ImGui::Button( "Cancel", ImVec2( buttonWidth, 0 ) ) )
                    {
                        EndRenameNode( false );
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::EndPopup();
            }
            ImGui::PopStyleVar();
        }

        // Rename
        //-------------------------------------------------------------------------

        else 
        {
            ImGui::OpenPopup( s_dialogID_Rename );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 6, 6 ) );
            ImGui::SetNextWindowSize( ImVec2( 400, -1 ) );
            if ( ImGui::BeginPopupModal( s_dialogID_Rename, nullptr, ImGuiWindowFlags_NoSavedSettings ) )
            {
                if ( ImGui::IsKeyPressed( ImGuiKey_Escape ) || m_selectedNodes.size() != 1 )
                {
                    EndRenameNode( false );
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

                    if ( ImGui::InputText( "##NodeName", m_textBuffer, 255, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter, FilterNodeNameChars ) )
                    {
                        if ( m_textBuffer[0] != 0 )
                        {
                            updateConfirmed = true;
                        }
                    }

                    ImGui::NewLine();

                    float const dialogWidth = ImGui::GetContentRegionAvail().x;
                    ImGui::SameLine( 0, dialogWidth - buttonSectionWidth );

                    ImGui::BeginDisabled( m_textBuffer[0] == 0 );
                    if ( ImGui::Button( "Ok", ImVec2( buttonWidth, 0 ) ) || updateConfirmed )
                    {
                        EndRenameNode( true );
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndDisabled();

                    ImGui::SameLine( 0, 4 );

                    if ( ImGui::Button( "Cancel", ImVec2( buttonWidth, 0 ) ) )
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