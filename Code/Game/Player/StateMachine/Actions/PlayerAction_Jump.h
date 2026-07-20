#pragma once

#include "Game/Player/StateMachine/PlayerAction.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------
 
namespace EE
{
    class PlayerAction_Jump final : public PlayerAction
    {
    public:

        EE_PLAYER_ACTION_ID( PlayerAction_Jump );

        virtual bool TryStartInternal( PlayerActionContext const& ctx ) override;
        virtual Status UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( PlayerActionContext const& ctx, StopReason reason ) override;

        inline bool IsChargedJumpReady() const { return m_isChargedJumpReady; }

    private:

        ManualTimer             m_jumpTimer;
        bool                    m_isChargedJumpReady = false;
        bool                    m_isPerformingChargedJump = false;
        float                   m_previousProgressThroughJump = 0.0;
    };
}