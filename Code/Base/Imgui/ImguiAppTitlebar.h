#pragma once
#include "ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    struct EE_BASE_API ApplicationTitleBar
    {
        constexpr static float const s_windowControlButtonWidth = 45;
        constexpr static float const s_minimumDraggableGap = 24; // Minimum open gap left open to allow dragging
        constexpr static float const s_sectionPadding = 8; // Padding between the window frame/window controls and the menu/control sections

        static inline float GetWindowsControlsWidth() { return s_windowControlButtonWidth * 3; }
        static void DrawWindowControls();

    public:

        // This function takes two delegates and sizes each representing the title bar menu and an extra optional controls section
        void Draw( TFunction<void()>&& menuSectionDrawFunction = TFunction<void()>(), float menuSectionWidth = 0, TFunction<void()>&& controlsSectionDrawFunction = TFunction<void()>(), float controlsSectionWidth = 0 );

        // Get the screen space rectangle for this title bar
        Math::ScreenSpaceRectangle const& GetScreenRectangle() const { return m_rect; }

    private:

        Math::ScreenSpaceRectangle  m_rect;
    };
}
#endif