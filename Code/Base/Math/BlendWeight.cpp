#include "BlendWeight.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    void BlendWeight::SetDesiredBlendTime( Seconds desiredBlendTime )
    {
        EE_ASSERT( desiredBlendTime >= 0.0f );

        if ( desiredBlendTime == 0.0f )
        {
            if ( m_state == State::TurningOn )
            {
                m_state = State::On;
            }
            else if ( m_state == State::TurningOff )
            {
                m_state = State::Off;
            }
        }
        else // Invert blend times
        {
            if ( m_state == State::TurningOn || m_state == State::TurningOff )
            {
                EE_ASSERT( m_desiredBlendTime != 0.0f );
                float const percentThroughBlend = m_blendTime / m_desiredBlendTime;
                m_blendTime = percentThroughBlend * desiredBlendTime;
            }
        }

        // Update the desired blend time
        m_desiredBlendTime = desiredBlendTime;
    }

    float BlendWeight::GetWeight() const
    {
        float weight = 0.0f;

        if ( m_state == State::On )
        {
            weight = 1.0f;
        }
        else if ( m_state == State::Off )
        {
            weight = 0.0f;
        }
        else // Turning on/off
        {
            EE_ASSERT( m_desiredBlendTime > 0.0f );

            weight = ( m_blendTime / m_desiredBlendTime );
            if ( m_easingOperation != Easing::Operation::Linear && m_easingOperation != Easing::Operation::None )
            {
                // Some easing functions will under/over shoot
                weight = Math::Clamp( Easing::Evaluate( m_easingOperation, weight ), 0.0f, 1.0f );
            }

            // Invert the weight if we are turning off
            if ( m_state == State::TurningOff )
            {
                weight = 1.0f - weight;
            }
        }

        EE_ASSERT( weight >= 0.0f && weight <= 1.0f );
        return weight;
    }

    float BlendWeight::Update( Seconds deltaTime, bool isOn )
    {
        // Update blend state
        //-------------------------------------------------------------------------

        switch ( m_state )
        {
            case State::On:
            {
                if ( !isOn )
                {
                    m_state = State::TurningOff;
                    m_blendTime = 0.0f;
                }
            }
            break;

            case State::Off:
            {
                if ( isOn )
                {
                    m_state = State::TurningOn;
                    m_blendTime = 0.0f;
                }
            }
            break;

            case State::TurningOn:
            {
                if ( !isOn )
                {
                    m_state = State::TurningOff;
                    m_blendTime = m_desiredBlendTime - m_blendTime;
                }
            }
            break;

            case State::TurningOff:
            {
                if ( isOn )
                {
                    m_state = State::TurningOn;
                    m_blendTime = m_desiredBlendTime - m_blendTime;
                }
            }
            break;
        }

        // Blend
        //-------------------------------------------------------------------------

        if ( IsBlending() )
        {
            m_blendTime += deltaTime;

            // Is the blend complete?
            if ( m_blendTime >= m_desiredBlendTime )
            {
                m_blendTime = 0.0f;
                m_state = ( m_state == State::TurningOn ) ? State::On : State::Off;
            }
        }

        return GetWeight();
    }
}