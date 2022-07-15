#pragma once

#include "System/Input/InputDevice.h"
#include "System/Input/InputStates/InputState_Controller.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Input
    {
        // Base class for all the various controller types and APIs
        //-------------------------------------------------------------------------

        class ControllerInputDevice : public InputDevice
        {

        public:

            struct Settings
            {

            public:

                float     m_leftStickInnerDeadzone = 0.0f;
                float     m_leftStickOuterDeadzone = 0.0f;
                float     m_leftTriggerThreshold = 0.0f;

                float     m_rightStickInnerDeadzone = 0.0f;
                float     m_rightStickOuterDeadzone = 0.0f;
                float     m_rightTriggerThreshold = 0.0f;

                bool    m_leftStickInvertY = false;
                bool    m_rightStickInvertY = false;
            };

        public:

            ControllerInputDevice() = default;

            inline Settings const& GetSettings() const { return m_settings; }
            inline ControllerInputState const& GetControllerState() const { return m_controllerState; }

            inline bool IsConnected() const { return m_isConnected; }

        protected:

            virtual void UpdateState( Seconds deltaTime ) override;

            // Simulate Press/Release events for controllers
            inline void GenerateButtonStateEvents( ControllerButton buttonID, bool isDown )
            {
                // Do we need to generate a pressed signal?
                if ( isDown )
                {
                    if ( !m_controllerState.IsHeldDown( buttonID ) )
                    {
                        m_controllerState.Press( buttonID );
                    }
                }
                else // If the button is up, check if we need to generate a release signal
                {
                    if ( m_controllerState.IsHeldDown( buttonID ) )
                    {
                        m_controllerState.Release( buttonID );
                    }
                }
            }

            void SetTriggerValues( float leftRawValue, float rightRawValue );
            void SetAnalogStickValues( Float2 const& leftRawValue, Float2 const& rightRawValue );
            void ClearControllerState() { m_controllerState.Clear(); }

        private:

            virtual DeviceCategory GetDeviceCategory() const override final { return DeviceCategory::Controller; }

        protected:

            Settings                                    m_settings;
            ControllerInputState                        m_controllerState;
            bool                                        m_isConnected = false;
        };
    }
}