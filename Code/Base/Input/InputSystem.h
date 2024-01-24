#pragma once

#include "Base/_Module/API.h"
#include "InputDevices/InputDevice_KeyboardMouse.h"
#include "InputDevices/InputDevice_Controller.h"
#include "Base/Types/Arrays.h"
#include "Base/Systems.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    //-------------------------------------------------------------------------
    // Input System
    //-------------------------------------------------------------------------
    // The global EE input system, manages all hardware devices and updates their state

    class EE_BASE_API InputSystem : public ISystem
    {
        friend class InputDebugView;

        constexpr static int32_t const s_maxControllers = 2;

    public:

        EE_SYSTEM( InputSystem );

    public:

        bool Initialize();
        void Shutdown();
        void Update( Seconds deltaTime );

        // Message Processing
        //-------------------------------------------------------------------------

        // Called before we process any new input messages
        void PrepareForNewMessages();

        // Forwards input messages to the various devices
        void ForwardInputMessageToInputDevices( GenericMessage const& inputMessage );

        // Input Devices
        //-------------------------------------------------------------------------

        inline TInlineVector<InputDevice*, 5> const& GetInputDevices() const { return m_inputDevices; }

        // Keyboard & Mouse
        //-------------------------------------------------------------------------

        bool HasConnectedKeyboardAndMouse() const { return m_keyboardMouse.IsConnected(); }
        KeyboardMouseDevice const* GetKeyboardMouse() const { return &m_keyboardMouse; }

        // Controllers
        //-------------------------------------------------------------------------

        uint32_t GetNumConnectedControllers() const;
        inline bool HasConnectedController() const { return GetNumConnectedControllers() > 0; }
        inline bool IsControllerConnected( int32_t controllerIdx ) const;
        ControllerDevice const* GetController( uint32_t controllerIdx = 0 ) const;
        ControllerDevice const* GetFirstController() const;

    private:

        KeyboardMouseDevice                 m_keyboardMouse;
        ControllerDevice*                   m_controllers[s_maxControllers];
        TInlineVector<InputDevice*, 5>      m_inputDevices;
    };
}