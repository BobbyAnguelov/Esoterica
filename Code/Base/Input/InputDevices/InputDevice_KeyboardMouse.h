#pragma once

#include "Base/Input/InputDevice.h"
#include "Base/Input/InputStates/InputState_Mouse.h"
#include "Base/Input/InputStates/InputState_Keyboard.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    class KeyboardMouseInputDevice : public InputDevice
    {

    public:

        KeyboardMouseInputDevice() = default;

        inline MouseInputState const& GetMouseState() const { return m_mouseState; }
        inline KeyboardInputState const& GetKeyboardState() const { return m_keyboardState; }

    private:

        virtual DeviceCategory GetDeviceCategory() const override final { return DeviceCategory::KeyboardMouse; }

        virtual void Initialize() override final;
        virtual void Shutdown() override final;

        virtual void UpdateState( Seconds deltaTime ) override final;
        virtual void ClearFrameState( ResetType resetType = ResetType::Partial ) override final;
        virtual void ProcessMessage( GenericMessage const& message ) override final;

    private:

        MouseInputState                                         m_mouseState;
        KeyboardInputState                                      m_keyboardState;
    };
}