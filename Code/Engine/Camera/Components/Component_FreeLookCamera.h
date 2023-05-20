#pragma once

#include "Component_Camera.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINE_API FreeLookCameraComponent : public CameraComponent
    {
        EE_ENTITY_COMPONENT( FreeLookCameraComponent );

        friend class CameraDebugView;

    public:

        inline FreeLookCameraComponent() = default;
        inline FreeLookCameraComponent( StringID name ) : CameraComponent( name ) {}

        inline Radians GetPitch() const { return m_pitch; }
        inline Radians GetYaw() const { return m_yaw; }

        // Perform local adjustments to the camera's heading and pitch
        void AdjustPitchAndYaw( Radians headingDelta, Radians pitchDelta );

        // Reset the camera view to the default
        void ResetView();

        // Focus the camera to the specified bounds
        void FocusOn( OBB const& bounds );

        // Focus the camera to the specified bounds
        void FocusOn( AABB const& bounds );

        // Set the camera world position and world look at target
        void SetPositionAndLookAtTarget( Vector const& cameraPosition, Vector const& lookatTarget );

        // Set the camera world position and view direction
        void SetPositionAndLookAtDirection( Vector const& cameraPosition, Vector const& lookatDir );

    private:

        virtual void Initialize() override;
        virtual void OnWorldTransformUpdated() override;

        // Called whenever we explicitly set the camera position (so that derived classes can clear any buffered data)
        virtual void OnTeleport() {}

    protected:

        Radians                         m_yaw = 0;
        Radians                         m_pitch = 0;
    };
}