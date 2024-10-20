#include "EntityDescriptors.h"

#include "Entity.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Profiling.h"
#include "Base/Threading/TaskSystem.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    bool IsResourceAnEntityDescriptor( ResourceTypeID const& resourceTypeID )
    {
        return ( resourceTypeID == EntityModel::EntityMapDescriptor::GetStaticResourceTypeID() || resourceTypeID == EntityModel::EntityCollection::GetStaticResourceTypeID() );
    }

    //-------------------------------------------------------------------------
    // Entity Descriptor
    //-------------------------------------------------------------------------

    int32_t EntityDescriptor::FindComponentIndex( StringID const& componentName ) const
    {
        EE_ASSERT( componentName.IsValid() );

        int32_t const numComponents = (int32_t) m_components.size();
        for ( int32_t i = 0; i < numComponents; i++ )
        {
            if ( m_components[i].m_name == componentName )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    Entity* EntityDescriptor::CreateEntity( TypeSystem::TypeRegistry const& typeRegistry ) const
    {
        EE_ASSERT( IsValid() );

        auto pEntityTypeInfo = Entity::s_pTypeInfo;
        EE_ASSERT( pEntityTypeInfo != nullptr );

        // Create new entity
        //-------------------------------------------------------------------------

        auto pEntity = reinterpret_cast<Entity*>( pEntityTypeInfo->CreateType() );
        pEntity->m_name = m_name;

        #if EE_DEVELOPMENT_TOOLS
        // Restore entity ID if valid
        if ( m_transientEntityID.IsValid() )
        {
            pEntity->m_ID = m_transientEntityID;
        }
        #endif

        // Create entity components
        //-------------------------------------------------------------------------
        // Component descriptors are sorted during compilation, spatial components are first, followed by regular components

        for ( EntityModel::ComponentDescriptor const& componentDesc : m_components )
        {
            TypeSystem::TypeInfo const* pTypeInfo = typeRegistry.GetTypeInfo( componentDesc.m_typeID );
            if ( pTypeInfo == nullptr )
            {
                continue;
            }

            auto pEntityComponent = componentDesc.CreateType<EntityComponent>( typeRegistry );
            EE_ASSERT( pEntityComponent != nullptr );

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
                auto pSpatialEntityComponent = reinterpret_cast<SpatialEntityComponent*>( pEntityComponent );
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

        for ( int32_t spatialComponentIdx = 0; spatialComponentIdx < m_numSpatialComponents; spatialComponentIdx++ )
        {
            EntityModel::ComponentDescriptor const& spatialComponentDesc = m_components[spatialComponentIdx];
            EE_ASSERT( spatialComponentDesc.IsSpatialComponent() );

            // Skip the root component
            if ( spatialComponentDesc.IsRootComponent() )
            {
                EE_ASSERT( pEntity->GetRootSpatialComponent()->GetNameID() == spatialComponentDesc.m_name );
                continue;
            }

            // Todo: profile this lookup and if it becomes too costly, pre-compute the parent indices and serialize them
            int32_t const parentComponentIdx = FindComponentIndex( spatialComponentDesc.m_spatialParentName );
            EE_ASSERT( parentComponentIdx != InvalidIndex );

            auto pParentSpatialComponent = reinterpret_cast<SpatialEntityComponent*>( pEntity->m_components[parentComponentIdx] );
            if ( spatialComponentDesc.m_spatialParentName == pParentSpatialComponent->GetNameID() )
            {
                auto pSpatialComponent = reinterpret_cast<SpatialEntityComponent*>( pEntity->m_components[spatialComponentIdx] );
                pSpatialComponent->m_pSpatialParent = pParentSpatialComponent;

                pParentSpatialComponent->m_spatialChildren.emplace_back( pSpatialComponent );
            }
        }

        // Ensure that all world transforms are up to date!
        //-------------------------------------------------------------------------

        if ( pEntity->IsSpatialEntity() )
        {
            pEntity->GetRootSpatialComponent()->CalculateWorldTransform( false );
        }

        // Create entity systems
        //-------------------------------------------------------------------------

        for ( auto const& systemDesc : m_systems )
        {
            TypeSystem::TypeInfo const* pTypeInfo = typeRegistry.GetTypeInfo( systemDesc.m_typeID );
            if ( pTypeInfo == nullptr )
            {
                continue;
            }

            auto pEntitySystem = reinterpret_cast<EntitySystem*>( pTypeInfo->CreateType() );
            EE_ASSERT( pEntitySystem != nullptr );
            pEntity->m_systems.push_back( pEntitySystem );
        }

        // Add to collection
        //-------------------------------------------------------------------------

        return pEntity;
    }

    #if EE_DEVELOPMENT_TOOLS
    void EntityDescriptor::ClearAllSerializedIDs()
    {
        m_transientEntityID.Clear();

        for ( auto& component : m_components )
        {
            component.m_transientComponentID.Clear();
        }
    }
    #endif

    //-------------------------------------------------------------------------
    // Entity Collection
    //-------------------------------------------------------------------------

    TVector<EntityCollection::SearchResult> EntityCollection::GetComponentsOfType( TypeSystem::TypeRegistry const& typeRegistry, TypeSystem::TypeID typeID, bool allowDerivedTypes )
    {
        TVector<SearchResult> foundComponents;

        for ( auto& entityDesc : m_entityDescriptors )
        {
            for ( auto& componentDesc : entityDesc.m_components )
            {
                if ( componentDesc.m_typeID == typeID )
                {
                    auto& result = foundComponents.emplace_back( SearchResult() );
                    result.m_pEntity = &entityDesc;
                    result.m_pComponent = &componentDesc;
                }
                else if ( allowDerivedTypes )
                {
                    auto pTypeInfo = typeRegistry.GetTypeInfo( componentDesc.m_typeID );
                    EE_ASSERT( pTypeInfo != nullptr );

                    if ( pTypeInfo->IsDerivedFrom( typeID ) )
                    {
                        auto& result = foundComponents.emplace_back( SearchResult() );
                        result.m_pEntity = &entityDesc;
                        result.m_pComponent = &componentDesc;
                    }
                }
            }
        }

        //-------------------------------------------------------------------------

        return foundComponents;
    }

    TVector<Entity*> EntityCollection::CreateEntities( TypeSystem::TypeRegistry const& typeRegistry, TaskSystem* pTaskSystem ) const
    {
        EE_PROFILE_SCOPE_ENTITY( "Instantiate Entity Collection" );

        int32_t const numEntitiesToCreate = (int32_t) m_entityDescriptors.size();
        TVector<Entity*> createdEntities;
        createdEntities.resize( numEntitiesToCreate );

        //-------------------------------------------------------------------------

        // For small number of entities, just create them inline!
        if ( pTaskSystem == nullptr || numEntitiesToCreate <= 5 )
        {
            for ( auto i = 0; i < numEntitiesToCreate; i++ )
            {
                createdEntities[i] = m_entityDescriptors[i].CreateEntity( typeRegistry );
            }
        }
        else // Go wide and create all entities in parallel
        {
            struct EntityCreationTask : public ITaskSet
            {
                EntityCreationTask( TypeSystem::TypeRegistry const& typeRegistry, TVector<EntityDescriptor> const& descriptors, TVector<Entity*>& createdEntities )
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
                        m_createdEntities[i] = m_descriptors[i].CreateEntity( m_typeRegistry );
                    }
                }

            private:

                TypeSystem::TypeRegistry const&                     m_typeRegistry;
                TVector<EntityDescriptor> const&          m_descriptors;
                TVector<Entity*>&                                   m_createdEntities;
            };

            //-------------------------------------------------------------------------

            // Create all entities in parallel
            EntityCreationTask updateTask( typeRegistry, m_entityDescriptors, createdEntities );
            pTaskSystem->ScheduleTask( &updateTask );
            pTaskSystem->WaitForTask( &updateTask );
        }

        // Resolve spatial connections
        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_ENTITY( "Resolve spatial connections" );

            for ( auto const& entityAttachmentInfo : m_entitySpatialAttachmentInfo )
            {
                EE_ASSERT( entityAttachmentInfo.m_entityIdx != InvalidIndex && entityAttachmentInfo.m_parentEntityIdx != InvalidIndex );

                auto const& entityDesc = m_entityDescriptors[entityAttachmentInfo.m_entityIdx];
                EE_ASSERT( entityDesc.HasSpatialParent() );

                // The entity collection compiler will guaranteed that entities are always sorted so that parents are created/initialized before their attached entities
                EE_ASSERT( entityAttachmentInfo.m_parentEntityIdx < entityAttachmentInfo.m_entityIdx );

                //-------------------------------------------------------------------------

                Entity* pEntity = createdEntities[entityAttachmentInfo.m_entityIdx];
                EE_ASSERT( pEntity->IsSpatialEntity() );

                Entity* pParentEntity = createdEntities[entityAttachmentInfo.m_parentEntityIdx];
                EE_ASSERT( pParentEntity->IsSpatialEntity() );

                pEntity->SetSpatialParent( pParentEntity, entityDesc.m_attachmentSocketID, Entity::SpatialAttachmentRule::KeepLocalTranform );
            }
        }

        //-------------------------------------------------------------------------

        return createdEntities;
    }

    void EntityCollection::RebuildLookupMap()
    {
        m_entityLookupMap.clear();

        int32_t const numEntities = (int32_t) m_entityDescriptors.size();
        m_entityLookupMap.reserve( numEntities );

        for ( int32_t i = 0; i < numEntities; i++ )
        {
            m_entityLookupMap.insert( TPair<StringID, int32_t>( m_entityDescriptors[i].m_name, i ) );
        }
    }

    #if EE_DEVELOPMENT_TOOLS
    void EntityCollection::Clear()
    {
        m_entityDescriptors.clear();
        m_entityLookupMap.clear();
        m_entitySpatialAttachmentInfo.clear();
    }

    void EntityCollection::SetCollectionData( TVector<EntityDescriptor>&& entityDescriptors )
    {
        // Set entity descriptors
        //-------------------------------------------------------------------------

        m_entityDescriptors.swap( entityDescriptors );
        int32_t const numEntities = (int32_t) m_entityDescriptors.size();

        // Generate spatial hierarchy depths
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < numEntities; i++ )
        {
            auto& entityDesc = m_entityDescriptors[i];
            EE_ASSERT( entityDesc.IsValid() );
            entityDesc.m_spatialHierarchyDepth = 0;

            if ( entityDesc.IsSpatialEntity() )
            {
                // Calculate the spatial hierarchy depth
                StringID parentID = entityDesc.m_spatialParentName;
                while ( parentID.IsValid() )
                {
                    entityDesc.m_spatialHierarchyDepth++;

                    int32_t const parentIdx = m_entityLookupMap[parentID];
                    EE_ASSERT( parentIdx != InvalidIndex );
                    parentID = m_entityDescriptors[parentIdx].m_spatialParentName;
                }
            }
        }

        // Sort entity desc array by depth
        //-------------------------------------------------------------------------

        auto SortComparator = [] ( EntityDescriptor const& entityA, EntityDescriptor const& entityB )
        {
            EE_ASSERT( entityA.m_spatialHierarchyDepth >= 0 && entityA.m_spatialHierarchyDepth >= 0 );
            return entityA.m_spatialHierarchyDepth < entityB.m_spatialHierarchyDepth;
        };

        eastl::sort( m_entityDescriptors.begin(), m_entityDescriptors.end(), SortComparator );

        // Create lookup map
        //-------------------------------------------------------------------------

        RebuildLookupMap();

        // Generate spatial attachment info
        //-------------------------------------------------------------------------

        m_entitySpatialAttachmentInfo.clear();
        m_entitySpatialAttachmentInfo.reserve( m_entityDescriptors.size() );

        for ( int32_t i = 0; i < numEntities; i++ )
        {
            auto const& entityDesc = m_entityDescriptors[i];
            if ( !entityDesc.IsSpatialEntity() || !entityDesc.HasSpatialParent() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            SpatialAttachmentInfo attachmentInfo;
            attachmentInfo.m_entityIdx = i;
            attachmentInfo.m_parentEntityIdx = FindEntityIndex( entityDesc.m_spatialParentName );

            if ( attachmentInfo.m_parentEntityIdx != InvalidIndex )
            {
                m_entitySpatialAttachmentInfo.push_back( attachmentInfo );
            }
        }
    }

    void EntityCollection::GetAllReferencedResources( TVector<ResourceID>& outReferencedResources ) const
    {
        outReferencedResources.clear();
        for ( auto const& entityDesc : m_entityDescriptors )
        {
            for ( auto const& resID : entityDesc.m_referencedResources )
            {
                outReferencedResources.emplace_back( resID );
            }
        }
    }
    #endif
}