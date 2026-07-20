#pragma once

#include "Base/_Module/API.h"
#include "Base/Types/Color.h"
#include "Base/ThirdParty/imgui/imgui.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    struct EE_BASE_API Style
    {
        friend class ImguiSystem;

    public:

        // Base colors
        //-------------------------------------------------------------------------

        static Color const s_colorGray0; // Brightest
        static Color const s_colorGray1;
        static Color const s_colorGray2;
        static Color const s_colorGray3;
        static Color const s_colorGray4;
        static Color const s_colorGray5;
        static Color const s_colorGray6;
        static Color const s_colorGray7;
        static Color const s_colorGray8;
        static Color const s_colorGray9; // Darkest

        static Color const s_colorText;
        static Color const s_colorTextDisabled;

        static Color const s_axisColorX;
        static Color const s_axisColorY;
        static Color const s_axisColorZ;
        static Color const s_axisColorW;

        static Color const s_axisColors[4];

        // Accents
        //-------------------------------------------------------------------------

        static Color const s_colorAccent0;  // Brightest
        static Color const s_colorAccent1;
        static Color const s_colorAccent2;  // Darkest

        // Generic Widths
        //-------------------------------------------------------------------------

        constexpr static float const s_iconButtonWidthTiny = 31.0f;     // The width for a button with just an icon at the tiny font size
        constexpr static float const s_iconButtonWidthSmall = 32.0f;    // The width for a button with just an icon at the small font size
        constexpr static float const s_iconButtonWidth = 34.0f;         // The width for a button with just an icon at the regular font size
        constexpr static float const s_iconButtonWidthLarge = 36.0f;    // The width for a button with just an icon at the large font size

        // Misc settings
        //-------------------------------------------------------------------------

        constexpr static float const s_toolTipDelay = 0.4f;

    public:

        static void Apply();

        static float GetMaxDpiScale();
    };
}
#endif