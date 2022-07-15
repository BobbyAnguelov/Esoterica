#include "Component_Camera.h"

//-------------------------------------------------------------------------

namespace EE
{
    void CameraComponent::Initialize()
    {
        SpatialEntityComponent::Initialize();
        m_viewVolume = Math::ViewVolume( Float2( 1.0f ), m_depthRange, Radians( m_FOV ), GetWorldTransform().ToMatrix() );
    }

    //-------------------------------------------------------------------------

    void CameraComponent::OnWorldTransformUpdated()
    {
        Transform const& worldTransform = GetWorldTransform();
        m_viewVolume.SetView( worldTransform.GetTranslation(), worldTransform.GetForwardVector(), worldTransform.GetUpVector() );
    }
}