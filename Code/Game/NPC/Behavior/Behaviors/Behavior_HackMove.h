#pragma once
#include "Game/NPC/Behavior/Behavior.h"
#include "Base/Math/Line.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE
{
    class HackPrototypeBehavior : public Behavior
    {
    public:

        EE_BEHAVIOR_ID( HackPrototypeBehavior );

    private:

        virtual void StartInternal( BehaviorContext const& ctx ) override;
        virtual Status UpdateInternal( BehaviorContext const& ctx ) override;
        virtual void StopInternal( BehaviorContext const& ctx, StopReason reason ) override;

    private:

        ManualCountdownTimer        m_waitTimer;
        LineSegment                 m_path = LineSegment( Vector::Zero, Vector::Zero );
        float                       m_distanceAlongPath = 0;
        bool                        m_isWaiting = false;
    };
}