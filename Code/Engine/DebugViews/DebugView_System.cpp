#include "DebugView_System.h"
#include "System/Imgui/ImguiX.h"
#include "System/Profiling.h"
#include "Engine/UpdateContext.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    void SystemDebugView::DrawFrameLimiterMenu( UpdateContext& context )
    {
        if ( ImGui::BeginMenu( EE_ICON_CAR_SPEED_LIMITER ) )
        {
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

            ImGui::EndMenu();
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

    SystemLogView::SystemLogView()
    {
        m_logFilter.resize( 255 );
    }

    bool SystemLogView::Draw( UpdateContext const& context )
    {
        bool isLogWindowOpen = true;

        if ( ImGui::Begin( "System Log", &isLogWindowOpen ) )
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Filter:" );
            ImGui::SameLine();
            ImGui::BeginDisabled();
            if ( ImGui::InputText( "##Filter", m_logFilter.data(), 255 ) )
            {
                // TODO
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            ImGui::SameLine();
            ImGui::Checkbox( "Messages", &m_showLogMessages );
            ImGui::SameLine();
            ImGui::Checkbox( "Warnings", &m_showLogWarnings );
            ImGui::SameLine();
            ImGui::Checkbox( "Errors", &m_showLogErrors );

            //-------------------------------------------------------------------------

            if ( ImGui::BeginTable( "System Log Table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImGui::GetContentRegionAvail() ) )
            {
                ImGui::TableSetupColumn( "Time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 55 );
                ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 55 );
                ImGui::TableSetupColumn( "Channel", ImGuiTableColumnFlags_WidthFixed, 60 );
                ImGui::TableSetupColumn( "Message", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupScrollFreeze( 0, 1 );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                //-------------------------------------------------------------------------

                auto const& logEntries = Log::GetLogEntries();

                ImGuiListClipper clipper;
                clipper.Begin( (int32_t) logEntries.size() );
                while ( clipper.Step() )
                {
                    for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                    {
                        auto const& entry = logEntries[i];

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
                        }

                        //-------------------------------------------------------------------------

                        ImGui::TableNextRow();

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 0 );
                        ImGui::Text( entry.m_timestamp.c_str() );

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 1 );
                        switch ( entry.m_severity )
                        {
                            case Log::Severity::Warning:
                            ImGui::TextColored( Colors::Yellow.ToFloat4(), "Warning" );
                            break;

                            case Log::Severity::Error:
                            ImGui::TextColored( Colors::Red.ToFloat4(), "Error" );
                            break;

                            case Log::Severity::Message:
                            ImGui::Text( "Message" );
                            break;
                        }

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 2 );
                        ImGui::Text( entry.m_channel.c_str() );

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 3 );
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
}
#endif