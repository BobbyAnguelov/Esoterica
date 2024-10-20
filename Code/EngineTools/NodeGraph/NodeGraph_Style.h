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
        constexpr static uint32_t const     s_activeIndicatorBorderColor = IM_COL32( 50, 205, 50, 255 );

        constexpr static uint32_t const     s_connectionColor = IM_COL32( 185, 185, 185, 255 );
        constexpr static uint32_t const     s_connectionColorValid = IM_COL32( 0, 255, 0, 255 );
        constexpr static uint32_t const     s_connectionColorInvalid = IM_COL32( 255, 0, 0, 255 );
        constexpr static uint32_t const     s_connectionColorActive = IM_COL32( 50, 205, 50, 255 );
        constexpr static uint32_t const     s_connectionColorHovered = IM_COL32( 255, 255, 255, 255 );
        constexpr static uint32_t const     s_connectionColorSelected = IM_COL32( 255, 255, 0, 255 );

        constexpr static uint32_t const     s_genericNodeSeparatorColor = IM_COL32( 100, 100, 100, 255 );
        constexpr static uint32_t const     s_genericNodeInternalRegionDefaultColor = IM_COL32( 40, 40, 40, 255 );

        constexpr static float const        s_activeBorderIndicatorThickness = 2;
        constexpr static float const        s_activeBorderIndicatorPadding = s_activeBorderIndicatorThickness + 3;

    };
}