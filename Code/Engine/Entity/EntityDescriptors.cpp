#include "EntityDescriptors.h"
#include "System/Log.h"
#include "Entity.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Profiling.h"
#include "System/Threading/TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
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
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    void EntityCollectionDescriptor::Reserve( int32_t numEntities )
    {
        m_entityDescriptors.reserve( numEntities );
        m_entityLookupMap.reserve( numEntities );
    }

    void EntityCollectionDescriptor::GenerateSpatialAttachmentInfo()
    {
        m_entitySpatialAttachmentInfo.clear();
        m_entitySpatialAttachmentInfo.reserve( m_entityDescriptors.size() );

        int32_t const numEntities = (int32_t) m_entityDescriptors.size();
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

    TVector<EntityCollectionDescriptor::SearchResult> EntityCollectionDescriptor::GetComponentsOfType( TypeSystem::TypeRegistry const& typeRegistry, TypeSystem::TypeID typeID, bool allowDerivedTypes )
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

    //-------------------------------------------------------------------------

    TVector<Entity*> EntityCollectionDescriptor::InstantiateCollection( TaskSystem* pTaskSystem, TypeSystem::TypeRegistry const& typeRegistry ) const
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
                createdEntities[i] = Entity::CreateFromDescriptor( typeRegistry, m_entityDescriptors[i] );
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
                        m_createdEntities[i] = Entity::CreateFromDescriptor( m_typeRegistry, m_descriptors[i] );
                    }
                }

            private:

                TypeSystem::TypeRegistry const&         m_typeRegistry;
                TVector<EntityDescriptor> const&        m_descriptors;
                TVector<Entity*>&                       m_createdEntities;
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

                pEntity->SetSpatialParent( pParentEntity, entityDesc.m_attachmentSocketID );
            }
        }

        //-------------------------------------------------------------------------

        return createdEntities;
    }

    #if EE_DEVELOPMENT_TOOLS
    void EntityCollectionDescriptor::GetAllReferencedResources( TVector<ResourceID>& outReferencedResources ) const
    {
        outReferencedResources.clear();

        TypeSystem::TypeID const resourceIDTypeID = TypeSystem::CoreTypeRegistry::GetTypeID( TypeSystem::CoreTypeID::ResourceID );
        TypeSystem::TypeID const resourcePathTypeID = TypeSystem::CoreTypeRegistry::GetTypeID( TypeSystem::CoreTypeID::ResourcePath );
        TypeSystem::TypeID const resourcePtrTypeID = TypeSystem::CoreTypeRegistry::GetTypeID( TypeSystem::CoreTypeID::ResourcePtr );
        TypeSystem::TypeID const templateResourcePtrTypeID = TypeSystem::CoreTypeRegistry::GetTypeID( TypeSystem::CoreTypeID::TResourcePtr );

        for ( auto const& entityDesc : m_entityDescriptors )
        {
            for ( auto const& componentDesc : entityDesc.m_components )
            {
                for ( auto const& propertyDesc : componentDesc.m_properties )
                {
                    if ( propertyDesc.m_typeID == resourceIDTypeID || propertyDesc.m_typeID == resourcePathTypeID || propertyDesc.m_typeID == resourcePtrTypeID || propertyDesc.m_typeID == templateResourcePtrTypeID )
                    {
                        VectorEmplaceBackUnique<ResourceID>( outReferencedResources, ResourceID( propertyDesc.m_stringValue ) );
                    }
                }
            }
        }
    }
    #endif
}