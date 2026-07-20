#pragma once
#include "Game/NPC/Behavior/Behavior.h"
#include "Game/NPC/Behavior/Actions/BehaviorAction_Death.h"

//-------------------------------------------------------------------------

namespace EE
{
    class DeathBehavior : public Behavior
    {
    public:

        EE_BEHAVIOR_ID( DeathBehavior );

    private:

        virtual void StartInternal( BehaviorContext const& ctx ) override;
        virtual Status UpdateInternal( BehaviorContext const& ctx ) override;
        virtual void StopInternal( BehaviorContext const& ctx, StopReason reason ) override;

    private:

        BehaviorAction_Death       m_deathAction;
    };
}