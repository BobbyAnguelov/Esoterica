#include "ImguiFont.h"
#include "Base/ThirdParty/imgui/imgui.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    ImFont* SystemFonts::s_fonts[(int32_t) Font::NumFonts] = { nullptr, nullptr, nullptr, nullptr };

    //-------------------------------------------------------------------------

    ScopedFont::ScopedFont( Font font )
    {
        EE_ASSERT( font != Font::NumFonts && SystemFonts::s_fonts[(uint8_t) font] != nullptr );
        ImGui::PushFont( SystemFonts::s_fonts[(uint8_t) font] );
        m_fontApplied = true;
    }

    ScopedFont::ScopedFont( Font font, Color const& color )
    {
        EE_ASSERT( font != Font::NumFonts && SystemFonts::s_fonts[(uint8_t) font] != nullptr );
        ImGui::PushFont( SystemFonts::s_fonts[(uint8_t) font] );
        m_fontApplied = true;

        ImGui::PushStyleColor( ImGuiCol_Text, color.ToFloat4() );
        m_colorApplied = true;
    }

    ScopedFont::ScopedFont( Color const& color )
    {
        ImGui::PushStyleColor( ImGuiCol_Text, color.ToFloat4() );
        m_colorApplied = true;
    }

    ScopedFont::~ScopedFont()
    {
        if ( m_colorApplied )
        {
            ImGui::PopStyleColor();
        }

        if ( m_fontApplied )
        {
            ImGui::PopFont();
        }
    }
}
#endif