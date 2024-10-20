#ifdef _WIN32
#include "Base/Input/InputDevices/InputDevice_XBoxController.h"
#include <windows.h>
#include <XInput.h>

//-------------------------------------------------------------------------

namespace EE::Input
{
    static float const g_maxThumbstickRange = 32767.0f;
    static float const g_maxTriggerRange = 255.0f;

    //-------------------------------------------------------------------------

    void XBoxControllerInputDevice::Initialize()
    {
        XINPUT_STATE state;
        DWORD result = XInputGetState( m_hardwareControllerIdx, &state );
        m_isConnected = ( result == ERROR_SUCCESS );
    }

    void XBoxControllerInputDevice::Shutdown()
    {
        m_isConnected = false;
    }

    void XBoxControllerInputDevice::Update( Seconds deltaTime )
    {
        XINPUT_STATE state;
        DWORD result = XInputGetState( m_hardwareControllerIdx, &state );
        m_isConnected = ( result == ERROR_SUCCESS );
        if ( m_isConnected )
        {
            // Set stick and trigger raw normalized values
            SetTriggerValues( state.Gamepad.bLeftTrigger / g_maxTriggerRange, state.Gamepad.bRightTrigger / g_maxTriggerRange );
            SetAnalogStickValues( Float2( state.Gamepad.sThumbLX / g_maxThumbstickRange, state.Gamepad.sThumbLY / g_maxThumbstickRange ), Float2( state.Gamepad.sThumbRX / g_maxThumbstickRange, state.Gamepad.sThumbRY / g_maxThumbstickRange ) );

            // Set button state
            UpdateControllerButtonState( InputID::Controller_DPadUp, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP ) > 0 );
            UpdateControllerButtonState( InputID::Controller_DPadDown, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN ) > 0 );
            UpdateControllerButtonState( InputID::Controller_DPadLeft, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT ) > 0 );
            UpdateControllerButtonState( InputID::Controller_DPadRight, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ) > 0 );
            UpdateControllerButtonState( InputID::Controller_System0, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK ) > 0 );
            UpdateControllerButtonState( InputID::Controller_System1, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_START ) > 0 );
            UpdateControllerButtonState( InputID::Controller_LeftStick, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB ) > 0 );
            UpdateControllerButtonState( InputID::Controller_RightStick, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB ) > 0 );
            UpdateControllerButtonState( InputID::Controller_LeftShoulder, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER ) > 0 );
            UpdateControllerButtonState( InputID::Controller_RightShoulder, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER ) > 0 );
            UpdateControllerButtonState( InputID::Controller_FaceButtonDown, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_A ) > 0 );
            UpdateControllerButtonState( InputID::Controller_FaceButtonRight, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_B ) > 0 );
            UpdateControllerButtonState( InputID::Controller_FaceButtonLeft, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_X ) > 0 );
            UpdateControllerButtonState( InputID::Controller_FaceButtonUp, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_Y ) > 0 );
        }

        ControllerDevice::Update( deltaTime );
    }

    Float2 XBoxControllerInputDevice::GetDefaultLeftStickDeadZones() const
    {
        return Float2( XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / g_maxThumbstickRange, 0.0f );
    }

    Float2 XBoxControllerInputDevice::GetDefaultRightStickDeadZones() const
    {
        return Float2( XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / g_maxThumbstickRange, 0.0f );
    }

    float XBoxControllerInputDevice::GetDefaultTriggerThreshold() const
    {
        return XINPUT_GAMEPAD_TRIGGER_THRESHOLD / g_maxTriggerRange;
    }
}
#endif