#include "SystemLogWidget.h"
#include "Engine/UpdateContext.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    namespace
    {
        enum LogCategoryState : uint64_t
        {
            SomeChildrenChecked = -1,
            Unchecked = 0,
            Checked = 1,
        };

        //-------------------------------------------------------------------------

        static void UpdateCategoryCheckedStateRecursive( Category<SystemLog::Entry>& category )
        {
            if ( category.m_childCategories.size() <= 1 )
            {
                return;
            }

            //-------------------------------------------------------------------------

            int64_t const numChildCategories = (int64_t) category.m_childCategories.size();
            int64_t numChecked = 0;
            int64_t numUnchecked = 0;

            for ( auto& childCategory : category.m_childCategories )
            {
                UpdateCategoryCheckedStateRecursive( childCategory );
                bool const isChecked = childCategory.m_userData == LogCategoryState::Checked;
                numUnchecked += ( !isChecked ) ? 1 : 0;
                numChecked += isChecked ? 1 : 0;
            }

            if ( numChecked == numChildCategories )
            {
                category.m_userData = LogCategoryState::Checked;
            }
            else if ( numUnchecked == numChildCategories )
            {
                category.m_userData = LogCategoryState::Unchecked;
            }
            else
            {
                category.m_userData = LogCategoryState::SomeChildrenChecked;
            }
        }
    }

    SystemLogWidget::SystemLogWidget()
    {
        m_categoryFilterTree.GetRootCategory().m_userData = LogCategoryState::Checked;
    }

    void SystemLogWidget::UpdateAndDraw( UpdateContext const& context )
    {
        EE_PROFILE_FUNCTION_DEVTOOLS();

        // Reflect
        //-------------------------------------------------------------------------

        bool updateFilteredList = false;

        int32_t const numLogEntries = SystemLog::GetNumEntries();
        if ( numLogEntries > m_lastReflectedEntryIdx )
        {
            int32_t const numEntriesToReflect = numLogEntries - m_lastReflectedEntryIdx;
            SystemLog::GetLogEntries( m_lastReflectedEntryIdx, numEntriesToReflect, m_tempBuffer );
            for ( auto pEntry : m_tempBuffer )
            {
                AddToCategoryFilterTree( *pEntry );
            }

            if ( m_lastReflectedEntryIdx == 0 )
            {
                m_categoryFilterTree.GetRootCategory().SetUserDataRecursive( LogCategoryState::Checked );
            }
            else
            {
                UpdateCategoryTreeCheckedState();
            }

            m_lastReflectedEntryIdx = numLogEntries;
            updateFilteredList = true;
        }

        // Draw
        //-------------------------------------------------------------------------

        updateFilteredList |= DrawToolbar( context );

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( ImGui::GetStyle().CellPadding.x, 0 ) );
        bool const drawSplitterTable = ImGui::BeginTable( "Splitter", 2, ImGuiTableFlags_Resizable, ImGui::GetContentRegionAvail() );

        if ( drawSplitterTable )
        {
            ImGui::TableSetupColumn( "Log", ImGuiTableColumnFlags_WidthStretch, 0.9f );
            ImGui::TableSetupColumn( "Category", ImGuiTableColumnFlags_WidthStretch, 0.1f );
            ImGui::TableNextRow();

            //-------------------------------------------------------------------------

            ImGui::TableSetColumnIndex( 1 );
            updateFilteredList |= DrawCategoryFilterTree( context );
            if ( updateFilteredList )
            {
                UpdateFilteredList( context );
            }

            //-------------------------------------------------------------------------

            ImGui::TableSetColumnIndex( 0 );
            DrawLogTable( context );

            ImGui::EndTable();
        }
        ImGui::PopStyleVar( 1 );
    }

    void SystemLogWidget::AddToCategoryFilterTree( SystemLog::Entry const& entry )
    {
        String categoryStr = entry.m_category.c_str();
        for ( StringID ID : entry.m_sourceInfo )
        {
            categoryStr.append( "/" );
            categoryStr.append( ID.c_str() );
        }

        auto pCategory = m_categoryFilterTree.FindCategory( categoryStr );
        bool categoryAdded = false;
        if ( pCategory == nullptr )
        {
            pCategory = m_categoryFilterTree.AddCategory( categoryStr );
            categoryAdded = true;
        }

        // Inherit parent category checked state on creation
        if ( categoryAdded )
        {
            auto pParentCategory = m_categoryFilterTree.GetParentCategory( *pCategory );
            if ( pParentCategory->m_userData == LogCategoryState::Checked )
            {
                pCategory->m_userData = LogCategoryState::Checked;
            }
        }

        // Add entry
        pCategory->AddItem( CategoryItem<SystemLog::Entry>( entry.m_ID.ToString().c_str(), entry ) );
    }

    bool SystemLogWidget::DrawCategoryFilterTree( UpdateContext const& context )
    {
        bool updateFilteredList = false;

        auto& rootCategory = m_categoryFilterTree.GetRootCategory();
        for ( auto& childCategory : rootCategory.m_childCategories )
        {
            DrawLogCategory( childCategory, 0, updateFilteredList );
        }

        if ( updateFilteredList )
        {
            UpdateCategoryTreeCheckedState();
        }

        return updateFilteredList;
    }

    void SystemLogWidget::DrawLogCategory( Category<SystemLog::Entry>& category, int32_t indentLevel, bool& updateFilteredList )
    {
        ImGui::Indent();

        int32_t value = (int32_t) category.m_userData;
        if ( ImGuiX::CheckBoxTristate( category.m_name.c_str(), &value ) )
        {
            if ( ImGui::IsKeyDown( ImGuiMod_Ctrl ) )
            {
                m_categoryFilterTree.GetRootCategory().SetUserDataRecursive( 0 );
                category.m_userData = LogCategoryState::Checked;
            }
            else
            {
                category.SetUserDataRecursive( value );
            }

            updateFilteredList = true;
        }

        for ( auto& childCategory : category.m_childCategories )
        {
            DrawLogCategory( childCategory, indentLevel + 1, updateFilteredList );
        }

        ImGui::Unindent();
    }

    void SystemLogWidget::UpdateCategoryTreeCheckedState()
    {
        auto& rootCategory = m_categoryFilterTree.GetRootCategory();
        for ( auto& childCategory : rootCategory.m_childCategories )
        {
            UpdateCategoryCheckedStateRecursive( childCategory );
        }
    }

    void SystemLogWidget::ReflectFilteredEntriesFromCategory( Category<SystemLog::Entry> const& category, UUID const& previouslySelectedEntryID )
    {
        if ( category.m_userData == LogCategoryState::Unchecked )
        {
            return;
        }

        // Reflect items
        //-------------------------------------------------------------------------

        if ( category.m_userData == LogCategoryState::Checked )
        {
            for ( auto const& entryItem : category.m_items )
            {
                auto const& entry = entryItem.m_data;

                String categoryStr;
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

                bool const entryMatchesFilter = m_filterWidget.MatchesFilter( entry.m_category.c_str() ) || m_filterWidget.MatchesFilter( entry.m_message ) || m_filterWidget.MatchesFilter( entry.m_sourceInfoStr );
                if ( entryMatchesFilter )
                {
                    m_filteredEntries.emplace_back( entry );

                    // Restore selection
                    if ( entry.m_ID == previouslySelectedEntryID )
                    {
                        m_selectedEntryID = entry.m_ID;
                    }
                }
            }
        }

        // Reflect child categories
        //-------------------------------------------------------------------------

        for ( auto const& childCategory : category.m_childCategories )
        {
            ReflectFilteredEntriesFromCategory( childCategory, previouslySelectedEntryID );
        }
    }

    void SystemLogWidget::UpdateFilteredList( UpdateContext const& context )
    {
        UUID const previouslySelectedEntryID = m_selectedEntryID;
        m_selectedEntryID.Clear();

        //-------------------------------------------------------------------------

        m_filteredEntries.clear();
        ReflectFilteredEntriesFromCategory( m_categoryFilterTree.GetRootCategory(), previouslySelectedEntryID );

        // Sort
        //-------------------------------------------------------------------------

        switch ( m_sortRule )
        {
            case SortRule::TimeAscending:
            {
                auto SortPredicate = [] ( SystemLog::Entry const& lhs, SystemLog::Entry const& rhs )
                {
                    return lhs.m_timestamp < rhs.m_timestamp;
                };

                eastl::sort( m_filteredEntries.begin(), m_filteredEntries.end(), SortPredicate );
            }
            break;

            case SortRule::TimeDescending:
            {
                auto SortPredicate = [] ( SystemLog::Entry const& lhs, SystemLog::Entry const& rhs )
                {
                    return lhs.m_timestamp > rhs.m_timestamp;
                };

                eastl::sort( m_filteredEntries.begin(), m_filteredEntries.end(), SortPredicate );
            }
            break;

            case SortRule::SeverityAscending:
            {
                auto SortPredicate = [] ( SystemLog::Entry const& lhs, SystemLog::Entry const& rhs )
                {
                    return lhs.m_severity < rhs.m_severity;
                };

                eastl::sort( m_filteredEntries.begin(), m_filteredEntries.end(), SortPredicate );
            }
            break;

            case SortRule::SeverityDescending:
            {
                auto SortPredicate = [] ( SystemLog::Entry const& lhs, SystemLog::Entry const& rhs )
                {
                    return lhs.m_severity > rhs.m_severity;
                };

                eastl::sort( m_filteredEntries.begin(), m_filteredEntries.end(), SortPredicate );
            }
            break;

            case SortRule::ChannelAscending:
            {
                auto SortPredicate = [] ( SystemLog::Entry const& lhs, SystemLog::Entry const& rhs )
                {
                    return lhs.m_category < rhs.m_category;
                };

                eastl::sort( m_filteredEntries.begin(), m_filteredEntries.end(), SortPredicate );
            }
            break;

            case SortRule::ChannelDescending:
            {
                auto SortPredicate = [] ( SystemLog::Entry const& lhs, SystemLog::Entry const& rhs )
                {
                    return lhs.m_category > rhs.m_category;
                };

                eastl::sort( m_filteredEntries.begin(), m_filteredEntries.end(), SortPredicate );
            }
            break;
        }
    }

    bool SystemLogWidget::DrawToolbar( UpdateContext const& context )
    {
        bool shouldUpdateFilteredList = false;

        int32_t const numErrors = SystemLog::GetNumErrors();
        int32_t const numWarnings = SystemLog::GetNumWarnings();
        int32_t const numMessages = SystemLog::GetNumMessages();
        int32_t const maxValue = Math::Max( numErrors, Math::Max( numWarnings, numMessages ) );

        InlineString str;
        str.sprintf( EE_ICON_CLOSE_CIRCLE" %d", maxValue );

        float const sidebarButtonWidth = ImGuiX::CalculateButtonWidth( str.c_str() );
        ImVec2 const sidebarButtonSize( sidebarButtonWidth, 0 );
        float const filterTextWidth = ImGui::GetContentRegionAvail().x - ( 3 * sidebarButtonWidth ) - ( 4 * ImGui::GetStyle().ItemSpacing.x );

        //-------------------------------------------------------------------------

        if ( m_filterWidget.UpdateAndDraw( filterTextWidth ) )
        {
            shouldUpdateFilteredList = true;
        }

        ImGui::SameLine();
        str.sprintf( EE_ICON_CLOSE_CIRCLE" %d##ShowErrors", numErrors );

        if ( ImGuiX::ToggleButton( str.c_str(), str.c_str(), m_showLogErrors, sidebarButtonSize, ImGuiX::GetSeverityColor( Severity::Error ), ImGuiX::Style::s_colorTextDisabled ) )
        {
            shouldUpdateFilteredList = true;
        }
        ImGuiX::ItemTooltip( "Errors" );

        ImGui::SameLine();
        str.sprintf( EE_ICON_ALERT" %d##ShowWarnings", numWarnings );

        if ( ImGuiX::ToggleButton( str.c_str(), str.c_str(), m_showLogWarnings, sidebarButtonSize, ImGuiX::GetSeverityColor( Severity::Warning ), ImGuiX::Style::s_colorTextDisabled ) )
        {
            shouldUpdateFilteredList = true;
        }
        ImGuiX::ItemTooltip( "Warnings" );

        ImGui::SameLine();
        str.sprintf( EE_ICON_INFORMATION" %d##ShowMessages", numMessages );

        if ( ImGuiX::ToggleButton( str.c_str(), str.c_str(), m_showLogMessages, sidebarButtonSize, ImGuiX::GetSeverityColor( Severity::Info ), ImGuiX::Style::s_colorTextDisabled ) )
        {
            shouldUpdateFilteredList = true;
        }
        ImGuiX::ItemTooltip( "Messages" );

        return true;
    }

    void SystemLogWidget::DrawLogTable( UpdateContext const& context )
    {
        int32_t selectedEntryIdx = InvalidIndex;
        ImVec2 tableSize = ImGui::GetContentRegionAvail();

        ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 0, 2 ) );
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 0 ) );
        if ( ImGui::BeginTable( "System Log Table", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable, tableSize ) )
        {
            ImGui::TableSetupColumn( "Time", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 60 );
            ImGui::TableSetupColumn( "Source", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 200 );
            ImGui::TableSetupColumn( "Message", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupScrollFreeze( 0, 1 );

            //-------------------------------------------------------------------------

            ImGui::TableHeadersRow();

            // Sort table contents
            //-------------------------------------------------------------------------

            if ( ImGuiTableSortSpecs* pSortSpecs = ImGui::TableGetSortSpecs() )
            {
                if ( pSortSpecs->SpecsDirty )
                {
                    bool const isAscending = pSortSpecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
                    if ( pSortSpecs->Specs[0].ColumnIndex == 0 )
                    {
                        m_sortRule = isAscending ? SortRule::SeverityAscending : SortRule::SeverityDescending;
                    }
                    else if ( pSortSpecs->Specs[0].ColumnIndex == 1 )
                    {
                        m_sortRule = isAscending ? SortRule::TimeAscending : SortRule::TimeDescending;
                    }
                    else if ( pSortSpecs->Specs[0].ColumnIndex == 2 )
                    {
                        m_sortRule = isAscending ? SortRule::ChannelAscending : SortRule::ChannelDescending;
                    }

                    UpdateFilteredList( context );
                    pSortSpecs->SpecsDirty = false;
                }
            }

            // Draw log entries
            //-------------------------------------------------------------------------

            InlineString buffer;

            ImGuiListClipper clipper;
            clipper.Flags |= ImGuiListClipperFlags_NoSetTableRowCounters;
            clipper.Begin( (int32_t) m_filteredEntries.size() );
            while ( clipper.Step() )
            {
                for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                {
                    auto const& entry = m_filteredEntries[i];

                    ImGui::PushID( i );
                    ImGui::TableNextRow();

                    //-------------------------------------------------------------------------

                    bool isSelected = ( m_selectedEntryID == entry.m_ID );

                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::AlignTextToFramePadding();
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

                    ImGui::TableSetColumnIndex( 1 );
                    ImGui::AlignTextToFramePadding();
                    {
                        if ( entry.m_sourceInfo.empty() )
                        {
                            ImGui::TextColored( ImGuiX::GetSeverityColor( entry.m_severity ), entry.m_category.c_str() );
                            ImGuiX::ItemTooltip( entry.m_category.c_str() );
                        }
                        else
                        {
                            buffer.clear();
                            buffer.append_sprintf( "%s/%s", entry.m_category.c_str(), entry.m_sourceInfoStr.c_str() );
                            ImGui::TextColored( ImGuiX::GetSeverityColor( entry.m_severity ), buffer.c_str() );
                            ImGuiX::ItemTooltip( buffer.c_str() );
                        }
                    }

                    //-------------------------------------------------------------------------

                    ImGui::TableSetColumnIndex( 2 );
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( entry.m_message.c_str() );
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
        ImGui::PopStyleVar( 2 );
    }

    //-------------------------------------------------------------------------

    SystemLogSummaryWidget::SystemLogSummaryWidget()
    {
        m_warnings.sprintf( EE_ICON_ALERT" %d ", SystemLog::GetNumWarnings() );
        m_errors.sprintf( EE_ICON_CLOSE_CIRCLE" %d ", SystemLog::GetNumErrors() );
        m_messages.sprintf( EE_ICON_INFORMATION_SLAB_CIRCLE" %d", SystemLog::GetNumMessages() );

        m_combined.sprintf( "%s%s%s", m_warnings.c_str(), m_errors.c_str(), m_messages.c_str() );

        m_size = ImVec2( ImGuiX::CalculateButtonWidth( m_combined.c_str() ), ImGui::GetFrameHeight() );
    }

    bool SystemLogSummaryWidget::Draw() const
    {
        ImVec2 const startPos = ImGui::GetCursorPos();
        float const yOffset = ( ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeight() ) / 2;

        // Button
        bool const clicked = ImGuiX::FlatButton( "##Summary", m_size );
        ImGuiX::ItemTooltip( "Open System Log" );
        ImVec2 const endPos = ImGui::GetCursorPos();

        // Foreground
        ImGui::SetCursorPos( startPos + ImVec2( ImGui::GetStyle().FramePadding.x, yOffset ) );
        ImGui::TextColored( Colors::Gold, m_warnings.c_str() );
        ImGui::SameLine( 0, 0 );
        ImGui::TextColored( Colors::Red, m_errors.c_str() );
        ImGui::SameLine( 0, 0 );
        ImGui::TextColored( Colors::CornflowerBlue, m_messages.c_str() );

        // Dummy
        ImGui::SetCursorPos( startPos );
        ImGui::Dummy( m_size );

        return clicked;
    }
}
#endif