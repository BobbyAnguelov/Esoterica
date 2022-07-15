#pragma once

#include "System/_Module/API.h"
#include "System/ThirdParty/imgui/imgui.h"

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    struct EE_SYSTEM_API Style
    {
        friend class ImguiSystem;

    public:

        // Core imgui colors
        static ImColor const s_colorGray0;
        static ImColor const s_colorGray1;
        static ImColor const s_colorGray2;
        static ImColor const s_colorGray3;
        static ImColor const s_colorGray4;
        static ImColor const s_colorGray5;
        static ImColor const s_colorGray6;
        static ImColor const s_colorGray7;
        static ImColor const s_colorGray8;
        static ImColor const s_colorGray9;

        static ImColor const s_colorText;
        static ImColor const s_colorTextDisabled;

        static ImColor const s_colorAccent0;
        static ImColor const s_colorAccent1;
        static ImColor const s_colorAccent2;

        // Additional tool colors
        static ImColor const s_gridBackgroundColor;
        static ImColor const s_gridLineColor;
        static ImColor const s_selectionBoxOutlineColor;
        static ImColor const s_selectionBoxFillColor;

        // Misc settings
        constexpr static float const s_toolTipDelay = 0.4f;

    public:

        static void Apply();
    };
}