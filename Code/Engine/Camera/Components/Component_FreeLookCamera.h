#pragma once

#include "Component_Camera.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINE_API FreeLookCameraComponent : public CameraComponent
    {
        EE_REGISTER_ENTITY_COMPONENT( FreeLookCameraComponent );

        friend class CameraDebugView;

    public:

        inline FreeLookCameraComponent() = default;
        inline FreeLookCameraComponent( StringID name ) : CameraComponent( name ) {}

        inline Radians GetPitch() const { return m_pitch; }
        inline Radians GetYaw() const { return m_yaw; }

        // Perform local adjustments to the camera's heading and pitch
        void AdjustPitchAndYaw( Radians headingDelta, Radians pitchDelta );

        // Set the camera world position and world look at target
        void SetPositionAndLookatTarget( Vector const& cameraPosition, Vector const& lookatTarget );

    protected:

        Radians                         m_yaw = 0;
        Radians                         m_pitch = 0;
    };
}