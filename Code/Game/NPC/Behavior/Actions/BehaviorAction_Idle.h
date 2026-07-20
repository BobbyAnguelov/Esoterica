#pragma once
#include "Game/NPC/Behavior/BehaviorAction.h"
#include "Base/Time/Timers.h"
#include "Base/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE
{
    class BehaviorAction_Idle : public BehaviorAction
    {
    public:

        void Start( BehaviorContext const& ctx );
        virtual Status Update( BehaviorContext const& ctx ) override;

    private:

        void ResetIdleBreakerTimer();

    private:

        ManualCountdownTimer        m_idleBreakerCooldown;
    };
}