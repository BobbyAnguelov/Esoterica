#pragma once

#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "System/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    class EE_ENGINE_API AISpawnComponent : public SpatialEntityComponent
    {
        EE_REGISTER_ENTITY_COMPONENT( AISpawnComponent );

    public:

        inline AISpawnComponent() = default;

        inline EntityModel::EntityCollectionDescriptor const* GetEntityCollectionDesc() const { return m_pAIEntityDesc.GetPtr(); }

    private:

        EE_EXPOSE TResourcePtr<EntityModel::EntityCollectionDescriptor>    m_pAIEntityDesc = nullptr;
    };
}