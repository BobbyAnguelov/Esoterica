#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------
// Player Spawn Component
//-------------------------------------------------------------------------
// This component defines the spawning location for the player in the game world

namespace EE::Player
{
    class EE_ENGINE_API PlayerSpawnComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( PlayerSpawnComponent );

    public:

        inline PlayerSpawnComponent() = default;

        inline EntityModel::SerializedEntityCollection const* GetEntityCollectionDesc() const { return m_pPlayerEntityDesc.GetPtr(); }

    private:

        EE_REFLECT() TResourcePtr<EntityModel::SerializedEntityCollection>    m_pPlayerEntityDesc = nullptr;
    };
}