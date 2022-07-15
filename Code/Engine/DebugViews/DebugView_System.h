#pragma once

#include "Engine/Entity/EntityWorldDebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class UpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SystemDebugView final : public EntityWorldDebugView
    {
        EE_REGISTER_TYPE( SystemDebugView );

    public:

        static void DrawFrameLimiterMenu( UpdateContext& context );

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

        SystemLogView();
        bool Draw( UpdateContext const& context );

    public:

        bool                                                m_showLogMessages = true;
        bool                                                m_showLogWarnings = true;
        bool                                                m_showLogErrors = true;

    private:

        InlineString                                        m_logFilter = "TODO";
    };
}
#endif