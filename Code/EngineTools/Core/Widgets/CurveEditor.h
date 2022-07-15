#pragma once

#include "EngineTools/_Module/API.h"
#include "System/Imgui/ImguiX.h"
#include "System/Math/FloatCurve.h"
#include "System/Time/Timers.h"
#include "System/Types/UUID.h"

//-------------------------------------------------------------------------
// Curve Editor
//-------------------------------------------------------------------------
// Controls:
//  hold middle mouse - pan view
//  mouse wheel - zoom view
//  ctrl + mouse wheel - zoom horizontal view
//  alt + mouse wheel - zoom vertical views
//  hold shift - show cursor position tooltip

namespace EE
{
    struct CurveEditorContext;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API CurveEditor
    {
        constexpr static float const s_fitViewExtraMarginPercentage = 0.1f;
        constexpr static float const s_handleRadius = 6.0f;
        constexpr static float const s_zoomScaleStep = 0.1f;
        constexpr static float const s_pixelsPerGridBlock = 45;
        constexpr static float const s_slopeHandleLength = 45.0f;
        constexpr static float const s_gridLegendWidth = 30;
        constexpr static float const s_gridLegendHeight = 16;

        constexpr static char const* const s_pointContextMenuName = "PointCtxMenu";
        constexpr static char const* const s_gridContextMenuName = "GridCtxMenu";

        static ImColor const s_curveColor;
        static ImColor const s_curvePointHandleColor;
        static ImColor const s_curveSelectedPointHandleColor;
        static ImColor const s_curveInTangentHandleColor;
        static ImColor const s_curveOutTangentHandleColor;

    public:

        CurveEditor( FloatCurve& curve );

        // Call this to notify the curve editor that the curve has been externally updated
        void OnCurveExternallyUpdated() { m_curve.RegeneratePointIDs(); }

        // Resets the view and clears the selection 
        void ResetView() { ViewEntireCurve(); m_selectedPointIdx = InvalidIndex; }

        // Draw the the curve editor and edit the curve - returns true if a curve edit completes - you can retrieve the previous state of the curve using the "GetPreEditCurveState()" function
        // Pass true to this function to add a button that will open the same editor in a modal dialog (useful for property grids, etc.)
        bool UpdateAndDraw( bool isMiniView );

        // Returns the state of the curve before the edit operation, only valid on the same frame that UpdateAndDraw() returns true
        String const& GetPreEditCurveState() const { EE_ASSERT( !m_isEditing && !m_valueBeforeEdit.empty() ); return m_valueBeforeEdit; }

        // Adjust view range so that the entire curve is visible
        void ViewEntireCurve();

        // Adjust view range so that the entire curve parameter range is visible
        void ViewEntireHorizontalRange();

        // Adjust view range so that the entire curve value range is visible
        void ViewEntireVerticalRange();

    private:

        void CreatePoint( float parameter, float value );
        void DeletePoint( int32_t pointIdx );

        //-------------------------------------------------------------------------

        void InitializeDrawingState();

        void DrawToolbar();
        void DrawGrid();
        void DrawCurve();
        bool DrawInTangentHandle( int32_t pointIdx );
        bool DrawOutTangentHandle( int32_t pointIdx );
        bool DrawPointHandle( int32_t pointIdx );
        void DrawContextMenus();
        void HandleFrameInput();

        inline Float2 GetCurvePosFromScreenPos( Float2 const& screenPos ) const
        {
            ImVec2 curvePos( screenPos - m_curveCanvasStart );
            curvePos.x /= m_curveCanvasWidth;
            curvePos.y /= m_curveCanvasHeight;
            curvePos.x = m_horizontalViewRange.GetValueForPercentageThrough( curvePos.x );
            curvePos.y = m_verticalViewRange.GetValueForPercentageThrough( 1.0f - curvePos.y );
            return curvePos;
        }

        inline Float2 GetScreenPosFromCurvePos( float parameter, float value ) const
        {
            Float2 screenPos;
            screenPos.m_x = m_curveCanvasStart.m_x + ( m_horizontalViewRange.GetPercentageThrough( parameter ) * m_curveCanvasWidth );
            screenPos.m_y = m_curveCanvasEnd.m_y - ( m_verticalViewRange.GetPercentageThrough( value ) * m_curveCanvasHeight );
            return screenPos;
        }

        EE_FORCE_INLINE Float2 GetScreenPosFromCurvePos( Float2 const& curvePos ) const { return GetScreenPosFromCurvePos( curvePos.m_x, curvePos.m_y ); }
        EE_FORCE_INLINE Float2 GetScreenPosFromCurvePos( FloatCurve::Point const& point ) const { return GetScreenPosFromCurvePos( point.m_parameter, point.m_value ); }

        void StartEditing();
        void StopEditing();

        void SelectPoint( int32_t pointIdx );
        void ClearSelectedPoint();

    private:

        // Persistent State
        FloatCurve&                                         m_curve;
        FloatRange                                          m_horizontalViewRange = FloatRange( 0, 1 );
        FloatRange                                          m_verticalViewRange = FloatRange( 0, 1 );
        int32_t                                             m_selectedPointIdx = InvalidIndex;
        Float2                                              m_selectedPointValue;
        String                                              m_valueBeforeEdit;
        bool                                                m_isEditing = false;

        // Per Frame State
        ImDrawList*                                         m_pDrawList = nullptr;

        Float2                                              m_windowPos;
        Float2                                              m_canvasStart;
        Float2                                              m_canvasEnd;
        float                                               m_canvasWidth;
        float                                               m_canvasHeight;

        Float2                                              m_curveCanvasStart;
        Float2                                              m_curveCanvasEnd;
        float                                               m_curveCanvasWidth;
        float                                               m_curveCanvasHeight;

        float                                               m_horizontalRangeLength;
        float                                               m_verticalRangeLength;
        float                                               m_pixelsPerUnitHorizontal;
        float                                               m_pixelsPerUnitVertical;
        bool                                                m_wasPointSelected = false;
        bool                                                m_wasCurveEdited = false;
    };
}