#pragma once

#include "Engine/DebugViews/DebugView.h"

//-------------------------------------------------------------------------

namespace EE { class PlayerManager; }

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Player
{
    class PlayerController;

    //-------------------------------------------------------------------------

    class PlayerDebugView : public DebugView
    {
        EE_REFLECT_TYPE( PlayerDebugView );

    public:

        PlayerDebugView() : DebugView( "Game/Player" ) {}

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;
        virtual void DrawOverlayElements( EntityWorldUpdateContext const& context ) override;
        virtual void Update( EntityWorldUpdateContext const& context ) override;

        void DrawActionDebugger( EntityWorldUpdateContext const& context, bool isFocused );
        void DrawCharacterControllerState( EntityWorldUpdateContext const& context, bool isFocused );

        // HACK since we dont have a UI system yet
        void HACK_DrawPlayerHUD( EntityWorldUpdateContext const& context, PlayerController* pController );

    private:

        PlayerManager*                              m_pPlayerManager = nullptr;
        PlayerController*                           m_pPlayerController = nullptr;
    };
}
#endif