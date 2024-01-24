#include "InputDevice_Controller.h"
#include "Base\Math\Vector.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    void ControllerDevice::SetLeftStickDeadzones( float innerDeadzone, float outerDeadzone )
    {
        EE_ASSERT( innerDeadzone >= 0.0f && innerDeadzone <= 1.0f );
        EE_ASSERT( outerDeadzone >= 0.0f && outerDeadzone <= 1.0f );
        m_leftStickInnerDeadzone = innerDeadzone;
        m_leftStickOuterDeadzone = outerDeadzone;
    }

    void ControllerDevice::SetRightStickDeadzones( float innerDeadzone, float outerDeadzone )
    {
        EE_ASSERT( innerDeadzone >= 0.0f && innerDeadzone <= 1.0f );
        EE_ASSERT( outerDeadzone >= 0.0f && outerDeadzone <= 1.0f );
        m_rightStickInnerDeadzone = innerDeadzone;
        m_rightStickOuterDeadzone = outerDeadzone;
    }

    //-------------------------------------------------------------------------

    void ControllerDevice::SetTriggerValues( float leftRawValue, float rightRawValue )
    {
        auto calculateFilteredTriggerValue = [] ( float rawValue, float threshold )
        {
            EE_ASSERT( threshold >= 0 && threshold <= 1.0f );

            float filteredValue = 0.0f;
            if ( rawValue > threshold )
            {
                filteredValue = ( rawValue - threshold ) / ( 1.0f - threshold );
            }

            return filteredValue;
        };

        //-------------------------------------------------------------------------

        float const leftFilteredValue = calculateFilteredTriggerValue( leftRawValue, m_leftTriggerThreshold );
        if ( leftFilteredValue > 0.0f )
        {
            Press( InputID::Controller_LeftTrigger, leftRawValue, leftFilteredValue );
        }
        else
        {
            Release( InputID::Controller_LeftTrigger );
        }

        //-------------------------------------------------------------------------

        float const rightFilteredValue = calculateFilteredTriggerValue( rightRawValue, m_rightTriggerThreshold );
        if ( rightFilteredValue > 0.0f )
        {
            Press( InputID::Controller_RightTrigger, rightRawValue, rightFilteredValue );
        }
        else
        {
            Release( InputID::Controller_RightTrigger );
        }
    }

    void ControllerDevice::SetAnalogStickValues( Float2 const& leftValue, Float2 const& rightValue )
    {
        auto CalculateRawValue = [] ( Float2 const rawValue, bool bInvertY )
        {
            float const normalizedX = Math::Clamp( rawValue.m_x, -1.0f, 1.0f );
            float const normalizedY = Math::Clamp( rawValue.m_y, -1.0f, 1.0f );
            return Float2( normalizedX, bInvertY ? -normalizedY : normalizedY );
        };

        auto CalculateFilteredValue = [] ( Float2 const rawValue, float const innerDeadzoneRange, float const outerDeadzoneRange )
        {
            EE_ASSERT( innerDeadzoneRange >= 0 && innerDeadzoneRange <= 1.0f && outerDeadzoneRange >= 0 && outerDeadzoneRange <= 1.0f );

            Float2 filteredValue;

            // Get the direction and magnitude
            Vector vDirection;
            float magnitude;
            Vector( rawValue ).ToDirectionAndLength2( vDirection, magnitude );

            // Apply dead zones
            if ( magnitude > innerDeadzoneRange )
            {
                float const remainingRange = ( 1.0f - outerDeadzoneRange - innerDeadzoneRange );
                float const newMagnitude = Math::Min( 1.0f, ( magnitude - innerDeadzoneRange ) / remainingRange );
                filteredValue = ( vDirection * newMagnitude ).ToFloat2();
            }
            else // Set the value to zero
            {
                filteredValue = Float2::Zero;
            }

            return filteredValue;
        };

        //-------------------------------------------------------------------------

        Float2 const leftRawValue = CalculateRawValue( leftValue, m_leftStickInvertY );
        Float2 const leftFilteredValue = CalculateFilteredValue( leftValue, m_leftStickInnerDeadzone, m_leftStickOuterDeadzone );
        SetValue( InputID::Controller_LeftStickHorizontal, leftRawValue.m_x, leftFilteredValue.m_x );
        SetValue( InputID::Controller_LeftStickVertical, leftRawValue.m_y, leftFilteredValue.m_y );

        //-------------------------------------------------------------------------

        Float2 const rightRawValue = CalculateRawValue( rightValue, m_rightStickInvertY );
        Float2 const rightFilteredValue = CalculateFilteredValue( rightValue, m_rightStickInnerDeadzone, m_rightStickOuterDeadzone );
        SetValue( InputID::Controller_RightStickHorizontal, rightRawValue.m_x, rightFilteredValue.m_x );
        SetValue( InputID::Controller_RightStickVertical, rightRawValue.m_y, rightFilteredValue.m_y );
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