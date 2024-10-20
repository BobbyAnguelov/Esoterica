#include "InputDevice.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    void InputDevice::LogicalInput::Update()
    {
        switch ( m_signal )
        {
            case Signal::None:
            {
                if ( m_state == InputState::Pressed )
                {
                    m_state = InputState::Held;
                }
                else if ( m_state == InputState::Released )
                {
                    m_state = InputState::None;
                    m_value = 0.0f;
                }
            }
            break;

            case Signal::Pressed:
            {
                if ( m_state == InputState::None || m_state == InputState::Released )
                {
                    m_state = InputState::Pressed;
                }

                m_signal = Signal::None;
            }
            break;

            case Signal::Released:
            {
                if ( m_state == InputState::Pressed || m_state == InputState::Held )
                {
                    m_state = InputState::Released;
                }

                m_signal = Signal::None;
                m_value = 0.0f;
            }
            break;

            case Signal::Impulse:
            {
                m_state = InputState::Pressed;
                m_signal = Signal::Released;
            }
        }
    }

    //-------------------------------------------------------------------------

    void InputDevice::Press( InputID ID )
    {
        LogicalInput& input = m_inputs[(int32_t) ID];
        input.m_signal = LogicalInput::Signal::Pressed;
        input.m_value = 1.0f;
    }

    void InputDevice::Press( InputID ID, float value )
    {
        LogicalInput& input = m_inputs[(int32_t) ID];
        input.m_signal = LogicalInput::Signal::Pressed;
        input.m_value = value;
    }

    void InputDevice::ApplyImpulse( InputID ID, float value )
    {
        LogicalInput& input = m_inputs[(int32_t) ID];
        input.m_signal = LogicalInput::Signal::Impulse;
        input.m_value = value;
    }

    void InputDevice::SetValue( InputID ID, float value )
    {
        LogicalInput& input = m_inputs[(int32_t) ID];
        input.m_state = InputState::None;
        input.m_signal = LogicalInput::Signal::None;
        input.m_value = value;
    }

    void InputDevice::Release( InputID ID )
    {
        LogicalInput& input = m_inputs[(int32_t) ID];
        input.m_signal = LogicalInput::Signal::Released;
        input.m_value = 0.0f;
    }

    void InputDevice::Update( Seconds deltaTime )
    {
        for ( auto& input : m_inputs )
        {
            input.Update();
        }
    }

    void InputDevice::Clear()
    {
        for ( auto& input : m_inputs )
        {
            input.Clear();
        }
    }
}