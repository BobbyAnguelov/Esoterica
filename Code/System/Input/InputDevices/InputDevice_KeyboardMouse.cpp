#include "InputDevice_KeyboardMouse.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Input
    {
        void KeyboardMouseInputDevice::UpdateState( Seconds deltaTime )
        {
            m_mouseState.Update( deltaTime );
            m_keyboardState.Update( deltaTime );
        }

        void KeyboardMouseInputDevice::ClearFrameState( ResetType resetType )
        {
            m_mouseState.ResetFrameState( resetType);
            m_keyboardState.ResetFrameState( resetType );
        }
    }
}