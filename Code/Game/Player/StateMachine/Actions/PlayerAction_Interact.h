#pragma once

#include "Game/Player/StateMachine/PlayerAction.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------
 
namespace EE
{
    class ExternalController;

    class PlayerAction_Interact final : public PlayerAction
    {
    public:

        EE_PLAYER_ACTION_ID( PlayerAction_Interact );

        virtual bool TryStartInternal( PlayerActionContext const& ctx ) override;
        virtual Status UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( PlayerActionContext const& ctx, StopReason reason ) override;

        inline bool CanInteract() const { return m_canInteract; }

    private:

        bool                    m_canInteract = false;
        ExternalController*     m_pController = nullptr;
    };
}