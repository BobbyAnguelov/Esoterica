#include "InputDevice_Controller.h"
#include "Base\Math\Vector.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    void ControllerDevice::SetTriggerValues( float leftRawValue, float rightRawValue )
    {
        if ( leftRawValue > 0.0f )
        {
            Press( InputID::Controller_LeftTrigger, leftRawValue );
        }
        else
        {
            Release( InputID::Controller_LeftTrigger );
        }

        //-------------------------------------------------------------------------

        if ( rightRawValue > 0.0f )
        {
            Press( InputID::Controller_RightTrigger, rightRawValue );
        }
        else
        {
            Release( InputID::Controller_RightTrigger );
        }
    }

    void ControllerDevice::SetAnalogStickValues( Float2 const& leftValue, Float2 const& rightValue )
    {
        auto CalculateRawValue = [] ( Float2 const rawValue )
        {
            float const normalizedX = Math::Clamp( rawValue.m_x, -1.0f, 1.0f );
            float const normalizedY = Math::Clamp( rawValue.m_y, -1.0f, 1.0f );
            return Float2( normalizedX, normalizedY );
        };

        //-------------------------------------------------------------------------

        Float2 const leftRawValue = CalculateRawValue( leftValue );
        SetValue( InputID::Controller_LeftStickHorizontal, leftRawValue.m_x );
        SetValue( InputID::Controller_LeftStickVertical, leftRawValue.m_y );

        //-------------------------------------------------------------------------

        Float2 const rightRawValue = CalculateRawValue( rightValue );
        SetValue( InputID::Controller_RightStickHorizontal, rightRawValue.m_x );
        SetValue( InputID::Controller_RightStickVertical, rightRawValue.m_y );
    }

    void ControllerDevice::UpdateControllerButtonState( InputID ID, bool isDown )
    {
        // Do we need to generate a pressed signal?
        if ( isDown )
        {
            if ( !IsHeldDown( ID ) )
            {
                Press( ID );
            }
        }
        else // If the button is up, check if we need to generate a release signal
        {
            if ( IsHeldDown( ID ) )
            {
                Release( ID );
            }
        }
    }
}