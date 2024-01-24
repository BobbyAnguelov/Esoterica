#include "Base/Input/InputSystem.h"
#include "InputDevices/InputDevice_XBoxController.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    bool InputSystem::Initialize()
    {
        m_inputDevices.emplace_back( &m_keyboardMouse );

        // Create a single controller for now
        //-------------------------------------------------------------------------
        // TODO: build a controller manager that can detect and automatically create controllers

        for ( int32_t i = 0; i < s_maxControllers; i++ )
        {
            m_controllers[i] = EE::New<XBoxControllerInputDevice>( i );
            m_inputDevices.emplace_back( m_controllers[i] );
        }

        //-------------------------------------------------------------------------

        for ( auto pDevice : m_inputDevices )
        {
            pDevice->Initialize();
        }

        return true;
    }

    void InputSystem::Shutdown()
    {
        for ( auto& pInputDevice : m_inputDevices )
        {
            pInputDevice->Shutdown();
        }

        m_inputDevices.clear();

        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < s_maxControllers; i++ )
        {
            EE::Delete( m_controllers[i] );
        }
    }

    void InputSystem::Update( Seconds deltaTime )
    {
        for ( auto pInputDevice : m_inputDevices )
        {
            pInputDevice->Update( deltaTime );
        }
    }

    void InputSystem::PrepareForNewMessages()
    {
        for ( auto pInputDevice : m_inputDevices )
        {
            pInputDevice->PrepareForNewMessages();
        }
    }

    void InputSystem::ForwardInputMessageToInputDevices( GenericMessage const& inputMessage )
    {
        for ( auto pInputDevice : m_inputDevices )
        {
            pInputDevice->ProcessMessage( inputMessage );
        }
    }

    //-------------------------------------------------------------------------

    uint32_t InputSystem::GetNumConnectedControllers() const
    {
        uint32_t numConnectedControllers = 0;
        for ( int32_t i = 0; i < s_maxControllers; i++ )
        {
            if ( m_controllers[i]->IsConnected() )
            {
                numConnectedControllers++;
            }
        }

        return numConnectedControllers;
    }

    bool InputSystem::IsControllerConnected( int32_t controllerIdx ) const
    {
        EE_ASSERT( controllerIdx < s_maxControllers );
        return m_controllers[controllerIdx]->IsConnected();
    }

    ControllerDevice const* InputSystem::GetController( uint32_t controllerIdx ) const
    {
        EE_ASSERT( controllerIdx < s_maxControllers );
        return m_controllers[controllerIdx];
    }

    ControllerDevice const* InputSystem::GetFirstController() const
    {
        for ( int32_t i = 0; i < s_maxControllers; i++ )
        {
            if ( m_controllers[i]->IsConnected() )
            {
                return m_controllers[i];
            }
        }

        return nullptr;
    }
}