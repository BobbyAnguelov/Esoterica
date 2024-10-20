#include "Input.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    DeviceType GetDeviceTypeForInputID( InputID ID )
    {
        if ( ID >= InputID::Keyboard_A && ID <= InputID::Mouse_DeltaMovementVertical )
        {
            return DeviceType::KeyboardMouse;
        }

        if ( ID >= InputID::Controller_DPadUp && ID <= InputID::Controller_RightStickVertical )
        {
            return DeviceType::Controller;
        }

        return DeviceType::Unknown;
    }
}