#pragma once
#include "Base/Time/Timers.h"
#include "Base/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    struct BehaviorContext;

    //-------------------------------------------------------------------------

    class IdleAction
    {
    public:

        void Start( BehaviorContext const& ctx );
        void Update( BehaviorContext const& ctx );

    private:

        void ResetIdleBreakerTimer();

    private:

        ManualCountdownTimer        m_idleBreakerCooldown;
    };
}