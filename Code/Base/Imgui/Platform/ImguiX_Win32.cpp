#include "Base/Imgui/ImguiX.h"

#if EE_DEVELOPMENT_TOOLS
#if _WIN32
#include <windows.h>

//-------------------------------------------------------------------------

#define EE_ENABLE_IMGUIX_DEBUG 0

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    void ApplicationTitleBar::Draw( TFunction<void()>&& menuDrawFunction, float menuSectionDesiredWidth, TFunction<void()>&& controlsSectionDrawFunction, float controlsSectionDesiredWidth )
    {
        m_rect.Reset();

        //-------------------------------------------------------------------------

        ImVec2 const titleBarPadding( 0, 8 );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, titleBarPadding );
        ImGuiViewport* pViewport = ImGui::GetMainViewport();
        if ( ImGui::BeginViewportSideBar( "TitleBar", pViewport, ImGuiDir_Up, 40, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            ImGui::PopStyleVar();

            // Calculate sizes
            //-------------------------------------------------------------------------

            float const titleBarWidth = ImGui::GetWindowSize().x;
            float const titleBarHeight = ImGui::GetWindowSize().y;
            m_rect = Math::ScreenSpaceRectangle( Float2::Zero, Float2( titleBarWidth, titleBarHeight ) );

            float const windowControlsWidth = GetWindowsControlsWidth();
            float const windowControlsStartPosX = Math::Max( 0.0f, ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - windowControlsWidth );
            ImVec2 const windowControlsStartPos( windowControlsStartPosX, ImGui::GetCursorPosY() - titleBarPadding.y );

            // Calculate the available space
            float const availableSpace = titleBarWidth - windowControlsWidth - s_minimumDraggableGap - ( s_sectionPadding * 2 );
            float remainingSpace = availableSpace;

            // Calculate section widths
            float const menuSectionFinalWidth = ( remainingSpace - menuSectionDesiredWidth ) > 0 ? menuSectionDesiredWidth : Math::Max( 0.0f, remainingSpace );
            remainingSpace -= menuSectionFinalWidth;
            ImVec2 const menuSectionStartPos( s_sectionPadding, ImGui::GetCursorPosY() );

            float const controlSectionFinalWidth = ( remainingSpace - controlsSectionDesiredWidth ) > 0 ? controlsSectionDesiredWidth : Math::Max( 0.0f, remainingSpace );
            remainingSpace -= controlSectionFinalWidth;
            ImVec2 const controlSectionStartPos( windowControlsStartPos.x - s_sectionPadding - controlSectionFinalWidth, ImGui::GetCursorPosY() - titleBarPadding.y );

            // Draw sections
            //-------------------------------------------------------------------------

            if ( menuSectionFinalWidth > 0 )
            {
                #if EE_ENABLE_IMGUIX_DEBUG
                ImGui::PushStyleColor( ImGuiCol_ChildBg, Colors::Green ); // Debug Color
                #endif

                ImGui::SetCursorPos( menuSectionStartPos );
                ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
                ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImGui::GetStyle().FramePadding + ImVec2( 0, 2 ) );
                bool const drawMenuSection = ImGui::BeginChild( "Left", ImVec2( menuSectionFinalWidth, titleBarHeight ), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar );
                ImGui::PopStyleVar( 2 );

                if ( drawMenuSection )
                {
                    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 16, 8 ) );
                    if ( ImGui::BeginMenuBar() )
                    {
                        menuDrawFunction();
                        ImGui::EndMenuBar();
                    }
                    ImGui::PopStyleVar();
                }
                ImGui::EndChild();

                #if EE_ENABLE_IMGUIX_DEBUG
                ImGui::PopStyleColor();
                #endif
            }

            if ( controlSectionFinalWidth > 0 )
            {
                #if EE_ENABLE_IMGUIX_DEBUG
                ImGui::PushStyleColor( ImGuiCol_ChildBg, Colors::Red ); // Debug Color
                #endif

                ImGui::SetCursorPos( controlSectionStartPos );
                ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
                bool const drawControlsSection = ImGui::BeginChild( "Right", ImVec2( controlSectionFinalWidth, titleBarHeight ), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoDecoration );
                ImGui::PopStyleVar();

                if ( drawControlsSection )
                {
                    controlsSectionDrawFunction();
                }
                ImGui::EndChild();

                #if EE_ENABLE_IMGUIX_DEBUG
                ImGui::PopStyleColor();
                #endif
            }

            // Draw window controls
            //-------------------------------------------------------------------------

            #if EE_ENABLE_IMGUIX_DEBUG
            ImGui::PushStyleColor( ImGuiCol_ChildBg, Colors::Blue ); // Debug Color
            #endif

            ImGui::SetCursorPos( windowControlsStartPos );
            if ( ImGui::BeginChild( "WindowControls", ImVec2( windowControlsWidth, titleBarHeight ), 0, ImGuiWindowFlags_NoDecoration ) )
            {
                DrawWindowControls();
            }
            ImGui::EndChild();

            #if EE_ENABLE_IMGUIX_DEBUG
            ImGui::PopStyleColor();
            #endif

            //-------------------------------------------------------------------------

            ImGui::End();
        }
    }

    void ApplicationTitleBar::DrawWindowControls()
    {
        ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 0 );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );

        //-------------------------------------------------------------------------

        HWND hwnd = (HWND) ImGui::GetWindowViewport()->PlatformHandleRaw;

        // Minimize
        //-------------------------------------------------------------------------

        if ( ImGuiX::FlatIconButton( EE_ICON_WINDOW_MINIMIZE, "##Min", Colors::White, ImVec2( s_windowControlButtonWidth, -1 ), true ) )
        {
            if ( hwnd )
            {
                ShowWindow( hwnd, SW_MINIMIZE );
            }
        }

        // Maximize/Restore
        //-------------------------------------------------------------------------

        bool isMaximized = false;
        if ( hwnd )
        {
            WINDOWPLACEMENT placement;
            if ( ::GetWindowPlacement( hwnd, &placement ) )
            {
                isMaximized = placement.showCmd == SW_MAXIMIZE;
            }
        }

        ImGui::SameLine();

        if ( isMaximized )
        {
            if ( ImGuiX::FlatIconButton( EE_ICON_WINDOW_RESTORE, "##Res", Colors::White, ImVec2( s_windowControlButtonWidth, -1 ), true ) )
            {
                if ( hwnd )
                {
                    ShowWindow( hwnd, SW_RESTORE );
                }
            }
        }
        else
        {
            if ( ImGuiX::FlatIconButton( EE_ICON_WINDOW_MAXIMIZE, "##Max", Colors::White, ImVec2( s_windowControlButtonWidth, -1 ), true ) )
            {
                if ( hwnd )
                {
                    ShowWindow( hwnd, SW_MAXIMIZE );
                }
            }
        }

        // Close
        //-------------------------------------------------------------------------

        ImGui::SameLine();

        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, 0xFF1C2BC4 );

        ImU32 const backgroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Button] );
        ImU32 const hoverColor = 0xFF1C2BC4;
        ImU32 const activeColor = 0xFF141E89;

        ImGuiX::ButtonSettings settings{ .m_backgroundColor = Colors::Transparent, .m_hoverColor = hoverColor, .m_activeColor = activeColor, .m_iconColor = Colors::White, .m_foregroundColor = Colors::White, .m_shouldCenterContents = true };
        if ( ImGuiX::ButtonEx( EE_ICON_WINDOW_CLOSE, "##X", ImVec2( s_windowControlButtonWidth, -1 ), settings ) )
        {
            if ( hwnd )
            {
                SendMessage( hwnd, WM_CLOSE, 0, 0 );
            }
        }

        //-------------------------------------------------------------------------

        ImGui::PopStyleColor();
        ImGui::PopStyleVar( 2 );
    }
}
#endif
#endif