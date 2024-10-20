#pragma once

#include "Game/Player/StateMachine/PlayerAction.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------
 
namespace EE::Player
{
    class ExternalController;

    class InteractAction final : public Action
    {
    public:

        EE_PLAYER_ACTION_ID( InteractAction );

        virtual bool TryStartInternal( ActionContext const& ctx ) override;
        virtual Status UpdateInternal( ActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( ActionContext const& ctx, StopReason reason ) override;

        inline bool CanInteract() const { return m_canInteract; }

    private:

        bool                    m_canInteract = false;
        ExternalController*     m_pController = nullptr;
    };
}