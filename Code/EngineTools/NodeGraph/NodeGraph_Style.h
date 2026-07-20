#pragma once
#include "Base/Types/Color.h"
#include "imgui.h"

//-------------------------------------------------------------------------

namespace EE::NodeGraph
{
    struct Style
    {
        constexpr static uint32_t const     s_defaultTitleColor = IM_COL32( 95, 95, 95, 255 );
        constexpr static uint32_t const     s_nodeBackgroundColor = IM_COL32( 53, 53, 53, 255 );
        constexpr static uint32_t const     s_nodeSelectedBorderColor = IM_COL32( 200, 200, 0, 255 );
        constexpr static uint32_t const     s_activeBorderColor = IM_COL32( 50, 205, 50, 255 );
        constexpr static uint32_t const     s_invalidBorderColor = IM_COL32( 255, 0, 0, 255 );

        constexpr static uint32_t const     s_connectionColor = IM_COL32( 185, 185, 185, 255 );
        constexpr static uint32_t const     s_connectionColorValid = IM_COL32( 0, 255, 0, 255 );
        constexpr static uint32_t const     s_connectionColorInvalid = IM_COL32( 255, 0, 0, 255 );
        constexpr static uint32_t const     s_connectionColorHovered = IM_COL32( 255, 255, 255, 255 );
        constexpr static uint32_t const     s_connectionColorSelected = IM_COL32( 255, 255, 0, 255 );

        constexpr static uint32_t const     s_genericNodeSeparatorColor = IM_COL32( 100, 100, 100, 255 );
        constexpr static uint32_t const     s_genericNodeInternalRegionDefaultColor = IM_COL32( 40, 40, 40, 255 );

        constexpr static uint32_t const     s_gridBackgroundColor = IM_COL32( 40, 40, 40, 200 );
        constexpr static uint32_t const     s_gridLineColor = IM_COL32( 200, 200, 200, 40 );
        constexpr static uint32_t const     s_graphTitleColor = IM_COL32( 255, 255, 255, 255 );
        constexpr static uint32_t const     s_graphTitleReadOnlyColor = IM_COL32( 196, 196, 196, 255 );
        constexpr static uint32_t const     s_selectionBoxOutlineColor = IM_COL32( 61, 224, 133, 150 );
        constexpr static uint32_t const     s_selectionBoxFillColor = IM_COL32( 61, 224, 133, 30 );
        constexpr static uint32_t const     s_readOnlyCanvasBorderColor = IM_COL32( 240, 128, 128, 255 );

        //-------------------------------------------------------------------------

        constexpr static float const        s_activeBorderIndicatorThickness = 2;
        constexpr static float const        s_activeBorderIndicatorPadding = s_activeBorderIndicatorThickness + 3;
        static inline ImVec2 const          s_graphTitleMargin = ImVec2( 16, 10 );
        constexpr static float const        s_gridSpacing = 30;
        constexpr static float const        s_nodeSelectionBorderThickness = 2.0f;
        constexpr static float const        s_connectionSelectionExtraRadius = 5.0f;
        constexpr static float const        s_transitionArrowWidth = 3.0f;
        constexpr static float const        s_transitionArrowOffset = 8.0f;
        constexpr static float const        s_titleBarColorItemWidth = 8.0f;
        constexpr static float const        s_spacingBetweenTitleAndNodeContents = 6.0f;
        constexpr static float const        s_pinRadius = 6.0f;
        constexpr static float const        s_spacingBetweenInputOutputPins = 16.0f;
    };
}