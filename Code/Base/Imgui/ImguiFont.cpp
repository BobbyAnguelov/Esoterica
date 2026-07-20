#include "ImguiFont.h"
#include "Base/ThirdParty/imgui/imgui.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    ImFont* SystemFonts::s_fonts[(int32_t) FontType::NumFontTypes];

    //-------------------------------------------------------------------------

    ScopedFont::ScopedFont( Font font )
    {
        ImGui::PushFont( GetFont( font ), GetFontSize( font ) * ImGui::GetWindowDpiScale() );
        m_fontApplied = true;
    }

    ScopedFont::ScopedFont( Font font, Color const& color )
    {
        ImGui::PushFont( GetFont( font ), GetFontSize( font ) * ImGui::GetWindowDpiScale() );
        m_fontApplied = true;

        ImGui::PushStyleColor( ImGuiCol_Text, color.ToFloat4() );
        m_colorApplied = true;
    }

    ScopedFont::ScopedFont( Color const& color )
    {
        ImGui::PushStyleColor( ImGuiCol_Text, color.ToFloat4() );
        m_colorApplied = true;
    }

    ScopedFont::ScopedFont( FontType fontType, float size )
    {
        EE_ASSERT( size > 0 );

        int32_t const fontIdx = (int32_t) fontType;
        ImGui::PushFont( SystemFonts::s_fonts[fontIdx], size * ImGui::GetWindowDpiScale() );
        m_fontApplied = true;
    }

    ScopedFont::ScopedFont( FontType fontType, float size, Color const& color )
    {
        EE_ASSERT( size > 0 );

        int32_t const fontIdx = (int32_t) fontType;
        ImGui::PushFont( SystemFonts::s_fonts[fontIdx], size * ImGui::GetWindowDpiScale() );
        ImGui::PushStyleColor( ImGuiCol_Text, color.ToFloat4() );
        m_fontApplied = true;
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

    //-------------------------------------------------------------------------

    void PushFont( Font font )
    {
        ImGui::PushFont( GetFont( font ), GetFontSize( font ) * ImGui::GetWindowDpiScale() );
    }

    void PushFont( FontType fontType, float size )
    {
        EE_ASSERT( size > 0 );
        ImGui::PushFont( GetFont( fontType ), size * ImGui::GetWindowDpiScale() );
    }

    void PushFontAndColor( Font font, Color const& color )
    {
        ImGui::PushFont( GetFont( font ), GetFontSize( font ) * ImGui::GetWindowDpiScale() );
        ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( color.m_byteColor.m_r, color.m_byteColor.m_g, color.m_byteColor.m_b, color.m_byteColor.m_a ) );
    }

    void PushFontAndColor( FontType fontType, float size, Color const& color )
    {
        EE_ASSERT( size > 0 );
        ImGui::PushFont( GetFont( fontType ), size * ImGui::GetWindowDpiScale() );
        ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( color.m_byteColor.m_r, color.m_byteColor.m_g, color.m_byteColor.m_b, color.m_byteColor.m_a ) );
    }
}
#endif