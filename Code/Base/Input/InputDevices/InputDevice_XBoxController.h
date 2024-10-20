#pragma once

#include "InputDevice_Controller.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    class XBoxControllerInputDevice : public ControllerDevice
    {

    public:

        XBoxControllerInputDevice( uint32_t hardwareControllerIdx ) : m_hardwareControllerIdx( hardwareControllerIdx ) {}

    private:

        virtual void Initialize() override final;
        virtual void Shutdown() override final;
        virtual void Update( Seconds deltaTime ) override final;

        virtual Float2 GetDefaultLeftStickDeadZones() const override;
        virtual Float2 GetDefaultRightStickDeadZones() const override;
        virtual float GetDefaultTriggerThreshold() const override;

    private:

        uint32_t m_hardwareControllerIdx = 0xFFFFFFFF;
    };
}