#pragma once

#include "Engine/Entity/EntityWorldDebugView.h"
#include "System/Imgui/ImguiX.h"
#include "System/Logging/LoggingSystem.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class UpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SystemDebugView final : public EntityWorldDebugView
    {
        EE_REFLECT_TYPE( SystemDebugView );

    public:

        static void DrawFrameLimiterCombo( UpdateContext& context );

    public:

        SystemDebugView();

    private:

        virtual void DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass ) override {};
        void DrawMenu( EntityWorldUpdateContext const& context );
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SystemLogView
    {
    public:

        bool Draw( UpdateContext const& context );

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
}
#endif