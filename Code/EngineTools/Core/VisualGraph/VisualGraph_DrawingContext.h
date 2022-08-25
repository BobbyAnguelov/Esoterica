#pragma once

#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    struct VisualSettings
    {
        // Colors
        //-------------------------------------------------------------------------

        constexpr static uint32_t const   s_graphTitleColor = IM_COL32( 255, 255, 255, 255 );

        constexpr static uint32_t const   s_genericNodeTitleTextColor = IM_COL32( 0, 0, 0, 255 );
        constexpr static uint32_t const   s_genericNodeTitleColor = IM_COL32( 28, 28, 28, 255 );
        constexpr static uint32_t const   s_genericNodeBackgroundColor = IM_COL32( 75, 75, 75, 255 );
        constexpr static uint32_t const   s_genericNodeBorderColorSelected = IM_COL32( 255, 255, 255, 255 );
        constexpr static uint32_t const   s_genericActiveColor = IM_COL32( 50, 205, 50, 255 );
        constexpr static uint32_t const   s_genericHoverColor = IM_COL32( 200, 200, 200, 255 );
        constexpr static uint32_t const   s_genericSelectionColor = IM_COL32( 255, 216, 0, 255 );

        constexpr static uint32_t const   s_connectionColor = IM_COL32( 185, 185, 185, 255 );
        constexpr static uint32_t const   s_connectionColorValid = IM_COL32( 0, 255, 0, 255 );
        constexpr static uint32_t const   s_connectionColorInvalid = IM_COL32( 255, 0, 0, 255 );
        constexpr static uint32_t const   s_connectionColorHovered = IM_COL32( 255, 255, 255, 255 );

        // UI
        //-------------------------------------------------------------------------

        static ImVec2 const             s_graphTitleMargin;
        constexpr static float const    s_gridSpacing = 20;
        constexpr static float const    s_nodeSelectionBorder = 3.0f;
        constexpr static float const    s_connectionSelectionExtraRadius = 5.0f;
    };

    //-------------------------------------------------------------------------
    // Drawing Context
    //-------------------------------------------------------------------------

    struct DrawContext
    {
        // Convert from a position relative to the window TL to a position relative to the screen TL
        EE_FORCE_INLINE ImVec2 WindowToScreenPosition( ImVec2 const& windowPosition ) const
        {
            return windowPosition + m_windowRect.Min;
        }

        // Convert from a position relative to the window TL to a position relative to the canvas origin
        EE_FORCE_INLINE ImVec2 WindowPositionToCanvasPosition( ImVec2 const& windowPosition ) const
        {
            return windowPosition + m_viewOffset;
        }

        // Convert from a position relative to the canvas origin to a position relative to the window TL
        EE_FORCE_INLINE ImVec2 CanvasPositionToWindowPosition( ImVec2 const& canvasPosition ) const
        {
            return canvasPosition - m_viewOffset;
        }

        // Convert from a position relative to the canvas origin to a position relative to the screen TL
        EE_FORCE_INLINE ImVec2 CanvasPositionToScreenPosition( ImVec2 const& canvasPosition ) const
        {
            return WindowToScreenPosition( CanvasPositionToWindowPosition( canvasPosition ) );
        }

        // Convert from a position relative to the screen TL to a position relative to the window TL
        EE_FORCE_INLINE ImVec2 ScreenPositionToWindowPosition( ImVec2 const& screenPosition ) const
        {
            return screenPosition - m_windowRect.Min;
        }

        // Convert from a position relative to the screen TL to a position relative to the canvas origin
        EE_FORCE_INLINE ImVec2 ScreenPositionToCanvasPosition( ImVec2 const& screenPosition ) const
        {
            return WindowPositionToCanvasPosition( ScreenPositionToWindowPosition( screenPosition ) );
        }

        // Is a supplied rect within the canvas visible area
        EE_FORCE_INLINE bool IsItemVisible( ImRect const& itemCanvasRect ) const
        {
            return m_canvasVisibleRect.Overlaps( itemCanvasRect );
        }

    public:

        ImDrawList*         m_pDrawList = nullptr;
        ImVec2              m_viewOffset = ImVec2( 0, 0 );
        ImRect              m_windowRect;
        ImRect              m_canvasVisibleRect;
        ImVec2              m_mouseScreenPos = ImVec2( 0, 0 );
        ImVec2              m_mouseCanvasPos = ImVec2( 0, 0 );
    };
}