#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Entity/EntityWorldSystemSignal.h"
#include "Engine/Render/RenderProxies.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class RenderWorldSystem;
    class DeviceRenderWorld;

    class EE_ENGINE_API LightComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( LightComponent );
        friend class RenderDebugView;

    public:

        inline float GetTemperature() const { return m_temperature; }
        inline float GetMaxIntensity() const { return m_maxIntensity; }
        inline Color const& GetTint() const { return m_tint; }
        inline Vector const& GetLightPosition() const { return GetWorldTransform().GetTranslation(); }
        inline bool GetShadowed() const { return m_shadowed; }

    private:

        EE_REFLECT();
        float                                               m_temperature = 6500.0F;                // Kelvins, blackbody temperature

        EE_REFLECT();
        float                                               m_maxIntensity = 10.0F;                 // Peak intensity at light center

        EE_REFLECT();
        Color                                               m_tint = Color( 255, 255, 255, 0 );     // Artistic color tint

        EE_REFLECT();
        bool                                                m_shadowed = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API DirectionalLightComponent : public LightComponent
    {
        EE_ENTITY_COMPONENT( DirectionalLightComponent );
        friend class RenderDebugView;
        friend class RenderWorldSystem;

    public:

        inline Vector GetLightDirection() const { return GetWorldTransform().GetForwardVector(); }

    protected:

        virtual void OnWorldTransformUpdated() override;

        // Internal renderer data
        //---------------------------------------------------------------------

        uint16_t                                            m_cascadedShadowIndex = 0xFFFF;
        LightInstanceProxy                                  m_lightInstanceProxy = {};
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PointLightComponent : public LightComponent
    {
        EE_ENTITY_COMPONENT( PointLightComponent );
        friend class RenderDebugView;
        friend class RenderWorldSystem;

    public:

        inline float GetMaxRadius() const { return m_maxRadius; }
        inline float GetFalloff() const { return m_falloff; }

    protected:

        virtual void OnWorldTransformUpdated() override;

    private:

        EE_REFLECT();
        float                                               m_maxRadius = 10.0F;    // Distance where light intensity reaches zero

        EE_REFLECT();
        float                                               m_falloff = 2.0F;       // Decay rate

        // Internal renderer data
        //---------------------------------------------------------------------

        LightInstanceProxy                                  m_lightInstanceProxy = {};
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SpotLightComponent : public LightComponent
    {
        EE_ENTITY_COMPONENT( SpotLightComponent );
        friend class RenderDebugView;
        friend class RenderWorldSystem;

    public:

        inline Degrees GetBeamAngle() const { return m_beamAngle; }
        inline float GetBlend() const { return m_blend; }
        inline float GetMaxRadius() const { return m_maxRadius; }
        inline float GetFalloff() const { return m_falloff; }
        inline Vector GetLightDirection() const { return GetWorldTransform().GetForwardVector(); }

    protected:

        virtual void OnWorldTransformUpdated() override;

    private:

        EE_REFLECT();
        Degrees                                         m_beamAngle = 45.0F;       // Total cone angle in degrees

        EE_REFLECT();
        float                                           m_blend = 0.15F;           // 0..1 edge softness (0 = hard, 1 = fully soft)

        EE_REFLECT();
        float                                           m_maxRadius = 10.0F;       // Distance where light intensity reaches zero

        EE_REFLECT();
        float                                           m_falloff = 2.0F;          // Decay rate

    private:

        // Internal renderer data
        //---------------------------------------------------------------------

        LightInstanceProxy                              m_lightInstanceProxy = {};
    };
}
