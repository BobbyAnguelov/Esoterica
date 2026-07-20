#pragma once

#include "Engine/_Module/API.h"
#include "Base/Types/String.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class UpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PerformanceStatsWidget
    {
        constexpr static float const s_verticalMargin = 3.0f;
        constexpr static float const s_startTextOffset = 8.0f;
        constexpr static float const s_individualStatSpacing = 16.0f;
        constexpr static float const s_profilerButtonSpacing = 8.0f;

    public:

        PerformanceStatsWidget( UpdateContext const& context );
        void Draw() const;

    public:

        float                                               m_fps = 0;
        float                                               m_allocatedRAM;
        float                                               m_allocatedVRAM;

        ImVec2                                              m_totalSize;

    private:

        ImVec2                                              m_windowPadding = ImVec2( 0, 0 );
        ImVec2                                              m_profilerButtonSize;

        Color                                               m_fpsColor;
        float                                               m_fpsExtraSpacing = 0;
        TInlineString<10>                                   m_str_FPS;
        ImVec2                                              m_strSize_FPS;
        ImVec2                                              m_strSize_FPS_value;

        TInlineString<10>                                   m_str_RAM;
        TInlineString<10>                                   m_str_VRAM;
    };
}
#endif