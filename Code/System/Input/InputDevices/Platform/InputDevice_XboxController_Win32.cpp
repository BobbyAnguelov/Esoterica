#ifdef _WIN32
#include "System/Input/InputDevices/InputDevice_XBoxController.h"
#include "System/Math/Vector.h"
#include "System/Types/Arrays.h"
#include <windows.h>
#include <XInput.h>

//-------------------------------------------------------------------------

namespace EE
{
    namespace Input
    {
        static float const g_maxThumbstickRange = 32767.0f;
        static float const g_maxTriggerRange = 255.0f;

        //-------------------------------------------------------------------------

        void XBoxControllerInputDevice::Initialize()
        {
            XINPUT_STATE state;
            DWORD result = XInputGetState( m_hardwareControllerIdx, &state );
            m_isConnected = ( result == ERROR_SUCCESS );

            m_settings.m_leftStickInnerDeadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / g_maxThumbstickRange;
            m_settings.m_leftStickOuterDeadzone = 0.0f;
            m_settings.m_leftTriggerThreshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / g_maxTriggerRange;

            m_settings.m_rightStickInnerDeadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / g_maxThumbstickRange;
            m_settings.m_rightStickOuterDeadzone = 0.0f;
            m_settings.m_rightTriggerThreshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / g_maxTriggerRange;
        }

        void XBoxControllerInputDevice::Shutdown()
        {
            m_isConnected = false;
        }

        void XBoxControllerInputDevice::UpdateState( Seconds deltaTime )
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
                GenerateButtonStateEvents( Input::ControllerButton::DPadUp, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP ) > 0 );
                GenerateButtonStateEvents( Input::ControllerButton::DPadDown, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN ) > 0 );
                GenerateButtonStateEvents( Input::ControllerButton::DPadLeft, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT ) > 0 );
                GenerateButtonStateEvents( Input::ControllerButton::DPadRight, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ) > 0 );
                GenerateButtonStateEvents( Input::ControllerButton::System0, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK ) > 0);
                GenerateButtonStateEvents( Input::ControllerButton::System1, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_START ) > 0 );
                GenerateButtonStateEvents( Input::ControllerButton::ThumbstickLeft, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB ) > 0 );
                GenerateButtonStateEvents( Input::ControllerButton::ThumbstickRight, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB ) > 0 );
                GenerateButtonStateEvents( Input::ControllerButton::ShoulderLeft, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER ) > 0 );
                GenerateButtonStateEvents( Input::ControllerButton::ShoulderRight, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER ) > 0 );
                GenerateButtonStateEvents( Input::ControllerButton::FaceButtonDown, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_A ) > 0 );
                GenerateButtonStateEvents( Input::ControllerButton::FaceButtonRight, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_B ) > 0 );
                GenerateButtonStateEvents( Input::ControllerButton::FaceButtonLeft, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_X ) > 0 );
                GenerateButtonStateEvents( Input::ControllerButton::FaceButtonUp, ( state.Gamepad.wButtons & XINPUT_GAMEPAD_Y ) > 0 );

                ControllerInputDevice::UpdateState( deltaTime );
            }
            else
            {
                ClearControllerState();
            }
        }
    }
}
#endif