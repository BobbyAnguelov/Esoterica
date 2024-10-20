#pragma once

#include "Base/Input/InputDevice.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    //-------------------------------------------------------------------------
    // Keyboard Mouse Device
    //-------------------------------------------------------------------------

    class KeyboardMouseDevice : public InputDevice
    {

    public:

        KeyboardMouseDevice() = default;

        inline Float2 GetMouseDelta() const { return Float2( GetValue( InputID::Mouse_DeltaMovementHorizontal ), GetValue( InputID::Mouse_DeltaMovementVertical ) ); }

        // Get the char key pressed this frame. If no key pressed, this returns 0;
        inline uint8_t GetCharKeyPressed() const { return m_charKeyPressed; }

    private:

        virtual DeviceType GetDeviceType() const override final { return DeviceType::KeyboardMouse; }

        virtual void Initialize() override final;
        virtual void Shutdown() override final;

        virtual void Update( Seconds deltaTime ) override;

        virtual void PrepareForNewMessages() override final;
        virtual void ProcessMessage( GenericMessage const& message ) override final;

        virtual void Clear() override;

    private:

        uint8_t                             m_charKeyPressed = 0;
        Int2                                m_position = Float2::Zero;
        Float2                              m_movementDelta = Float2::Zero;
    };
}