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

        virtual Float2 GetDefaultLeftStickDeadZones() const = 0;
        virtual Float2 GetDefaultRightStickDeadZones() const = 0;
        virtual float GetDefaultTriggerThreshold() const = 0;

        inline Float2 GetLeftStickValue() const { return Float2( GetValue( InputID::Controller_LeftStickHorizontal ), GetValue( InputID::Controller_LeftStickVertical ) ); }
        inline Float2 GetRightStickValue() const { return Float2( GetValue( InputID::Controller_RightStickHorizontal ), GetValue( InputID::Controller_RightStickVertical ) ); }

    protected:

        void SetTriggerValues( float leftValue, float rightValue );
        void SetAnalogStickValues( Float2 const& leftRawValue, Float2 const& rightRawValue );
        void UpdateControllerButtonState( InputID ID, bool isDown );

    private:

        virtual DeviceType GetDeviceType() const override final { return DeviceType::Controller; }
    };
}