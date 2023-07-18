#pragma once

#include "Engine/Entity/Components/Component_EntityCollection.h"
#include "Engine/Entity/EntityWorldSystem.h"
#include "Base/Types/IDVector.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EE_ENGINE_API EntityCollectionSpawner : public EntityWorldSystem
    {
        struct CollectionRecord
        {
            CollectionRecord( EntityCollectionComponent* pComponent )
                : m_pComponent( pComponent )
            {}

            inline ComponentID GetID() const { return m_pComponent->GetID(); }

        public:

            EntityCollectionComponent*              m_pComponent = nullptr;
            TVector<EntityID>                       m_createdEntities;
        };

    public:

        EE_ENTITY_WORLD_SYSTEM( EntityCollectionSpawner, RequiresUpdate( UpdateStage::PrePhysics ) );

    private:

        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

    private:

        TIDVector<ComponentID, CollectionRecord>    m_entityCollectionReferences;
        TVector<EntityCollectionComponent*>         m_collectionsToSpawn;
        TVector<EntityID>                           m_entitiesToDestroy;
    };
} 