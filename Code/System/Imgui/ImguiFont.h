#pragma once
#include "../_Module/API.h"
#include "System/ThirdParty/imgui/imgui.h"
#include "System/Esoterica.h"
#include "System/Types/Color.h"

//-------------------------------------------------------------------------

struct ImFont;
struct ImVec4;

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    enum class Font : uint8_t
    {
        Tiny,
        TinyBold,
        Small,
        SmallBold,
        Medium,
        MediumBold,
        Large,
        LargeBold,

        NumFonts,
        Default = Medium,
    };

    //-------------------------------------------------------------------------

    struct EE_SYSTEM_API SystemFonts
    {
        static ImFont* s_fonts[(int32_t) Font::NumFonts];
    };

    EE_FORCE_INLINE ImFont* GetFont( Font font ) { return SystemFonts::s_fonts[(int32_t) font]; }

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API [[nodiscard]] ScopedFont
    {
    public:

        ScopedFont( Font font );
        ScopedFont( ImColor const& color );
        ScopedFont( Font font, ImColor const& color );
        ~ScopedFont();

        explicit ScopedFont( Font font, Color const& color ) : ScopedFont( font, (ImColor) IM_COL32( color.m_byteColor.m_r, color.m_byteColor.m_g, color.m_byteColor.m_b, color.m_byteColor.m_a ) ) {}

    private:

        bool m_fontApplied = false;
        bool m_colorApplied = false;
    };

    //-------------------------------------------------------------------------

    EE_SYSTEM_API inline void PushFont( Font font ) 
    {
        ImGui::PushFont( SystemFonts::s_fonts[(int8_t) font] ); 
    }

    EE_SYSTEM_API inline void PushFontAndColor( Font font, ImColor const& color ) 
    {
        ImGui::PushFont( SystemFonts::s_fonts[(int8_t) font] );
        ImGui::PushStyleColor( ImGuiCol_Text, color.Value );
    }

    EE_SYSTEM_API inline void PushFontAndColor( Font font, Color const& color )
    {
        ImGui::PushFont( SystemFonts::s_fonts[(int8_t) font] );
        ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( color.m_byteColor.m_r, color.m_byteColor.m_g, color.m_byteColor.m_b, color.m_byteColor.m_a ) );
    }
}

//-------------------------------------------------------------------------

#include "MaterialDesignIcons.h"

#endif