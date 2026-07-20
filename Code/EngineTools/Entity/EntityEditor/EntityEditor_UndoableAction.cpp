#include "EntityEditor_UndoableAction.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EntityEditor_Context.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorld.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityUndoableAction::EntityUndoableAction( EditorContext* pContext )
        : m_pContext( pContext )
    {
        EE_ASSERT( m_pContext != nullptr );
        m_pTypeRegistry = m_pContext->m_pToolsContext->m_pTypeRegistry;
        m_pWorld = m_pContext->GetWorld();
    }

    void EntityUndoableAction::RecordCreate( Entity** pEntities, size_t numEntities )
    {
        m_actionType = Type::CreateEntities;
        m_preModificationSelection.clear();
        m_postModificationSelection.clear();

        for ( int32_t i = 0; i < numEntities; i++ )
        {
            auto pEntity = pEntities[i];
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
            auto& serializedState = m_createdEntities.emplace_back();
            serializedState.m_mapID = pEntity->GetMapID();
            bool const result = CreateEntityDescriptor( *m_pTypeRegistry, m_log, pEntity, serializedState.m_desc );
            EE_ASSERT( result );

            m_postModificationSelection.emplace_back( pEntity );
        }

        // Wait for all state change action to complete
        m_pWorld->ProcessAllEntityStateChangeRequests();
    }

    //-------------------------------------------------------------------------

    void EntityUndoableAction::RecordDelete( Entity** pEntities, size_t numEntities )
    {
        m_actionType = Type::DeleteEntities;
        m_preModificationSelection.clear();
        m_postModificationSelection.clear();

        for ( int32_t i = 0; i < numEntities; i++ )
        {
            auto pEntity = pEntities[i];
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
            auto& serializedState = m_deletedEntities.emplace_back();
            serializedState.m_mapID = pEntity->GetMapID();
            bool const result = CreateEntityDescriptor( *m_pTypeRegistry, m_log, pEntity, serializedState.m_desc );
            EE_ASSERT( result );

            m_preModificationSelection.emplace_back( pEntity );
        }

        // Wait for all state change action to complete
        m_pWorld->ProcessAllEntityStateChangeRequests();
    }

    //-------------------------------------------------------------------------

    static void AddAttachedEntitiesToModifiedList( Entity* pEntity, TVector<Entity*>& outEntities )
    {
        for ( Entity* pAttachedEntity : pEntity->GetAttachedEntities() )
        {
            VectorEmplaceBackUnique( outEntities, pAttachedEntity );
            AddAttachedEntitiesToModifiedList( pAttachedEntity, outEntities );
        }
    }

    void EntityUndoableAction::RecordBeginEdit( EntityEditorItem* pItemsToBeModified, size_t numItems )
    {
        EE_ASSERT( numItems > 0 );
        EE_ASSERT( m_editedEntities.size() == 0 );

        m_actionType = Type::ModifyEntities;
        m_preModificationSelection.clear();
        m_postModificationSelection.clear();

        // Track relevant entities
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < numItems; i++ )
        {
            EntityEditorItem const& item = pItemsToBeModified[i];
            EE_ASSERT( !VectorContains( m_editedEntities, item.m_pEntity ) );
            m_editedEntities.emplace_back( item.m_pEntity );
            m_preModificationSelection.emplace_back( item );
            m_postModificationSelection.emplace_back( item );

            AddAttachedEntitiesToModifiedList( item.m_pEntity, m_editedEntities );
        }

        // Serialize all modified entities' before state
        //-------------------------------------------------------------------------

        for ( auto pEntity : m_editedEntities )
        {
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );

            auto& serializedState = m_entityDescPreModification.emplace_back();
            serializedState.m_mapID = pEntity->GetMapID();
            bool const result = CreateEntityDescriptor( *m_pTypeRegistry, m_log, pEntity, serializedState.m_desc );
            m_log.ReflectToSystemLogAndClear( LogCategory::Entity, "Undo/Redo" );
            EE_ASSERT( result );
        }
    }

    void EntityUndoableAction::RecordEndEdit()
    {
        EE_ASSERT( m_actionType == ModifyEntities );
        EE_ASSERT( m_editedEntities.size() > 0 );

        // Wait for all entity state changes to go through before recording the final result
        //-------------------------------------------------------------------------

        while ( m_pWorld->HasPendingEntityStateChangeActions() )
        {
            m_pWorld->UpdateLoading();
        }

        // Record the edited state
        //-------------------------------------------------------------------------

        for ( auto pEntity : m_editedEntities )
        {
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
            auto& serializedState = m_entityDescPostModification.emplace_back();
            serializedState.m_mapID = pEntity->GetMapID();
            bool const result = CreateEntityDescriptor( *m_pTypeRegistry, m_log, pEntity, serializedState.m_desc );
            m_log.ReflectToSystemLogAndClear( LogCategory::Entity, "Undo/Redo" );
            EE_ASSERT( result );
        }

        m_editedEntities.clear();
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

    static void AddSpatialHierarchyToEditedList( TVector<Entity*>& editedEntities, Entity* pEntity )
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
            AddSpatialHierarchyToEditedList( editedEntities, pAttachedEntity );
        }
    }

    void EntityUndoableAction::RecordBeginTransformEdit( EntityEditorItem* pItemsToBeModified, size_t numItems )
    {
        EE_ASSERT( numItems > 0 );
        EE_ASSERT( m_editedEntities.size() == 0 );

        m_actionType = Type::TransformUpdate;
        m_preModificationSelection.clear();
        m_postModificationSelection.clear();

        // Record changes to the entire spatial hierarchy
        for ( int32_t i = 0; i < numItems; i++ )
        {
            EntityEditorItem const& item = pItemsToBeModified[i];

            m_preModificationSelection.emplace_back( item );
            m_postModificationSelection.emplace_back( item );

            if ( item.IsSpatialEntity() )
            {
                auto pRootEntity = GetRootOfSpatialHierarchy( item.m_pEntity );
                EE_ASSERT( pRootEntity != nullptr );
                AddSpatialHierarchyToEditedList( m_editedEntities, pRootEntity );
            }
            else // Just add entity to the modified list
            {
                VectorEmplaceBackUnique( m_editedEntities, item.m_pEntity );
            }
        }

        // Serialize all modified entities
        for ( auto pEntity : m_editedEntities )
        {
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
            auto& serializedState = m_entityDescPreModification.emplace_back();
            serializedState.m_mapID = pEntity->GetMapID();
            bool const result = CreateEntityDescriptor( *m_pTypeRegistry, m_log, pEntity, serializedState.m_desc );
            m_log.ReflectToSystemLogAndClear( LogCategory::Entity, "Undo/Redo" );
            EE_ASSERT( result );

            // Serialize Transforms
            for ( auto pComponent : pEntity->GetComponents() )
            {
                auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent );
                if ( pSpatialComponent == nullptr )
                {
                    continue;
                }

                auto& serializedTransform = m_transformsPreModification.emplace_back();
                serializedTransform.m_entityID = pEntity->GetID();
                serializedTransform.m_componentID = pSpatialComponent->GetID();
                serializedTransform.m_transform = pSpatialComponent->GetLocalTransform();

                if ( pSpatialComponent->SupportsNonUniformScale() )
                {
                    serializedTransform.m_nonUniformScale = pSpatialComponent->GetNonUniformScale();
                }
            }
        }
    }

    void EntityUndoableAction::RecordEndTransformEdit()
    {
        EE_ASSERT( m_actionType == TransformUpdate );
        EE_ASSERT( m_editedEntities.size() > 0 );

        for ( auto pEntity : m_editedEntities )
        {
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
            auto& serializedState = m_entityDescPostModification.emplace_back();
            serializedState.m_mapID = pEntity->GetMapID();
            bool const result = CreateEntityDescriptor( *m_pTypeRegistry, m_log, pEntity, serializedState.m_desc );
            m_log.ReflectToSystemLogAndClear( LogCategory::Entity, "Undo/Redo" );
            EE_ASSERT( result );

            // Serialize transforms
            for ( auto pComponent : pEntity->GetComponents() )
            {
                auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent );
                if ( pSpatialComponent == nullptr )
                {
                    continue;
                }

                auto& serializedTransform = m_transformsPostModification.emplace_back();
                serializedTransform.m_entityID = pEntity->GetID();
                serializedTransform.m_componentID = pSpatialComponent->GetID();
                serializedTransform.m_transform = pSpatialComponent->GetLocalTransform();

                if ( pSpatialComponent->SupportsNonUniformScale() )
                {
                    serializedTransform.m_nonUniformScale = pSpatialComponent->GetNonUniformScale();
                }
            }
        }

        m_editedEntities.clear();
    }

    //-------------------------------------------------------------------------

    void EntityUndoableAction::UpdateSpatialParent( EntityMapID mapID, EntityDescriptor const& desc ) const
    {
        if ( desc.HasSpatialParent() )
        {
            auto pMap = m_pWorld->GetMap( mapID );

            auto pParentEntity = pMap->FindEntityByName( desc.m_spatialParentName );
            EE_ASSERT( pParentEntity != nullptr );

            auto pChildEntity = pMap->FindEntityByName( desc.m_name );
            EE_ASSERT( pChildEntity != nullptr );

            pChildEntity->SetSpatialParent( pParentEntity, desc.m_attachmentSocketID, Entity::SpatialAttachmentRule::KeepLocalTransform );
        }
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

                m_pWorld->ProcessAllEntityStateChangeRequests();
            }
            break;

            case EntityUndoableAction::DeleteEntities:
            {
                // Recreate all entities
                for ( auto const& serializedEntity : m_deletedEntities )
                {
                    auto pEntity = serializedEntity.m_desc.CreateEntity( *m_pTypeRegistry );
                    auto pMap = m_pWorld->GetMap( serializedEntity.m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    pMap->AddEntity( pEntity );
                }

                m_pWorld->ProcessAllEntityStateChangeRequests();

                // Fix up spatial hierarchy
                int32_t const numDeletedEntities = (int32_t) m_deletedEntities.size();
                for ( int32_t i = 0; i < numDeletedEntities; i++ )
                {
                    UpdateSpatialParent( m_deletedEntities[i].m_mapID, m_deletedEntities[i].m_desc );
                }
            }
            break;

            case EntityUndoableAction::ModifyEntities:
            {
                int32_t const numEntities = (int32_t) m_entityDescPostModification.size();
                EE_ASSERT( m_entityDescPostModification.size() == m_entityDescPreModification.size() );

                // Destroy old entities
                //-------------------------------------------------------------------------

                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    // Remove current entities
                    auto pMap = m_pWorld->GetMap( m_entityDescPostModification[i].m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    auto pExistingEntity = pMap->FindEntityByName( m_entityDescPostModification[i].m_desc.m_name );
                    EE_ASSERT( pExistingEntity != nullptr );
                    pMap->DestroyEntity( pExistingEntity->GetID() );
                }

                m_pWorld->ProcessAllEntityStateChangeRequests();

                // Recreate Entities
                //-------------------------------------------------------------------------

                // Create entities
                TVector<Entity*> createdEntities;
                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    auto pNewEntity = m_entityDescPreModification[i].m_desc.CreateEntity( *m_pTypeRegistry );
                    createdEntities.emplace_back( pNewEntity );
                }

                // Add to map
                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    auto pMap = m_pWorld->GetMap( m_entityDescPostModification[i].m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    pMap->AddEntity( createdEntities[i] );
                }

                m_pWorld->ProcessAllEntityStateChangeRequests();

                // Set spatial hierarchy
                // Since we record the entire spatial hierarchy for modifications, all hierarchy entities should be in the created entities list
                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    UpdateSpatialParent( m_entityDescPreModification[i].m_mapID, m_entityDescPreModification[i].m_desc );
                }
            }
            break;

            case EntityUndoableAction::TransformUpdate:
            {
                int32_t const numEntities = (int32_t) m_entityDescPostModification.size();
                EE_ASSERT( m_entityDescPostModification.size() == m_entityDescPreModification.size() );

                // Restore original transforms
                //-------------------------------------------------------------------------

                for ( auto const& st : m_transformsPreModification )
                {
                    auto pEntity = m_pWorld->FindEntity( st.m_entityID );
                    auto pSpatialComponent = TryCast<SpatialEntityComponent>( pEntity->FindComponent( st.m_componentID ) );
                    pSpatialComponent->SetLocalTransform( st.m_transform );

                    if ( pSpatialComponent->SupportsNonUniformScale() )
                    {
                        pSpatialComponent->SetNonUniformScale( st.m_nonUniformScale );
                    }
                }
            }
            break;

            default:
            EE_UNREACHABLE_CODE();
            break;
        }

        // Wait for all state change action to complete
        m_pWorld->ProcessAllEntityStateChangeRequests();
    }

    void EntityUndoableAction::Redo()
    {
        switch ( m_actionType )
        {
            case EntityUndoableAction::CreateEntities:
            {
                // Recreate entities
                for ( auto const& serializedEntity : m_createdEntities )
                {
                    auto pMap = m_pWorld->GetMap( serializedEntity.m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    auto pEntity = serializedEntity.m_desc.CreateEntity( *m_pTypeRegistry );
                    pMap->AddEntity( pEntity );
                }

                m_pWorld->ProcessAllEntityStateChangeRequests();

                // Fix up spatial hierarchy
                int32_t const numCreatedEntities = (int32_t) m_createdEntities.size();
                for ( int32_t i = 0; i < numCreatedEntities; i++ )
                {
                    UpdateSpatialParent( m_createdEntities[i].m_mapID, m_createdEntities[i].m_desc );
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

                m_pWorld->ProcessAllEntityStateChangeRequests();
            }
            break;

            case EntityUndoableAction::ModifyEntities:
            {
                int32_t const numEntities = (int32_t) m_entityDescPreModification.size();
                EE_ASSERT( m_entityDescPostModification.size() == m_entityDescPreModification.size() );

                // Destroy old entities
                //-------------------------------------------------------------------------

                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    auto pMap = m_pWorld->GetMap( m_entityDescPreModification[i].m_mapID );
                    EE_ASSERT( pMap != nullptr );

                    auto pExistingEntity = pMap->FindEntityByName( m_entityDescPreModification[i].m_desc.m_name );
                    EE_ASSERT( pExistingEntity != nullptr );
                    pMap->DestroyEntity( pExistingEntity->GetID() );
                }

                m_pWorld->ProcessAllEntityStateChangeRequests();

                // Recreate modified entities
                //-------------------------------------------------------------------------

                // Create entities
                TVector<Entity*> createdEntities;
                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    auto pNewEntity = m_entityDescPostModification[i].m_desc.CreateEntity( *m_pTypeRegistry );
                    createdEntities.emplace_back( pNewEntity );
                }

                // Add entities to map
                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    auto pMap = m_pWorld->GetMap( m_entityDescPostModification[i].m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    pMap->AddEntity( createdEntities[i] );
                }

                m_pWorld->ProcessAllEntityStateChangeRequests();

                // Restore spatial hierarchy
                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    UpdateSpatialParent( m_entityDescPostModification[i].m_mapID, m_entityDescPostModification[i].m_desc );
                }
            }
            break;

            case EntityUndoableAction::TransformUpdate:
            {
                int32_t const numEntities = (int32_t) m_entityDescPreModification.size();
                EE_ASSERT( m_entityDescPostModification.size() == m_entityDescPreModification.size() );

                // Set new transforms
                //-------------------------------------------------------------------------

                for ( auto const& st : m_transformsPostModification )
                {
                    auto pEntity = m_pWorld->FindEntity( st.m_entityID );
                    auto pSpatialComponent = TryCast<SpatialEntityComponent>( pEntity->FindComponent( st.m_componentID ) );
                    pSpatialComponent->SetLocalTransform( st.m_transform );

                    if ( pSpatialComponent->SupportsNonUniformScale() )
                    {
                        pSpatialComponent->SetNonUniformScale( st.m_nonUniformScale );
                    }
                }
            }
            break;

            default:
            EE_UNREACHABLE_CODE();
            break;
        }
    }

    TVector<EE::EntityModel::EntityEditorItem> const& EntityUndoableAction::GetRecordedSelection( UndoStack::Operation operation ) const
    {
        if ( operation == UndoStack::Operation::Undo )
        {
            return m_preModificationSelection;
        }
        else
        {
            return m_postModificationSelection;
        }
    }

    //-------------------------------------------------------------------------

    void EntityGroupsUndoableAction::Begin( )
    {
        m_preModificationGroups = m_pContext->m_entityGroups;
    }

    void EntityGroupsUndoableAction::End()
    {
        m_postModificationGroups = m_pContext->m_entityGroups;
    }

    void EntityGroupsUndoableAction::Undo()
    {
        m_pContext->SetEntityGroups( m_preModificationGroups );
    }

    void EntityGroupsUndoableAction::Redo()
    {
        m_pContext->SetEntityGroups( m_postModificationGroups );
    }
}