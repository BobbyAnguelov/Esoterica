#pragma once

#include "Component_FreeLookCamera.h"
#include "System/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINE_API DebugCameraComponent : public FreeLookCameraComponent
    {

    public:

        constexpr static float const                s_minSpeed = 0.5f; // m/s
        constexpr static float const                s_maxSpeed = 100.0f; // m/s
        constexpr static float const                s_moveSpeedStepSize = 0.5f; // m/s 
        constexpr static float const                s_maxAngularVelocity = Math::Pi * 5.0f;

    public:

        using FreeLookCameraComponent::FreeLookCameraComponent;

        inline bool IsEnabled() const { return m_isEnabled; }
        inline void SetEnabled( bool isEnabled ) { m_isEnabled = isEnabled; }

        // Camera Movement
        //-------------------------------------------------------------------------

        // Set the default move speed i.e. the value we will reset the camera to
        inline void SetDefaultMoveSpeed( float newDefaultSpeed ) { m_defaultMoveSpeed = Math::Clamp( newDefaultSpeed, s_minSpeed, s_maxSpeed ); }

        // Reset the move speed to the default value
        inline void ResetMoveSpeed() { m_moveSpeed = m_defaultMoveSpeed; }

        // Get the current move speed
        inline float GetMoveSpeed() const { return m_moveSpeed; }

        // Set the current move speed
        inline void SetMoveSpeed( float newSpeed ) { m_moveSpeed = Math::Clamp( newSpeed, s_minSpeed, s_maxSpeed ); }

        // Adjust the move speed by +/- X discrete steps
        void AdjustMoveSpeed( float steps );

        // Move the camera position. Input units are a multiplier of the move speed i.e. 1.0f = full speed. 
        // Note: the camera will not move faster than the move speed no matter the input value
        void MoveCamera( Seconds deltaTime, float forward, float right, float up );

        // Camera Orientation
        //-------------------------------------------------------------------------

        // Orient the camera. Input units are the actual angle delta
        // Note: the camera will not orient faster than the max angular velocity set
        void OrientCamera( Seconds deltaTime, Radians horizontalDelta, Radians verticalDelta );

    private:

        virtual void OnTeleport() override;

    private:

        EE_EXPOSE float                             m_defaultMoveSpeed = 5.0f;
        float                                       m_moveSpeed = 0;
        Vector                                      m_directionChangeAccumulator = Vector::Zero;
        bool                                        m_isEnabled = true;
    };
}