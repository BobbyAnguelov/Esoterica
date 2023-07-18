#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API LightComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( LightComponent );
        friend class RenderDebugView;

    public:

        EE_FORCE_INLINE float GetLightIntensity() const { return m_intensity; }
        EE_FORCE_INLINE Color const& GetLightColor() const { return m_color; }
        EE_FORCE_INLINE Vector const& GetLightPosition() const { return GetWorldTransform().GetTranslation(); }
        EE_FORCE_INLINE bool GetShadowed() const { return m_shadowed; }

    private:

        EE_REFLECT() Color                m_color = Colors::White;
        EE_REFLECT() float                m_intensity = 1.0f;
        EE_REFLECT() bool                 m_shadowed = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API DirectionalLightComponent: public LightComponent
    {
        EE_ENTITY_COMPONENT( DirectionalLightComponent );
        friend class RenderDebugView;

    public:

        EE_FORCE_INLINE Vector GetLightDirection() const { return GetWorldTransform().GetForwardVector(); }

    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PointLightComponent : public LightComponent
    {
        EE_ENTITY_COMPONENT( PointLightComponent );
        friend class RenderDebugView;

    public:

        EE_FORCE_INLINE float GetLightRadius() const { return m_radius; }

    private:

        EE_REFLECT() float                m_radius = 1.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SpotLightComponent : public LightComponent
    {
        EE_ENTITY_COMPONENT( SpotLightComponent );
        friend class RenderDebugView;

    public:

        EE_FORCE_INLINE float GetLightRadius() const { return m_radius; }
        EE_FORCE_INLINE Degrees GetLightInnerUmbraAngle() const { return m_innerUmbraAngle; }
        EE_FORCE_INLINE Degrees GetLightOuterUmbraAngle() const { return m_outerUmbraAngle; }
        EE_FORCE_INLINE Vector GetLightDirection() const { return GetWorldTransform().GetForwardVector(); }

    private:

        EE_REFLECT() Degrees              m_innerUmbraAngle = 0.0f;
        EE_REFLECT() Degrees              m_outerUmbraAngle = 45.0f;
        EE_REFLECT() float                m_radius = 1.0f;

        // TODO: Fall-off parameters
    };
}