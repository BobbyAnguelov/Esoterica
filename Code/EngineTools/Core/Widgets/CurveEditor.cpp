#include "CurveEditor.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Math/Curves.h"

//-------------------------------------------------------------------------

namespace EE
{
    Color const CurveEditor::s_curveColor( 255, 255, 255, 255 );
    Color const CurveEditor::s_curvePointHandleColor = Colors::White;
    Color const CurveEditor::s_curveSelectedPointHandleColor = Colors::Yellow;
    Color const CurveEditor::s_curveInTangentHandleColor( 0xFF90EE90 );
    Color const CurveEditor::s_curveOutTangentHandleColor( 0xFF32CD32 );

    //-------------------------------------------------------------------------

    CurveEditor::CurveEditor( FloatCurve& curve )
        : m_curve( curve )
    {
        m_curve.RegeneratePointIDs();
        ViewEntireCurve();
    }

    //-------------------------------------------------------------------------

    void CurveEditor::CreatePoint( float parameter, float value )
    {
        StartEditing();
        m_curve.AddPoint( parameter, value );
        ClearSelectedPoint();
        StopEditing();
    }

    void CurveEditor::DeletePoint( int32_t pointIdx )
    {
        EE_ASSERT( pointIdx >= 0 && pointIdx < m_curve.GetNumPoints() );
        StartEditing();
        m_curve.RemovePoint( pointIdx );
        ClearSelectedPoint();
        StopEditing();
    }

    //-------------------------------------------------------------------------

    void CurveEditor::InitializeDrawingState()
    {
        m_windowPos = ImGui::GetWindowPos();
        m_canvasStart = m_windowPos + Float2( ImGui::GetCursorPos() );
        m_canvasEnd = m_windowPos + Float2( ImGui::GetWindowContentRegionMax() );
        m_canvasWidth = m_canvasEnd.m_x - m_canvasStart.m_x;
        m_canvasHeight = m_canvasEnd.m_y - m_canvasStart.m_y;

        m_curveCanvasStart = m_canvasStart;
        m_curveCanvasEnd = m_canvasEnd - Float2( s_gridLegendWidth, s_gridLegendHeight );
        m_curveCanvasWidth = m_curveCanvasEnd.m_x - m_curveCanvasStart.m_x;
        m_curveCanvasHeight = m_curveCanvasEnd.m_y - m_curveCanvasStart.m_y;

        m_horizontalRangeLength = m_horizontalViewRange.GetLength();
        m_verticalRangeLength = m_verticalViewRange.GetLength();
        m_pixelsPerUnitHorizontal = m_curveCanvasWidth / m_horizontalRangeLength;
        m_pixelsPerUnitVertical = m_curveCanvasHeight / m_verticalRangeLength;
    }

    //-------------------------------------------------------------------------

    void CurveEditor::ViewEntireCurve()
    {
        ViewEntireHorizontalRange();
        ViewEntireVerticalRange();
    }

    void CurveEditor::ViewEntireHorizontalRange()
    {
        if ( m_curve.GetNumPoints() == 0 )
        {
            m_horizontalViewRange = FloatRange( -s_fitViewExtraMarginPercentage, 1.0f + s_fitViewExtraMarginPercentage );
        }
        else
        {
            m_horizontalViewRange = m_curve.GetParameterRange();
            float const parameterRangeLength = m_horizontalViewRange.GetLength();
            if ( Math::IsNearZero( parameterRangeLength ) )
            {
                m_horizontalViewRange.m_begin -= 0.5f;
                m_horizontalViewRange.m_end += 0.5f;
            }
            else
            {
                m_horizontalViewRange.m_begin -= parameterRangeLength * s_fitViewExtraMarginPercentage;
                m_horizontalViewRange.m_end += parameterRangeLength * s_fitViewExtraMarginPercentage;
            }
        }

        EE_ASSERT( !Math::IsNearZero( m_horizontalViewRange.GetLength() ) );
    }

    void CurveEditor::ViewEntireVerticalRange()
    {
        if ( m_curve.GetNumPoints() == 0 )
        {
            m_verticalViewRange = FloatRange( -s_fitViewExtraMarginPercentage, 1.0f + s_fitViewExtraMarginPercentage );
        }
        else
        {
            m_verticalViewRange = m_curve.GetValueRange();

            float const valueRangeLength = m_verticalViewRange.GetLength();
            if ( Math::IsNearZero( valueRangeLength ) )
            {
                m_verticalViewRange.m_begin -= 0.5f;
                m_verticalViewRange.m_end += 0.5f;
            }
            else
            {
                m_verticalViewRange.m_begin -= valueRangeLength * s_fitViewExtraMarginPercentage;
                m_verticalViewRange.m_end += valueRangeLength * s_fitViewExtraMarginPercentage;
            }
        }

        EE_ASSERT( !Math::IsNearZero( m_verticalViewRange.GetLength() ) );
    }

    //-------------------------------------------------------------------------

    void CurveEditor::DrawToolbar()
    {
        // View Controls
        //-------------------------------------------------------------------------

        if ( ImGui::Button( EE_ICON_FIT_TO_PAGE_OUTLINE ) )
        {
            ViewEntireCurve();
        }
        ImGuiX::ItemTooltip( "Fit entire curve" );

        ImGui::SameLine();

        if ( ImGui::Button( EE_ICON_ARROW_EXPAND_HORIZONTAL ) )
        {
            ViewEntireHorizontalRange();
        }
        ImGuiX::ItemTooltip( "Fit curve horizontally" );

        ImGui::SameLine();

        if ( ImGui::Button( EE_ICON_ARROW_EXPAND_VERTICAL ) )
        {
            ViewEntireVerticalRange();
        }
        ImGuiX::ItemTooltip( "Fit curve vertically" );

        // Point Editor
        //-------------------------------------------------------------------------

        ImGui::SameLine( ImGui::GetContentRegionAvail().x - 194, 0 );
        ImGui::Text( "Point:" );
        ImGui::SameLine();

        ImGui::BeginDisabled( m_selectedPointIdx == InvalidIndex );
        ImGui::SetNextItemWidth( 150 );
        ImGui::InputFloat2( "##PointEditor", &m_selectedPointValue.m_x, "%.2f" );
        if ( ImGui::IsItemDeactivatedAfterEdit() )
        {
            StartEditing();
            m_curve.EditPoint( m_selectedPointIdx, m_selectedPointValue.m_x, m_selectedPointValue.m_y );
            StopEditing();
            ViewEntireCurve();
        }
        ImGui::EndDisabled();
    }

    void CurveEditor::DrawGridAndLegend( ImDrawList* pDrawList )
    {
        TInlineString<10> legendString;
        pDrawList->AddRectFilled( m_canvasStart, m_canvasEnd, ImGuiX::Style::s_colorGray7 );

        int32_t const numVerticalLines = Math::FloorToInt( m_curveCanvasWidth / s_pixelsPerGridBlock );
        for ( auto i = 0; i <= numVerticalLines; i++ )
        {
            Float2 const lineStart( m_canvasStart.m_x + ( i * s_pixelsPerGridBlock ), m_canvasStart.m_y );
            Float2 const lineEnd( lineStart.m_x, m_canvasEnd.m_y - s_gridLegendHeight );
            pDrawList->AddLine( lineStart, lineEnd, ImGuiX::Style::s_colorGray5 );

            float const legendValue = m_horizontalViewRange.GetValueForPercentageThrough( ( lineStart.m_x - m_canvasStart.m_x ) / m_curveCanvasWidth );
            legendString.sprintf( "%.2f", legendValue );

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                Float2 const textSize = ImGui::CalcTextSize( legendString.c_str() );
                pDrawList->AddText( ImVec2( lineStart.m_x - ( textSize.m_x / 2 ), lineEnd.m_y ), 0xFFFFFFFF, legendString.c_str() );
            }
        }

        int32_t const numHorizontalLines = Math::FloorToInt( m_curveCanvasHeight / s_pixelsPerGridBlock );
        for ( auto i = 0; i <= numHorizontalLines; i++ )
        {
            Float2 const lineStart( m_canvasStart.m_x, m_canvasStart.m_y + ( i * s_pixelsPerGridBlock ) );
            Float2 const lineEnd( m_canvasEnd.m_x - s_gridLegendWidth, lineStart.m_y );
            pDrawList->AddLine( lineStart, lineEnd, ImGuiX::Style::s_colorGray5 );

            float const legendValue = m_verticalViewRange.GetValueForPercentageThrough( 1.0f - ( ( lineEnd.m_y - m_canvasStart.m_y ) / m_curveCanvasHeight ) );
            legendString.sprintf( "%.2f", legendValue );

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                Float2 const textSize = ImGui::CalcTextSize( legendString.c_str() );
                pDrawList->AddText( ImVec2( lineEnd.m_x, lineEnd.m_y - ( textSize.m_y / 2 ) ), 0xFFFFFFFF, legendString.c_str() );
            }
        }
    }

    void CurveEditor::DrawCurve( ImDrawList* pDrawList )
    {
        int32_t const numPointsToDraw = Math::RoundToInt( m_curveCanvasWidth / 2 ) + 1;
        float const stepT = m_horizontalRangeLength / ( numPointsToDraw - 1 );

        TVector<ImVec2> curvePoints;
        for ( auto i = 0; i < numPointsToDraw; i++ )
        {
            float const t = m_horizontalViewRange.m_begin + ( i * stepT );
            Float2 curvePoint( t, m_curve.Evaluate( t ) );
            curvePoint.m_x = m_curveCanvasStart.m_x + ( m_horizontalViewRange.GetPercentageThrough( curvePoint.m_x ) * m_curveCanvasWidth );
            curvePoint.m_y = m_curveCanvasEnd.m_y - ( m_verticalViewRange.GetPercentageThrough( curvePoint.m_y ) * m_curveCanvasHeight );
            curvePoints.emplace_back( curvePoint );
        }

        pDrawList->AddPolyline( curvePoints.data(), numPointsToDraw, s_curveColor, 0, 2.0f );
    }

    bool CurveEditor::DrawInTangentHandle( ImDrawList* pDrawList, int32_t pointIdx )
    {
        EE_ASSERT( pointIdx >= 0 && pointIdx < m_curve.GetNumPoints() );
        FloatCurve::Point const& point = m_curve.GetPoint( pointIdx );
        Float2 const pointCenter = GetScreenPosFromCurvePos( point );

        //-------------------------------------------------------------------------

        // Calculate the tangent offset based on the current slope (invert it since it's incoming)
        Vector tangentOffset( -m_pixelsPerUnitHorizontal, -point.m_inTangent * m_pixelsPerUnitVertical, 0 );
        tangentOffset.Normalize2();
        tangentOffset *= s_slopeHandleLength;

        Float2 tangentHandleCenter;
        tangentHandleCenter.m_x = ( pointCenter.m_x + tangentOffset.GetX() );
        tangentHandleCenter.m_y = ( pointCenter.m_y - tangentOffset.GetY() );

        // Draw visual handle
        pDrawList->AddLine( pointCenter, tangentHandleCenter, s_curveInTangentHandleColor );
        pDrawList->AddCircleFilled( tangentHandleCenter, s_handleRadius, s_curveInTangentHandleColor );

        // Draw handle button
        TInlineString<50> tangentHandleID;
        tangentHandleID.sprintf( "in_%u", point.m_ID );
        ImGui::SetCursorScreenPos( tangentHandleCenter - Float2( s_handleRadius, s_handleRadius ) );
        ImGui::InvisibleButton( tangentHandleID.c_str(), ImVec2( 2 * s_handleRadius, 2 * s_handleRadius ) );

        //-------------------------------------------------------------------------

        bool isCurrentlyEditing = false;
        if ( ImGui::IsItemActive() && ImGui::IsMouseDragging( 0 ) )
        {
            if ( !m_isEditing )
            {
                StartEditing();
            }

            // Get visual tangent offset
            auto const& io = ImGui::GetIO();
            Float2 tangentCanvasOffset( io.MousePos.x - pointCenter.m_x, io.MousePos.y - pointCenter.m_y );
            tangentCanvasOffset.m_x = Math::Min( tangentCanvasOffset.m_x, 0.0f ); // Lock outgoing tangents to the left hemisphere
            tangentCanvasOffset.m_y = -tangentCanvasOffset.m_y; // Handle y flip here

            // Convert to curve units (invert direction here)
            tangentCanvasOffset.m_x = -tangentCanvasOffset.m_x / m_pixelsPerUnitHorizontal;
            tangentCanvasOffset.m_y = -tangentCanvasOffset.m_y / m_pixelsPerUnitVertical;
            tangentCanvasOffset = Vector( tangentCanvasOffset ).Normalize2().ToFloat2();

            // Calculate and clamp tangent
            float newTangentSlope;
            if ( Math::IsNearZero( tangentCanvasOffset.m_x ) )
            {
                newTangentSlope = ( tangentCanvasOffset.m_y < 0 ) ? -30.0f : 30.0f;
            }
            else
            {
                newTangentSlope = tangentCanvasOffset.m_y / tangentCanvasOffset.m_x;
            }

            m_curve.SetPointInTangent( pointIdx, newTangentSlope );
            isCurrentlyEditing = true;
        }
        return isCurrentlyEditing;
    }

    bool CurveEditor::DrawOutTangentHandle( ImDrawList* pDrawList, int32_t pointIdx )
    {
        EE_ASSERT( pointIdx >= 0 && pointIdx < m_curve.GetNumPoints() );
        FloatCurve::Point const& point = m_curve.GetPoint( pointIdx );
        Float2 const pointCenter = GetScreenPosFromCurvePos( point );

        //-------------------------------------------------------------------------

        // Calculate the tangent offset based on the current slope
        Vector tangentOffset( m_pixelsPerUnitHorizontal, point.m_outTangent * m_pixelsPerUnitVertical, 0 );
        tangentOffset.Normalize2();
        tangentOffset *= s_slopeHandleLength;
        
        Float2 tangentHandleCenter;
        tangentHandleCenter.m_x = ( pointCenter.m_x + tangentOffset.GetX() );
        tangentHandleCenter.m_y = ( pointCenter.m_y - tangentOffset.GetY() );

        // Draw visual handle
        pDrawList->AddLine( pointCenter, tangentHandleCenter, s_curveOutTangentHandleColor );
        pDrawList->AddCircleFilled( tangentHandleCenter, s_handleRadius, s_curveOutTangentHandleColor );

        // Draw handle button
        TInlineString<50> tangentHandleID;
        tangentHandleID.sprintf( "out_%u", point.m_ID );
        ImGui::SetCursorScreenPos( tangentHandleCenter - Float2( s_handleRadius, s_handleRadius ) );
        ImGui::InvisibleButton( tangentHandleID.c_str(), ImVec2( 2 * s_handleRadius, 2 * s_handleRadius ) );

        //-------------------------------------------------------------------------

        bool isCurrentlyEditing = false;
        if ( ImGui::IsItemActive() && ImGui::IsMouseDragging( 0 ) )
        {
            if ( !m_isEditing )
            {
                StartEditing();
            }

            // Get visual tangent offset
            auto const& io = ImGui::GetIO();
            Float2 tangentCanvasOffset( io.MousePos.x - pointCenter.m_x, io.MousePos.y - pointCenter.m_y );
            tangentCanvasOffset.m_x = Math::Max( 0.0f, tangentCanvasOffset.m_x ); // Lock outgoing tangents to the right hemisphere
            tangentCanvasOffset.m_y = -tangentCanvasOffset.m_y; // Handle y flip here

            // Convert to curve units
            tangentCanvasOffset.m_x = tangentCanvasOffset.m_x / m_pixelsPerUnitHorizontal;
            tangentCanvasOffset.m_y = tangentCanvasOffset.m_y / m_pixelsPerUnitVertical;
            tangentCanvasOffset = Vector( tangentCanvasOffset ).Normalize2().ToFloat2();

            // Calculate and clamp tangent
            float newTangentSlope;
            if ( Math::IsNearZero( tangentCanvasOffset.m_x ) )
            {
                newTangentSlope = ( tangentCanvasOffset.m_y < 0 ) ? -30.0f : 30.0f;
            }
            else
            {
                newTangentSlope = tangentCanvasOffset.m_y / tangentCanvasOffset.m_x;
            }

            m_curve.SetPointOutTangent( pointIdx, newTangentSlope );

            if ( point.m_tangentMode == FloatCurve::Locked )
            {
                m_curve.SetPointInTangent( pointIdx, newTangentSlope );
            }

            isCurrentlyEditing = true;
        }
        return isCurrentlyEditing;
    }

    bool CurveEditor::DrawPointHandle( ImDrawList* pDrawList, int32_t pointIdx )
    {
        FloatCurve::Point const& point = m_curve.GetPoint( pointIdx );
        Float2 const pointCenter = GetScreenPosFromCurvePos( point );

        pDrawList->AddCircleFilled( pointCenter, s_handleRadius, ( pointIdx == m_selectedPointIdx ) ? s_curveSelectedPointHandleColor : s_curvePointHandleColor );

        //-------------------------------------------------------------------------

        TInlineString<50> pointHandleID;
        pointHandleID.sprintf( "point_%u", point.m_ID );

        ImGui::SetCursorScreenPos( pointCenter - Float2( s_handleRadius, s_handleRadius ) );
        if ( ImGui::InvisibleButton( pointHandleID.c_str(), Float2( 2 * s_handleRadius, 2 * s_handleRadius ) ) )
        {
            SelectPoint( pointIdx );
        }

        if ( ImGui::IsItemActive() || ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "%4.3f, %4.3f", point.m_parameter, point.m_value );
        }

        if ( ImGui::IsItemHovered() && ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
        {
            SelectPoint( pointIdx );
            ImGui::OpenPopup( s_pointContextMenuName );
        }

        //-------------------------------------------------------------------------

        bool isCurrentlyEditing = false;
        if ( ImGui::IsItemActive() && ImGui::IsMouseDragging( 0 ) )
        {
            if ( !m_isEditing )
            {
                StartEditing();
            }

            SelectPoint( pointIdx );

            ImRect const curveRect = GetCurveRect();
            ImVec2 const clampedMousePos = ImGuiX::ClampToRect( curveRect, ImGui::GetMousePos() );
            float const updatedParameter = ( ( clampedMousePos.x - curveRect.Min.x ) / m_pixelsPerUnitHorizontal ) + m_horizontalViewRange.m_begin;
            float const updatedValue = m_verticalViewRange.GetLength() - ( ( clampedMousePos.y - curveRect.Min.y ) / m_pixelsPerUnitVertical ) + m_verticalViewRange.m_begin;

            m_curve.EditPoint( pointIdx, updatedParameter, updatedValue );
            isCurrentlyEditing = true;
        }
        return isCurrentlyEditing;
    }

    //-------------------------------------------------------------------------

    void CurveEditor::StartEditing()
    {
        EE_ASSERT( !m_isEditing );
        m_isEditing = true;
        m_valueBeforeEdit = m_curve.ToString();
    }

    void CurveEditor::StopEditing()
    {
        EE_ASSERT( m_isEditing );
        m_isEditing = false;
        m_wasCurveEdited = true;
    }

    void CurveEditor::SelectPoint( int32_t pointIdx )
    {
        EE_ASSERT( pointIdx >= 0 && pointIdx < m_curve.GetNumPoints() );
        m_selectedPointIdx = pointIdx;
        m_selectedPointValue = Float2( m_curve.GetPoint( pointIdx ).m_parameter, m_curve.GetPoint( pointIdx ).m_value );
        m_wasPointSelected = true;
    }

    void CurveEditor::ClearSelectedPoint()
    {
        m_selectedPointIdx = InvalidIndex;
        m_selectedPointValue = Float2::Zero;
    }

    //-------------------------------------------------------------------------

    void CurveEditor::DrawContextMenus( bool isMiniView )
    {
        if ( ImGui::BeginPopup( s_gridContextMenuName ) )
        {
            ImGuiX::TextSeparator( "Editing" );

            if ( ImGui::MenuItem( EE_ICON_PLUS" Create Point" ) )
            {
                Float2 const mouseCurvePos = GetCurvePosFromScreenPos( ImGui::GetWindowPos() );
                CreatePoint( mouseCurvePos.m_x, mouseCurvePos.m_y );
            }

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Zoom" );

            if ( ImGui::MenuItem( EE_ICON_FIT_TO_PAGE_OUTLINE" Fit entire curve" ) )
            {
                ViewEntireCurve();
            }

            if ( ImGui::MenuItem( EE_ICON_ARROW_EXPAND_HORIZONTAL" Fit curve horizontally" ) )
            {
                ViewEntireHorizontalRange();
            }

            if ( ImGui::MenuItem( EE_ICON_ARROW_EXPAND_VERTICAL" Fit curve vertically" ) )
            {
                ViewEntireVerticalRange();
            }

            //-------------------------------------------------------------------------

            if( isMiniView )
            {
                ImGuiX::TextSeparator( "Editing" );

                if ( ImGui::MenuItem( EE_ICON_PLAYLIST_EDIT" Open Full Curve Editor" ) )
                {
                    m_requestOpenFullEditor = true;
                }
            }

            //-------------------------------------------------------------------------

            ImGui::EndPopup();
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginPopup( s_pointContextMenuName ) )
        {
            EE_ASSERT( m_selectedPointIdx != InvalidIndex );

            auto const& point = m_curve.GetPoint( m_selectedPointIdx );

            ImGui::SetNextItemWidth( -1 );
            ImGui::InputFloat2( "##PointEditor", &m_selectedPointValue.m_x, "%.2f" );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                StartEditing();
                m_curve.EditPoint( m_selectedPointIdx, m_selectedPointValue.m_x, m_selectedPointValue.m_y );
                StopEditing();
                ViewEntireCurve();
            }

            if ( point.m_tangentMode == FloatCurve::Free )
            {
                if ( ImGui::MenuItem( EE_ICON_LOCK" Lock Tangents" ) )
                {
                    StartEditing();
                    m_curve.SetPointTangentMode( m_selectedPointIdx, FloatCurve::Locked );
                    StopEditing();
                }
            }

            if ( point.m_tangentMode == FloatCurve::Locked )
            {
                if ( ImGui::MenuItem( EE_ICON_LOCK_OPEN" Unlock Tangents" ) )
                {
                    StartEditing();
                    m_curve.SetPointTangentMode( m_selectedPointIdx, FloatCurve::Free );
                    StopEditing();
                }
            }

            if ( ImGui::MenuItem( EE_ICON_DELETE" Delete Point" ) )
            {
                DeletePoint( m_selectedPointIdx );
            }
            ImGui::EndPopup();
        }
    }

    void CurveEditor::HandleFrameInput( bool isMiniView )
    {
        auto const& io = ImGui::GetIO();
        if ( ImGui::IsWindowHovered() )
        {
            // Position tooltip
            //-------------------------------------------------------------------------
            
            if ( io.KeyShift )
            {
                Float2 const mouseCurvePos = GetCurvePosFromScreenPos( io.MousePos );
                ImGui::SetTooltip( "%4.3f, %4.3f", mouseCurvePos.m_x, mouseCurvePos.m_y );
            }

            // Pan view 
            //-------------------------------------------------------------------------

            if ( ImGui::IsMouseDragging( ImGuiMouseButton_Middle ) )
            {
                m_horizontalViewRange.ShiftRange( -io.MouseDelta.x / m_pixelsPerUnitHorizontal );
                m_verticalViewRange.ShiftRange( io.MouseDelta.y / m_pixelsPerUnitVertical );
            }

            // Zoom View
            //-------------------------------------------------------------------------

            if ( io.MouseWheel != 0 )
            {
                float const scale = io.MouseWheel * s_zoomScaleStep;
                bool const zoomHorizontal = ( !io.KeyAlt && io.KeyCtrl ) || ( !io.KeyAlt && !io.KeyCtrl );
                bool const zoomVertical = ( io.KeyAlt && !io.KeyCtrl ) || ( !io.KeyAlt && !io.KeyCtrl );

                // Horizontal zoom
                if ( zoomHorizontal )
                {
                    float const mp = m_horizontalViewRange.GetMidpoint();
                    float const rl = m_horizontalViewRange.GetLength();
                    float const nhl = ( rl * ( 1 - scale ) ) / 2;
                    m_horizontalViewRange.m_begin = mp - nhl;
                    m_horizontalViewRange.m_end = mp + nhl;
                }

                // Vertical zoom
                if ( zoomVertical )
                {
                    float const mp = m_verticalViewRange.GetMidpoint();
                    float const rl = m_verticalViewRange.GetLength();
                    float const nhl = ( rl * ( 1 - scale ) ) / 2;
                    m_verticalViewRange.m_begin = mp - nhl;
                    m_verticalViewRange.m_end = mp + nhl;
                }
            }

            // Mouse Actions
            //-------------------------------------------------------------------------

            // Clear point selection
            if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
            {
                if ( !ImGui::IsPopupOpen( s_pointContextMenuName ) && !m_wasPointSelected )
                {
                    ClearSelectedPoint();
                }
            }

            if ( isMiniView )
            {
                if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                {
                    m_requestOpenFullEditor = true;
                }
            }

            // Key actions
            //-------------------------------------------------------------------------

            if ( m_selectedPointIdx != InvalidIndex && ImGui::IsKeyPressed( ImGuiKey_Delete ) )
            {
                DeletePoint( m_selectedPointIdx );
            }

            if ( ImGui::IsKeyPressed( ImGuiKey_Space ) )
            {
                Float2 const mouseCurvePos = GetCurvePosFromScreenPos( io.MousePos );
                CreatePoint( mouseCurvePos.m_x, mouseCurvePos.m_y );
            }

            // Context Menu
            //-------------------------------------------------------------------------

            if ( !ImGui::IsPopupOpen( s_pointContextMenuName ) && ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
            {
                ImGui::OpenPopup( s_gridContextMenuName );
            }
        }
    }

    bool CurveEditor::UpdateAndDraw( bool isMiniView )
    {
        EE_ASSERT( !Math::IsNearZero( m_horizontalViewRange.GetLength() ) && !Math::IsNearZero( m_verticalViewRange.GetLength() ) );

        //-------------------------------------------------------------------------

        // Ensure selected point index is always valid since the curve could be externally edited too
        if ( m_selectedPointIdx >= m_curve.GetNumPoints() )
        {
            ClearSelectedPoint();
        }

        // Clear the PreEditState if we're not editing and it is set
        if ( !m_isEditing && m_valueBeforeEdit.empty() )
        {
            m_valueBeforeEdit.clear();
        }

        // Clear selection/edit frame state
        m_wasPointSelected = false;
        m_wasCurveEdited = false;

        // Toolbar has to be drawn first before resetting the frame state since we need to shift the cursor position
        if ( !isMiniView )
        {
            DrawToolbar();
        }
        InitializeDrawingState();

        //-------------------------------------------------------------------------

        ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
        if ( ImGui::BeginChild( "CurveView", m_canvasEnd - m_canvasStart, false, ImGuiWindowFlags_NoScrollbar) )
        {
            auto pDrawList = ImGui::GetWindowDrawList();

            // Draw Grid and legend
            //-------------------------------------------------------------------------

            DrawGridAndLegend( pDrawList );

            // Draw Curve and Points
            //-------------------------------------------------------------------------

            auto const curveRect = GetCurveRect();
            pDrawList->PushClipRect( curveRect.Min, curveRect.Max );

            DrawCurve( pDrawList );

            bool stillEditing = false;
            int32_t const numPoints = m_curve.GetNumPoints();
            for ( int32_t i = 0; i < numPoints; i++ )
            {
                if ( i != 0 && m_curve.GetPoint(i).m_tangentMode == FloatCurve::Free )
                {
                    stillEditing |= DrawInTangentHandle( pDrawList, i );
                }

                if ( i != numPoints - 1 )
                {
                    stillEditing |= DrawOutTangentHandle( pDrawList, i );
                }
                
                stillEditing |= DrawPointHandle( pDrawList, i );
            }

            // If we completed an edit operation
            if ( m_isEditing && !stillEditing )
            {
                StopEditing();
            }

            pDrawList->PopClipRect();

            //-------------------------------------------------------------------------

            HandleFrameInput( isMiniView );
            DrawContextMenus( isMiniView );
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        //-------------------------------------------------------------------------

        // Draw full editor in a modal window
        if ( isMiniView )
        {
            if ( m_requestOpenFullEditor )
            {
                ImGui::OpenPopup( "Curve Editor" );
                m_requestOpenFullEditor = false;
            }

            bool isOpen = true;
            ImGui::SetNextWindowSize( ImVec2( 800, 600 ), ImGuiCond_FirstUseEver );
            if ( ImGui::BeginPopupModal( "Curve Editor", &isOpen ) )
            {
                UpdateAndDraw( false );
                ImGui::EndPopup();
            }
        }

        //-------------------------------------------------------------------------

        return m_wasCurveEdited;
    }
}