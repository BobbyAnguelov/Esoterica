#include "GameInputUpdaters.h"
#include "Base/Input/InputSystem.h"
#include "Base/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    static bool GetInputValue( InputSystem const* pInputSystem, InputID ID, float& outValue )
    {
        DeviceType const deviceType = GetDeviceTypeForInputID( ID );
        if ( deviceType == DeviceType::KeyboardMouse )
        {
            if ( pInputSystem->HasConnectedKeyboardAndMouse() )
            {
                outValue = pInputSystem->GetKeyboardMouse()->GetValue( ID );
                return true;
            }
        }
        else // Controller
        {
            auto pController = pInputSystem->GetFirstConnectedController();
            if ( pController != nullptr )
            {
                outValue = pController->GetValue( ID );
                return true;
            }
        }

        outValue = 0.0f;
        return false;
    }

    static bool GetInputState( InputSystem const* pInputSystem, InputID ID, InputState& outState, float* pOptionalOutValue = nullptr )
    {
        DeviceType const deviceType = GetDeviceTypeForInputID( ID );
        if ( deviceType == DeviceType::KeyboardMouse )
        {
            if ( pInputSystem->HasConnectedKeyboardAndMouse() )
            {
                auto pKBM = pInputSystem->GetKeyboardMouse();

                outState = pKBM->GetState( ID );

                if ( pOptionalOutValue != nullptr )
                {
                    *pOptionalOutValue = pKBM->GetValue( ID );
                }

                return true;
            }
        }
        else // Controller
        {
            auto pController = pInputSystem->GetFirstConnectedController();
            if ( pController != nullptr )
            {
                outState = pController->GetState( ID );

                if ( pOptionalOutValue != nullptr )
                {
                    *pOptionalOutValue = pController->GetValue( ID );
                }

                return true;
            }
        }

        outState = InputState::None;
        if ( pOptionalOutValue != nullptr )
        {
            *pOptionalOutValue = 0.0f;
        }
        return false;
    }

    // Axis Updater
    //-------------------------------------------------------------------------

    void MouseAxisUpdater::Update( InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta )
    {
        if ( !IsValid() )
        {
            m_value = Float2::Zero;
            return;
        }

        // Get device value
        //-------------------------------------------------------------------------

        if ( pInputSystem->HasConnectedKeyboardAndMouse() )
        {
            auto pKBM = pInputSystem->GetKeyboardMouse();
            m_value = pKBM->GetMouseDelta();
        }

        // Apply additional modifications
        //-------------------------------------------------------------------------

        if ( m_invertY )
        {
            m_value.m_y = -m_value.m_y;
        }

        m_value *= m_sensitivity;
    }

    //-------------------------------------------------------------------------

    bool ControllerAxisUpdater::IsValid() const
    {
        if ( !GameInputAxisUpdater::IsValid() )
        {
            return false;
        }

        return m_deadZone.IsValid();
    }

    void ControllerAxisUpdater::Update( InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta )
    {
        if ( !IsValid() )
        {
            m_rawValue = Float2::Zero;
            m_value = Float2::Zero;
            return;
        }

        // Get device value
        //-------------------------------------------------------------------------

        auto pController = pInputSystem->GetFirstConnectedController();
        if ( pController != nullptr )
        {
            if ( m_useLeftStick )
            {
                m_rawValue = pController->GetLeftStickValue();
            }
            else
            {
                m_rawValue = pController->GetRightStickValue();
            }
        }

        // Apply dead zones
        //-------------------------------------------------------------------------

        // Get the direction and magnitude
        Vector vDirection;
        float magnitude;
        Vector( m_rawValue ).ToDirectionAndLength2( vDirection, magnitude );

        // Apply dead zones
        if ( magnitude > m_deadZone.m_inner )
        {
            float const remainingRange = ( 1.0f - m_deadZone.m_outer - m_deadZone.m_inner );
            float const newMagnitude = Math::Min( 1.0f, ( magnitude - m_deadZone.m_inner ) / remainingRange );
            m_value = ( vDirection * newMagnitude ).ToFloat2();
        }
        else // Set the value to zero
        {
            m_value = Float2::Zero;
        }

        // Apply additional modifications
        //-------------------------------------------------------------------------

        if ( m_invertY )
        {
            m_value.m_y = -m_value.m_y;
        }

        m_value *= m_sensitivity;
    }

    //-------------------------------------------------------------------------

    bool KeyboardAxisUpdater::IsValid() const
    {
        if ( !GameInputAxisUpdater::IsValid() )
        {
            return false;
        }

        return IsValidInputID( m_up ) && IsValidInputID( m_down ) && IsValidInputID( m_left ) && IsValidInputID( m_right );
    }

    void KeyboardAxisUpdater::Update( InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta )
    {
        if ( !IsValid() )
        {
            m_value = Float2::Zero;
            return;
        }

        // Get device values
        //-------------------------------------------------------------------------

        float up, down, left, right;

        GetInputValue( pInputSystem, m_up, up );
        GetInputValue( pInputSystem, m_down, down );
        GetInputValue( pInputSystem, m_left, left );
        GetInputValue( pInputSystem, m_right, right );

        m_value.m_x = right - left;
        m_value.m_y = up - down;

        // Apply additional modifications
        //-------------------------------------------------------------------------

        m_value *= m_sensitivity;
    }

    // Value Updater
    //-------------------------------------------------------------------------

    bool GameInputValueUpdater::IsValid() const
    {
        return IsValidInputID( m_ID ) && m_threshold >= 0.0f && m_threshold <= 1.0f;
    }

    void GameInputValueUpdater::Update( InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta )
    {
        if ( !IsValid() )
        {
            m_value = 0.0f;
            return;
        }

        // Try read raw value from source device
        //-------------------------------------------------------------------------

        GetInputValue( pInputSystem, m_ID, m_value );

        if ( m_threshold > 0.0f )
        {
            m_value = Math::RemapRange( m_value, m_threshold, 1.0f, 0.0f, 1.0f );
        }
    }

    // Signal Updater
    //-------------------------------------------------------------------------

    bool GameInputSignalUpdater::IsValid() const
    {
        return IsValidInputID( m_ID ) && m_threshold >= 0.0f && m_threshold <= 1.0f;
    }

    void GameInputSignalUpdater::Update( InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta )
    {
        if ( !IsValid() )
        {
            m_state = InputState::None;
            return;
        }

        // Try read raw value from source device
        //-------------------------------------------------------------------------

        float value;
        InputState newState;
        bool const hasValidDevice = GetInputState( pInputSystem, m_ID, newState, &value );

        // Handle source device disconnection
        //-------------------------------------------------------------------------

        if ( !hasValidDevice )
        {
             if ( m_state == InputState::Pressed || m_state == InputState::Held )
             {
                 m_state = InputState::Released;
             }
             else if( m_state == InputState::Released )
             {
                 m_state = InputState::None;
             }

            return;
        }

        // Update state
        //-------------------------------------------------------------------------

        bool const exceededThreshold = ( value > m_threshold );
        if ( exceededThreshold )
        {
            m_state = newState;
        }
        else // No input
        {
            if ( m_state == InputState::Released )
            {
                m_state = InputState::None;
            }
            else if( m_state == InputState::Pressed || m_state == InputState::Held )
            {
                m_state = InputState::Released;
            }
        }
    }
}