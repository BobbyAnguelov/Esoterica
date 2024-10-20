
#include "DebugView_System.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Profiling.h"
#include "Base/Logging/SystemLog.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    SystemLogView::LogEntryEx::LogEntryEx( Log::Entry const& entry )
        : Log::Entry( entry )
    {
        size_t const newlineIdx = entry.m_message.find_first_of( '\n' );
        if ( newlineIdx != String::npos )
        {
            m_truncatedMessage = entry.m_message.substr( 0, newlineIdx );
            m_truncatedMessage.append( "..." );
        }
        else
        {
            m_truncatedMessage = entry.m_message;
        }
    }

    //-------------------------------------------------------------------------

    void SystemLogView::Draw( UpdateContext const& context )
    {
        constexpr static float const controlButtonWidth = 30.0f;
        constexpr static float const detailsBoxHeight = 100.0f;

        // Draw filter
        //-------------------------------------------------------------------------

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Filter:" );
        ImGui::SameLine();
        float const filterWidth = ImGui::GetContentRegionAvail().x - ( controlButtonWidth + ImGui::GetStyle().ItemSpacing.x ) * 2;
        if ( m_filterWidget.UpdateAndDraw( filterWidth ) )
        {
            UpdateFilteredList( context );
        }

        //-------------------------------------------------------------------------

        ImGui::SameLine();

        ImGui::SetNextItemWidth( controlButtonWidth );
        ImGui::PushStyleColor( ImGuiCol_FrameBg, ImGuiX::Style::s_colorGray3 );
        ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImGuiX::Style::s_colorGray2 );
        ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImGuiX::Style::s_colorGray1 );
        bool const drawCombo = ImGui::BeginCombo( "##FilterOptions", EE_ICON_FILTER_COG, ImGuiComboFlags_NoArrowButton );
        ImGui::PopStyleColor( 3 );

        if ( drawCombo )
        {
            bool shouldUpdateFilteredList = false;
            shouldUpdateFilteredList |= ImGuiX::Checkbox( "Messages", &m_showLogMessages );
            shouldUpdateFilteredList |= ImGuiX::Checkbox( "Warnings", &m_showLogWarnings );
            shouldUpdateFilteredList |= ImGuiX::Checkbox( "Errors", &m_showLogErrors );

            if ( shouldUpdateFilteredList )
            {
                UpdateFilteredList( context );
            }
            ImGui::EndCombo();
        }

        //-------------------------------------------------------------------------

        ImGui::SameLine();
        ImGuiX::ToggleButton( EE_ICON_TEXT_BOX_OUTLINE"##details", EE_ICON_TEXT_BOX"##details", m_showDetails, ImVec2( controlButtonWidth, 0 ) );
        ImGuiX::ItemTooltip( "Show Details Window" );

        // Check if there are more entries than we know about, if so updated the filtered list
        //-------------------------------------------------------------------------

        if ( m_numLogEntriesWhenFiltered != SystemLog::GetLogEntries().size() )
        {
            UpdateFilteredList( context );
        }

        // Draw Entries
        //-------------------------------------------------------------------------

        int32_t selectedEntryIdx = InvalidIndex;
        ImVec2 tableSize = ImGui::GetContentRegionAvail();
        if ( m_showDetails )
        {
            tableSize.y -= detailsBoxHeight + ImGui::GetStyle().ItemSpacing.y;
        }

        ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
        if ( ImGui::BeginTable( "System Log Table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, tableSize ) )
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

                    ImGui::PushID( i );
                    ImGui::TableNextRow();

                    //-------------------------------------------------------------------------

                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::AlignTextToFramePadding();
                    switch ( entry.m_severity )
                    {
                        case Severity::Info:
                        ImGui::Text( EE_ICON_INFORMATION );
                        break;

                        case Severity::Warning:
                        ImGui::TextColored( Colors::Yellow.ToFloat4(), EE_ICON_ALERT );
                        break;

                        case Severity::Error:
                        ImGui::TextColored( Colors::Red.ToFloat4(), EE_ICON_CLOSE_CIRCLE );
                        break;

                        default:
                        break;
                    }
                    //-------------------------------------------------------------------------

                    bool isSelected = ( m_selectedEntryID == entry.m_ID );

                    ImGui::TableSetColumnIndex( 1 );
                    if ( ImGui::Selectable( entry.m_timestamp.c_str(), &isSelected, ImGuiSelectableFlags_SpanAllColumns ) )
                    {
                        if ( isSelected )
                        {
                            m_selectedEntryID = entry.m_ID;
                        }
                    }

                    // Track selected entry idx - taking into account that we might have just changed it
                    if ( m_selectedEntryID == entry.m_ID )
                    {
                        selectedEntryIdx = i;
                    }

                    //-------------------------------------------------------------------------

                    ImGui::TableSetColumnIndex( 2 );
                    ImGui::Text( entry.m_category.c_str() );

                    //-------------------------------------------------------------------------

                    ImGui::TableSetColumnIndex( 3 );
                    if ( !entry.m_sourceInfo.empty() )
                    {
                        ImGui::Text( entry.m_sourceInfo.c_str() );
                        ImGuiX::ItemTooltip( entry.m_sourceInfo.c_str() );
                    }

                    //-------------------------------------------------------------------------

                    ImGui::TableSetColumnIndex( 4 );
                    ImGui::Text( entry.m_truncatedMessage.c_str() );
                    ImGuiX::ItemTooltip( entry.m_message.c_str() );

                    //-------------------------------------------------------------------------

                    ImGui::PopID();
                }
            }

            // Auto scroll the table
            if ( ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
            {
                ImGui::SetScrollHereY( 1.0f );
            }

            ImGui::EndTable();
        }

        // Show Details
        //-------------------------------------------------------------------------
        
        if ( m_showDetails )
        {
            char* pBuffer = nullptr;
            size_t length = 0;

            if ( selectedEntryIdx != InvalidIndex )
            {
                pBuffer = (char*) m_filteredEntries[selectedEntryIdx].m_message.c_str();
                length = m_filteredEntries[selectedEntryIdx].m_message.length();
            }
            else
            {
                constexpr static char const * const nothingSelectedText = "Nothing Selected";
                pBuffer = (char*) nothingSelectedText;
                length = 17;
            }

            ImGui::InputTextMultiline( "##detailsml", pBuffer, length, ImVec2( -1, -1 ) );
        }
    }

    void SystemLogView::UpdateFilteredList( UpdateContext const& context )
    {
        auto const& logEntries = SystemLog::GetLogEntries();

        UUID const previousSelection = m_selectedEntryID;
        m_selectedEntryID.Clear();

        //-------------------------------------------------------------------------

        m_filteredEntries.clear();
        m_filteredEntries.reserve( logEntries.size() );

        for ( auto const& entry : logEntries )
        {
            switch ( entry.m_severity )
            {
                case Severity::Warning:
                if ( !m_showLogWarnings )
                {
                    continue;
                }
                break;

                case Severity::Error:
                if ( !m_showLogErrors )
                {
                    continue;
                }
                break;

                case Severity::Info:
                if ( !m_showLogMessages )
                {
                    continue;
                }
                break;

                default:
                break;
            }

            //-------------------------------------------------------------------------

            bool const entryMatchesFilter = m_filterWidget.MatchesFilter( entry.m_category ) || m_filterWidget.MatchesFilter( entry.m_message ) || m_filterWidget.MatchesFilter( entry.m_sourceInfo );
            if ( entryMatchesFilter )
            {
                m_filteredEntries.emplace_back( entry );

                // Restore selection
                if ( entry.m_ID == previousSelection )
                {
                    m_selectedEntryID = entry.m_ID;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

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

            bool is15FPS = context.HasFrameRateLimit() && context.GetFrameRateLimit() == 15.0f;
            if ( ImGui::MenuItem( "15 FPS", nullptr, &is15FPS ) )
            {
                context.SetFrameRateLimit( 15.0f );
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

    //-------------------------------------------------------------------------

    void SystemDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_windows.emplace_back( "System Log", [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t ) { DrawLogWindow( context, isFocused ); } );
    }

    void SystemDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        if ( ImGui::MenuItem( "Open Profiler" ) )
        {
            Profiling::OpenProfiler();
        }
    }

    void SystemDebugView::DrawLogWindow( EntityWorldUpdateContext const& context, bool isFocused )
    {
        m_logView.Draw( context );
    }
}
#endif