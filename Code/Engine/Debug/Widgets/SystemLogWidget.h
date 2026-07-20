#pragma once

#include "Engine/_Module/API.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Logging/SystemLog.h"
#include "Base/Utils/CategoryTree.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class UpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SystemLogWidget
    {
        enum class SortRule
        {
            TimeAscending,
            TimeDescending,
            SeverityAscending,
            SeverityDescending,
            ChannelAscending,
            ChannelDescending
        };

    public:

        SystemLogWidget();

        void UpdateAndDraw( UpdateContext const& context );

    private:

        void AddToCategoryFilterTree( SystemLog::Entry const& entry );
        bool DrawCategoryFilterTree( UpdateContext const& context );
        void DrawLogCategory( Category<SystemLog::Entry>& category, int32_t indentLevel, bool& updateFilteredList );
        void UpdateCategoryTreeCheckedState();

        void UpdateFilteredList( UpdateContext const& context );
        void ReflectFilteredEntriesFromCategory( Category<SystemLog::Entry> const& category, UUID const& previouslySelectedEntryID );

        bool DrawToolbar( UpdateContext const& context );
        void DrawLogTable( UpdateContext const& context );

    public:

        bool                                                m_showLogMessages = true;
        bool                                                m_showLogWarnings = true;
        bool                                                m_showLogErrors = true;

    private:

        ImGuiX::FilterWidget                                m_filterWidget;
        TVector<SystemLog::Entry const*>                    m_tempBuffer;
        TVector<SystemLog::Entry>                           m_filteredEntries;
        int32_t                                             m_lastReflectedEntryIdx = 0;
        size_t                                              m_numLogEntriesWhenFiltered = 0;
        UUID                                                m_selectedEntryID;
        SortRule                                            m_sortRule = SortRule::TimeAscending;
        CategoryTree<SystemLog::Entry>                      m_categoryFilterTree;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SystemLogSummaryWidget
    {

    public:

        SystemLogSummaryWidget();
        bool Draw() const;

    public:

        TInlineString<10>                                   m_errors;
        TInlineString<10>                                   m_warnings;
        TInlineString<10>                                   m_messages;
        TInlineString<10>                                   m_combined;
        ImVec2                                              m_size;
    };
}
#endif