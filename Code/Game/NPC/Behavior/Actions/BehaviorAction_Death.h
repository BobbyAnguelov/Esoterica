#pragma once
#include "Game/NPC/Behavior/BehaviorAction.h"

//-------------------------------------------------------------------------

namespace EE
{
    class BehaviorAction_Death : public BehaviorAction
    {
    public:

        virtual Status Update( BehaviorContext const& ctx ) override;
    };
}