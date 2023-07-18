#include "WorldSystem_EntityCollectionSpawner.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityMap.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Threading/TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    void EntityCollectionSpawner::ShutdownSystem()
    {
        EE_ASSERT( m_entityCollectionReferences.empty() );
    }

    void EntityCollectionSpawner::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pEntityCollectionComponent = TryCast<EntityCollectionComponent>( pComponent ) )
        {
            m_entityCollectionReferences.Emplace( pEntityCollectionComponent->GetID(), pEntityCollectionComponent );
            m_collectionsToSpawn.emplace_back( pEntityCollectionComponent );
        }
    }

    void EntityCollectionSpawner::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pEntityCollectionComponent = TryCast<EntityCollectionComponent>( pComponent ) )
        {
            CollectionRecord* pRecord = m_entityCollectionReferences.FindItem( pEntityCollectionComponent->GetID() );
            EE_ASSERT( pRecord != nullptr );
            m_entitiesToDestroy.insert( m_entitiesToDestroy.end(), pRecord->m_createdEntities.begin(), pRecord->m_createdEntities.end() );

            m_entityCollectionReferences.Remove( pEntityCollectionComponent->GetID() );
            m_collectionsToSpawn.erase_first( pEntityCollectionComponent );
        }
    }

    //-------------------------------------------------------------------------

    void EntityCollectionSpawner::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        auto pPersistentMap = ctx.GetPersistentMap();

        //-------------------------------------------------------------------------

        if ( !m_collectionsToSpawn.empty() )
        {
            auto pTypeRegistry = ctx.GetSystem<TypeSystem::TypeRegistry>();
            auto pTaskSystem = ctx.GetSystem<TaskSystem>();

            for ( auto pCollectionToSpawn : m_collectionsToSpawn )
            {
                if ( pCollectionToSpawn->GetEntityCollectionDesc() != nullptr )
                {
                    CollectionRecord* pRecord = m_entityCollectionReferences.FindItem( pCollectionToSpawn->GetID() );
                    EE_ASSERT( pRecord != nullptr );

                    TVector<Entity*> createdEntities;
                    pPersistentMap->AddEntityCollection( pTaskSystem, *pTypeRegistry, *pCollectionToSpawn->GetEntityCollectionDesc(), pCollectionToSpawn->GetWorldTransform(), &createdEntities );

                    for ( auto pCreatedEntity : createdEntities )
                    {
                        pRecord->m_createdEntities.emplace_back( pCreatedEntity->GetID() );
                    }
                }
            }

            m_collectionsToSpawn.clear();
        }

        //-------------------------------------------------------------------------

        if ( !m_entitiesToDestroy.empty() )
        {
            for ( auto const& entityID : m_entitiesToDestroy )
            {
                pPersistentMap->DestroyEntity( entityID );
            }

            m_entitiesToDestroy.clear();
        }
    }
}