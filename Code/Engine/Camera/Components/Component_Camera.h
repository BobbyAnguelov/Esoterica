#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "System/Math/Math.h"
#include "System/Math/ViewVolume.h"
#include "System/Math/Rectangle.h"

//-------------------------------------------------------------------------
// Camera
//-------------------------------------------------------------------------
// In EE, cameras respect the EE coordinate system (i.e. -Y forward, Z-up )
// This means that they look down the forward axis

namespace EE
{
    class EE_ENGINE_API CameraComponent : public SpatialEntityComponent
    {
        EE_REGISTER_SINGLETON_ENTITY_COMPONENT( CameraComponent );

        friend class CameraDebugView;

    public:

        enum class ProjectionType
        {
            EE_REGISTER_ENUM

            Perspective = 0,
            Orthographic
        };

    public:

        inline CameraComponent() = default;

        inline void UpdateViewDimensions( Float2 const& viewDimensions ) { m_viewVolume.SetViewDimensions( viewDimensions ); }
        inline Math::ViewVolume const& GetViewVolume() const { return m_viewVolume; }

        inline void GetDepthRange() const { m_viewVolume.GetDepthRange(); }
        inline void SetDepthRange( FloatRange depthRange ) { m_viewVolume.SetDepthRange( depthRange ); }

        // Set the horizontal field of view
        inline void SetHorizontalFOV( Radians FOV ) { m_viewVolume.SetHorizontalFOV( FOV ); }

    protected:

        using SpatialEntityComponent::SpatialEntityComponent;

        virtual void Initialize() override;
        virtual void OnWorldTransformUpdated() override;

    protected:

        // Initial Camera Settings - These do not change at runtime, if you want the actual settings, query the view volume
        EE_EXPOSE Degrees               m_FOV = 90.0f;
        EE_EXPOSE FloatRange            m_depthRange = FloatRange( 0.1f, 500.0f );
        EE_EXPOSE ProjectionType        m_projectionType = ProjectionType::Perspective;

        // Runtime Data
        Math::ViewVolume                m_viewVolume;
    };
}