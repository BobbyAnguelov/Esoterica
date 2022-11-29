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

        // Base colors
        //-------------------------------------------------------------------------

        static ImColor const s_colorGray0; // Brightest
        static ImColor const s_colorGray1;
        static ImColor const s_colorGray2;
        static ImColor const s_colorGray3;
        static ImColor const s_colorGray4;
        static ImColor const s_colorGray5;
        static ImColor const s_colorGray6;
        static ImColor const s_colorGray7;
        static ImColor const s_colorGray8;
        static ImColor const s_colorGray9; // Darkest

        static ImColor const s_colorText;
        static ImColor const s_colorTextDisabled;

        // Accents
        //-------------------------------------------------------------------------

        static ImColor const s_colorAccent0;  // Brightest
        static ImColor const s_colorAccent1;
        static ImColor const s_colorAccent2;  // Darkest

        // Additional tool colors
        //-------------------------------------------------------------------------

        static ImColor const s_gridBackgroundColor;
        static ImColor const s_gridLineColor;
        static ImColor const s_selectionBoxOutlineColor;
        static ImColor const s_selectionBoxFillColor;

        // Misc settings
        //-------------------------------------------------------------------------

        constexpr static float const s_toolTipDelay = 0.4f;

    public:

        static void Apply();
    };

    //-------------------------------------------------------------------------

    struct EE_SYSTEM_API ImColors
    {
        static ImColor const AliceBlue;
        static ImColor const AntiqueWhite;
        static ImColor const Aqua;
        static ImColor const Aquamarine;
        static ImColor const Azure;
        static ImColor const Beige;
        static ImColor const Bisque;
        static ImColor const Black;
        static ImColor const BlanchedAlmond;
        static ImColor const Blue;
        static ImColor const BlueViolet;
        static ImColor const Brown;
        static ImColor const BurlyWood;
        static ImColor const CadetBlue;
        static ImColor const Chartreuse;
        static ImColor const Chocolate;
        static ImColor const Coral;
        static ImColor const CornflowerBlue;
        static ImColor const Cornsilk;
        static ImColor const Crimson;
        static ImColor const Cyan;
        static ImColor const DarkBlue;
        static ImColor const DarkCyan;
        static ImColor const DarkGoldenRod;
        static ImColor const DarkGray;
        static ImColor const DarkGreen;
        static ImColor const DarkKhaki;
        static ImColor const DarkMagenta;
        static ImColor const DarkOliveGreen;
        static ImColor const DarkOrange;
        static ImColor const DarkOrchid;
        static ImColor const DarkRed;
        static ImColor const DarkSalmon;
        static ImColor const DarkSeaGreen;
        static ImColor const DarkSlateBlue;
        static ImColor const DarkSlateGray;
        static ImColor const DarkTurquoise;
        static ImColor const DarkViolet;
        static ImColor const DeepPink;
        static ImColor const DeepSkyBlue;
        static ImColor const DimGray;
        static ImColor const DodgerBlue;
        static ImColor const FireBrick;
        static ImColor const FloralWhite;
        static ImColor const ForestGreen;
        static ImColor const Fuchsia;
        static ImColor const Gainsboro;
        static ImColor const GhostWhite;
        static ImColor const Gold;
        static ImColor const GoldenRod;
        static ImColor const Gray;
        static ImColor const Green;
        static ImColor const GreenYellow;
        static ImColor const HoneyDew;
        static ImColor const HotPink;
        static ImColor const IndianRed;
        static ImColor const Indigo;
        static ImColor const Ivory;
        static ImColor const Khaki;
        static ImColor const Lavender;
        static ImColor const LavenderBlush;
        static ImColor const LawnGreen;
        static ImColor const LemonChiffon;
        static ImColor const LightBlue;
        static ImColor const LightCoral;
        static ImColor const LightCyan;
        static ImColor const LightGoldenRodYellow;
        static ImColor const LightGray;
        static ImColor const LightGreen;
        static ImColor const LightPink;
        static ImColor const LightSalmon;
        static ImColor const LightSeaGreen;
        static ImColor const LightSkyBlue;
        static ImColor const LightSlateGray;
        static ImColor const LightSteelBlue;
        static ImColor const LightYellow;
        static ImColor const Lime;
        static ImColor const LimeGreen;
        static ImColor const Linen;
        static ImColor const Magenta;
        static ImColor const Maroon;
        static ImColor const MediumAquaMarine;
        static ImColor const MediumBlue;
        static ImColor const MediumOrchid;
        static ImColor const MediumPurple;
        static ImColor const MediumSeaGreen;
        static ImColor const MediumSlateBlue;
        static ImColor const MediumSpringGreen;
        static ImColor const MediumTurquoise;
        static ImColor const MediumVioletRed;
        static ImColor const MidnightBlue;
        static ImColor const MintCream;
        static ImColor const MistyRose;
        static ImColor const Moccasin;
        static ImColor const NavajoWhite;
        static ImColor const Navy;
        static ImColor const OldLace;
        static ImColor const Olive;
        static ImColor const OliveDrab;
        static ImColor const Orange;
        static ImColor const OrangeRed;
        static ImColor const Orchid;
        static ImColor const PaleGoldenRod;
        static ImColor const PaleGreen;
        static ImColor const PaleTurquoise;
        static ImColor const PaleVioletRed;
        static ImColor const PapayaWhip;
        static ImColor const PeachPuff;
        static ImColor const Peru;
        static ImColor const Pink;
        static ImColor const Plum;
        static ImColor const PowderBlue;
        static ImColor const Purple;
        static ImColor const Red;
        static ImColor const MediumRed;
        static ImColor const RosyBrown;
        static ImColor const RoyalBlue;
        static ImColor const SaddleBrown;
        static ImColor const Salmon;
        static ImColor const SandyBrown;
        static ImColor const SeaGreen;
        static ImColor const SeaShell;
        static ImColor const Sienna;
        static ImColor const Silver;
        static ImColor const SkyBlue;
        static ImColor const SlateBlue;
        static ImColor const SlateGray;
        static ImColor const Snow;
        static ImColor const SpringGreen;
        static ImColor const SteelBlue;
        static ImColor const Tan;
        static ImColor const Teal;
        static ImColor const Thistle;
        static ImColor const Tomato;
        static ImColor const Turquoise;
        static ImColor const Violet;
        static ImColor const Wheat;
        static ImColor const White;
        static ImColor const WhiteSmoke;
        static ImColor const Yellow;
        static ImColor const YellowGreen;
    };
}