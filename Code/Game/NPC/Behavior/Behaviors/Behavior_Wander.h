#pragma once
#include "Game/NPC/Behavior/Behavior.h"
#include "Game/NPC/Behavior/Actions/BehaviorAction_MoveTo.h"
#include "Game/NPC/Behavior/Actions/BehaviorAction_Idle.h"

//-------------------------------------------------------------------------

namespace EE
{
    class WanderBehavior : public Behavior
    {
    public:

        EE_BEHAVIOR_ID( WanderBehavior );

    private:

        virtual void StartInternal( BehaviorContext const& ctx ) override;
        virtual Status UpdateInternal( BehaviorContext const& ctx ) override;
        virtual void StopInternal( BehaviorContext const& ctx, StopReason reason ) override;

    private:

        BehaviorAction_MoveTo       m_moveToAction;
        BehaviorAction_Idle         m_idleAction;
        ManualCountdownTimer        m_waitTimer;
    };
}