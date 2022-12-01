#include "System/Imgui/ImguiX.h"

#if EE_DEVELOPMENT_TOOLS
#if _WIN32
#include <windows.h>

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    Float2 const ApplicationTitleBar::s_windowControlButtonSize = Float2( 45, 31 );

    //-------------------------------------------------------------------------

    void ApplicationTitleBar::Draw( TFunction<void()>&& leftSectionDrawFunction, float leftSectionDesiredWidth, TFunction<void()>&& midSectionDrawFunction, float midSectionDesiredWidth, TFunction<void()>&& rightSectionDrawFunction, float rightSectionDesiredWidth )
    {
        m_draggableRegions.clear();

        //-------------------------------------------------------------------------

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 2 ) );
        ImGuiViewport* pViewport = ImGui::GetMainViewport();
        if ( ImGui::BeginViewportSideBar( "TitleBar", pViewport, ImGuiDir_Up, 34, ImGuiWindowFlags_NoDecoration ) )
        {
            ImGui::PopStyleVar();

            // Calculate sizes
            //-------------------------------------------------------------------------

            float const titleBarWidth = ImGui::GetWindowSize().x;
            float const titleBarHeight = ImGui::GetWindowSize().y;

            float const windowControlsWidth = GetWindowsControlsWidth();
            float const windowControlsStartPosX = Math::Max( 0.0f, ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - windowControlsWidth );
            ImVec2 const windowControlsStartPos( windowControlsStartPosX, ImGui::GetCursorPosY() );

            // Calculate the available space
            constexpr float const leftSectionExtraPadding = 6.f;
            float const availableSpace = titleBarWidth - windowControlsWidth - s_minimumDraggableGap - leftSectionExtraPadding;
            float remainingSpace = availableSpace;

            // Calculate section widths
            float leftSectionFinalWidth = ( remainingSpace - leftSectionDesiredWidth ) > 0 ? leftSectionDesiredWidth : Math::Max( 0.0f, remainingSpace );
            remainingSpace -= leftSectionFinalWidth;
            float midSectionFinalWidth = ( remainingSpace - midSectionDesiredWidth ) > 0 ? midSectionDesiredWidth : Math::Max( 0.0f, remainingSpace );
            remainingSpace -= midSectionFinalWidth;
            float rightSectionFinalWidth = ( remainingSpace - rightSectionDesiredWidth ) > 0 ? rightSectionDesiredWidth : Math::Max( 0.0f, remainingSpace );
            remainingSpace -= rightSectionFinalWidth;

            // Calculate start positions
            ImVec2 const leftSectionStartPos( leftSectionExtraPadding, ImGui::GetCursorPosY() );
            ImVec2 midSectionStartPos( leftSectionFinalWidth, 0 );
            ImVec2 rightSectionStartPos( midSectionStartPos.x + midSectionFinalWidth, 0 );

            // We have remaining space so calculate gaps
            if ( remainingSpace > 0 )
            {
                rightSectionStartPos.x = windowControlsStartPos.x - rightSectionFinalWidth;

                float idealMidSectionStartPosX = ( ( titleBarWidth - midSectionFinalWidth - ( 2.0f * s_minimumDraggableGap ) ) / 2.0f );
                if ( ( idealMidSectionStartPosX + midSectionFinalWidth + s_minimumDraggableGap ) < rightSectionStartPos.x )
                {
                    midSectionStartPos.x = idealMidSectionStartPosX;
                }
                else
                {
                    midSectionStartPos.x = rightSectionStartPos.x - midSectionFinalWidth - s_minimumDraggableGap;
                }

                // Add draggable regions
                Float2 const leftToMidRegionStart( leftSectionFinalWidth, 0.0f );
                Float2 const leftToMidRegionSize( midSectionStartPos.x - leftSectionFinalWidth, titleBarHeight );
                m_draggableRegions.emplace_back( Math::ScreenSpaceRectangle( leftToMidRegionStart, leftToMidRegionSize ) );

                Float2 const midToRightRegionStart( midSectionStartPos.x + midSectionFinalWidth, 0.0f );
                Float2 const midToRightRegionSize( rightSectionStartPos.x - midToRightRegionStart.m_x, titleBarHeight );
                m_draggableRegions.emplace_back( Math::ScreenSpaceRectangle( midToRightRegionStart, midToRightRegionSize ) );
            }
            // Right section is cut-off
            else if ( rightSectionFinalWidth > 0 && ( rightSectionFinalWidth != rightSectionDesiredWidth ) )
            {
                Float2 const grabRegionStart( windowControlsStartPos.x - rightSectionFinalWidth - s_minimumDraggableGap, 0.0f );
                Float2 const grabRegionSize( s_minimumDraggableGap, titleBarHeight );
                m_draggableRegions.emplace_back( Math::ScreenSpaceRectangle( grabRegionStart, grabRegionSize ) );

                rightSectionStartPos.x = windowControlsStartPos.x - rightSectionFinalWidth;
            }
            // Mid section is cut-off 
            else if ( midSectionFinalWidth > 0 && ( midSectionFinalWidth != midSectionDesiredWidth ) )
            {
                EE_ASSERT( rightSectionFinalWidth == 0.0f );

                Float2 const grabRegionStart( windowControlsStartPos.x - rightSectionFinalWidth - s_minimumDraggableGap, 0.0f );
                Float2 const grabRegionSize( s_minimumDraggableGap, titleBarHeight );
                m_draggableRegions.emplace_back( Math::ScreenSpaceRectangle( grabRegionStart, grabRegionSize ) );

                midSectionStartPos.x = leftSectionFinalWidth;
            }
            // Left section is cut off
            else
            {
                Float2 const grabRegionStart( windowControlsStartPos.x - s_minimumDraggableGap, 0.0f );
                Float2 const grabRegionSize( s_minimumDraggableGap, titleBarHeight );
                m_draggableRegions.emplace_back( Math::ScreenSpaceRectangle( grabRegionStart, grabRegionSize ) );
            }

            // Draw sections
            //-------------------------------------------------------------------------

            /*static */ImVec2 const titleBarSectionPadding( 0, 0 );

            if ( leftSectionFinalWidth > 0 )
            {
                //ImGui::PushStyleColor( ImGuiCol_ChildBg, 0xFFFF0000 ); // Debug Color
                ImGui::SetCursorPos( leftSectionStartPos );
                ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, titleBarSectionPadding );
                if ( ImGui::BeginChild( "Left", ImVec2( leftSectionFinalWidth, titleBarHeight ), false, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysUseWindowPadding ) )
                {
                    ImGui::PopStyleVar();
                    leftSectionDrawFunction();
                }
                ImGui::EndChild();
                //ImGui::PopStyleColor();
            }

            if ( midSectionFinalWidth > 0 )
            {
                //ImGui::PushStyleColor( ImGuiCol_ChildBg, 0xFF00FF00 ); // Debug Color
                ImGui::SetCursorPos( midSectionStartPos );
                ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, titleBarSectionPadding );
                if ( ImGui::BeginChild( "Middle", ImVec2( midSectionFinalWidth, titleBarHeight ), false, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysUseWindowPadding ) )
                {
                    ImGui::PopStyleVar();
                    midSectionDrawFunction();
                }
                ImGui::EndChild();
                //ImGui::PopStyleColor();
            }

            if ( rightSectionFinalWidth > 0 )
            {
                //ImGui::PushStyleColor( ImGuiCol_ChildBg, 0xFF0000FF ); // Debug Color
                ImGui::SetCursorPos( rightSectionStartPos );
                ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, titleBarSectionPadding );
                if ( ImGui::BeginChild( "Right", ImVec2( rightSectionFinalWidth, titleBarHeight ), false, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysUseWindowPadding ) )
                {
                    ImGui::PopStyleVar();
                    rightSectionDrawFunction();
                }
                ImGui::EndChild();
                //ImGui::PopStyleColor();
            }

            // Draw window controls
            //-------------------------------------------------------------------------

            ImGui::SetCursorPos( windowControlsStartPos );
            if ( ImGui::BeginChild( "WindowControls", ImVec2( windowControlsWidth, titleBarHeight ), false, ImGuiWindowFlags_NoDecoration ) )
            {
                DrawWindowControls();
            }
            ImGui::EndChild();

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

        if ( ImGuiX::FlatButton( EE_ICON_WINDOW_MINIMIZE"##Min", s_windowControlButtonSize ) )
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
            if ( ImGuiX::FlatButton( EE_ICON_WINDOW_RESTORE"##Res", s_windowControlButtonSize ) )
            {
                if ( hwnd )
                {
                    ShowWindow( hwnd, SW_RESTORE );
                }
            }
        }
        else
        {
            if ( ImGuiX::FlatButton( EE_ICON_WINDOW_MAXIMIZE"##Max", s_windowControlButtonSize ) )
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
        if ( ImGuiX::FlatButton( EE_ICON_WINDOW_CLOSE"##X", s_windowControlButtonSize) )
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