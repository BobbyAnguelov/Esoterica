#pragma once

#include "Base/Input/InputDevice.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    //-------------------------------------------------------------------------
    // Base class for all the various controller types and APIs
    //-------------------------------------------------------------------------

    class ControllerDevice : public InputDevice
    {

    public:

        ControllerDevice() = default;

        void SetLeftStickDeadzones( float innerDeadzone, float outerDeadzone );
        inline float GetLeftStickInnerDeadzone() const { return m_leftStickInnerDeadzone; }
        inline float GetLeftStickOuterDeadzone() const { return m_leftStickOuterDeadzone; }

        void SetRightStickDeadzones( float innerDeadzone, float outerDeadzone );
        inline float GetRightStickInnerDeadzone() const { return m_rightStickInnerDeadzone; }
        inline float GetRightStickOuterDeadzone() const { return m_rightStickOuterDeadzone; }

        void SetLeftTriggerThreshold( float threshold ) { EE_ASSERT( threshold >= 0.0f ); m_leftTriggerThreshold = threshold; }
        inline float GetLeftTriggerThreshold() const { return m_leftTriggerThreshold; }

        void SetRightTriggerThreshold( float threshold ) { EE_ASSERT( threshold >= 0.0f ); m_rightTriggerThreshold = threshold; }
        inline float GetRightTriggerThreshold() const { return m_rightTriggerThreshold; }

        void SetLeftStickInvert( bool isInverted ) { m_leftStickInvertY = true; }
        inline bool IsLeftStickInverted() const { return m_leftStickInvertY; }

        void SetRightStickInvert( bool isInverted ) { m_rightStickInvertY = true; }
        inline bool IsRightStickInverted() const { return m_rightStickInvertY; }

        inline Float2 GetLeftStickValue() const { return Float2( GetValue( InputID::Controller_LeftStickHorizontal ), GetValue( InputID::Controller_LeftStickVertical ) ); }
        inline Float2 GetRightStickValue() const { return Float2( GetValue( InputID::Controller_RightStickHorizontal ), GetValue( InputID::Controller_RightStickVertical ) ); }

    protected:

        void SetTriggerValues( float leftValue, float rightValue );
        void SetAnalogStickValues( Float2 const& leftRawValue, Float2 const& rightRawValue );
        void UpdateControllerButtonState( InputID ID, bool isDown );

    private:

        virtual DeviceCategory GetDeviceCategory() const override final { return DeviceCategory::Controller; }

    protected:

        float                                       m_leftStickInnerDeadzone = 0.0f;
        float                                       m_leftStickOuterDeadzone = 0.0f;
        float                                       m_leftTriggerThreshold = 0.0f;

        float                                       m_rightStickInnerDeadzone = 0.0f;
        float                                       m_rightStickOuterDeadzone = 0.0f;
        float                                       m_rightTriggerThreshold = 0.0f;

        bool                                        m_leftStickInvertY = false;
        bool                                        m_rightStickInvertY = false;
    };
}