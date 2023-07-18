#pragma once

#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EE_ENGINE_API EntityCollectionComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( EntityCollectionComponent );

    public:

        inline EntityCollectionComponent() = default;

        inline EntityModel::SerializedEntityCollection const* GetEntityCollectionDesc() const 
        {
            if ( !m_entityCollectionDesc.IsSet() )
            {
                return nullptr;
            }

            return m_entityCollectionDesc.IsLoaded() ? m_entityCollectionDesc.GetPtr() : nullptr;
        }

    private:

        EE_REFLECT();
        TResourcePtr<EntityModel::SerializedEntityCollection> m_entityCollectionDesc;
    };
}