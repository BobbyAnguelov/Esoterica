#include "Component_Camera.h"

//-------------------------------------------------------------------------

namespace EE
{
    void CameraComponent::Initialize()
    {
        SpatialEntityComponent::Initialize();
        m_viewVolume = Math::ViewVolume::CreatePerspective( Math::ViewVolume::AspectRatio_4_3, m_depthRange, Radians( m_horizontalFOV ), GetWorldTransform().ToMatrix() );
    }

    void CameraComponent::SwitchToOrthographic( float viewWidth )
    {
        float const aspectRatio = GetAspectRatio();
        FloatRange const depthRange = m_viewVolume.GetDepthRange();
        Transform const worldTransform = GetWorldTransform();

        m_viewVolume = Math::ViewVolume::CreateOrthographic( viewWidth, aspectRatio, depthRange, worldTransform );
    }

    void CameraComponent::SwitchToPerspective( Radians horizontalFOV )
    {
        float const aspectRatio = GetAspectRatio();
        FloatRange const depthRange = m_viewVolume.GetDepthRange();
        Transform const worldTransform = GetWorldTransform();

        m_viewVolume = Math::ViewVolume::CreatePerspective( aspectRatio, depthRange, horizontalFOV, worldTransform );
    }

    void CameraComponent::OnWorldTransformUpdated()
    {
        Transform const& worldTransform = GetWorldTransform();
        m_viewVolume.SetView( worldTransform.GetTranslation(), worldTransform.GetForwardVector(), worldTransform.GetUpVector() );
    }
}