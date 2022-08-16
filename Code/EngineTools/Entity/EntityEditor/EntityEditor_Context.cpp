#include "EntityEditor_Context.h"
#include "Engine/Entity/EntitySystem.h"
#include "Engine/Entity/EntitySerialization.h"
#include "Engine/Volumes/Components/Component_Volumes.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    // Compound undoable action - handles all scene manipulation operations
    // All operations are at the granularity of an entity, we dont manage component actions individually
    class EntityEditorUndoableAction final : public IUndoableAction
    {
        struct SpatialState
        {
            StringID    m_entityName;
            StringID    m_componentName;
            Transform   m_transform;
        };

    public:

        enum Type
        {
            Invalid = 0,
            CreateEntities,
            DeleteEntities,
            ModifyEntities,
            MoveComponents,
        };

    public:

        EntityEditorUndoableAction( TypeSystem::TypeRegistry const& typeRegistry, EntityMap* pMap )
            : m_typeRegistry( typeRegistry )
            , m_pMap( pMap )
        {
            EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        }

        void RecordCreate( TVector<Entity*> createdEntities )
        {
            m_actionType = Type::CreateEntities;

            for ( auto pEntity : createdEntities )
            {
                EE_ASSERT( pEntity != nullptr );
                bool const result = Serializer::SerializeEntity( m_typeRegistry, pEntity, m_createdEntities.emplace_back() );
                EE_ASSERT( result );
            }
        }

        void RecordDelete( TVector<Entity*> entitiesToDelete )
        {
            m_actionType = Type::DeleteEntities;

            for ( auto pEntity : entitiesToDelete )
            {
                EE_ASSERT( pEntity != nullptr && pEntity->GetMapID() == m_pMap->GetID() );
                bool const result = Serializer::SerializeEntity( m_typeRegistry, pEntity, m_deletedEntities.emplace_back() );
                EE_ASSERT( result );
            }
        }

        void RecordBeginEdit( TVector<Entity*> entitiesToBeModified, bool wereEntitiesDuplicated = false )
        {
            EE_ASSERT( entitiesToBeModified.size() > 0 );

            m_actionType = Type::ModifyEntities;
            m_editedEntities = entitiesToBeModified;
            m_entitiesWereDuplicated = wereEntitiesDuplicated;

            for ( auto pEntity : m_editedEntities )
            {
                EE_ASSERT( pEntity != nullptr );
                EE_ASSERT( wereEntitiesDuplicated ? true : pEntity->GetMapID() == m_pMap->GetID() );
                bool const result = Serializer::SerializeEntity( m_typeRegistry, pEntity, m_entityDescPreModification.emplace_back() );
                EE_ASSERT( result );
            }
        }

        void RecordEndEdit()
        {
            EE_ASSERT( m_actionType == ModifyEntities );
            EE_ASSERT( m_editedEntities.size() > 0 );

            for ( auto pEntity : m_editedEntities )
            {
                EE_ASSERT( pEntity != nullptr && pEntity->GetMapID() == m_pMap->GetID() );
                bool const result = Serializer::SerializeEntity( m_typeRegistry, pEntity, m_entityDescPostModification.emplace_back() );
                EE_ASSERT( result );
            }

            m_editedEntities.clear();
        }

    private:

        virtual void Undo() override
        {
            switch ( m_actionType )
            {
                case EntityEditorUndoableAction::CreateEntities:
                {
                    for ( auto const& entityDesc : m_createdEntities )
                    {
                        auto pEntity = m_pMap->FindEntityByName( entityDesc.m_name );
                        m_pMap->DestroyEntity( pEntity );
                    }
                }
                break;

                case EntityEditorUndoableAction::DeleteEntities:
                {
                    for ( auto const& entityDesc : m_deletedEntities )
                    {
                        auto pEntity = Serializer::CreateEntity( m_typeRegistry, entityDesc );
                        m_pMap->AddEntity( pEntity );
                    }
                }
                break;

                case EntityEditorUndoableAction::ModifyEntities:
                {
                    int32_t const numEntities = (int32_t) m_entityDescPostModification.size();
                    EE_ASSERT( m_entityDescPostModification.size() == m_entityDescPreModification.size() );

                    for ( int32_t i = 0; i < numEntities; i++ )
                    {
                        // Remove current entities
                        auto pExistingEntity = m_pMap->FindEntityByName( m_entityDescPostModification[i].m_name );
                        EE_ASSERT( pExistingEntity != nullptr );
                        m_pMap->DestroyEntity( pExistingEntity );

                        //-------------------------------------------------------------------------

                        // Only recreate the entities if they werent duplicated, if they were duplicate, then deleting them was enough to undo the action
                        if ( !m_entitiesWereDuplicated )
                        {
                            auto pNewEntity = Serializer::CreateEntity( m_typeRegistry, m_entityDescPreModification[i] );
                            m_pMap->AddEntity( pNewEntity );
                        }
                    }
                }
                break;

                case EntityEditorUndoableAction::MoveComponents:
                {

                }
                break;

                default:
                EE_UNREACHABLE_CODE();
                break;
            }
        }

        virtual void Redo() override
        {
            switch ( m_actionType )
            {
                case EntityEditorUndoableAction::CreateEntities:
                {
                    for ( auto const& entityDesc : m_createdEntities )
                    {
                        auto pEntity = Serializer::CreateEntity( m_typeRegistry, entityDesc );
                        m_pMap->AddEntity( pEntity );
                    }
                }
                break;

                case EntityEditorUndoableAction::DeleteEntities:
                {
                    for ( auto const& entityDesc : m_deletedEntities )
                    {
                        auto pEntity = m_pMap->FindEntityByName( entityDesc.m_name );
                        m_pMap->DestroyEntity( pEntity );
                    }
                }
                break;

                case EntityEditorUndoableAction::ModifyEntities:
                {
                    int32_t const numEntities = (int32_t) m_entityDescPreModification.size();
                    EE_ASSERT( m_entityDescPostModification.size() == m_entityDescPreModification.size() );

                    for ( int32_t i = 0; i < numEntities; i++ )
                    {
                        // Destroy the current entities (only if there are not duplicates since if they are they wont exist)
                        if ( !m_entitiesWereDuplicated )
                        {
                            auto pExistingEntity = m_pMap->FindEntityByName( m_entityDescPreModification[i].m_name );
                            EE_ASSERT( pExistingEntity != nullptr );
                            m_pMap->DestroyEntity( pExistingEntity );
                        }

                        //-------------------------------------------------------------------------

                        auto pNewEntity = Serializer::CreateEntity( m_typeRegistry, m_entityDescPostModification[i] );
                        m_pMap->AddEntity( pNewEntity );
                    }
                }
                break;

                case EntityEditorUndoableAction::MoveComponents:
                {

                }
                break;

                default:
                EE_UNREACHABLE_CODE();
                break;
            }
        }

    private:

        TypeSystem::TypeRegistry const&         m_typeRegistry;
        EntityMap*                              m_pMap = nullptr;
        Type                                    m_actionType = Invalid;

        // Data: Create
        TVector<SerializedEntityDescriptor>     m_createdEntities;

        // Data: Delete
        TVector<SerializedEntityDescriptor>     m_deletedEntities;

        // Data: Modify
        TVector<Entity*>                        m_editedEntities; // Temporary storage that is only valid between a Begin and End call
        TVector<SerializedEntityDescriptor>     m_entityDescPreModification;
        TVector<SerializedEntityDescriptor>     m_entityDescPostModification;
        bool                                    m_entitiesWereDuplicated = false;

        // Data: Move
        TVector<SpatialState>                   m_componentSpatialStatePreMove;
        TVector<SpatialState>                   m_componentSpatialStatePostMove;
    };
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityEditorContext::EntityEditorContext( ToolsContext const* pToolsContext, EntityWorld* pWorld, UndoStack& undoStack )
        : m_pToolsContext( pToolsContext )
        , m_undoStack( undoStack )
        , m_pWorld( pWorld )
        , m_volumeTypes( pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( VolumeComponent::GetStaticTypeID(), false, false, true ) )
    {
        EE_ASSERT( m_pToolsContext != nullptr && m_pToolsContext->IsValid() );
    }

    void EntityEditorContext::SetMapToUse( ResourceID const& resourceID )
    {
        auto pMap = m_pWorld->GetMap( resourceID );
        EE_ASSERT( pMap != nullptr );
        EE_ASSERT( m_mapID != pMap->GetID() );

        ClearSelection();
        m_entityDeletionRequests.clear();
        m_mapID = pMap->GetID();
        m_pMap = nullptr;
    }

    void EntityEditorContext::SetMapToUse( EntityMapID const& mapID )
    {
        EE_ASSERT( m_mapID != mapID );

        ClearSelection();
        m_entityDeletionRequests.clear();
        m_mapID = mapID;
        m_pMap = nullptr;
    }

    void EntityEditorContext::Update( UpdateContext const& context )
    {
        // Process all deletion requests
        //-------------------------------------------------------------------------

        if ( !m_entityDeletionRequests.empty() )
        {
            EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
            EE_ASSERT( m_pActiveUndoableAction == nullptr );

            for ( auto pEntity : m_entityDeletionRequests )
            {
                m_pMap->DestroyEntity( pEntity );
            }

            m_entityDeletionRequests.clear();
            ClearSelection();
        }

        // Update map ptr
        //-------------------------------------------------------------------------

        if ( m_pMap == nullptr )
        {
            if ( m_mapID.IsValid() && m_pWorld->IsMapActive( m_mapID ) )
            {
                m_pMap = m_pWorld->GetMap( m_mapID );
                EE_ASSERT( m_pMap != nullptr );
            }
            else
            {
                m_pMap = nullptr;
            }
        }
    }

    //-------------------------------------------------------------------------

    void EntityEditorContext::OnUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        auto pUndoableAction = static_cast<EntityEditorUndoableAction const*>( pAction );
        // TODO: record addition info in undo command to be able to restore selection
        ClearSelection();
    }

    void EntityEditorContext::BeginEditEntities( TVector<Entity*> const& entities )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );

        // Record undo action
        m_pActiveUndoableAction = EE::New<EntityEditorUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pMap );
        m_pActiveUndoableAction->RecordBeginEdit( entities );
    }

    void EntityEditorContext::EndEditEntities()
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( m_pActiveUndoableAction != nullptr );

        m_pActiveUndoableAction->RecordEndEdit();
        m_undoStack.RegisterAction( m_pActiveUndoableAction );
        m_pActiveUndoableAction = nullptr;

        CalculateSelectionSpatialInfo();
    }

    void EntityEditorContext::BeginEditComponent( EntityComponent* pComponent )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        EE_ASSERT( pComponent != nullptr );

        auto pEntity = m_pMap->FindEntity( pComponent->GetEntityID() );
        EE_ASSERT( pEntity != nullptr );

        BeginEditEntities( { pEntity } );

        //-------------------------------------------------------------------------

        m_pWorld->PrepareComponentForEditing( m_pMap->GetID(), pComponent->GetEntityID(), pComponent->GetID() );
    }

    //-------------------------------------------------------------------------

    bool EntityEditorContext::IsSelected( Entity const* pEntity ) const
    {
        for ( auto pSelectedEntity : m_selectedEntities )
        {
            if ( pSelectedEntity == pEntity )
            {
                return true;
            }
        }

        return false;
    }

    bool EntityEditorContext::IsSelected( EntityComponent const* pComponent ) const
    {
        for ( auto pSelectedComponent : m_selectedComponents )
        {
            if ( pSelectedComponent == pComponent )
            {
                return true;
            }
        }

        return false;
    }

    void EntityEditorContext::SelectEntity( Entity* pEntity )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );

        ClearSelection();
        m_selectedEntities.emplace_back( pEntity );
        OnSelectionModified();
    }

    void EntityEditorContext::SelectEntities( TVector<Entity*> const& entities )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );

        for ( auto pEntity : entities )
        {
            EE_ASSERT( pEntity != nullptr );
        }

        m_selectedEntities = entities;
        m_selectedComponents.clear();
        m_pSelectedSystem = nullptr;

        OnSelectionModified();
    }

    void EntityEditorContext::AddToSelectedEntities( Entity* pEntity )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( pEntity->GetMapID() == m_pMap->GetID() );
        
        if ( !VectorContains( m_selectedEntities, pEntity ) )
        {
            m_pSelectedSystem = nullptr;
            m_selectedComponents.clear();

            m_selectedEntities.emplace_back( pEntity );
            OnSelectionModified();
        }
    }

    void EntityEditorContext::RemoveFromSelectedEntities( Entity* pEntity )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( pEntity->GetMapID() == m_pMap->GetID() );
        EE_ASSERT( m_selectedComponents.empty() && m_pSelectedSystem == nullptr );

        if ( VectorContains( m_selectedEntities, pEntity ) )
        {
            m_selectedEntities.erase_first( pEntity );
            OnSelectionModified();
        }
    }

    void EntityEditorContext::SelectComponent( EntityComponent* pComponent )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( m_selectedEntities.size() == 1 );
        EE_ASSERT( pComponent->GetEntityID() == m_selectedEntities[0]->GetID() );

        m_pSelectedSystem = nullptr;

        m_selectedComponents.clear();
        m_selectedComponents.emplace_back( pComponent );
        OnSelectionModified();
    }

    void EntityEditorContext::AddToSelectedComponents( EntityComponent* pComponent )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( m_selectedEntities.size() == 1 && m_pSelectedSystem == nullptr );
        EE_ASSERT( pComponent->GetEntityID() == m_selectedEntities[0]->GetID() );
        EE_ASSERT( !VectorContains( m_selectedComponents, pComponent ) );

        m_selectedComponents.emplace_back( pComponent );
        OnSelectionModified();
    }

    void EntityEditorContext::RemoveFromSelectedComponents( EntityComponent* pComponent )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( m_selectedEntities.size() == 1 && m_pSelectedSystem == nullptr );
        EE_ASSERT( pComponent->GetEntityID() == m_selectedEntities[0]->GetID() );
        EE_ASSERT( VectorContains( m_selectedComponents, pComponent ) );

        m_selectedComponents.erase_first( pComponent );
        OnSelectionModified();
    }

    void EntityEditorContext::SelectSystem( EntitySystem* pSystem )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        EE_ASSERT( pSystem != nullptr );
        EE_ASSERT( m_selectedEntities.size() == 1 );
        EE_ASSERT( VectorContains( m_selectedEntities[0]->GetSystems(), pSystem ) );

        m_selectedComponents.clear();
        m_pSelectedSystem = pSystem;
        OnSelectionModified();
    }

    void EntityEditorContext::ClearSelection()
    {
        EE_ASSERT( m_pActiveUndoableAction == nullptr );

        m_selectedEntities.clear();
        m_selectedComponents.clear();
        m_pSelectedSystem = nullptr;
        OnSelectionModified();
    }

    void EntityEditorContext::OnSelectionModified()
    {
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        CalculateSelectionSpatialInfo();
    }

    void EntityEditorContext::CalculateSelectionSpatialInfo()
    {
        TVector<OBB> bounds;

        m_selectionTransform = Transform::Identity;
        m_selectionOffsetTransforms.clear();
        m_hasSpatialSelection = false;

        // Check if we have spatial selection
        //-------------------------------------------------------------------------
        // Record the the transforms of all found spatial elements in the offset array

        if ( m_pSelectedSystem == nullptr )
        {
            // Check all selected components
            if ( m_selectedComponents.size() > 0 )
            {
                for ( auto pComponent : m_selectedComponents )
                {
                    if ( auto pSC = TryCast<SpatialEntityComponent>( pComponent ) )
                    {
                        m_selectionOffsetTransforms.emplace_back( pSC->GetWorldTransform() );
                        bounds.emplace_back( pSC->GetWorldBounds() );
                        m_hasSpatialSelection = true;
                    }
                }
            }
            else // Check all selected entities
            {
                for ( auto pSelectedEntity : m_selectedEntities )
                {
                    if ( pSelectedEntity->IsSpatialEntity() )
                    {
                        m_selectionOffsetTransforms.emplace_back( pSelectedEntity->GetWorldTransform() );
                        bounds.emplace_back( pSelectedEntity->GetCombinedWorldBounds() ); // TODO: calculate combined bounds
                        m_hasSpatialSelection = true;
                    }
                }
            }
        }

        // Update selection transform
        //-------------------------------------------------------------------------

        if ( m_hasSpatialSelection )
        {
            if ( m_selectionOffsetTransforms.size() == 1 )
            {
                m_selectionTransform = m_selectionOffsetTransforms[0];
                m_selectionOffsetTransforms[0] = Transform::Identity;
            }
            else
            {
                // Calculate the average position of all transforms found
                Vector averagePosition = Vector::Zero;
                for ( auto const& t : m_selectionOffsetTransforms )
                {
                    averagePosition += t.GetTranslation();
                }
                averagePosition /= (float) m_selectionOffsetTransforms.size();

                // Set the average transform
                m_selectionTransform = Transform( Quaternion::Identity, averagePosition );

                // Calculate the offsets
                Transform const inverseSelectionTransform = m_selectionTransform.GetInverse();
                for ( auto& t : m_selectionOffsetTransforms )
                {
                    t = t * inverseSelectionTransform;
                }
            }
        }

        // Update selection bounds
        //-------------------------------------------------------------------------

        if ( m_hasSpatialSelection )
        {
            if ( bounds.size() == 1 )
            {
                m_selectionBounds = bounds[0];
            }
            else
            {
                TVector<Vector> points;

                for ( auto itemBounds : bounds )
                {
                    Vector corners[8];
                    itemBounds.GetCorners( corners );
                    for ( auto i = 0; i < 8; i++ )
                    {
                        points.emplace_back( corners[i] );
                    }
                }

                m_selectionBounds = OBB( points.data(), (uint32_t) points.size() );
            }
        }

        CalculateSelectionSpatialBounds();
    }

    void EntityEditorContext::CalculateSelectionSpatialBounds()
    {
        TVector<OBB> bounds;

        // Check if we have spatial selection
        //-------------------------------------------------------------------------
        // Record the the transforms of all found spatial elements in the offset array

        if ( m_pSelectedSystem == nullptr )
        {
            // Check all selected components
            if ( m_selectedComponents.size() > 0 )
            {
                for ( auto pComponent : m_selectedComponents )
                {
                    if ( auto pSC = TryCast<SpatialEntityComponent>( pComponent ) )
                    {
                        bounds.emplace_back( pSC->GetWorldBounds() );
                        m_hasSpatialSelection = true;
                    }
                }
            }
            else // Check all selected entities
            {
                for ( auto pSelectedEntity : m_selectedEntities )
                {
                    if ( pSelectedEntity->IsSpatialEntity() )
                    {
                        bounds.emplace_back( pSelectedEntity->GetCombinedWorldBounds() ); // TODO: calculate combined bounds
                        m_hasSpatialSelection = true;
                    }
                }
            }
        }

        // Update selection bounds
        //-------------------------------------------------------------------------

        if ( m_hasSpatialSelection )
        {
            if ( bounds.size() == 1 )
            {
                m_selectionBounds = bounds[0];
            }
            else
            {
                TVector<Vector> points;

                for ( auto itemBounds : bounds )
                {
                    Vector corners[8];
                    itemBounds.GetCorners( corners );
                    for ( auto i = 0; i < 8; i++ )
                    {
                        points.emplace_back( corners[i] );
                    }
                }

                m_selectionBounds = OBB( points.data(), (uint32_t) points.size() );
            }
        }
    }

    //-------------------------------------------------------------------------

    void EntityEditorContext::BeginTransformManipulation( Transform const& newTransform, bool duplicateSelection )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( !m_selectedEntities.empty() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );

        // Should we duplicate the selection?
        if ( duplicateSelection )
        {
            DuplicateSelectedEntities();
        }

        // Create undo action
        m_pActiveUndoableAction = EE::New<EntityEditorUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pMap );
        m_pActiveUndoableAction->RecordBeginEdit( m_selectedEntities, duplicateSelection );

        // Apply transform modification
        ApplyTransformManipulation( newTransform );
    }

    void EntityEditorContext::ApplyTransformManipulation( Transform const& newTransform )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( !m_selectedEntities.empty() );
        EE_ASSERT( m_pActiveUndoableAction != nullptr );

        // Update all selected components
        if ( m_selectedComponents.size() > 0 )
        {
            int32_t offsetIdx = 0;
            for ( auto pComponent : m_selectedComponents )
            {
                if ( auto pSC = TryCast<SpatialEntityComponent>( pComponent ) )
                {
                    pSC->SetWorldTransform( m_selectionOffsetTransforms[offsetIdx] * newTransform );
                    offsetIdx++;
                }
            }
        }
        else // Update all selected entities
        {
            int32_t offsetIdx = 0;
            for ( auto pSelectedEntity : m_selectedEntities )
            {
                if ( pSelectedEntity->IsSpatialEntity() )
                {
                    pSelectedEntity->SetWorldTransform( m_selectionOffsetTransforms[offsetIdx] * newTransform );
                    offsetIdx++;
                }
            }
        }

        m_selectionTransform = newTransform;
        CalculateSelectionSpatialBounds();
    }

    void EntityEditorContext::EndTransformManipulation( Transform const& newTransform )
    {
        ApplyTransformManipulation( newTransform );

        m_pActiveUndoableAction->RecordEndEdit();
        m_undoStack.RegisterAction( m_pActiveUndoableAction );
        m_pActiveUndoableAction = nullptr;
    }

    //-------------------------------------------------------------------------

    Entity* EntityEditorContext::CreateEntity()
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        Entity* pEntity = EE::New<Entity>( StringID( "Entity" ) );
        AddEntity( pEntity );
        return pEntity;
    }

    void EntityEditorContext::AddEntity( Entity* pEntity )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( pEntity != nullptr && !pEntity->IsAddedToMap() );

        // Add entity to map
        m_pMap->AddEntity( pEntity );
        SelectEntity( pEntity );

        // Record undo action
        m_pActiveUndoableAction = EE::New<EntityEditorUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pMap );
        m_pActiveUndoableAction->RecordCreate( { pEntity } );
        m_undoStack.RegisterAction( m_pActiveUndoableAction );
        m_pActiveUndoableAction = nullptr;
    }

    void EntityEditorContext::DuplicateSelectedEntities()
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );

        if ( m_selectedEntities.empty() )
        {
            return;
        }

        TVector<Entity*> duplicatedEntities;
        for ( auto pEntity : m_selectedEntities )
        {
            SerializedEntityDescriptor entityDesc;
            if ( Serializer::SerializeEntity( *m_pToolsContext->m_pTypeRegistry, pEntity, entityDesc ) )
            {
                auto pDuplicatedEntity = Serializer::CreateEntity( *m_pToolsContext->m_pTypeRegistry, entityDesc );
                duplicatedEntities.emplace_back( pDuplicatedEntity );
                m_pMap->AddEntity( pDuplicatedEntity );
            }
        }

        //-------------------------------------------------------------------------

        // Update selection
        SelectEntities( duplicatedEntities );
    }

    void EntityEditorContext::DestroyEntity( Entity* pEntity )
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );
        EE_ASSERT( pEntity != nullptr && pEntity->GetMapID() == m_pMap->GetID() );
        m_entityDeletionRequests.emplace_back( pEntity );
        ClearSelection();

        // Record undo action
        m_pActiveUndoableAction = EE::New<EntityEditorUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pMap );
        m_pActiveUndoableAction->RecordDelete( { pEntity } );
        m_undoStack.RegisterAction( m_pActiveUndoableAction );
        m_pActiveUndoableAction = nullptr;
    }

    void EntityEditorContext::DestroySelectedEntities()
    {
        EE_ASSERT( m_pMap != nullptr && m_pMap->IsActivated() );

        if ( m_selectedEntities.empty() )
        {
            return;
        }

        for ( auto pEntity : m_selectedEntities )
        {
            m_entityDeletionRequests.emplace_back( pEntity );
        }
        ClearSelection();

        // Record undo action
        m_pActiveUndoableAction = EE::New<EntityEditorUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pMap );
        m_pActiveUndoableAction->RecordDelete( m_entityDeletionRequests );
        m_undoStack.RegisterAction( m_pActiveUndoableAction );
        m_pActiveUndoableAction = nullptr;
    }

    void EntityEditorContext::CreateSystem( Entity* pEntity, TypeSystem::TypeInfo const* pSystemTypeInfo )
    {
        EE_ASSERT( pSystemTypeInfo != nullptr );
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( m_pMap->ContainsEntity( pEntity->GetID() ) );

        m_pActiveUndoableAction = EE::New<EntityEditorUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pMap );
        m_pActiveUndoableAction->RecordBeginEdit( { pEntity } );
        pEntity->CreateSystem( pSystemTypeInfo );
        m_pActiveUndoableAction->RecordEndEdit();
        m_undoStack.RegisterAction( m_pActiveUndoableAction );
        m_pActiveUndoableAction = nullptr;
    }

    void EntityEditorContext::DestroySystem( Entity* pEntity, TypeSystem::TypeID systemTypeID )
    {
        EE_ASSERT( systemTypeID.IsValid() );
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( m_pMap->ContainsEntity( pEntity->GetID() ) );

        ClearSelectedSystem(); // Todo only clear if the system is selected

        m_pActiveUndoableAction = EE::New<EntityEditorUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pMap );
        m_pActiveUndoableAction->RecordBeginEdit( { pEntity } );
        pEntity->DestroySystem( systemTypeID );
        m_pActiveUndoableAction->RecordEndEdit();
        m_undoStack.RegisterAction( m_pActiveUndoableAction );
        m_pActiveUndoableAction = nullptr;
    }

    void EntityEditorContext::DestroySystem( Entity* pEntity, EntitySystem* pSystem )
    {
        EE_ASSERT( pSystem != nullptr );
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( VectorContains( pEntity->GetSystems(), pSystem ) );
        EE_ASSERT( m_pMap->ContainsEntity( pEntity->GetID() ) );

        if ( m_pSelectedSystem == pSystem )
        {
            m_pSelectedSystem = nullptr;
        }

        m_pActiveUndoableAction = EE::New<EntityEditorUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pMap );
        m_pActiveUndoableAction->RecordBeginEdit( { pEntity } );
        pEntity->DestroySystem( pSystem->GetTypeID() );
        m_pActiveUndoableAction->RecordEndEdit();
        m_undoStack.RegisterAction( m_pActiveUndoableAction );
        m_pActiveUndoableAction = nullptr;
    }

    void EntityEditorContext::CreateComponent( Entity* pEntity, TypeSystem::TypeInfo const* pComponentTypeInfo, ComponentID const& parentSpatialComponentID )
    {
        EE_ASSERT( pComponentTypeInfo != nullptr );
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( m_pMap->ContainsEntity( pEntity->GetID() ) );

        m_pActiveUndoableAction = EE::New<EntityEditorUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pMap );
        m_pActiveUndoableAction->RecordBeginEdit( { pEntity } );
        pEntity->CreateComponent( pComponentTypeInfo, parentSpatialComponentID );
        m_pActiveUndoableAction->RecordEndEdit();
        m_undoStack.RegisterAction( m_pActiveUndoableAction );
        m_pActiveUndoableAction = nullptr;
    }

    void EntityEditorContext::DestroyComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( m_pMap->ContainsEntity( pEntity->GetID() ) );

        m_selectedComponents.erase_first_unsorted( pComponent );

        m_pActiveUndoableAction = EE::New<EntityEditorUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pMap );
        m_pActiveUndoableAction->RecordBeginEdit( { pEntity } );
        pEntity->DestroyComponent( pComponent->GetID() );
        m_pActiveUndoableAction->RecordEndEdit();
        m_undoStack.RegisterAction( m_pActiveUndoableAction );
        m_pActiveUndoableAction = nullptr;
    }
}