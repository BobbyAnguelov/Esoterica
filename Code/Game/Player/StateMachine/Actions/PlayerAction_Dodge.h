#pragma once

#include "Game/Player/StateMachine/PlayerAction.h"
#include "Base/Time/Timers.h"
#include "Base/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE
{
    class PlayerAction_Dodge final : public PlayerAction
    {
    public:

        EE_PLAYER_ACTION_ID( PlayerAction_Dodge );

        virtual bool TryStartInternal( PlayerActionContext const& ctx ) override;
        virtual Status UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( PlayerActionContext const& ctx, StopReason reason ) override;

    private:

        ManualCountdownTimer        m_cooldownTimer;
        ManualTimer                 m_dashDurationTimer;
        ManualTimer                 m_hackSettleTimer;
        Vector                      m_dashDirection = Vector::Zero;
        Vector                      m_initialVelocity = Vector::Zero;
        bool                        m_isInSettle = false;

        #if EE_DEVELOPMENT_TOOLS
        Vector                      m_debugStartPosition = Vector::Zero;
        #endif
    };
}