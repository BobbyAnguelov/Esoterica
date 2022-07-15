#pragma once

#include "System/_Module/API.h"
#include "InputDevices/InputDevice_KeyboardMouse.h"
#include "InputDevices/InputDevice_Controller.h"
#include "System/Types/Arrays.h"
#include "System/Systems.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    class InputState;

    //-------------------------------------------------------------------------
    // Input System
    //-------------------------------------------------------------------------
    // The global EE input system, manages all hardware devices and updates their state

    class EE_SYSTEM_API InputSystem : public ISystem
    {
        friend class InputDebugView;

    public:

        EE_SYSTEM_ID( InputSystem );

        static MouseInputState const s_emptyMouseState;
        static KeyboardInputState const s_emptyKeyboardState;
        static ControllerInputState const s_emptyControllerState;

    public:

        bool Initialize();
        void Shutdown();
        void Update( Seconds deltaTime );
        void ClearFrameState();
        void ForwardInputMessageToInputDevices( GenericMessage const& inputMessage );

        // Keyboard & Mouse
        //-------------------------------------------------------------------------

        inline bool HasConnectedKeyboardAndMouse() { return GetKeyboardMouseDevice() != nullptr; }

        inline MouseInputState const* GetMouseState() const
        {
            if ( auto pDevice = GetKeyboardMouseDevice() )
            {
                return &pDevice->GetMouseState();
            }

            return &s_emptyMouseState;
        }

        inline KeyboardInputState const* GetKeyboardState() const
        {
            if ( auto pDevice = GetKeyboardMouseDevice() )
            {
                return &pDevice->GetKeyboardState();
            }

            return &s_emptyKeyboardState;
        }

        // Controllers
        //-------------------------------------------------------------------------

        uint32_t GetNumConnectedControllers() const;

        inline ControllerInputState const* GetControllerState( uint32_t controllerIdx = 0 ) const
        {
            if ( auto pDevice = GetControllerDevice( controllerIdx ) )
            {
                return &pDevice->GetControllerState();
            }

            return &s_emptyControllerState;
        }

        // Reflection
        //-------------------------------------------------------------------------

        void ReflectState( Seconds const deltaTime, float timeScale, InputState& outReflectedState ) const;

    private:

        KeyboardMouseInputDevice const* GetKeyboardMouseDevice() const;
        ControllerInputDevice const* GetControllerDevice( uint32_t controllerIdx = 0 ) const;

    private:

        TVector<InputDevice*>       m_inputDevices;
    };

    //-------------------------------------------------------------------------
    // Input State
    //-------------------------------------------------------------------------
    // A copy of the input state, used to contextually manage input state per system/world/etc...

    class EE_SYSTEM_API InputState
    {
        friend class InputSystem;

    public:

        InputState() = default;

        void Clear();

        EE_FORCE_INLINE MouseInputState const* GetMouseState() const
        {
            return &m_mouseState;
        }

        EE_FORCE_INLINE KeyboardInputState const* GetKeyboardState() const
        {
            return &m_keyboardState;
        }

        EE_FORCE_INLINE ControllerInputState const* GetControllerState( uint32_t controllerIdx = 0 ) const
        {
            if ( controllerIdx < m_controllerStates.size() )
            {
                return &m_controllerStates[controllerIdx];
            }

            return &InputSystem::s_emptyControllerState;
        }

    private:

        MouseInputState                             m_mouseState;
        KeyboardInputState                          m_keyboardState;
        TInlineVector<ControllerInputState, 4>      m_controllerStates;
    };
}