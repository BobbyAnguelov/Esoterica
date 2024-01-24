#include "InputDevice_KeyboardMouse.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    void KeyboardMouseDevice::PrepareForNewMessages()
    {
        m_charKeyPressed = 0;
        m_movementDelta = Float2::Zero;
    }

    void KeyboardMouseDevice::SetMouseSensitivity( Float2 sensitivity )
    {
        EE_ASSERT( sensitivity.m_x > 0.0f );
        EE_ASSERT( sensitivity.m_y > 0.0f );

        m_sensitivity = sensitivity;
        if ( m_invertY )
        {
            m_sensitivity.m_y = -m_sensitivity.m_y;
        }
    }

    void KeyboardMouseDevice::SetMouseInverted( bool isInverted )
    {
        m_invertY = isInverted;
        m_sensitivity.m_y = m_invertY ? -Math::Abs( m_sensitivity.m_y ) : Math::Abs( m_sensitivity.m_y );
    }

    void KeyboardMouseDevice::Clear()
    {
        m_charKeyPressed = 0;
        m_position = Float2::Zero;
        m_movementDelta = Float2::Zero;
        InputDevice::Clear();
    }
}