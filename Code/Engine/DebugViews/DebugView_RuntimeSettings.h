#pragma once

#include "Engine/Entity/EntityWorldDebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class UpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API RuntimeSettingsDebugView final : public EntityWorldDebugView
    {
        EE_REGISTER_TYPE( RuntimeSettingsDebugView );

    public:

        static bool DrawRuntimeSettingsView( UpdateContext const& context );

    public:

        RuntimeSettingsDebugView();

    private:

        virtual void DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass ) override {};
        void DrawMenu( EntityWorldUpdateContext const& context );
    };
}
#endif