#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "System/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------
// Player Spawn Component
//-------------------------------------------------------------------------
// This component defines the spawning location for the player in the game world

namespace EE::Player
{
    class EE_ENGINE_API PlayerSpawnComponent : public SpatialEntityComponent
    {
        EE_REGISTER_ENTITY_COMPONENT( PlayerSpawnComponent );

    public:

        inline PlayerSpawnComponent() = default;

        inline EntityModel::SerializedEntityCollection const* GetEntityCollectionDesc() const { return m_pPlayerEntityDesc.GetPtr(); }

    private:

        EE_EXPOSE TResourcePtr<EntityModel::SerializedEntityCollection>    m_pPlayerEntityDesc = nullptr;
    };
}