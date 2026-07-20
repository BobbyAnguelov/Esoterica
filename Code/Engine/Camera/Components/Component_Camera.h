#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Base/Math/Math.h"
#include "Base/Math/ViewVolume.h"

//-------------------------------------------------------------------------
// Camera
//-------------------------------------------------------------------------
// In EE, cameras respect the EE coordinate system (i.e. -Y forward, Z-up )
// This means that they look down the forward axis

namespace EE
{
    class EntityWorldUpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CameraComponent : public SpatialEntityComponent
    {
        EE_SINGLETON_ENTITY_COMPONENT( CameraComponent );

        friend class CameraDebugView;
        friend class CameraSystem;

    public:

        enum class ProjectionType
        {
            EE_REFLECT_ENUM

            Perspective = 0,
            Orthographic
        };

    public:

        using SpatialEntityComponent::SpatialEntityComponent;

        // Get the full view volume for this camera
        inline Math::ViewVolume const& GetViewVolume() const { return m_viewVolume; }

        // Get the view position for this camera
        inline Vector const& GetViewPosition() const { return m_viewVolume.GetViewPosition(); }

        // Get the direction for this camera
        inline Vector const& GetViewDirection() const { return m_viewVolume.GetViewForwardVector(); }

        // Switch to an orthographic projection
        void SwitchToOrthographic( float viewWidth );

        // Switch to an orthographic projection
        void SwitchToPerspective( Radians horizontalFOV );

        // Set the aspect ratio for this camera (Horizontal/Vertical)
        inline void SetAspectRatio( float aspectRatio ) { m_viewVolume.SetAspectRatio( aspectRatio ); }

        // Get the aspect ratio for this camera (Horizontal/Vertical)
        inline float GetAspectRatio() const { return m_viewVolume.GetAspectRatio(); }

        // Set the horizontal field of view
        inline void SetHorizontalFOV( Radians FOV ) { m_viewVolume.SetHorizontalFOV( FOV ); }

        // Set the vertical field of view
        inline void GetHorizontalFOV( Radians FOV ) { m_viewVolume.GetHorizontalFOV(); }

        // Set the vertical field of view
        inline void GetVerticalFOV( Radians FOV ) { m_viewVolume.GetVerticalFOV(); }

        // Set the depth range of this camera in world units
        inline void SetDepthRange( FloatRange depthRange ) { m_viewVolume.SetDepthRange( depthRange ); }

        // Get the depth range of this camera
        inline void GetDepthRange() const { m_viewVolume.GetDepthRange(); }

    protected:

        virtual void Initialize() override;
        virtual void OnWorldTransformUpdated() override;

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Override this function to draw custom imgui controls in the camera debug view
        virtual void DrawDebugUI() {}
        #endif

    protected:

        // Initial Camera Settings - These do not change at runtime, if you want the actual settings, query the view volume
        EE_REFLECT();
        Degrees                                 m_horizontalFOV = 90.0f;

        EE_REFLECT();
        FloatRange                              m_depthRange = FloatRange( 0.1f, 500.0f );

        EE_REFLECT();
        ProjectionType                          m_projectionType = ProjectionType::Perspective;

        // Runtime Data
        //-------------------------------------------------------------------------

        Math::ViewVolume                        m_viewVolume;
        Radians                                 m_yaw = 0;
        Radians                                 m_pitch = 0;
    };
}