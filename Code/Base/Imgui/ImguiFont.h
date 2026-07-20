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
    enum class FontType
    {
        Regular = 0,
        Italic,
        Bold,
        BoldItalic,

        NumFontTypes,
    };

    enum class FontSize
    {
        Tiny = 0,
        Small,
        Medium,
        Large,

        NumFontSizes,
    };

    //-------------------------------------------------------------------------

    struct EE_BASE_API SystemFonts
    {
        constexpr static float      s_fontSizes[4] = { 12, 14, 16, 20 };
        static ImFont*              s_fonts[4];
    };

    EE_FORCE_INLINE ImFont* GetDefaultFont() { return SystemFonts::s_fonts[(int32_t) FontType::Regular]; }
    EE_FORCE_INLINE float GetDefaultFontSize() { return SystemFonts::s_fontSizes[(int32_t) FontSize::Medium]; }
    EE_FORCE_INLINE ImFont* GetFont( FontType fontType ) { return SystemFonts::s_fonts[(int32_t) fontType]; }
    EE_FORCE_INLINE float GetFontSize( FontSize fontSize ) { return SystemFonts::s_fontSizes[(int32_t) fontSize]; }

    //-------------------------------------------------------------------------

    // Helper for all common permutations of size and type
    enum class Font : uint8_t
    {
        Tiny = 0,
        TinyItalic,
        TinyBold,
        TinyBoldItalic,

        Small,
        SmallItalic,
        SmallBold,
        SmallBoldItalic,

        Medium,
        MediumItalic,
        MediumBold,
        MediumBoldItalic,

        Large,
        LargeItalic,
        LargeBold,
        LargeBoldItalic,

        NumFonts,
        Default = Medium
    };

    EE_FORCE_INLINE ImFont* GetFont( Font font )
    {
        EE_ASSERT( font < Font::NumFonts );
        int32_t fontIdx = (int32_t) font % (int32_t) FontType::NumFontTypes;
        return SystemFonts::s_fonts[fontIdx];
    }

    EE_FORCE_INLINE float GetFontSize( Font font )
    {
        EE_ASSERT( font < Font::NumFonts );
        int32_t fontSizeIdx = (int32_t) font / (int32_t) FontType::NumFontTypes;
        return SystemFonts::s_fontSizes[fontSizeIdx];
    }

    //-------------------------------------------------------------------------

    class EE_BASE_API [[nodiscard]] ScopedFont
    {
    public:

        ScopedFont( Font font );
        ScopedFont( Font font, Color const& color );

        ScopedFont( Color const& color );

        ScopedFont( FontType fontType, float size );
        ScopedFont( FontType fontType, float size, Color const& color );

        ~ScopedFont();

        ScopedFont& operator=( ScopedFont const& ) = default;

    private:

        bool m_fontApplied = false;
        bool m_colorApplied = false;
    };

    //-------------------------------------------------------------------------

    EE_BASE_API void PushFont( Font font );
    EE_BASE_API void PushFont( FontType fontType, float size );
    EE_BASE_API void PushFontAndColor( Font font, Color const& color );
    EE_BASE_API void PushFontAndColor( FontType fontType, float size, Color const& color );

    EE_FORCE_INLINE void PopFont()
    {
        ImGui::PopFont();
    }

    EE_FORCE_INLINE void PopFontAndColor()
    {
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }
}

//-------------------------------------------------------------------------

#include "MaterialDesignIcons.h"

#endif