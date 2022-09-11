#include "EntitySerialization.h"
#include "Entity.h"
#include "EntityDescriptors.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Profiling.h"
#include "System/Threading/TaskSystem.h"
#include "EntityLog.h"
#include "EntitySystem.h"
#include "EntityMap.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    Entity* Serializer::CreateEntity( TypeSystem::TypeRegistry const& typeRegistry, SerializedEntityDescriptor const& entityDesc )
    {
        EE_ASSERT( entityDesc.IsValid() );

        auto pEntityTypeInfo = Entity::s_pTypeInfo;
        EE_ASSERT( pEntityTypeInfo != nullptr );

        // Create new entity
        //-------------------------------------------------------------------------

        auto pEntity = reinterpret_cast<Entity*>( pEntityTypeInfo->CreateType() );
        pEntity->m_name = entityDesc.m_name;

        #if EE_DEVELOPMENT_TOOLS
        // Restore entity ID if valid
        if ( entityDesc.m_transientEntityID.IsValid() )
        {
            pEntity->m_ID = entityDesc.m_transientEntityID;
        }
        #endif

        // Create entity components
        //-------------------------------------------------------------------------
        // Component descriptors are sorted during compilation, spatial components are first, followed by regular components

        for ( EntityModel::SerializedComponentDescriptor const& componentDesc : entityDesc.m_components )
        {
            auto pEntityComponent = componentDesc.CreateTypeInstance<EntityComponent>( typeRegistry );
            EE_ASSERT( pEntityComponent != nullptr );

            TypeSystem::TypeInfo const* pTypeInfo = pEntityComponent->GetTypeInfo();
            EE_ASSERT( pTypeInfo != nullptr );

            // Set IDs and add to component lists
            pEntityComponent->m_name = componentDesc.m_name;
            pEntityComponent->m_entityID = pEntity->m_ID;
            pEntity->m_components.push_back( pEntityComponent );

            #if EE_DEVELOPMENT_TOOLS
            // Restore component ID if valid
            if ( componentDesc.m_transientComponentID.IsValid() )
            {
                pEntityComponent->m_ID = componentDesc.m_transientComponentID;
            }
            #endif

            //-------------------------------------------------------------------------

            if ( componentDesc.IsSpatialComponent() )
            {
                // Set parent socket ID
                auto pSpatialEntityComponent = static_cast<SpatialEntityComponent*>( pEntityComponent );
                pSpatialEntityComponent->m_parentAttachmentSocketID = componentDesc.m_attachmentSocketID;

                // Set as root component
                if ( componentDesc.IsRootComponent() )
                {
                    EE_ASSERT( pEntity->m_pRootSpatialComponent == nullptr );
                    pEntity->m_pRootSpatialComponent = pSpatialEntityComponent;
                }
            }
        }

        // Create component spatial hierarchy
        //-------------------------------------------------------------------------

        int32_t const numComponents = (int32_t) pEntity->m_components.size();
        for ( int32_t spatialComponentIdx = 0; spatialComponentIdx < entityDesc.m_numSpatialComponents; spatialComponentIdx++ )
        {
            EntityModel::SerializedComponentDescriptor const& spatialComponentDesc = entityDesc.m_components[spatialComponentIdx];
            EE_ASSERT( spatialComponentDesc.IsSpatialComponent() );

            // Skip the root component
            if ( spatialComponentDesc.IsRootComponent() )
            {
                EE_ASSERT( pEntity->GetRootSpatialComponent()->GetNameID() == spatialComponentDesc.m_name );
                continue;
            }

            // Todo: profile this lookup and if it becomes too costly, pre-compute the parent indices and serialize them
            int32_t const parentComponentIdx = entityDesc.FindComponentIndex( spatialComponentDesc.m_spatialParentName );
            EE_ASSERT( parentComponentIdx != InvalidIndex );

            auto pParentSpatialComponent = static_cast<SpatialEntityComponent*>( pEntity->m_components[parentComponentIdx] );
            if ( spatialComponentDesc.m_spatialParentName == pParentSpatialComponent->GetNameID() )
            {
                auto pSpatialComponent = static_cast<SpatialEntityComponent*>( pEntity->m_components[spatialComponentIdx] );
                pSpatialComponent->m_pSpatialParent = pParentSpatialComponent;

                pParentSpatialComponent->m_spatialChildren.emplace_back( pSpatialComponent );
            }
        }

        // Create entity systems
        //-------------------------------------------------------------------------

        for ( auto const& systemDesc : entityDesc.m_systems )
        {
            TypeSystem::TypeInfo const* pTypeInfo = typeRegistry.GetTypeInfo( systemDesc.m_typeID );
            auto pEntitySystem = reinterpret_cast<EntitySystem*>( pTypeInfo->CreateType() );
            EE_ASSERT( pEntitySystem != nullptr );

            pEntity->m_systems.push_back( pEntitySystem );
        }

        // Add to collection
        //-------------------------------------------------------------------------

        return pEntity;
    }

    TVector<Entity*> Serializer::CreateEntities( TaskSystem* pTaskSystem, TypeSystem::TypeRegistry const& typeRegistry, SerializedEntityCollection const& entityCollection )
    {
        EE_PROFILE_SCOPE_ENTITY( "Instantiate Entity Collection" );

        int32_t const numEntitiesToCreate = (int32_t) entityCollection.m_entityDescriptors.size();
        TVector<Entity*> createdEntities;
        createdEntities.resize( numEntitiesToCreate );

        //-------------------------------------------------------------------------

        // For small number of entities, just create them inline!
        if ( pTaskSystem == nullptr || numEntitiesToCreate <= 5 )
        {
            for ( auto i = 0; i < numEntitiesToCreate; i++ )
            {
                createdEntities[i] = CreateEntity( typeRegistry, entityCollection.m_entityDescriptors[i] );
            }
        }
        else // Go wide and create all entities in parallel
        {
            struct EntityCreationTask : public ITaskSet
            {
                EntityCreationTask( TypeSystem::TypeRegistry const& typeRegistry, TVector<SerializedEntityDescriptor> const& descriptors, TVector<Entity*>& createdEntities )
                    : m_typeRegistry( typeRegistry )
                    , m_descriptors( descriptors )
                    , m_createdEntities( createdEntities )
                {
                    m_SetSize = (uint32_t) descriptors.size();
                    m_MinRange = 10;
                }

                virtual void ExecuteRange( TaskSetPartition range, uint32_t threadnum ) override final
                {
                    EE_PROFILE_SCOPE_ENTITY( "Entity Creation Task" );
                    for ( uint64_t i = range.start; i < range.end; ++i )
                    {
                        m_createdEntities[i] = CreateEntity( m_typeRegistry, m_descriptors[i] );
                    }
                }

            private:

                TypeSystem::TypeRegistry const&                     m_typeRegistry;
                TVector<SerializedEntityDescriptor> const&          m_descriptors;
                TVector<Entity*>&                                   m_createdEntities;
            };

            //-------------------------------------------------------------------------

            // Create all entities in parallel
            EntityCreationTask updateTask( typeRegistry, entityCollection.m_entityDescriptors, createdEntities );
            pTaskSystem->ScheduleTask( &updateTask );
            pTaskSystem->WaitForTask( &updateTask );
        }

        // Resolve spatial connections
        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_ENTITY( "Resolve spatial connections" );

            for ( auto const& entityAttachmentInfo : entityCollection.m_entitySpatialAttachmentInfo )
            {
                EE_ASSERT( entityAttachmentInfo.m_entityIdx != InvalidIndex && entityAttachmentInfo.m_parentEntityIdx != InvalidIndex );

                auto const& entityDesc = entityCollection.m_entityDescriptors[entityAttachmentInfo.m_entityIdx];
                EE_ASSERT( entityDesc.HasSpatialParent() );

                // The entity collection compiler will guaranteed that entities are always sorted so that parents are created/initialized before their attached entities
                EE_ASSERT( entityAttachmentInfo.m_parentEntityIdx < entityAttachmentInfo.m_entityIdx );

                //-------------------------------------------------------------------------

                Entity* pEntity = createdEntities[entityAttachmentInfo.m_entityIdx];
                EE_ASSERT( pEntity->IsSpatialEntity() );

                Entity* pParentEntity = createdEntities[entityAttachmentInfo.m_parentEntityIdx];
                EE_ASSERT( pParentEntity->IsSpatialEntity() );

                pEntity->SetSpatialParent( pParentEntity, entityDesc.m_attachmentSocketID );
            }
        }

        //-------------------------------------------------------------------------

        return createdEntities;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    bool Serializer::SerializeEntity( TypeSystem::TypeRegistry const& typeRegistry, Entity const* pEntity, EntityModel::SerializedEntityDescriptor& outDesc )
    {
        EE_ASSERT( !outDesc.IsValid() );
        outDesc.m_name = pEntity->m_name;

        #if EE_DEVELOPMENT_TOOLS
        outDesc.m_transientEntityID = pEntity->m_ID;
        #endif

        // Get spatial parent
        if ( pEntity->HasSpatialParent() )
        {
            outDesc.m_spatialParentName = pEntity->m_pParentSpatialEntity->GetNameID();
            outDesc.m_attachmentSocketID = pEntity->GetAttachmentSocketID();
        }

        // Components
        //-------------------------------------------------------------------------

        auto ComponentComparator = [] ( EntityComponent const* pA, EntityComponent const* pB )
        {
            auto const pSpatialA = TryCast<SpatialEntityComponent>( pA );
            auto const pSpatialB = TryCast<SpatialEntityComponent>( pB );

            // If both are spatial then provide some arbitrary sort
            if ( pSpatialA != nullptr && pSpatialB != nullptr )
            {
                if ( pSpatialA->IsRootComponent() )
                {
                    return true;
                }
                else if ( pSpatialB->IsRootComponent() )
                {
                    return false;
                }

                return (uintptr_t) pA < (uintptr_t) pB;
            }
            // If neither are spatial then provide some arbitrary sort
            else if ( pSpatialA == nullptr && pSpatialB == nullptr )
            {
                return (uintptr_t) pA < (uintptr_t) pB;
            }
            else // Only one is a spatial component
            {
                return pSpatialA != nullptr;
            }
        };

        TInlineVector<EntityComponent*, 10> sortedComponents( pEntity->m_components.begin(), pEntity->m_components.end() );
        eastl::sort( sortedComponents.begin(), sortedComponents.end(), ComponentComparator );

        //-------------------------------------------------------------------------

        TVector<StringID> entityComponentList;
        for ( auto pComponent : sortedComponents )
        {
            EntityModel::SerializedComponentDescriptor componentDesc;
            componentDesc.m_name = pComponent->GetNameID();

            #if EE_DEVELOPMENT_TOOLS
            componentDesc.m_transientComponentID = pComponent->m_ID;
            #endif

            // Check for unique names
            if ( VectorContains( entityComponentList, componentDesc.m_name ) )
            {
                // Duplicate name detected!!
                EE_LOG_ENTITY_ERROR( pEntity, "Entity", "Failed to create entity descriptor, duplicate component name detected: %s on entity %s", pComponent->GetNameID().c_str(), pEntity->GetNameID().c_str() );
                return false;
            }
            else
            {
                entityComponentList.emplace_back( componentDesc.m_name );
            }

            // Spatial info
            auto pSpatialEntityComponent = TryCast<SpatialEntityComponent>( pComponent );
            if ( pSpatialEntityComponent != nullptr )
            {
                if ( !pSpatialEntityComponent->IsRootComponent() )
                {
                    EntityComponent const* pSpatialParentComponent = pEntity->FindComponent( pSpatialEntityComponent->GetSpatialParentID() );
                    componentDesc.m_spatialParentName = pSpatialParentComponent->GetNameID();
                    componentDesc.m_attachmentSocketID = pSpatialEntityComponent->GetAttachmentSocketID();
                }

                componentDesc.m_isSpatialComponent = true;
            }

            // Type descriptor - Properties
            componentDesc.DescribeTypeInstance( typeRegistry, pComponent, true );

            // Add component
            outDesc.m_components.emplace_back( componentDesc );
            if ( componentDesc.m_isSpatialComponent )
            {
                outDesc.m_numSpatialComponents++;
            }
        }

        // Systems
        //-------------------------------------------------------------------------

        for ( EntitySystem const* pSystem : pEntity->m_systems )
        {
            EntityModel::SerializedSystemDescriptor systemDesc;
            systemDesc.m_typeID = pSystem->GetTypeID();
            outDesc.m_systems.emplace_back( systemDesc );
        }

        return true;
    }

    bool Serializer::SerializeEntityMap( TypeSystem::TypeRegistry const& typeRegistry, EntityMap const* pMap, SerializedEntityCollection& outCollection )
    {
        EE_ASSERT( pMap->IsLoaded() || pMap->IsActivated() );
        EE_ASSERT( pMap->m_entitiesToLoad.empty() && pMap->m_entitiesToRemove.empty() );
        EE_ASSERT( pMap->m_entitiesToHotReload.empty() );

        THashMap<StringID, StringID> entityNameMap;
        entityNameMap.reserve( pMap->GetNumEntities() );

        TVector<SerializedEntityDescriptor> entityDescs;
        entityDescs.reserve( pMap->GetNumEntities() );

        for ( auto pEntity : pMap->GetEntities() )
        {
            // Check for unique names - This should never happen but we're paranoid so let's keep the extra validation
            if ( entityNameMap.find( pEntity->GetNameID() ) != entityNameMap.end() )
            {
                EE_LOG_ENTITY_ERROR( pEntity, "Entity", "Failed to create entity collection descriptor, duplicate entity name found: %s", pEntity->GetNameID().c_str() );
                return false;
            }
            else
            {
                entityNameMap.insert( TPair<StringID, StringID>( pEntity->GetNameID(), pEntity->GetNameID() ) );
            }

            //-------------------------------------------------------------------------

            SerializedEntityDescriptor entityDesc;
            if ( !SerializeEntity( typeRegistry, pEntity, entityDesc ) )
            {
                return false;
            }

            entityDescs.emplace_back( entityDesc );
        }

        //-------------------------------------------------------------------------

        outCollection.SetCollectionData( eastl::move( entityDescs ) );
        return true;
    }
    #endif
}