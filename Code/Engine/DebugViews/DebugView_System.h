#pragma once

#include "DebugView.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Logging/LoggingSystem.h"
#include "Engine/_Module/API.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class UpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SystemLogView
    {
    public:

        void Draw( UpdateContext const& context );

    private:

        void UpdateFilteredList( UpdateContext const& context );

    public:

        bool                                                m_showLogMessages = true;
        bool                                                m_showLogWarnings = true;
        bool                                                m_showLogErrors = true;

    private:

        ImGuiX::FilterWidget                                m_filterWidget;
        TVector<Log::LogEntry>                              m_filteredEntries;
        size_t                                              m_numLogEntriesWhenFiltered = 0;
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