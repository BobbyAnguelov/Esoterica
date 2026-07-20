#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Render/RenderTexture.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API LocalEnvironmentMapComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( LocalEnvironmentMapComponent );

    public:

        inline TextureResource const* GetEnvironmentMapTexture() const { return m_environmentMapTexture.GetPtr(); }

    private:

        EE_REFLECT()
        TResourcePtr<TextureResource> m_environmentMapTexture;
    };
}
