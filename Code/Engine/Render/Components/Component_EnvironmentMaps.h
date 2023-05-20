#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "System/Render/RenderTexture.h"
#include "System/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------
namespace EE::Render
{
    class EE_ENGINE_API LocalEnvironmentMapComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( LocalEnvironmentMapComponent );

    public:

        inline CubemapTexture const* GetEnvironmentMapTexture() const { return m_environmentMapTexture.GetPtr(); }

    private:

        EE_REFLECT() TResourcePtr<CubemapTexture> m_environmentMapTexture;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API GlobalEnvironmentMapComponent : public EntityComponent
    {
        EE_SINGLETON_ENTITY_COMPONENT( GlobalEnvironmentMapComponent );

    public:

        inline bool HasSkyboxTexture() const { return m_skyboxTexture.IsLoaded(); }
        inline CubemapTexture const* GetSkyboxTexture() const { return m_skyboxTexture.GetPtr(); }

        inline bool HasSkyboxRadianceTexture() const { return m_skyboxRadianceTexture.IsLoaded(); }
        inline CubemapTexture const* GetSkyboxRadianceTexture() const { return m_skyboxRadianceTexture.GetPtr(); }

        inline float GetSkyboxIntensity() const { return m_skyboxIntensity; }

        //TODO: lighting hack
        inline float GetExposure() const { return m_exposure; }

    private:

        EE_REFLECT() TResourcePtr<CubemapTexture> m_skyboxTexture;
        EE_REFLECT() TResourcePtr<CubemapTexture> m_skyboxRadianceTexture;
        EE_REFLECT() float m_skyboxIntensity = 1.0;
        EE_REFLECT() float m_exposure = -1.0;
    };
}