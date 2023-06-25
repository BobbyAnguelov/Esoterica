#pragma once

#include "Engine/Entity/EntityWorldDebugView.h"

//-------------------------------------------------------------------------

namespace EE { class PlayerManager; }

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Player
{
    class PlayerController;

    //-------------------------------------------------------------------------

    class PlayerDebugView : public EntityWorldDebugView
    {
        EE_REFLECT_TYPE( PlayerDebugView );

    public:

        PlayerDebugView();

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass ) override;

        void DrawMenu( EntityWorldUpdateContext const& context );

        virtual void DrawOverlayElements( EntityWorldUpdateContext const& context ) override;

        // HACK since we dont have a UI system yet
        void HACK_DrawPlayerHUD( EntityWorldUpdateContext const& context, PlayerController* pController );

    private:

        EntityWorld const*                          m_pWorld = nullptr;
        PlayerManager*                              m_pPlayerManager = nullptr;
        bool                                        m_isActionDebugWindowOpen = false;
        bool                                        m_isCharacterControllerDebugWindowOpen = false;
    };
}
#endif