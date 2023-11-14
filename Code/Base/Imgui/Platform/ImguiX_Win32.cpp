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
        if ( ImGui::BeginViewportSideBar( "TitleBar", pViewport, ImGuiDir_Up, 40, ImGuiWindowFlags_NoDecoration ) )
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
            ImVec2 const controlSectionStartPos( windowControlsStartPos.x - s_sectionPadding - controlSectionFinalWidth, ImGui::GetCursorPosY() );

            // Draw sections
            //-------------------------------------------------------------------------

            if ( menuSectionFinalWidth > 0 )
            {
                #if EE_ENABLE_IMGUIX_DEBUG
                ImGui::PushStyleColor( ImGuiCol_ChildBg, 0xFFFF0000 ); // Debug Color
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
                ImGui::PushStyleColor( ImGuiCol_ChildBg, 0xFF0000FF ); // Debug Color
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
            ImGui::PushStyleColor( ImGuiCol_ChildBg, 0xFF00FF00 ); // Debug Color
            #endif

            ImGui::SetCursorPos( windowControlsStartPos );
            if ( ImGui::BeginChild( "WindowControls", ImVec2( windowControlsWidth, titleBarHeight ), false, ImGuiWindowFlags_NoDecoration ) )
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

        auto hwnd = GetActiveWindow();

        // Minimize
        //-------------------------------------------------------------------------

        if ( ImGuiX::FlatButton( EE_ICON_WINDOW_MINIMIZE"##Min", ImVec2( s_windowControlButtonWidth, -1 ) ) )
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
            if ( ImGuiX::FlatButton( EE_ICON_WINDOW_RESTORE"##Res", ImVec2( s_windowControlButtonWidth, -1 ) ) )
            {
                if ( hwnd )
                {
                    ShowWindow( hwnd, SW_RESTORE );
                }
            }
        }
        else
        {
            if ( ImGuiX::FlatButton( EE_ICON_WINDOW_MAXIMIZE"##Max", ImVec2( s_windowControlButtonWidth, -1 ) ) )
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
        if ( ImGuiX::FlatButton( EE_ICON_WINDOW_CLOSE"##X", ImVec2( s_windowControlButtonWidth, -1 ) ) )
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