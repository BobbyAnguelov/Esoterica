#pragma once
#include "Easing.h"
#include "Base/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    // Helper to calculate the weight of a blend over time
    struct EE_BASE_API BlendWeight
    {
        enum class State : uint8_t
        {
            Off,
            TurningOff,
            TurningOn,
            On,
        };

    public:

        void Reset( Seconds desiredBlendTime, bool startOn )
        {
            m_state = startOn ? State::On : State::Off;
            m_desiredBlendTime = desiredBlendTime;
            m_blendTime = desiredBlendTime;
        }

        // Are currently blending
        inline bool IsBlending() const { return m_state == State::TurningOn || m_state == State::TurningOff; }

        // Set the time it should take to switch from 'on' to 'off'
        void SetDesiredBlendTime( Seconds flDesiredBlendTime );

        // Get the current blend weight
        float GetWeight() const;

        // Update the weight and get the blend weight [0,1] where 0 is off and 1 is on
        float Update( Seconds flDeltaTime, bool isOn );

    public:

        State               m_state = State::Off;
        Easing::Operation   m_easingOperation = Easing::Operation::None;
        Seconds             m_blendTime = 0.0f;
        Seconds             m_desiredBlendTime = 0.2f;
    };
}