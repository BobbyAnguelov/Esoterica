#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityComponent.h"
#include "Base/Math/Transform.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class EE_ENGINE_API NavmeshTesterComponent : public EntityComponent
    {
        EE_ENTITY_COMPONENT( NavmeshTesterComponent );
        friend class NavmeshWorldSystem;
        friend class NavmeshTesterComponentVisualizer;

    public:

        using EntityComponent::EntityComponent;

    private:

        EE_REFLECT();
        Transform m_startTransform;

        EE_REFLECT();
        Transform m_endTransform;
    };
}