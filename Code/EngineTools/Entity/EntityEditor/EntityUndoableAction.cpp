#include "EntityUndoableAction.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntitySerialization.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityUndoableAction::EntityUndoableAction( TypeSystem::TypeRegistry const& typeRegistry, EntityWorld* pWorld )
        : m_typeRegistry( typeRegistry )
        , m_pWorld( pWorld )
    {
        EE_ASSERT( m_pWorld != nullptr );
    }

    //-------------------------------------------------------------------------

    void EntityUndoableAction::RecordCreate( TVector<Entity*> createdEntities )
    {
        m_actionType = Type::CreateEntities;

        for ( auto pEntity : createdEntities )
        {
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
            auto& serializedState = m_createdEntities.emplace_back();
            serializedState.m_mapID = pEntity->GetMapID();
            bool const result = Serializer::SerializeEntity( m_typeRegistry, pEntity, serializedState.m_desc );
            EE_ASSERT( result );
        }
    }

    void EntityUndoableAction::RecordDelete( TVector<Entity*> entitiesToDelete )
    {
        m_actionType = Type::DeleteEntities;

        for ( auto pEntity : entitiesToDelete )
        {
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
            auto& serializedState = m_deletedEntities.emplace_back();
            serializedState.m_mapID = pEntity->GetMapID();
            bool const result = Serializer::SerializeEntity( m_typeRegistry, pEntity, serializedState.m_desc );
            EE_ASSERT( result );
        }
    }

    //-------------------------------------------------------------------------

    static Entity* GetRootOfSpatialHierarchy( Entity* pEntity )
    {
        EE_ASSERT( pEntity->IsSpatialEntity() );
        auto pRootEntity = pEntity;
        if ( pRootEntity->HasSpatialParent() )
        {
            pRootEntity = pRootEntity->GetSpatialParent();
        }

        return pRootEntity;
    }

    static void AddSpatialHierarchyToBeModified( TVector<Entity*>& editedEntities, Entity* pEntity )
    {
        // Add parent
        //-------------------------------------------------------------------------

        if ( !VectorContains( editedEntities, pEntity ) )
        {
            editedEntities.emplace_back( pEntity );
        }

        // Add Children
        //-------------------------------------------------------------------------

        for ( auto pAttachedEntity : pEntity->GetAttachedEntities() )
        {
            AddSpatialHierarchyToBeModified( editedEntities, pAttachedEntity );
        }
    }

    void EntityUndoableAction::RecordBeginEdit( TVector<Entity*> entitiesToBeModified, bool wereEntitiesDuplicated )
    {
        EE_ASSERT( entitiesToBeModified.size() > 0 );
        EE_ASSERT( m_editedEntities.size() == 0 );

        m_actionType = Type::ModifyEntities;
        m_entitiesWereDuplicated = wereEntitiesDuplicated;

        // Record changes to the entire spatial hierarchy
        for ( auto pEntity : entitiesToBeModified )
        {
            if ( pEntity->IsSpatialEntity() )
            {
                auto pRootEntity = GetRootOfSpatialHierarchy( pEntity );
                EE_ASSERT( pRootEntity != nullptr );
                AddSpatialHierarchyToBeModified( m_editedEntities, pRootEntity );
            }
            else // Just add entity to the modified list
            {
                EE_ASSERT( !VectorContains( m_editedEntities, pEntity ) );
                m_editedEntities.emplace_back( pEntity );
            }
        }

        // Serialize all modified entities
        for ( auto pEntity : m_editedEntities )
        {
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
            auto& serializedState = m_entityDescPreModification.emplace_back();
            serializedState.m_mapID = pEntity->GetMapID();
            bool const result = Serializer::SerializeEntity( m_typeRegistry, pEntity, serializedState.m_desc );
            EE_ASSERT( result );
        }
    }

    void EntityUndoableAction::RecordEndEdit()
    {
        EE_ASSERT( m_actionType == ModifyEntities );
        EE_ASSERT( m_editedEntities.size() > 0 );

        for ( auto pEntity : m_editedEntities )
        {
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
            auto& serializedState = m_entityDescPostModification.emplace_back();
            serializedState.m_mapID = pEntity->GetMapID();
            bool const result = Serializer::SerializeEntity( m_typeRegistry, pEntity, serializedState.m_desc );
            EE_ASSERT( result );
        }

        m_editedEntities.clear();
    }

    //-------------------------------------------------------------------------

    void EntityUndoableAction::Undo()
    {
        switch ( m_actionType )
        {
            case EntityUndoableAction::CreateEntities:
            {
                for ( auto const& serializedEntity : m_createdEntities )
                {
                    auto pMap = m_pWorld->GetMap( serializedEntity.m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    auto pEntity = pMap->FindEntityByName( serializedEntity.m_desc.m_name );
                    pMap->DestroyEntity( pEntity->GetID() );
                }
            }
            break;

            case EntityUndoableAction::DeleteEntities:
            {
                for ( auto const& serializedEntity : m_deletedEntities )
                {
                    auto pEntity = Serializer::CreateEntity( m_typeRegistry, serializedEntity.m_desc );
                    auto pMap = m_pWorld->GetMap( serializedEntity.m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    pMap->AddEntity( pEntity );

                    // Set spatial parent if set
                    if ( serializedEntity.m_desc.HasSpatialParent() )
                    {
                        auto pParentEntity = pMap->FindEntityByName( serializedEntity.m_desc.m_spatialParentName );
                        EE_ASSERT( pParentEntity != nullptr );
                        pEntity->SetSpatialParent( pParentEntity, serializedEntity.m_desc.m_attachmentSocketID );
                    }
                }
            }
            break;

            case EntityUndoableAction::ModifyEntities:
            {
                int32_t const numEntities = (int32_t) m_entityDescPostModification.size();
                EE_ASSERT( m_entityDescPostModification.size() == m_entityDescPreModification.size() );

                // Restore State
                //-------------------------------------------------------------------------

                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    // Remove current entities
                    auto pMap = m_pWorld->GetMap( m_entityDescPostModification[i].m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    auto pExistingEntity = pMap->FindEntityByName( m_entityDescPostModification[i].m_desc.m_name );
                    EE_ASSERT( pExistingEntity != nullptr );
                    pMap->DestroyEntity( pExistingEntity->GetID() );

                    //-------------------------------------------------------------------------

                    // Only recreate the entities if they werent duplicated, if they were duplicate, then deleting them was enough to undo the action
                    if ( !m_entitiesWereDuplicated )
                    {
                        auto pNewEntity = Serializer::CreateEntity( m_typeRegistry, m_entityDescPreModification[i].m_desc );
                        pMap->AddEntity( pNewEntity );
                    }
                }

                // Restore spatial hierarchy
                //-------------------------------------------------------------------------

                if ( !m_entitiesWereDuplicated )
                {
                    for ( int32_t i = 0; i < numEntities; i++ )
                    {
                        auto pMap = m_pWorld->GetMap( m_entityDescPreModification[i].m_mapID );
                        EE_ASSERT( pMap != nullptr );

                        if ( m_entityDescPreModification[i].m_desc.HasSpatialParent() )
                        {
                            auto pParentEntity = pMap->FindEntityByName( m_entityDescPreModification[i].m_desc.m_spatialParentName );
                            EE_ASSERT( pParentEntity != nullptr );
                            auto pChildEntity = pMap->FindEntityByName( m_entityDescPreModification[i].m_desc.m_name );
                            EE_ASSERT( pChildEntity != nullptr );

                            pChildEntity->SetSpatialParent( pParentEntity, m_entityDescPreModification[i].m_desc.m_attachmentSocketID, Entity::SpatialAttachmentRule::KeepLocalTranform );
                        }
                    }
                }
            }
            break;

            default:
            EE_UNREACHABLE_CODE();
            break;
        }
    }

    void EntityUndoableAction::Redo()
    {
        switch ( m_actionType )
        {
            case EntityUndoableAction::CreateEntities:
            {
                for ( auto const& serializedEntity : m_createdEntities )
                {
                    auto pMap = m_pWorld->GetMap( serializedEntity.m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    auto pEntity = Serializer::CreateEntity( m_typeRegistry, serializedEntity.m_desc );
                    pMap->AddEntity( pEntity );

                    // Set spatial parent if set
                    if ( serializedEntity.m_desc.HasSpatialParent() )
                    {
                        auto pParentEntity = pMap->FindEntityByName( serializedEntity.m_desc.m_spatialParentName );
                        EE_ASSERT( pParentEntity != nullptr );
                        pEntity->SetSpatialParent( pParentEntity, serializedEntity.m_desc.m_attachmentSocketID );
                    }
                }
            }
            break;

            case EntityUndoableAction::DeleteEntities:
            {
                for ( auto const& serializedEntity : m_deletedEntities )
                {
                    auto pMap = m_pWorld->GetMap( serializedEntity.m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    auto pEntity = pMap->FindEntityByName( serializedEntity.m_desc.m_name );
                    pMap->DestroyEntity( pEntity->GetID() );
                }
            }
            break;

            case EntityUndoableAction::ModifyEntities:
            {
                int32_t const numEntities = (int32_t) m_entityDescPreModification.size();
                EE_ASSERT( m_entityDescPostModification.size() == m_entityDescPreModification.size() );

                // Restore State
                //-------------------------------------------------------------------------

                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    auto pMap = m_pWorld->GetMap( m_entityDescPreModification[i].m_mapID );
                    EE_ASSERT( pMap != nullptr );

                    // Destroy the current entities (only if there are not duplicates since if they are they wont exist)
                    if ( !m_entitiesWereDuplicated )
                    {
                        auto pExistingEntity = pMap->FindEntityByName( m_entityDescPreModification[i].m_desc.m_name );
                        EE_ASSERT( pExistingEntity != nullptr );
                        pMap->DestroyEntity( pExistingEntity->GetID() );
                    }

                    //-------------------------------------------------------------------------

                    auto pNewEntity = Serializer::CreateEntity( m_typeRegistry, m_entityDescPostModification[i].m_desc );
                    pMap->AddEntity( pNewEntity );
                }

                // Resource spatial hierarchy
                //-------------------------------------------------------------------------

                if ( !m_entitiesWereDuplicated )
                {
                    for ( int32_t i = 0; i < numEntities; i++ )
                    {
                        auto pMap = m_pWorld->GetMap( m_entityDescPostModification[i].m_mapID );
                        EE_ASSERT( pMap != nullptr );

                        if ( m_entityDescPostModification[i].m_desc.HasSpatialParent() )
                        {
                            auto pParentEntity = pMap->FindEntityByName( m_entityDescPostModification[i].m_desc.m_spatialParentName );
                            EE_ASSERT( pParentEntity != nullptr );
                            auto pChildEntity = pMap->FindEntityByName( m_entityDescPostModification[i].m_desc.m_name );
                            EE_ASSERT( pChildEntity != nullptr );

                            pChildEntity->SetSpatialParent( pParentEntity, m_entityDescPostModification[i].m_desc.m_attachmentSocketID, Entity::SpatialAttachmentRule::KeepLocalTranform );
                        }
                    }
                }
            }
            break;

            default:
            EE_UNREACHABLE_CODE();
            break;
        }
    }
}
