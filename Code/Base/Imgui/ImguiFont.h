#pragma once
#include "../_Module/API.h"
#include "Base/ThirdParty/imgui/imgui.h"
#include "Base/Esoterica.h"
#include "Base/Types/Color.h"

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

    struct EE_BASE_API SystemFonts
    {
        static ImFont* s_fonts[(int32_t) Font::NumFonts];
    };

    EE_FORCE_INLINE ImFont* GetFont( Font font ) { return SystemFonts::s_fonts[(int32_t) font]; }

    //-------------------------------------------------------------------------

    class EE_BASE_API [[nodiscard]] ScopedFont
    {
    public:

        ScopedFont( Font font );
        ScopedFont( Color const& color );
        ScopedFont( Font font, Color const& color );
        ~ScopedFont();

        ScopedFont& operator=( ScopedFont const& ) = default;

    private:

        bool m_fontApplied = false;
        bool m_colorApplied = false;
    };

    //-------------------------------------------------------------------------

    EE_BASE_API inline void PushFont( Font font ) 
    {
        ImGui::PushFont( SystemFonts::s_fonts[(int8_t) font] ); 
    }

    EE_BASE_API inline void PushFontAndColor( Font font, Color const& color )
    {
        ImGui::PushFont( SystemFonts::s_fonts[(int8_t) font] );
        ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( color.m_byteColor.m_r, color.m_byteColor.m_g, color.m_byteColor.m_b, color.m_byteColor.m_a ) );
    }
}

//-------------------------------------------------------------------------

#include "MaterialDesignIcons.h"

#endif