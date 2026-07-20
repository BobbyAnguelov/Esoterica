#pragma once
#include "BehaviorContext.h"

//-------------------------------------------------------------------------

namespace EE
{
    //-------------------------------------------------------------------------
    // An NPC Action
    //-------------------------------------------------------------------------
    // A specific actuation task (move, play anim, etc...) that a behavior requests to help achieve its goal

    class BehaviorAction
    {
    public:

        enum class Status : uint8_t
        {
            Running,
            Completed,
            Failed
        };

    public:

        virtual ~BehaviorAction() = default;
        virtual Status Update( BehaviorContext const& ctx ) = 0;
    };
}