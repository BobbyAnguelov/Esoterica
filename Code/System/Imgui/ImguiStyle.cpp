#include "ImguiStyle.h"
#include "System/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    ImColor const Style::s_colorGray0( 0xFF5B5B5B );
    ImColor const Style::s_colorGray1( 0xFF4C4C4C );
    ImColor const Style::s_colorGray2( 0xFF444444 );
    ImColor const Style::s_colorGray3( 0xFF3A3A3A );
    ImColor const Style::s_colorGray4( 0xFF303030 );
    ImColor const Style::s_colorGray5( 0xFF2C2C2C );
    ImColor const Style::s_colorGray6( 0xFF232323 );
    ImColor const Style::s_colorGray7( 0xFF1C1C1C );
    ImColor const Style::s_colorGray8( 0xFF161616 );
    ImColor const Style::s_colorGray9( 0xFF111111 );

    //-------------------------------------------------------------------------

    ImColor const Style::s_colorText( 0xFFFFFFFF );
    ImColor const Style::s_colorTextDisabled( 0xFF828282 );

    //-------------------------------------------------------------------------

    ImColor const Style::s_colorAccent0( 0xFF3FFF3F );
    ImColor const Style::s_colorAccent1( 0xFF32CD32 );
    ImColor const Style::s_colorAccent2( 0xFF249324 );

    //-------------------------------------------------------------------------

    void Style::Apply()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        //-------------------------------------------------------------------------
        // Colors
        //-------------------------------------------------------------------------

        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text] = s_colorText;
        colors[ImGuiCol_TextDisabled] = s_colorTextDisabled;
        colors[ImGuiCol_TextSelectedBg] = s_colorGray0;

        colors[ImGuiCol_TitleBg] = s_colorGray9;
        colors[ImGuiCol_TitleBgActive] = s_colorGray8;
        colors[ImGuiCol_TitleBgCollapsed] = s_colorGray8;

        colors[ImGuiCol_WindowBg] = s_colorGray6;
        colors[ImGuiCol_ChildBg] = s_colorGray6;
        colors[ImGuiCol_PopupBg] = s_colorGray6;
        colors[ImGuiCol_MenuBarBg] = s_colorGray6;

        colors[ImGuiCol_Border] = s_colorGray2;
        colors[ImGuiCol_BorderShadow] = s_colorGray6;

        colors[ImGuiCol_FrameBg] = s_colorGray8;
        colors[ImGuiCol_FrameBgHovered] = s_colorGray7;
        colors[ImGuiCol_FrameBgActive] = s_colorGray5;

        colors[ImGuiCol_Tab] = s_colorGray6;
        colors[ImGuiCol_TabActive] = s_colorGray4;
        colors[ImGuiCol_TabHovered] = s_colorGray3;
        colors[ImGuiCol_TabUnfocused] = s_colorGray6;
        colors[ImGuiCol_TabUnfocusedActive] = s_colorGray5;

        colors[ImGuiCol_Header] = s_colorGray3;
        colors[ImGuiCol_HeaderHovered] = s_colorGray2;
        colors[ImGuiCol_HeaderActive] = s_colorGray1;

        colors[ImGuiCol_Separator] = s_colorGray2;
        colors[ImGuiCol_SeparatorHovered] = s_colorGray1;
        colors[ImGuiCol_SeparatorActive] = s_colorGray0;

        colors[ImGuiCol_NavHighlight] = s_colorGray1;
        colors[ImGuiCol_DockingPreview] = s_colorGray1;

        colors[ImGuiCol_ScrollbarBg] = s_colorGray6;
        colors[ImGuiCol_ScrollbarGrab] = s_colorGray3;
        colors[ImGuiCol_ScrollbarGrabHovered] = s_colorGray2;
        colors[ImGuiCol_ScrollbarGrabActive] = s_colorGray1;

        colors[ImGuiCol_SliderGrab] = s_colorGray2;
        colors[ImGuiCol_SliderGrabActive] = s_colorGray1;

        colors[ImGuiCol_ResizeGrip] = s_colorGray3;
        colors[ImGuiCol_ResizeGripHovered] = s_colorGray2;
        colors[ImGuiCol_ResizeGripActive] = s_colorGray2;

        colors[ImGuiCol_Button] = s_colorGray3;
        colors[ImGuiCol_ButtonHovered] = s_colorGray2;
        colors[ImGuiCol_ButtonActive] = s_colorGray1;

        colors[ImGuiCol_CheckMark] = s_colorAccent1;

        colors[ImGuiCol_PlotLines] = s_colorAccent2;
        colors[ImGuiCol_PlotLinesHovered] = s_colorAccent1;
        colors[ImGuiCol_PlotHistogram] = s_colorAccent2;
        colors[ImGuiCol_PlotHistogramHovered] = s_colorAccent1;

        colors[ImGuiCol_TableRowBg] = s_colorGray6;
        colors[ImGuiCol_TableRowBgAlt] = s_colorGray5;

        colors[ImGuiCol_DragDropTarget] = s_colorGray6;

        //-------------------------------------------------------------------------
        // Style
        //-------------------------------------------------------------------------

        style.FramePadding = ImVec2( 6, 6 );
        style.WindowPadding = ImVec2( 8, 8 );
        style.ChildBorderSize = 0.0f;
        style.TabBorderSize = 1.0f;
        style.GrabRounding = 0.0f;
        style.GrabMinSize = 8.0f;
        style.WindowRounding = 0.0f;
        style.WindowBorderSize = 0.0f;
        style.FrameRounding = 3.0f;
        style.IndentSpacing = 8;
        style.ItemSpacing = ImVec2( 4, 6 );
        style.TabRounding = 6.0f;
        style.ScrollbarSize = 20.0f;
        style.ScrollbarRounding = 0.0f;
        style.CellPadding = ImVec2( 4, 6 );
    }

    //-------------------------------------------------------------------------

    constexpr static uint32_t ConvertFormat( uint32_t rgba )
    {
        uint32_t r = ( rgba >> 24 ) & 0x000000FF;
        uint32_t g = ( rgba >> 16 ) & 0x000000FF;
        uint32_t b = ( rgba >> 8 ) & 0x000000FF;
        uint32_t a = 0x000000FF;
        return IM_COL32( r, g, b, a );
    }

    //-------------------------------------------------------------------------

    ImColor const ImColors::AliceBlue = ConvertFormat( EE::Colors::AliceBlue );
    ImColor const ImColors::AntiqueWhite = ConvertFormat( EE::Colors::AntiqueWhite );
    ImColor const ImColors::Aqua = ConvertFormat( EE::Colors::Aqua );
    ImColor const ImColors::Aquamarine = ConvertFormat( EE::Colors::Aquamarine );
    ImColor const ImColors::Azure = ConvertFormat( EE::Colors::Azure );
    ImColor const ImColors::Beige = ConvertFormat( EE::Colors::Beige );
    ImColor const ImColors::Bisque = ConvertFormat( EE::Colors::Bisque );
    ImColor const ImColors::Black = ConvertFormat( EE::Colors::Black );
    ImColor const ImColors::BlanchedAlmond = ConvertFormat( EE::Colors::BlanchedAlmond );
    ImColor const ImColors::Blue = ConvertFormat( EE::Colors::Blue );
    ImColor const ImColors::BlueViolet = ConvertFormat( EE::Colors::BlueViolet );
    ImColor const ImColors::Brown = ConvertFormat( EE::Colors::Brown );
    ImColor const ImColors::BurlyWood = ConvertFormat( EE::Colors::BurlyWood );
    ImColor const ImColors::CadetBlue = ConvertFormat( EE::Colors::CadetBlue );
    ImColor const ImColors::Chartreuse = ConvertFormat( EE::Colors::Chartreuse );
    ImColor const ImColors::Chocolate = ConvertFormat( EE::Colors::Chocolate );
    ImColor const ImColors::Coral = ConvertFormat( EE::Colors::Coral );
    ImColor const ImColors::CornflowerBlue = ConvertFormat( EE::Colors::CornflowerBlue );
    ImColor const ImColors::Cornsilk = ConvertFormat( EE::Colors::Cornsilk );
    ImColor const ImColors::Crimson = ConvertFormat( EE::Colors::Crimson );
    ImColor const ImColors::Cyan = ConvertFormat( EE::Colors::Cyan );
    ImColor const ImColors::DarkBlue = ConvertFormat( EE::Colors::DarkBlue );
    ImColor const ImColors::DarkCyan = ConvertFormat( EE::Colors::DarkCyan );
    ImColor const ImColors::DarkGoldenRod = ConvertFormat( EE::Colors::DarkGoldenRod );
    ImColor const ImColors::DarkGray = ConvertFormat( EE::Colors::DarkGray );
    ImColor const ImColors::DarkGreen = ConvertFormat( EE::Colors::DarkGreen );
    ImColor const ImColors::DarkKhaki = ConvertFormat( EE::Colors::DarkKhaki );
    ImColor const ImColors::DarkMagenta = ConvertFormat( EE::Colors::DarkMagenta );
    ImColor const ImColors::DarkOliveGreen = ConvertFormat( EE::Colors::DarkOliveGreen );
    ImColor const ImColors::DarkOrange = ConvertFormat( EE::Colors::DarkOrange );
    ImColor const ImColors::DarkOrchid = ConvertFormat( EE::Colors::DarkOrchid );
    ImColor const ImColors::DarkRed = ConvertFormat( EE::Colors::DarkRed );
    ImColor const ImColors::DarkSalmon = ConvertFormat( EE::Colors::DarkSalmon );
    ImColor const ImColors::DarkSeaGreen = ConvertFormat( EE::Colors::DarkSeaGreen );
    ImColor const ImColors::DarkSlateBlue = ConvertFormat( EE::Colors::DarkSlateBlue );
    ImColor const ImColors::DarkSlateGray = ConvertFormat( EE::Colors::DarkSlateGray );
    ImColor const ImColors::DarkTurquoise = ConvertFormat( EE::Colors::DarkTurquoise );
    ImColor const ImColors::DarkViolet = ConvertFormat( EE::Colors::DarkViolet );
    ImColor const ImColors::DeepPink = ConvertFormat( EE::Colors::DeepPink );
    ImColor const ImColors::DeepSkyBlue = ConvertFormat( EE::Colors::DeepSkyBlue );
    ImColor const ImColors::DimGray = ConvertFormat( EE::Colors::DimGray );
    ImColor const ImColors::DodgerBlue = ConvertFormat( EE::Colors::DodgerBlue );
    ImColor const ImColors::FireBrick = ConvertFormat( EE::Colors::FireBrick );
    ImColor const ImColors::FloralWhite = ConvertFormat( EE::Colors::FloralWhite );
    ImColor const ImColors::ForestGreen = ConvertFormat( EE::Colors::ForestGreen );
    ImColor const ImColors::Fuchsia = ConvertFormat( EE::Colors::Fuchsia );
    ImColor const ImColors::Gainsboro = ConvertFormat( EE::Colors::Gainsboro );
    ImColor const ImColors::GhostWhite = ConvertFormat( EE::Colors::GhostWhite );
    ImColor const ImColors::Gold = ConvertFormat( EE::Colors::Gold );
    ImColor const ImColors::GoldenRod = ConvertFormat( EE::Colors::GoldenRod );
    ImColor const ImColors::Gray = ConvertFormat( EE::Colors::Gray );
    ImColor const ImColors::Green = ConvertFormat( EE::Colors::Green );
    ImColor const ImColors::GreenYellow = ConvertFormat( EE::Colors::GreenYellow );
    ImColor const ImColors::HoneyDew = ConvertFormat( EE::Colors::HoneyDew );
    ImColor const ImColors::HotPink = ConvertFormat( EE::Colors::HotPink );
    ImColor const ImColors::IndianRed = ConvertFormat( EE::Colors::IndianRed );
    ImColor const ImColors::Indigo = ConvertFormat( EE::Colors::Indigo );
    ImColor const ImColors::Ivory = ConvertFormat( EE::Colors::Ivory );
    ImColor const ImColors::Khaki = ConvertFormat( EE::Colors::Khaki );
    ImColor const ImColors::Lavender = ConvertFormat( EE::Colors::Lavender );
    ImColor const ImColors::LavenderBlush = ConvertFormat( EE::Colors::LavenderBlush );
    ImColor const ImColors::LawnGreen = ConvertFormat( EE::Colors::LawnGreen );
    ImColor const ImColors::LemonChiffon = ConvertFormat( EE::Colors::LemonChiffon );
    ImColor const ImColors::LightBlue = ConvertFormat( EE::Colors::LightBlue );
    ImColor const ImColors::LightCoral = ConvertFormat( EE::Colors::LightCoral );
    ImColor const ImColors::LightCyan = ConvertFormat( EE::Colors::LightCyan );
    ImColor const ImColors::LightGoldenRodYellow = ConvertFormat( EE::Colors::LightGoldenRodYellow );
    ImColor const ImColors::LightGray = ConvertFormat( EE::Colors::LightGray );
    ImColor const ImColors::LightGreen = ConvertFormat( EE::Colors::LightGreen );
    ImColor const ImColors::LightPink = ConvertFormat( EE::Colors::LightPink );
    ImColor const ImColors::LightSalmon = ConvertFormat( EE::Colors::LightSalmon );
    ImColor const ImColors::LightSeaGreen = ConvertFormat( EE::Colors::LightSeaGreen );
    ImColor const ImColors::LightSkyBlue = ConvertFormat( EE::Colors::LightSkyBlue );
    ImColor const ImColors::LightSlateGray = ConvertFormat( EE::Colors::LightSlateGray );
    ImColor const ImColors::LightSteelBlue = ConvertFormat( EE::Colors::LightSteelBlue );
    ImColor const ImColors::LightYellow = ConvertFormat( EE::Colors::LightYellow );
    ImColor const ImColors::Lime = ConvertFormat( EE::Colors::Lime );
    ImColor const ImColors::LimeGreen = ConvertFormat( EE::Colors::LimeGreen );
    ImColor const ImColors::Linen = ConvertFormat( EE::Colors::Linen );
    ImColor const ImColors::Magenta = ConvertFormat( EE::Colors::Magenta );
    ImColor const ImColors::Maroon = ConvertFormat( EE::Colors::Maroon );
    ImColor const ImColors::MediumAquaMarine = ConvertFormat( EE::Colors::MediumAquaMarine );
    ImColor const ImColors::MediumBlue = ConvertFormat( EE::Colors::MediumBlue );
    ImColor const ImColors::MediumOrchid = ConvertFormat( EE::Colors::MediumOrchid );
    ImColor const ImColors::MediumPurple = ConvertFormat( EE::Colors::MediumPurple );
    ImColor const ImColors::MediumSeaGreen = ConvertFormat( EE::Colors::MediumSeaGreen );
    ImColor const ImColors::MediumSlateBlue = ConvertFormat( EE::Colors::MediumSlateBlue );
    ImColor const ImColors::MediumSpringGreen = ConvertFormat( EE::Colors::MediumSpringGreen );
    ImColor const ImColors::MediumTurquoise = ConvertFormat( EE::Colors::MediumTurquoise );
    ImColor const ImColors::MediumVioletRed = ConvertFormat( EE::Colors::MediumVioletRed );
    ImColor const ImColors::MidnightBlue = ConvertFormat( EE::Colors::MidnightBlue );
    ImColor const ImColors::MintCream = ConvertFormat( EE::Colors::MintCream );
    ImColor const ImColors::MistyRose = ConvertFormat( EE::Colors::MistyRose );
    ImColor const ImColors::Moccasin = ConvertFormat( EE::Colors::Moccasin );
    ImColor const ImColors::NavajoWhite = ConvertFormat( EE::Colors::NavajoWhite );
    ImColor const ImColors::Navy = ConvertFormat( EE::Colors::Navy );
    ImColor const ImColors::OldLace = ConvertFormat( EE::Colors::OldLace );
    ImColor const ImColors::Olive = ConvertFormat( EE::Colors::Olive );
    ImColor const ImColors::OliveDrab = ConvertFormat( EE::Colors::OliveDrab );
    ImColor const ImColors::Orange = ConvertFormat( EE::Colors::Orange );
    ImColor const ImColors::OrangeRed = ConvertFormat( EE::Colors::OrangeRed );
    ImColor const ImColors::Orchid = ConvertFormat( EE::Colors::Orchid );
    ImColor const ImColors::PaleGoldenRod = ConvertFormat( EE::Colors::PaleGoldenRod );
    ImColor const ImColors::PaleGreen = ConvertFormat( EE::Colors::PaleGreen );
    ImColor const ImColors::PaleTurquoise = ConvertFormat( EE::Colors::PaleTurquoise );
    ImColor const ImColors::PaleVioletRed = ConvertFormat( EE::Colors::PaleVioletRed );
    ImColor const ImColors::PapayaWhip = ConvertFormat( EE::Colors::PapayaWhip );
    ImColor const ImColors::PeachPuff = ConvertFormat( EE::Colors::PeachPuff );
    ImColor const ImColors::Peru = ConvertFormat( EE::Colors::Peru );
    ImColor const ImColors::Pink = ConvertFormat( EE::Colors::Pink );
    ImColor const ImColors::Plum = ConvertFormat( EE::Colors::Plum );
    ImColor const ImColors::PowderBlue = ConvertFormat( EE::Colors::PowderBlue );
    ImColor const ImColors::Purple = ConvertFormat( EE::Colors::Purple );
    ImColor const ImColors::Red = ConvertFormat( EE::Colors::Red );
    ImColor const ImColors::MediumRed = ConvertFormat( EE::Colors::MediumRed );
    ImColor const ImColors::RosyBrown = ConvertFormat( EE::Colors::RosyBrown );
    ImColor const ImColors::RoyalBlue = ConvertFormat( EE::Colors::RoyalBlue );
    ImColor const ImColors::SaddleBrown = ConvertFormat( EE::Colors::SaddleBrown );
    ImColor const ImColors::Salmon = ConvertFormat( EE::Colors::Salmon );
    ImColor const ImColors::SandyBrown = ConvertFormat( EE::Colors::SandyBrown );
    ImColor const ImColors::SeaGreen = ConvertFormat( EE::Colors::SeaGreen );
    ImColor const ImColors::SeaShell = ConvertFormat( EE::Colors::SeaShell );
    ImColor const ImColors::Sienna = ConvertFormat( EE::Colors::Sienna );
    ImColor const ImColors::Silver = ConvertFormat( EE::Colors::Silver );
    ImColor const ImColors::SkyBlue = ConvertFormat( EE::Colors::SkyBlue );
    ImColor const ImColors::SlateBlue = ConvertFormat( EE::Colors::SlateBlue );
    ImColor const ImColors::SlateGray = ConvertFormat( EE::Colors::SlateGray );
    ImColor const ImColors::Snow = ConvertFormat( EE::Colors::Snow );
    ImColor const ImColors::SpringGreen = ConvertFormat( EE::Colors::SpringGreen );
    ImColor const ImColors::SteelBlue = ConvertFormat( EE::Colors::SteelBlue );
    ImColor const ImColors::Tan = ConvertFormat( EE::Colors::Tan );
    ImColor const ImColors::Teal = ConvertFormat( EE::Colors::Teal );
    ImColor const ImColors::Thistle = ConvertFormat( EE::Colors::Thistle );
    ImColor const ImColors::Tomato = ConvertFormat( EE::Colors::Tomato );
    ImColor const ImColors::Turquoise = ConvertFormat( EE::Colors::Turquoise );
    ImColor const ImColors::Violet = ConvertFormat( EE::Colors::Violet );
    ImColor const ImColors::Wheat = ConvertFormat( EE::Colors::Wheat );
    ImColor const ImColors::White = ConvertFormat( EE::Colors::White );
    ImColor const ImColors::WhiteSmoke = ConvertFormat( EE::Colors::WhiteSmoke );
    ImColor const ImColors::Yellow = ConvertFormat( EE::Colors::Yellow );
    ImColor const ImColors::YellowGreen = ConvertFormat( EE::Colors::YellowGreen );
}