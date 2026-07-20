#pragma once

#include "Game/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------
// Spawn Point Component
//-------------------------------------------------------------------------

namespace EE
{
    class EE_GAME_API SpawnPointComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( SpawnPointComponent );

    public:

        enum class Type
        {
            EE_REFLECT_ENUM
            Player,
            NPC
        };

    public:

        inline SpawnPointComponent() = default;

        inline bool IsEnabled() const { return m_enabled; }
        inline bool IsPlayerSpawn() const { return m_type == Type::Player; }
        inline bool IsNPCSpawn() const { return m_type == Type::NPC; }

        inline bool IsEntityCollectionSet() const { return m_entityDesc.IsSet(); }

        inline EntityModel::EntityCollection const* GetEntityCollectionDesc() const
        {
            return m_entityDesc.GetPtr();
        }

    private:

        EE_REFLECT();
        TResourcePtr<EntityModel::EntityCollection>     m_entityDesc = nullptr;

        EE_REFLECT();
        Type                                            m_type = Type::Player;

        EE_REFLECT();
        bool                                            m_enabled = true;
    };
}