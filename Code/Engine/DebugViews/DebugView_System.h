#pragma once

#include "Engine/_Module/API.h"
#include "DebugView.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Logging/LogEntry.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class UpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SystemLogView
    {
        struct LogEntryEx : public Log::Entry
        {
            LogEntryEx() = default;
            explicit LogEntryEx( Log::Entry const& entry );

        public:

            String                                          m_truncatedMessage;
        };

    public:

        void Draw( UpdateContext const& context );

    private:

        void UpdateFilteredList( UpdateContext const& context );

    public:

        bool                                                m_showLogMessages = true;
        bool                                                m_showLogWarnings = true;
        bool                                                m_showLogErrors = true;
        bool                                                m_showDetails = false;

    private:

        ImGuiX::FilterWidget                                m_filterWidget;
        TVector<LogEntryEx>                                 m_filteredEntries;
        size_t                                              m_numLogEntriesWhenFiltered = 0;
        UUID                                                m_selectedEntryID;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SystemDebugView final : public DebugView
    {
        EE_REFLECT_TYPE( SystemDebugView );

        friend class EngineDebugUI;

    public:

        static void DrawFrameLimiterCombo( UpdateContext& context );

    public:

        SystemDebugView() : DebugView( "System" ) {}

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;

        void DrawMenu( EntityWorldUpdateContext const& context ) override;

        void DrawLogWindow( EntityWorldUpdateContext const& context, bool isFocused );

    private:

        SystemLogView m_logView;
    };
}
#endif