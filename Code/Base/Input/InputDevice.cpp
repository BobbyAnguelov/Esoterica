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
                if ( m_state == State::Pressed )
                {
                    m_state = State::Held;
                }
                else if ( m_state == State::Released )
                {
                    m_state = State::None;
                    m_value = m_rawValue = 0.0f;
                }
            }
            break;

            case Signal::Pressed:
            {
                if ( m_state == State::None || m_state == State::Released )
                {
                    m_state = State::Pressed;
                }

                m_signal = Signal::None;
            }
            break;

            case Signal::Released:
            {
                if ( m_state == State::Pressed || m_state == State::Held )
                {
                    m_state = State::Released;
                }

                m_signal = Signal::None;
                m_value = m_rawValue = 0.0f;
            }
            break;

            case Signal::Impulse:
            {
                m_state = State::Pressed;
                m_signal = Signal::Released;
            }
        }
    }

    //-------------------------------------------------------------------------

    void InputDevice::Press( InputID ID )
    {
        LogicalInput& input = m_inputs[(int32_t) ID];
        input.m_signal = LogicalInput::Signal::Pressed;
        input.m_rawValue = 1.0f;
        input.m_value = 1.0f;
    }

    void InputDevice::Press( InputID ID, float value )
    {
        LogicalInput& input = m_inputs[(int32_t) ID];
        input.m_signal = LogicalInput::Signal::Pressed;
        input.m_rawValue = value;
        input.m_value = value;
    }

    void InputDevice::Press( InputID ID, float rawValue, float filteredValue )
    {
        LogicalInput& input = m_inputs[(int32_t) ID];
        input.m_signal = LogicalInput::Signal::Pressed;
        input.m_rawValue = rawValue;
        input.m_value = filteredValue;
    }

    void InputDevice::ApplyImpulse( InputID ID, float value )
    {
        LogicalInput& input = m_inputs[(int32_t) ID];
        input.m_signal = LogicalInput::Signal::Impulse;
        input.m_rawValue = value;
        input.m_value = value;
    }

    void InputDevice::SetValue( InputID ID, float value )
    {
        LogicalInput& input = m_inputs[(int32_t) ID];
        input.m_state = LogicalInput::State::None;
        input.m_signal = LogicalInput::Signal::None;
        input.m_rawValue = value;
        input.m_value = value;
    }

    void InputDevice::SetValue( InputID ID, float rawValue, float filteredValue )
    {
        LogicalInput& input = m_inputs[(int32_t) ID];
        input.m_state = LogicalInput::State::None;
        input.m_signal = LogicalInput::Signal::None;
        input.m_rawValue = rawValue;
        input.m_value = filteredValue;
    }

    void InputDevice::Release( InputID ID )
    {
        LogicalInput& input = m_inputs[(int32_t) ID];
        input.m_signal = LogicalInput::Signal::Released;
        input.m_rawValue = 0.0f;
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