#pragma once

#include "Component_Camera.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINE_API OrbitCameraComponent : public CameraComponent
    {
        EE_REGISTER_ENTITY_COMPONENT( OrbitCameraComponent );

        friend class CameraDebugView;

    public:

        inline OrbitCameraComponent() = default;

        virtual void Initialize() override;

        // Update camera position based on the orbit target
        void FinalizeCameraPosition();

        Vector GetCameraRelativeForwardVector() const { return GetWorldTransform().GetForwardVector(); }
        Vector GetCameraRelativeRightVector() const { return GetWorldTransform().GetRightVector(); }
        Vector GetCameraRelativeUpVector() const { return GetWorldTransform().GetUpVector(); }

        void AdjustOrbitAngle( Radians deltaHorizontalAngle, Radians deltaVerticalAngle );
        void AdjustOrbitTargetOffset( Float3 newTargetOffset );
        void AdjustOrbitDistance( float newDistance );

        void ResetCamera();

    protected:

        void CalculateCameraTransform();

    protected:

        EE_EXPOSE Float3                m_defaultOrbitTargetOffset = Float3::Zero;
        EE_EXPOSE float                 m_defaultOrbitDistance = 2.f;

        Radians                         m_yaw = 0;
        Radians                         m_pitch = 0;
        Transform                       m_orbitTransform; // The orbit transform (local to the orbit world space point)

        Float3                          m_orbitTargetOffset = Float3::Zero;
        float                           m_orbitDistance = 2.f;
    };
}