#pragma once
#include "Engine/Entity/EntitySpatialComponent.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsWorld;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PhysicsTestComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( PhysicsTestComponent );
    };
}