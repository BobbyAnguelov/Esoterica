#include "InputDevice_KeyboardMouse.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    void KeyboardMouseDevice::PrepareForNewMessages()
    {
        m_charKeyPressed = 0;
        m_movementDelta = Float2::Zero;
    }

    void KeyboardMouseDevice::Clear()
    {
        m_charKeyPressed = 0;
        m_position = Float2::Zero;
        m_movementDelta = Float2::Zero;
        InputDevice::Clear();
    }
}