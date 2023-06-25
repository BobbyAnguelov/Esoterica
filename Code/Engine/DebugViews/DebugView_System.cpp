#include "DebugView_System.h"
#include "Engine/UpdateContext.h"
#include "System/Imgui/ImguiX.h"
#include "System/Profiling.h"
#include "System/Logging/LoggingSystem.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    void SystemDebugView::DrawFrameLimiterCombo( UpdateContext& context )
    {
        ImGui::PushStyleColor( ImGuiCol_FrameBg, 0x00000000 );
        ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImGui::GetColorU32( ImGuiCol_ButtonHovered ) );
        ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImGui::GetColorU32( ImGuiCol_ButtonActive ) );
        ImGui::SetNextItemWidth( 30 );
        if ( ImGui::BeginCombo( "##FLC", EE_ICON_CAR_SPEED_LIMITER, ImGuiComboFlags_NoArrowButton ) )
        {
            ImGui::PopStyleColor( 3 );

            bool noLimit = !context.HasFrameRateLimit();
            if ( ImGui::MenuItem( "None", nullptr, &noLimit ) )
            {
                context.SetFrameRateLimit( 0.0f );
            }

            bool is30FPS = context.HasFrameRateLimit() && context.GetFrameRateLimit() == 30.0f;
            if ( ImGui::MenuItem( "30 FPS", nullptr, &is30FPS ) )
            {
                context.SetFrameRateLimit( 30.0f );
            }

            bool is60FPS = context.HasFrameRateLimit() && context.GetFrameRateLimit() == 60.0f;
            if ( ImGui::MenuItem( "60 FPS", nullptr, &is60FPS ) )
            {
                context.SetFrameRateLimit( 60.0f );
            }

            bool is120FPS = context.HasFrameRateLimit() && context.GetFrameRateLimit() == 120.0f;
            if ( ImGui::MenuItem( "120 FPS", nullptr, &is120FPS ) )
            {
                context.SetFrameRateLimit( 120.0f );
            }

            bool is144FPS = context.HasFrameRateLimit() && context.GetFrameRateLimit() == 144.0f;
            if ( ImGui::MenuItem( "144 FPS", nullptr, &is144FPS ) )
            {
                context.SetFrameRateLimit( 144.0f );
            }

            ImGui::EndCombo();
        }
        else
        {
            ImGui::PopStyleColor( 3 );
        }
    }

    SystemDebugView::SystemDebugView()
    {
        m_menus.emplace_back( DebugMenu( "System", [this] ( EntityWorldUpdateContext const& context ) { DrawMenu( context ); } ) );
    }

    void SystemDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        if ( ImGui::MenuItem( "Open Profiler" ) )
        {
            Profiling::OpenProfiler();
        }
    }

    //-------------------------------------------------------------------------

    bool SystemLogView::Draw( UpdateContext const& context )
    {
        bool isLogWindowOpen = true;

        if ( ImGui::Begin( "Log", &isLogWindowOpen ) )
        {
            constexpr static float const filterButtonWidth = 30.0f;

            // Draw filter
            //-------------------------------------------------------------------------

            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Filter:" );
            ImGui::SameLine();
            float const filterWidth = ImGui::GetContentRegionAvail().x - filterButtonWidth - ImGui::GetStyle().ItemSpacing.x;
            if ( m_filterWidget.DrawAndUpdate( filterWidth ) )
            {
                UpdateFilteredList( context );
            }

            //-------------------------------------------------------------------------

            ImGui::SameLine();

            float const buttonStartPosX = ImGui::GetCursorPosX();
            ImGui::Button( EE_ICON_FILTER_COG"##FilterOptions", ImVec2( filterButtonWidth, 0 ) );
            float const buttonEndPosY = ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y;
            ImGui::SetNextWindowPos( ImGui::GetWindowPos() + ImVec2( buttonStartPosX, buttonEndPosY ) );
            if ( ImGui::BeginPopupContextItem( "##FilterOptions", ImGuiPopupFlags_MouseButtonLeft ) )
            {
                bool shouldUpdateFilteredList = false;
                shouldUpdateFilteredList |= ImGuiX::Checkbox( "Messages", &m_showLogMessages );
                shouldUpdateFilteredList |= ImGuiX::Checkbox( "Warnings", &m_showLogWarnings );
                shouldUpdateFilteredList |= ImGuiX::Checkbox( "Errors", &m_showLogErrors );

                if ( shouldUpdateFilteredList )
                {
                    UpdateFilteredList( context );
                }

                ImGui::EndPopup();
            }

            // Check if there are more entries than we know about, if so updated the filtered list
            //-------------------------------------------------------------------------

            if ( m_numLogEntriesWhenFiltered != Log::System::GetLogEntries().size() )
            {
                UpdateFilteredList( context );
            }

            // Draw Entries
            //-------------------------------------------------------------------------

            ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
            if ( ImGui::BeginTable( "System Log Table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImGui::GetContentRegionAvail() ) )
            {
                ImGui::TableSetupColumn( "##Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 14 );
                ImGui::TableSetupColumn( "Time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 36 );
                ImGui::TableSetupColumn( "Channel", ImGuiTableColumnFlags_WidthFixed, 60 );
                ImGui::TableSetupColumn( "Source", ImGuiTableColumnFlags_WidthFixed, 120 );
                ImGui::TableSetupColumn( "Message", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupScrollFreeze( 0, 1 );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                //-------------------------------------------------------------------------

                ImGuiListClipper clipper;
                clipper.Begin( (int32_t) m_filteredEntries.size() );
                while ( clipper.Step() )
                {
                    for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                    {
                        auto const& entry = m_filteredEntries[i];

                        ImGui::TableNextRow();

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 0 );
                        ImGui::AlignTextToFramePadding();
                        switch ( entry.m_severity )
                        {
                            case Log::Severity::Message:
                            ImGui::Text( EE_ICON_MESSAGE );
                            break;

                            case Log::Severity::Warning:
                            ImGui::TextColored( Colors::Yellow.ToFloat4(), EE_ICON_ALERT );
                            break;

                            case Log::Severity::Error:
                            ImGui::TextColored( Colors::Red.ToFloat4(), EE_ICON_ALERT_CIRCLE_OUTLINE );
                            break;

                            default:
                            break;
                        }
                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 1 );
                        ImGui::Text( entry.m_timestamp.c_str() );

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 2 );
                        ImGui::Text( entry.m_category.c_str() );

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 3 );
                        if ( !entry.m_sourceInfo.empty() )
                        {
                            ImGui::SetNextItemWidth( -1 );
                            ImGui::PushID( i );
                            ImGui::InputText( "##RO", const_cast<char*>( entry.m_sourceInfo.c_str() ), entry.m_sourceInfo.size(), ImGuiInputTextFlags_ReadOnly );
                            ImGuiX::ItemTooltip( entry.m_sourceInfo.c_str() );
                            ImGui::PopID();
                        }

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 4 );
                        ImGui::Text( entry.m_message.c_str() );
                    }
                }

                // Auto scroll the table
                if ( ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
                {
                    ImGui::SetScrollHereY( 1.0f );
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();

        //-------------------------------------------------------------------------

        return isLogWindowOpen;
    }

    void SystemLogView::UpdateFilteredList( UpdateContext const& context )
    {
        auto const& logEntries = Log::System::GetLogEntries();

        m_filteredEntries.clear();
        m_filteredEntries.reserve( logEntries.size() );

        for ( auto const& entry : logEntries )
        {
            switch ( entry.m_severity )
            {
                case Log::Severity::Warning:
                if ( !m_showLogWarnings )
                {
                    continue;
                }
                break;

                case Log::Severity::Error:
                if ( !m_showLogErrors )
                {
                    continue;
                }
                break;

                case Log::Severity::Message:
                if ( !m_showLogMessages )
                {
                    continue;
                }
                break;

                default:
                break;
            }

            //-------------------------------------------------------------------------

            if ( m_filterWidget.MatchesFilter( entry.m_category ) )
            {
                m_filteredEntries.emplace_back( entry );
                continue;
            }

            if ( m_filterWidget.MatchesFilter( entry.m_message ) )
            {
                m_filteredEntries.emplace_back( entry );
                continue;
            }

            if ( m_filterWidget.MatchesFilter( entry.m_sourceInfo ) )
            {
                m_filteredEntries.emplace_back( entry );
                continue;
            }
        }
    }
}
#endif