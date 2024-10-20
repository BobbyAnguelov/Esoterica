#pragma once

#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    class EE_ENGINE_API AISpawnComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( AISpawnComponent );

    public:

        inline AISpawnComponent() = default;

        inline EntityModel::EntityCollection const* GetEntityCollectionDesc() const
        {
            if ( !m_AIEntityDesc.IsSet() )
            {
                return nullptr;
            }

            return m_AIEntityDesc.IsLoaded() ? m_AIEntityDesc.GetPtr() : nullptr;
        }

    private:

        EE_REFLECT() TResourcePtr<EntityModel::EntityCollection>    m_AIEntityDesc;
    };
}