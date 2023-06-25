#include "Workspace_EntityEditor.h"

#include "EngineTools/Entity/EntitySerializationTools.h"
#include "EngineTools/Core/Widgets/TreeListView.h"
#include "Engine/Navmesh/Systems/WorldSystem_Navmesh.h"
#include "Engine/Navmesh/DebugViews/DebugView_Navmesh.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/PhysicsCollisionMesh.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Volumes/Components/Component_Volumes.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntitySystem.h"
#include "Engine/Entity/EntitySerialization.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Math/Math.h"
#include "System/Math/MathUtils.h"


//-------------------------------------------------------------------------
// Undo/Redo
//-------------------------------------------------------------------------

namespace EE::EntityModel
{
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

    static void AddEntireSpatialHierarchyToEditedList( TVector<Entity*>& editedEntities, Entity* pEntity )
    {
        auto pRootEntity = GetRootOfSpatialHierarchy( pEntity );
        EE_ASSERT( pRootEntity != nullptr );
        AddSpatialHierarchyToEditedList( editedEntities, pRootEntity );
    }

    //-------------------------------------------------------------------------

    class EntityUndoableAction final : public IUndoableAction
    {
        struct SerializedEntity
        {
            EntityMapID                         m_mapID;
            SerializedEntityDescriptor          m_desc;
        };

        struct SerializedComponentTransform
        {
            EntityID                            m_entityID;
            ComponentID                         m_componentID;
            Transform                           m_transform;
        };

    public:

        enum Type
        {
            Invalid = 0,
            CreateEntities,
            DeleteEntities,
            ModifyEntities,
            TransformUpdate,
        };

    public:

        EntityUndoableAction( EntityEditorWorkspace* pWorkspace ) 
            : m_pWorkspace( pWorkspace )
        {
            EE_ASSERT( m_pWorkspace != nullptr );
        }

        Type GetActionType() const { return m_actionType; }

        void RecordCreate( TVector<Entity*> createdEntities );
        void RecordDelete( TVector<Entity*> entitiesToDelete );
        void RecordBeginEdit( TVector<Entity*> entitiesToBeModified );
        void RecordEndEdit();
        void RecordBeginTransformEdit( TVector<Entity*> entitiesToBeModified, bool wereEntitiesDuplicated = false );
        void RecordEndTransformEdit();

        EntityEditorWorkspace::SelectionSwitchRequest const& GetRecordedSelection() const { return m_selection; }

    private:

        void RecordSelection();

        virtual void Undo() override;
        virtual void Redo() override;

    private:

        EntityEditorWorkspace*                              m_pWorkspace = nullptr;
        Type                                                m_actionType = Invalid;
        EntityEditorWorkspace::SelectionSwitchRequest       m_selection;

        // Data: Create
        TVector<SerializedEntity>                           m_createdEntities;

        // Data: Delete
        TVector<SerializedEntity>                           m_deletedEntities;

        // Data: Modify
        TVector<Entity*>                                    m_editedEntities; // Temporary storage that is only valid between a Begin and End call
        TVector<SerializedEntity>                           m_entityDescPreModification;
        TVector<SerializedEntity>                           m_entityDescPostModification;
        TVector<SerializedComponentTransform>               m_transformsPreModification;
        TVector<SerializedComponentTransform>               m_transformsPostModification;
        bool                                                m_entitiesWereDuplicated = false;
    };

    //-------------------------------------------------------------------------

    void EntityUndoableAction::RecordCreate( TVector<Entity*> createdEntities )
    {
        m_actionType = Type::CreateEntities;

        for ( auto pEntity : createdEntities )
        {
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
            auto& serializedState = m_createdEntities.emplace_back();
            serializedState.m_mapID = pEntity->GetMapID();
            bool const result = Serializer::SerializeEntity( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, pEntity, serializedState.m_desc );
            EE_ASSERT( result );
        }

        RecordSelection();
    }

    void EntityUndoableAction::RecordDelete( TVector<Entity*> entitiesToDelete )
    {
        m_actionType = Type::DeleteEntities;

        for ( auto pEntity : entitiesToDelete )
        {
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
            auto& serializedState = m_deletedEntities.emplace_back();
            serializedState.m_mapID = pEntity->GetMapID();
            bool const result = Serializer::SerializeEntity( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, pEntity, serializedState.m_desc );
            EE_ASSERT( result );
        }

        RecordSelection();
    }

    //-------------------------------------------------------------------------

    void EntityUndoableAction::RecordBeginEdit( TVector<Entity*> entitiesToBeModified )
    {
        EE_ASSERT( entitiesToBeModified.size() > 0 );
        EE_ASSERT( m_editedEntities.size() == 0 );

        m_actionType = Type::ModifyEntities;

        // Record changes to all modified entities
        for ( auto pEntity : entitiesToBeModified )
        {
            EE_ASSERT( !VectorContains( m_editedEntities, pEntity ) );
            m_editedEntities.emplace_back( pEntity );
        }

        // Serialize all modified entities
        for ( auto pEntity : m_editedEntities )
        {
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
            auto& serializedState = m_entityDescPreModification.emplace_back();
            serializedState.m_mapID = pEntity->GetMapID();
            bool const result = Serializer::SerializeEntity( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, pEntity, serializedState.m_desc );
            EE_ASSERT( result );
        }

        RecordSelection();
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
            bool const result = Serializer::SerializeEntity( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, pEntity, serializedState.m_desc );
            EE_ASSERT( result );
        }

        m_editedEntities.clear();
    }

    //-------------------------------------------------------------------------

    void EntityUndoableAction::RecordBeginTransformEdit( TVector<Entity*> entitiesToBeModified, bool wereEntitiesDuplicated )
    {
        EE_ASSERT( entitiesToBeModified.size() > 0 );
        EE_ASSERT( m_editedEntities.size() == 0 );

        m_actionType = Type::TransformUpdate;
        m_entitiesWereDuplicated = wereEntitiesDuplicated;

        // Record changes to the entire spatial hierarchy
        for ( auto pEntity : entitiesToBeModified )
        {
            if ( pEntity->IsSpatialEntity() )
            {
                AddEntireSpatialHierarchyToEditedList( m_editedEntities, pEntity );
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
            bool const result = Serializer::SerializeEntity( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, pEntity, serializedState.m_desc );
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
            }
        }

        RecordSelection();
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
            bool const result = Serializer::SerializeEntity( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, pEntity, serializedState.m_desc );
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
            }
        }

        m_editedEntities.clear();
    }

    //-------------------------------------------------------------------------

    void EntityUndoableAction::Undo()
    {
        EntityWorld* pWorld = m_pWorkspace->m_pWorld;

        switch ( m_actionType )
        {
            case EntityUndoableAction::CreateEntities:
            {
                for ( auto const& serializedEntity : m_createdEntities )
                {
                    auto pMap = pWorld->GetMap( serializedEntity.m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    auto pEntity = pMap->FindEntityByName( serializedEntity.m_desc.m_name );
                    pMap->DestroyEntity( pEntity->GetID() );
                }

                // Ensure all entities are fully removed (components unregistered)
                pWorld->ProcessAllRemovalRequests();
            }
            break;

            case EntityUndoableAction::DeleteEntities:
            {
                // Recreate all entities
                for ( auto const& serializedEntity : m_deletedEntities )
                {
                    auto pEntity = Serializer::CreateEntity( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, serializedEntity.m_desc );
                    auto pMap = pWorld->GetMap( serializedEntity.m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    pMap->AddEntity( pEntity );
                }

                // Fix up spatial hierarchy
                int32_t const numDeletedEntities = (int32_t) m_deletedEntities.size();
                for ( int32_t i = 0; i < numDeletedEntities; i++ )
                {
                    if ( m_deletedEntities[i].m_desc.HasSpatialParent() )
                    {
                        auto pMap = pWorld->GetMap( m_deletedEntities[i].m_mapID );
                        EE_ASSERT( pMap != nullptr );
                        auto pParentEntity = pMap->FindEntityByName( m_deletedEntities[i].m_desc.m_spatialParentName );
                        EE_ASSERT( pParentEntity != nullptr );
                        auto pChildEntity = pMap->FindEntityByName( m_deletedEntities[i].m_desc.m_name );
                        EE_ASSERT( pChildEntity != nullptr );

                        pChildEntity->SetSpatialParent( pParentEntity, m_deletedEntities[i].m_desc.m_attachmentSocketID, Entity::SpatialAttachmentRule::KeepLocalTranform );
                    }
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
                    auto pMap = pWorld->GetMap( m_entityDescPostModification[i].m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    auto pExistingEntity = pMap->FindEntityByName( m_entityDescPostModification[i].m_desc.m_name );
                    EE_ASSERT( pExistingEntity != nullptr );
                    pMap->DestroyEntity( pExistingEntity->GetID() );
                }

                //-------------------------------------------------------------------------

                // Ensure all entities are fully removed (components unregistered) before re-adding them
                pWorld->ProcessAllRemovalRequests();

                // Recreate Entities
                //-------------------------------------------------------------------------

                // Create entities
                TVector<Entity*> createdEntities;
                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    auto pNewEntity = Serializer::CreateEntity( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, m_entityDescPreModification[i].m_desc );
                    createdEntities.emplace_back( pNewEntity );
                }

                // Set spatial hierarchy
                // Since we record the entire spatial hierarchy for modifications, all hierarchy entities should be in the created entities list
                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    if ( m_entityDescPreModification[i].m_desc.HasSpatialParent() )
                    {
                        auto Comparator = [] ( Entity const* pEntity, StringID nameID )
                        {
                            return pEntity->GetNameID() == nameID;
                        };

                        auto const parentIter = VectorFind( createdEntities, m_entityDescPreModification[i].m_desc.m_spatialParentName, Comparator );
                        EE_ASSERT( parentIter != createdEntities.end() );
                        auto pParentEntity = *parentIter;

                        auto const childIter = VectorFind( createdEntities, m_entityDescPreModification[i].m_desc.m_name, Comparator );
                        EE_ASSERT( childIter != createdEntities.end() );
                        auto pChildEntity = *childIter;

                        pChildEntity->SetSpatialParent( pParentEntity, m_entityDescPreModification[i].m_desc.m_attachmentSocketID, Entity::SpatialAttachmentRule::KeepLocalTranform );
                    }
                }

                // Add to map
                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    auto pMap = pWorld->GetMap( m_entityDescPostModification[i].m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    pMap->AddEntity( createdEntities[i] );
                }
            }
            break;

            case EntityUndoableAction::TransformUpdate:
            {
                int32_t const numEntities = (int32_t) m_entityDescPostModification.size();
                EE_ASSERT( m_entityDescPostModification.size() == m_entityDescPreModification.size() );

                // Destroy old entities
                //-------------------------------------------------------------------------

                if ( m_entitiesWereDuplicated )
                {
                    for ( int32_t i = 0; i < numEntities; i++ )
                    {
                        // Remove current entities
                        auto pMap = pWorld->GetMap( m_entityDescPostModification[i].m_mapID );
                        EE_ASSERT( pMap != nullptr );
                        auto pExistingEntity = pMap->FindEntityByName( m_entityDescPostModification[i].m_desc.m_name );
                        EE_ASSERT( pExistingEntity != nullptr );
                        pMap->DestroyEntity( pExistingEntity->GetID() );
                    }

                    // Ensure all entities are fully removed (components unregistered) before re-adding them
                    pWorld->ProcessAllRemovalRequests();
                }

                // Restore original transforms
                //-------------------------------------------------------------------------

                if( !m_entitiesWereDuplicated )
                {
                    for ( auto const& st : m_transformsPreModification )
                    {
                        auto pEntity = pWorld->FindEntity( st.m_entityID );
                        auto pSpatialComponent = TryCast<SpatialEntityComponent>( pEntity->FindComponent( st.m_componentID ) );
                        pSpatialComponent->SetLocalTransform( st.m_transform );
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
        EntityWorld* pWorld = m_pWorkspace->m_pWorld;

        switch ( m_actionType )
        {
            case EntityUndoableAction::CreateEntities:
            {
                // Recreate entities
                for ( auto const& serializedEntity : m_createdEntities )
                {
                    auto pMap = pWorld->GetMap( serializedEntity.m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    auto pEntity = Serializer::CreateEntity( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, serializedEntity.m_desc );
                    pMap->AddEntity( pEntity );
                }

                // Fix up spatial hierarchy
                int32_t const numCreatedEntities = (int32_t) m_createdEntities.size();
                for ( int32_t i = 0; i < numCreatedEntities; i++ )
                {
                    if ( m_createdEntities[i].m_desc.HasSpatialParent() )
                    {
                        auto pMap = pWorld->GetMap( m_createdEntities[i].m_mapID );
                        EE_ASSERT( pMap != nullptr );
                        auto pParentEntity = pMap->FindEntityByName( m_createdEntities[i].m_desc.m_spatialParentName );
                        EE_ASSERT( pParentEntity != nullptr );
                        auto pChildEntity = pMap->FindEntityByName( m_createdEntities[i].m_desc.m_name );
                        EE_ASSERT( pChildEntity != nullptr );

                        pChildEntity->SetSpatialParent( pParentEntity, m_createdEntities[i].m_desc.m_attachmentSocketID, Entity::SpatialAttachmentRule::KeepLocalTranform );
                    }
                }
            }
            break;

            case EntityUndoableAction::DeleteEntities:
            {
                for ( auto const& serializedEntity : m_deletedEntities )
                {
                    auto pMap = pWorld->GetMap( serializedEntity.m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    auto pEntity = pMap->FindEntityByName( serializedEntity.m_desc.m_name );
                    pMap->DestroyEntity( pEntity->GetID() );
                }

                // Ensure all entities are fully removed (components unregistered) before re-adding them
                pWorld->ProcessAllRemovalRequests();
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
                    auto pMap = pWorld->GetMap( m_entityDescPreModification[i].m_mapID );
                    EE_ASSERT( pMap != nullptr );

                    auto pExistingEntity = pMap->FindEntityByName( m_entityDescPreModification[i].m_desc.m_name );
                    EE_ASSERT( pExistingEntity != nullptr );
                    pMap->DestroyEntity( pExistingEntity->GetID() );
                }

                // Ensure all entities are fully removed (components unregistered) before re-adding them
                pWorld->ProcessAllRemovalRequests();

                // Recreate modified entities
                //-------------------------------------------------------------------------

                // Create entities
                TVector<Entity*> createdEntities;
                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    auto pNewEntity = Serializer::CreateEntity( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, m_entityDescPostModification[i].m_desc );
                    createdEntities.emplace_back( pNewEntity );
                }

                // Restore spatial hierarchy
                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    if ( m_entityDescPostModification[i].m_desc.HasSpatialParent() )
                    {
                        auto Comparator = [] ( Entity const* pEntity, StringID nameID )
                        {
                            return pEntity->GetNameID() == nameID;
                        };

                        auto const parentIter = VectorFind( createdEntities, m_entityDescPostModification[i].m_desc.m_spatialParentName, Comparator );
                        EE_ASSERT( parentIter != createdEntities.end() );
                        auto pParentEntity = *parentIter;

                        auto const childIter = VectorFind( createdEntities, m_entityDescPostModification[i].m_desc.m_name, Comparator );
                        EE_ASSERT( childIter != createdEntities.end() );
                        auto pChildEntity = *childIter;

                        pChildEntity->SetSpatialParent( pParentEntity, m_entityDescPostModification[i].m_desc.m_attachmentSocketID, Entity::SpatialAttachmentRule::KeepLocalTranform );
                    }
                }

                // Add entities to map
                for ( int32_t i = 0; i < numEntities; i++ )
                {
                    auto pMap = pWorld->GetMap( m_entityDescPostModification[i].m_mapID );
                    EE_ASSERT( pMap != nullptr );
                    pMap->AddEntity( createdEntities[i] );
                }
            }
            break;

            case EntityUndoableAction::TransformUpdate:
            {
                int32_t const numEntities = (int32_t) m_entityDescPreModification.size();
                EE_ASSERT( m_entityDescPostModification.size() == m_entityDescPreModification.size() );

                // Recreate Duplicate Entities
                //-------------------------------------------------------------------------

                if ( m_entitiesWereDuplicated )
                {
                    // Create entities
                    TVector<Entity*> createdEntities;
                    for ( int32_t i = 0; i < numEntities; i++ )
                    {
                        auto pNewEntity = Serializer::CreateEntity( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, m_entityDescPostModification[i].m_desc );
                        createdEntities.emplace_back( pNewEntity );
                    }

                    // Restore spatial hierarchy
                    for ( int32_t i = 0; i < numEntities; i++ )
                    {
                        if ( m_entityDescPostModification[i].m_desc.HasSpatialParent() )
                        {
                            auto Comparator = [] ( Entity const* pEntity, StringID nameID )
                            {
                                return pEntity->GetNameID() == nameID;
                            };

                            auto const parentIter = VectorFind( createdEntities, m_entityDescPostModification[i].m_desc.m_spatialParentName, Comparator );
                            EE_ASSERT( parentIter != createdEntities.end() );
                            auto pParentEntity = *parentIter;

                            auto const childIter = VectorFind( createdEntities, m_entityDescPostModification[i].m_desc.m_name, Comparator );
                            EE_ASSERT( childIter != createdEntities.end() );
                            auto pChildEntity = *childIter;

                            pChildEntity->SetSpatialParent( pParentEntity, m_entityDescPostModification[i].m_desc.m_attachmentSocketID, Entity::SpatialAttachmentRule::KeepLocalTranform );
                        }
                    }

                    // Add entities to map
                    for ( int32_t i = 0; i < numEntities; i++ )
                    {
                        auto pMap = pWorld->GetMap( m_entityDescPostModification[i].m_mapID );
                        EE_ASSERT( pMap != nullptr );
                        pMap->AddEntity( createdEntities[i] );
                    }
                }

                // Set new transforms
                //-------------------------------------------------------------------------

                for ( auto const& st : m_transformsPostModification )
                {
                    auto pEntity = pWorld->FindEntity( st.m_entityID );
                    auto pSpatialComponent = TryCast<SpatialEntityComponent>( pEntity->FindComponent( st.m_componentID ) );
                    pSpatialComponent->SetLocalTransform( st.m_transform );
                }
            }
            break;

            default:
            EE_UNREACHABLE_CODE();
            break;
        }
    }

    //-------------------------------------------------------------------------

    void EntityUndoableAction::RecordSelection()
    {
        m_selection.Clear();

        if ( m_pWorkspace->m_outlinerTreeView.HasSelection() )
        {
            for ( auto pSelectedItem : m_pWorkspace->m_outlinerTreeView.GetSelection() )
            {
                m_selection.m_selectedEntities.emplace_back( EntityID( pSelectedItem->GetUniqueID() ) );
            }

            for ( auto pSelectedItem : m_pWorkspace->m_structureEditorTreeView.GetSelection() )
            {
                m_selection.m_selectedComponents.emplace_back( ComponentID( pSelectedItem->GetUniqueID() ) );
            }
        }
    }
}

//-------------------------------------------------------------------------
// Workspace
//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityEditorWorkspace::EntityEditorWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
        : Workspace( pToolsContext, pWorld, resourceID )
        , m_allSystemTypes( pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( EntitySystem::GetStaticTypeID(), false, false, true ) )
        , m_allComponentTypes( pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( EntityComponent::GetStaticTypeID(), false, false, true ) )
        , m_propertyGrid( pToolsContext )
    {}

    EntityEditorWorkspace::EntityEditorWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, String const& displayName )
        : Workspace( pToolsContext, pWorld, displayName )
        , m_allSystemTypes( pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( EntitySystem::GetStaticTypeID(), false, false, true ) )
        , m_allComponentTypes( pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( EntityComponent::GetStaticTypeID(), false, false, true ) )
        , m_propertyGrid( pToolsContext )
    {}

    EntityEditorWorkspace::~EntityEditorWorkspace()
    {
    }

    //-------------------------------------------------------------------------

    void EntityEditorWorkspace::Initialize( UpdateContext const& context )
    {
        Workspace::Initialize( context );

        // Setup Tree Views
        //-------------------------------------------------------------------------

        m_outlinerContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildOutlinerTree( pRootItem ); };
        m_outlinerContext.m_drawItemContextMenuFunction = [this] ( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) { DrawOutlinerContextMenu( selectedItemsWithContextMenus ); };
        m_outlinerContext.m_onDragAndDropFunction = [this] ( TreeListViewItem* pDropTarget ) { HandleOutlinerDragAndDrop( pDropTarget ); };

        m_outlinerTreeView.SetFlag( TreeListView::Flags::ShowBranchesFirst, false );
        m_outlinerTreeView.SetFlag( TreeListView::Flags::ExpandItemsOnlyViaArrow );
        m_outlinerTreeView.SetFlag( TreeListView::Flags::MultiSelectionAllowed );
        m_outlinerTreeView.SetFlag( TreeListView::Flags::SortTree );
        m_outlinerTreeView.SetFlag( TreeListView::Flags::ViewTracksSelection );

        //-------------------------------------------------------------------------

        m_structureEditorContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildStructureEditorTree( pRootItem ); };
        m_structureEditorContext.m_drawItemContextMenuFunction = [this] ( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) { DrawStructureEditorContextMenu( selectedItemsWithContextMenus ); };
        m_structureEditorContext.m_onDragAndDropFunction = [this] ( TreeListViewItem* pDropTarget ) { HandleStructureEditorDragAndDrop( pDropTarget ); };

        m_structureEditorTreeView.SetFlag( TreeListView::Flags::ShowBranchesFirst, false );
        m_structureEditorTreeView.SetFlag( TreeListView::Flags::ExpandItemsOnlyViaArrow );
        m_structureEditorTreeView.SetFlag( TreeListView::Flags::MultiSelectionAllowed, false );
        m_structureEditorTreeView.SetFlag( TreeListView::Flags::DrawRowBackground, false );
        m_structureEditorTreeView.SetFlag( TreeListView::Flags::DrawBorders, false );
        m_structureEditorTreeView.SetFlag( TreeListView::Flags::SortTree );

        // Split components
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < m_allComponentTypes.size(); i++ )
        {
            if ( m_allComponentTypes[i]->IsDerivedFrom( SpatialEntityComponent::GetStaticTypeID() ) )
            {
                m_allSpatialComponentTypes.emplace_back( m_allComponentTypes[i] );
                m_allComponentTypes.erase( m_allComponentTypes.begin() + i );
                i--;
            }
        }

        // Event Bindings
        //-------------------------------------------------------------------------

        m_preEditPropertyBindingID = m_propertyGrid.OnPreEdit().Bind( [this] ( PropertyEditInfo const& eventInfo ) { PreEditProperty( eventInfo ); } );
        m_postEditPropertyBindingID = m_propertyGrid.OnPostEdit().Bind( [this] ( PropertyEditInfo const& eventInfo ) { PostEditProperty( eventInfo ); } );

        // Visualizations
        //-------------------------------------------------------------------------

        m_volumeTypes = context.GetSystem<TypeSystem::TypeRegistry>()->GetAllDerivedTypes( VolumeComponent::GetStaticTypeID(), false, false, true );

        // Tool Windows
        //-------------------------------------------------------------------------

        CreateToolWindow( "Outliner", [this] ( UpdateContext const& context, bool isFocused ) { DrawOutliner( context, isFocused ); } );
        CreateToolWindow( "Entity", [this] ( UpdateContext const& context, bool isFocused ) { DrawStructureEditor( context, isFocused ); } );
        CreateToolWindow( "Properties", [this] ( UpdateContext const& context, bool isFocused ) { DrawPropertyGrid( context, isFocused ); } );

        HideDescriptorWindow();
    }

    void EntityEditorWorkspace::Shutdown( UpdateContext const& context )
    {
        m_propertyGrid.OnPreEdit().Unbind( m_preEditPropertyBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_postEditPropertyBindingID );

        //-------------------------------------------------------------------------

        Workspace::Shutdown( context );
    }

    void EntityEditorWorkspace::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID topDockID = 0, bottomLeftDockID = 0, bottomCenterDockID = 0, bottomRightDockID = 0;
        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.5f, &bottomCenterDockID, &topDockID );
        ImGui::DockBuilderSplitNode( bottomCenterDockID, ImGuiDir_Right, 0.66f, &bottomCenterDockID, &bottomLeftDockID );
        ImGui::DockBuilderSplitNode( bottomCenterDockID, ImGuiDir_Right, 0.5f, &bottomRightDockID, &bottomCenterDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Viewport" ).c_str(), topDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Outliner" ).c_str(), bottomLeftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Properties" ).c_str(), bottomCenterDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Descriptor" ).c_str(), bottomRightDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Entity" ).c_str(), bottomRightDockID );
    }

    void EntityEditorWorkspace::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        EntityUndoableAction const* pUndoableAction = static_cast<EntityUndoableAction const*>( pAction );

        if ( pUndoableAction->GetActionType() != EntityUndoableAction::TransformUpdate )
        {
            m_outlinerState = TreeState::NeedsRebuild;
            m_structureEditorState = TreeState::NeedsRebuild;
            m_propertyGrid.SetTypeToEdit( nullptr );
        }

        //-------------------------------------------------------------------------

        if ( operation == UndoStack::Operation::Undo )
        {
            m_selectionSwitchRequest = pUndoableAction->GetRecordedSelection();
        }
    }

    void EntityEditorWorkspace::DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        Workspace::DrawViewportToolbar( context, pViewport );

        //-------------------------------------------------------------------------

        ImGui::SameLine();

        //-------------------------------------------------------------------------

        auto const& style = ImGui::GetStyle();
        float const separatorWidth = 3;
        float const widthButton0 = ImGui::CalcTextSize( m_gizmo.IsInWorldSpace() ? EE_ICON_EARTH : EE_ICON_MAP_MARKER ).x + ( style.FramePadding.x * 2 );
        float const widthButton1 = ImGui::CalcTextSize( EE_ICON_AXIS_ARROW ).x + ( style.FramePadding.x * 2 );
        float const widthButton2 = ImGui::CalcTextSize( EE_ICON_ROTATE_ORBIT ).x + ( style.FramePadding.x * 2 );
        float const widthButton3 = ImGui::CalcTextSize( EE_ICON_ARROW_TOP_RIGHT_BOTTOM_LEFT ).x + ( style.FramePadding.x * 2 );
        float const requiredWidth = widthButton0 + widthButton1 + widthButton2 + widthButton3 + separatorWidth;

        if ( BeginViewportToolbarGroup( "SpatialControls", ImVec2( requiredWidth, 0 ), ImVec2( 0, 0 ) ) )
        {
            TInlineString<10> const coordinateSpaceSwitcherLabel( TInlineString<10>::CtorSprintf(), "%s##CoordinateSpace", m_gizmo.IsInWorldSpace() ? EE_ICON_EARTH : EE_ICON_MAP_MARKER );
            ImGui::BeginDisabled( m_gizmo.GetMode() == ImGuiX::Gizmo::GizmoMode::Scale );
            if ( ImGuiX::FlatButton( coordinateSpaceSwitcherLabel.c_str(), ImVec2( widthButton0, ImGui::GetContentRegionAvail().y ) ) )
            {
                m_gizmo.SetCoordinateSystemSpace( m_gizmo.IsInWorldSpace() ? CoordinateSpace::Local : CoordinateSpace::World );
            }
            ImGui::EndDisabled();
            ImGuiX::ItemTooltip( "Current Mode: %s", m_gizmo.IsInWorldSpace() ? "World Space" : "Local Space" );

            //-------------------------------------------------------------------------

            ImGuiX::SameLineSeparator( separatorWidth );

            //-------------------------------------------------------------------------

            bool t = m_gizmo.GetMode() == ImGuiX::Gizmo::GizmoMode::Translation;
            ImGui::PushStyleColor( ImGuiCol_Text, t ? ImGuiX::Style::s_colorAccent1.Value : ImGuiX::Style::s_colorText.Value );
            if ( ImGuiX::FlatButton( EE_ICON_AXIS_ARROW, ImVec2( widthButton1, ImGui::GetContentRegionAvail().y ) ) )
            {
                m_gizmo.SwitchMode( ImGuiX::Gizmo::GizmoMode::Translation );
            }
            ImGui::PopStyleColor();
            ImGuiX::ItemTooltip( "Translate" );

            ImGui::SameLine( 0, 0 );

            bool r = m_gizmo.GetMode() == ImGuiX::Gizmo::GizmoMode::Rotation;
            ImGui::PushStyleColor( ImGuiCol_Text, r ? ImGuiX::Style::s_colorAccent1.Value : ImGuiX::Style::s_colorText.Value );
            if ( ImGuiX::FlatButton( EE_ICON_ROTATE_ORBIT, ImVec2( widthButton2, ImGui::GetContentRegionAvail().y ) ) )
            {
                m_gizmo.SwitchMode( ImGuiX::Gizmo::GizmoMode::Rotation );
            }
            ImGui::PopStyleColor();
            ImGuiX::ItemTooltip( "Rotate" );

            ImGui::SameLine( 0, 0 );

            bool s = m_gizmo.GetMode() == ImGuiX::Gizmo::GizmoMode::Scale;
            ImGui::PushStyleColor( ImGuiCol_Text, s ? ImGuiX::Style::s_colorAccent1.Value : ImGuiX::Style::s_colorText.Value );
            if ( ImGuiX::FlatButton( EE_ICON_ARROW_TOP_RIGHT_BOTTOM_LEFT, ImVec2( widthButton3, ImGui::GetContentRegionAvail().y ) ) )
            {
                m_gizmo.SwitchMode( ImGuiX::Gizmo::GizmoMode::Scale );
            }
            ImGui::PopStyleColor();
            ImGuiX::ItemTooltip( "Scale" );
        }
        EndViewportToolbarGroup();
    }

    void EntityEditorWorkspace::DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        auto drawingCtx = GetDrawingContext();

        // Overlay Widgets
        //-------------------------------------------------------------------------

        //auto DrawComponentWorldIcon = [this, pViewport] ( SpatialEntityComponent* pComponent, char icon[4], bool isSelected )
        //{
        //    if ( pViewport->IsWorldSpacePointVisible( pComponent->GetPosition() ) )
        //    {
        //        ImVec2 const positionScreenSpace = pViewport->WorldSpaceToScreenSpace( pComponent->GetPosition() );
        //        if ( ImGuiX::DrawOverlayIcon( positionScreenSpace, icon, pComponent, isSelected, ImColor( Colors::Yellow.ToFloat4() ) ) )
        //        {
        //            return true;
        //        }
        //    }

        //    return false;
        //};

        //auto const& registeredLights = m_pWorld->GetAllRegisteredComponentsOfType<Render::LightComponent>();
        //for ( auto pComponent : registeredLights )
        //{
        //    auto pLightComponent = const_cast<Render::LightComponent*>( pComponent );
        //    bool const isSelected = m_context.HasSingleSelectedComponent() && m_context.IsSelected( pComponent );
        //    if ( DrawComponentWorldIcon( pLightComponent, EE_ICON_LIGHTBULB, isSelected ) )
        //    {
        //        auto pEntity = pEditedMap->FindEntity( pLightComponent->GetEntityID() );
        //        m_context.SelectEntity( pEntity );
        //        m_context.SelectComponent( pLightComponent );
        //    }
        //}

        //auto const& registeredPlayerSpawns = m_pWorld->GetAllRegisteredComponentsOfType<Player::PlayerSpawnComponent>();
        //for ( auto pComponent : registeredPlayerSpawns )
        //{
        //    auto pSpawnComponent = const_cast<Player::PlayerSpawnComponent*>( pComponent );
        //    bool const isSelected = m_context.HasSingleSelectedComponent() && m_context.IsSelected( pComponent );
        //    if ( DrawComponentWorldIcon( pSpawnComponent, EE_ICON_GAMEPAD, isSelected ) )
        //    {
        //        auto pEntity = pEditedMap->FindEntity( pSpawnComponent->GetEntityID() );
        //        m_context.SelectEntity( pEntity );
        //        m_context.SelectComponent( pSpawnComponent );
        //    }

        //    auto const& spawnTransform = pComponent->GetWorldTransform();
        //    drawingCtx.DrawArrow( spawnTransform.GetTranslation(), spawnTransform.GetForwardVector(), 0.5f, Colors::Yellow, 3.0f );
        //}

        //auto const& registeredAISpawns = m_pWorld->GetAllRegisteredComponentsOfType<AI::AISpawnComponent>();
        //for ( auto pComponent : registeredAISpawns )
        //{
        //    auto pSpawnComponent = const_cast<AI::AISpawnComponent*>( pComponent );
        //    bool const isSelected = m_context.HasSingleSelectedComponent() && m_context.IsSelected( pComponent );
        //    if ( DrawComponentWorldIcon( pSpawnComponent, EE_ICON_ROBOT, isSelected ) )
        //    {
        //        auto pEntity = pEditedMap->FindEntity( pSpawnComponent->GetEntityID() );
        //        m_context.SelectEntity( pEntity );
        //        m_context.SelectComponent( pSpawnComponent );
        //    }

        //    auto const& spawnTransform = pComponent->GetWorldTransform();
        //    drawingCtx.DrawArrow( spawnTransform.GetTranslation(), spawnTransform.GetForwardVector(), 0.5f, Colors::Yellow, 3.0f );
        //}

        //// Draw light debug widgets
        ////-------------------------------------------------------------------------
        //// TODO: generalized component visualizers

        //if ( m_context.HasSingleSelectedComponent() )
        //{
        //    auto pSelectedComponent = m_context.GetSelectedComponent();
        //    if ( IsOfType<Render::DirectionalLightComponent>( pSelectedComponent ) )
        //    {
        //        auto pLightComponent = Cast<Render::DirectionalLightComponent>( pSelectedComponent );
        //        auto forwardDir = pLightComponent->GetForwardVector();
        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() + forwardDir, pLightComponent->GetLightColor(), 3.0f );
        //    }
        //    else if ( IsOfType<Render::SpotLightComponent>( pSelectedComponent ) )
        //    {
        //        auto pLightComponent = Cast<Render::SpotLightComponent>( pSelectedComponent );
        //        drawingCtx.DrawCone( pLightComponent->GetWorldTransform(), pLightComponent->GetLightInnerUmbraAngle(), 1.5f, pLightComponent->GetLightColor(), 3.0f );
        //        drawingCtx.DrawCone( pLightComponent->GetWorldTransform(), pLightComponent->GetLightOuterUmbraAngle(), 1.5f, pLightComponent->GetLightColor(), 3.0f );
        //    }
        //    else if ( IsOfType<Render::PointLightComponent>( pSelectedComponent ) )
        //    {
        //        auto pLightComponent = Cast<Render::PointLightComponent>( pSelectedComponent );
        //        auto forwardDir = pLightComponent->GetForwardVector();
        //        auto upDir = pLightComponent->GetUpVector();
        //        auto rightDir = pLightComponent->GetRightVector();

        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() + ( forwardDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() - ( forwardDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() + ( upDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() - ( upDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() + ( rightDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
        //        drawingCtx.DrawArrow( pLightComponent->GetPosition(), pLightComponent->GetPosition() - ( rightDir * 0.5f ), pLightComponent->GetLightColor(), 3.0f );
        //    }
        //}

        //// Volume Debug
        ////-------------------------------------------------------------------------

        //for ( auto pVisualizedVolumeTypeInfo : m_visualizedVolumeTypes )
        //{
        //    auto const& volumeComponents = m_pWorld->GetAllRegisteredComponentsOfType( pVisualizedVolumeTypeInfo->m_ID );
        //    for ( auto pComponent : volumeComponents )
        //    {
        //        auto pVolumeComponent = static_cast<VolumeComponent const*>( pComponent );
        //        pVolumeComponent->Draw( drawingCtx );
        //    }
        //}

        // Selection and Manipulation
        //-------------------------------------------------------------------------

        UpdateSelectionSpatialInfo();
        if ( m_hasSpatialSelection )
        {
            // Draw selection bounds
            //-------------------------------------------------------------------------

            for ( auto pEntity : GetSelectedSpatialEntities() )
            {
                if ( pEntity->IsSpatialEntity() )
                {
                    for ( auto pComponent : pEntity->GetComponents() )
                    {
                        if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent ) )
                        {
                            //drawingCtx.DrawWireBox( pSpatialComponent->GetWorldBounds(), Colors::Cyan, 3.0f, Drawing::EnableDepthTest );
                        }
                    }

                    //drawingCtx.DrawWireBox( pEntity->GetCombinedWorldBounds(), Colors::Orange, 3.0f, Drawing::EnableDepthTest );
                }
            }

            drawingCtx.DrawWireBox( m_selectionBounds, Colors::Yellow, 3.0f, Drawing::EnableDepthTest );

            // Update Gizmo
            //-------------------------------------------------------------------------

            auto const gizmoResult = m_gizmo.Draw( m_selectionTransform.GetTranslation(), m_selectionTransform.GetRotation(), *m_pWorld->GetViewport() );
            switch ( gizmoResult.m_state )
            {
                case ImGuiX::Gizmo::State::StartedManipulating:
                {
                    bool const hasSelectedComponent = !GetSelectedSpatialComponents().empty(); // Disallow component duplication
                    bool const duplicateSelection = !hasSelectedComponent && ( m_gizmo.GetMode() == ImGuiX::Gizmo::GizmoMode::Translation ) && ImGui::GetIO().KeyAlt;
                    BeginTransformManipulation( gizmoResult.GetModifiedTransform( m_selectionTransform ), duplicateSelection );
                }
                break;

                case ImGuiX::Gizmo::State::Manipulating:
                {
                    ApplyTransformManipulation( gizmoResult.GetModifiedTransform( m_selectionTransform ) );
                }
                break;

                case ImGuiX::Gizmo::State::StoppedManipulating:
                {
                    EndTransformManipulation( gizmoResult.GetModifiedTransform( m_selectionTransform ) );
                }
                break;

                default:
                break;
            }
        }
    }

    //-------------------------------------------------------------------------
    // Viewport
    //-------------------------------------------------------------------------

    void EntityEditorWorkspace::DropResourceInViewport( ResourceID const& resourceID, Vector const& worldPosition )
    {
        // Static Mesh Resource
        //-------------------------------------------------------------------------

        if ( resourceID.GetResourceTypeID() == Render::StaticMesh::GetStaticResourceTypeID() )
        {
            auto pMap = m_pWorld->GetFirstNonPersistentMap();
            if ( pMap != nullptr )
            {
                // Create entity
                StringID const entityName( resourceID.GetFileNameWithoutExtension().c_str() );
                auto pMeshEntity = EE::New<Entity>( entityName );

                // Create mesh component
                auto pMeshComponent = EE::New<Render::StaticMeshComponent>( StringID( "Mesh Component" ) );
                pMeshComponent->SetMesh( resourceID );
                pMeshComponent->SetWorldTransform( Transform( Quaternion::Identity, worldPosition ) );
                pMeshEntity->AddComponent( pMeshComponent );

                // Try create optional physics collision component
                ResourcePath physicsResourcePath = resourceID.GetResourcePath();
                physicsResourcePath.ReplaceExtension( Physics::CollisionMesh::GetStaticResourceTypeID().ToString() );
                ResourceID const physicsResourceID( physicsResourcePath );
                if ( m_pToolsContext->m_pResourceDatabase->DoesResourceExist( physicsResourceID ) )
                {
                    auto pPhysicsMeshComponent = EE::New<Physics::CollisionMeshComponent>( StringID( "Physics Component" ) );
                    pPhysicsMeshComponent->SetCollision( physicsResourceID );
                    pMeshEntity->AddComponent( pPhysicsMeshComponent );
                }

                // Add entity to map
                pMap->AddEntity( pMeshEntity );

                // Record undo action
                auto pActiveUndoableAction = EE::New<EntityUndoableAction>( this );
                pActiveUndoableAction->RecordCreate( { pMeshEntity } );
                m_undoStack.RegisterAction( pActiveUndoableAction );

                // Refresh outliner and switch selection
                m_outlinerState = TreeState::NeedsRebuild;
                m_selectionSwitchRequest.Clear();
                m_selectionSwitchRequest.m_selectedEntities.emplace_back( pMeshEntity->GetID() );
            }
        }
    }

    void EntityEditorWorkspace::OnMousePick( Render::PickingID pickingID )
    {
        if ( !pickingID.IsSet() )
        {
            m_outlinerTreeView.ClearSelection();
            return;
        }

        //-------------------------------------------------------------------------

        if ( m_gizmo.IsManipulating() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        EntityID const pickedEntityID( pickingID.m_0 );
        auto pEntity = m_pWorld->FindEntity( pickedEntityID );
        if ( pEntity == nullptr )
        {
            m_outlinerTreeView.ClearSelection();
            return;
        }

        //-------------------------------------------------------------------------

        // If we have alt-held, select the individual component
        if ( ImGui::GetIO().KeyAlt )
        {
            ComponentID const pickedComponentID( pickingID.m_1 );
            auto pComponent = pEntity->FindComponent( pickedComponentID );
            EE_ASSERT( pComponent != nullptr );

            m_selectionSwitchRequest.Clear();
            m_selectionSwitchRequest.m_selectedEntities.emplace_back( pEntity->GetID() );
            m_selectionSwitchRequest.m_selectedComponents.emplace_back( pComponent->GetID() );
        }
        // Else just select the entire entity
        else if ( ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl )
        {
            uint64_t const itemID = pEntity->GetID().m_value;
            if ( m_outlinerTreeView.IsItemSelected( itemID ) )
            {
                m_outlinerTreeView.RemoveFromSelection( itemID );
            }
            else
            {
                m_outlinerTreeView.AddToSelection( itemID );
            }
        }
        else // No modifier so just set selection
        {
            uint64_t const itemID = pEntity->GetID().m_value;
            if ( auto pItem = m_outlinerTreeView.FindItem( itemID ) )
            {
                m_outlinerTreeView.SetSelection( pItem );
            }
        }

        m_structureEditorState = TreeState::NeedsRebuild;
    }

    void EntityEditorWorkspace::UpdateSelectionSpatialInfo()
    {
        TVector<OBB> bounds;

        m_selectionTransform = Transform::Identity;
        m_selectionOffsetTransforms.clear();
        m_hasSpatialSelection = false;

        // Check if we have spatial selection
        //-------------------------------------------------------------------------
        // Record the the transforms of all found spatial elements in the offset array

        // Check all selected components
        auto const& selectedSpatialComponents = GetSelectedSpatialComponents();
        if ( selectedSpatialComponents.size() > 0 )
        {
            for ( auto pComponent : selectedSpatialComponents )
            {
                m_selectionOffsetTransforms.emplace_back( pComponent->GetWorldTransform() );
                bounds.emplace_back( pComponent->GetWorldBounds() );
                m_hasSpatialSelection = true;
            }
        }
        else // Check all selected entities
        {
            for ( auto pSelectedEntity : GetSelectedSpatialEntities() )
            {
                if ( pSelectedEntity->IsSpatialEntity() )
                {
                    m_selectionOffsetTransforms.emplace_back( pSelectedEntity->GetWorldTransform() );
                    bounds.emplace_back( pSelectedEntity->GetCombinedWorldBounds() ); // TODO: calculate combined bounds
                    m_hasSpatialSelection = true;
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
                for ( auto& t : m_selectionOffsetTransforms )
                {
                    t = Transform::Delta( m_selectionTransform, t );
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
    // Operations
    //-------------------------------------------------------------------------

    void EntityEditorWorkspace::StartEntityRename( Entity* pEntity )
    {
        EE_ASSERT( pEntity != nullptr );
        Printf( m_operationBuffer, 256, pEntity->GetNameID().c_str() );
        m_activeOperation = Operation::RenameEntity;
    }

    void EntityEditorWorkspace::RequestDestroyEntity( Entity* pEntity )
    {
        EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() && m_pWorld->FindEntity( pEntity->GetID() ) != nullptr );
        m_entityDeletionRequests.emplace_back( pEntity );
    }

    void EntityEditorWorkspace::RequestDestroyEntities( TVector<Entity*> entities )
    {
        for ( auto pEntity : entities )
        {
            EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() && m_pWorld->FindEntity( pEntity->GetID() ) != nullptr );
            m_entityDeletionRequests.emplace_back( pEntity );
        }
    }

    void EntityEditorWorkspace::RequestEntityReparent( Entity* pEntity, Entity* pNewParentEntity )
    {
        EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() && m_pWorld->FindEntity( pEntity->GetID() ) != nullptr );
        if ( pNewParentEntity != nullptr )
        {
            EE_ASSERT( pNewParentEntity->IsAddedToMap() && m_pWorld->FindEntity( pNewParentEntity->GetID() ) != nullptr );
        }

        m_spatialParentingRequests.emplace_back( pEntity, pNewParentEntity );
    }

    void EntityEditorWorkspace::StartEntityOperation( Operation operation, EntityComponent* pTargetComponent /*= nullptr */ )
    {
        Entity* pEditedEntity = GetEditedEntity();

        EE_ASSERT( pEditedEntity != nullptr );
        EE_ASSERT( operation != Operation::None );

        // Set operation common data
        //-------------------------------------------------------------------------

        m_activeOperation = operation;
        m_initializeFocus = true;
        m_pOperationTargetComponent = pTargetComponent;
        m_operationBuffer[0] = 0;
        m_operationOptions.clear();
        m_filteredOptions.clear();

        // Initialize operation custom data
        //-------------------------------------------------------------------------

        if ( operation == Operation::EntityRenameComponent )
        {
            EE_ASSERT( m_pOperationTargetComponent != nullptr );
            Printf( m_operationBuffer, 256, m_pOperationTargetComponent->GetNameID().c_str() );
        }

        // Filter available selection options
        //-------------------------------------------------------------------------

        auto AddAvailableSystemOptions = [this, pEditedEntity] ()
        {
            TInlineVector<TypeSystem::TypeID, 10> restrictions;
            for ( auto pSystem : pEditedEntity->GetSystems() )
            {
                restrictions.emplace_back( pSystem->GetTypeID() );
            }

            //-------------------------------------------------------------------------

            for ( auto pSystemTypeInfo : m_allSystemTypes )
            {
                bool isValidOption = true;
                for ( auto pExistingSystem : pEditedEntity->GetSystems() )
                {
                    if ( m_pToolsContext->m_pTypeRegistry->AreTypesInTheSameHierarchy( pExistingSystem->GetTypeInfo(), pSystemTypeInfo ) )
                    {
                        isValidOption = false;
                        break;
                    }
                }

                if ( isValidOption )
                {
                    m_operationOptions.emplace_back( pSystemTypeInfo );
                }
            }
        };

        auto AddAvailableComponentOptions = [this, pEditedEntity] ()
        {
            TInlineVector<TypeSystem::TypeID, 10> restrictions;
            for ( auto pComponent : pEditedEntity->GetComponents() )
            {
                if ( IsOfType<SpatialEntityComponent>( pComponent ) )
                {
                    continue;
                }

                if ( pComponent->IsSingletonComponent() )
                {
                    restrictions.emplace_back( pComponent->GetTypeID() );
                }
            }

            //-------------------------------------------------------------------------

            for ( auto pComponentTypeInfo : m_allComponentTypes )
            {
                bool isValidOption = true;
                for ( auto pExistingComponent : pEditedEntity->GetComponents() )
                {
                    if ( pExistingComponent->IsSingletonComponent() )
                    {
                        if ( m_pToolsContext->m_pTypeRegistry->AreTypesInTheSameHierarchy( pExistingComponent->GetTypeInfo(), pComponentTypeInfo ) )
                        {
                            isValidOption = false;
                            break;
                        }
                    }
                }

                if ( isValidOption )
                {
                    m_operationOptions.emplace_back( pComponentTypeInfo );
                }
            }
        };

        auto AddAvailableSpatialComponentOptions = [this, pEditedEntity] ()
        {
            if ( m_pOperationTargetComponent != nullptr )
            {
                EE_ASSERT( IsOfType<SpatialEntityComponent>( m_pOperationTargetComponent ) );
            }

            //-------------------------------------------------------------------------

            TInlineVector<TypeSystem::TypeID, 10> restrictions;
            for ( auto pComponent : pEditedEntity->GetComponents() )
            {
                if ( !IsOfType<SpatialEntityComponent>( pComponent ) )
                {
                    continue;
                }

                if ( pComponent->IsSingletonComponent() )
                {
                    restrictions.emplace_back( pComponent->GetTypeID() );
                }
            }

            //-------------------------------------------------------------------------

            for ( auto pComponentTypeInfo : m_allSpatialComponentTypes )
            {
                bool isValidOption = true;
                for ( auto pExistingComponent : pEditedEntity->GetComponents() )
                {
                    if ( pExistingComponent->IsSingletonComponent() )
                    {
                        if ( m_pToolsContext->m_pTypeRegistry->AreTypesInTheSameHierarchy( pExistingComponent->GetTypeInfo(), pComponentTypeInfo ) )
                        {
                            isValidOption = false;
                            break;
                        }
                    }
                }

                if ( isValidOption )
                {
                    m_operationOptions.emplace_back( pComponentTypeInfo );
                }
            }
        };

        if ( operation == Operation::EntityAddSystemOrComponent )
        {
            AddAvailableSystemOptions();
            AddAvailableComponentOptions();
            AddAvailableSpatialComponentOptions();
        }
        else if ( operation == Operation::EntityAddSystem )
        {
            AddAvailableSystemOptions();
        }
        else if ( operation == Operation::EntityAddComponent )
        {
            AddAvailableComponentOptions();
        }
        else if ( operation == Operation::EntityAddSpatialComponent )
        {
            AddAvailableSpatialComponentOptions();
        }

        //-------------------------------------------------------------------------

        if ( m_activeOperation != Operation::EntityRenameComponent )
        {
            m_pOperationSelectedOption = m_operationOptions.front();
            m_filteredOptions = m_operationOptions;
        }
    }

    //-------------------------------------------------------------------------
    // Commands
    //-------------------------------------------------------------------------

    void EntityEditorWorkspace::CreateEntity( EntityMapID const& mapID )
    {
        EE_ASSERT( !m_outlinerTreeView.IsCurrentlyDrawingTree() ); // This function rebuilds the tree so it cannot be called during tree drawing

        auto pMap = m_pWorld->GetMap( mapID );
        EE_ASSERT( pMap != nullptr && pMap->IsLoaded() );

        StringID const uniqueNameID = pMap->GenerateUniqueEntityNameID( "Entity" );
        Entity* pEntity = EE::New<Entity>( StringID( uniqueNameID ) );

        // Add entity to map
        pMap->AddEntity( pEntity );

        // Record undo action
        auto pActiveUndoableAction = EE::New<EntityUndoableAction>( this );
        pActiveUndoableAction->RecordCreate( { pEntity } );
        m_undoStack.RegisterAction( pActiveUndoableAction );

        // Update selection
        m_outlinerState = TreeState::NeedsRebuild;
        m_selectionSwitchRequest.Clear();
        m_selectionSwitchRequest.m_selectedEntities.emplace_back( pEntity->GetID() );
    }

    TVector<Entity*> EntityEditorWorkspace::DuplicateSelectedEntities()
    {
        EE_ASSERT( !m_outlinerTreeView.IsCurrentlyDrawingTree() ); // This function rebuilds the tree so it cannot be called during tree drawing

        TVector<Entity*> duplicatedEntities;

        //-------------------------------------------------------------------------

        TVector<Entity*> const selectedEntities = GetSelectedEntities();
        if ( selectedEntities.empty() )
        {
            return duplicatedEntities;
        }

        // Serialized all selected entities
        //-------------------------------------------------------------------------

        TVector<TPair<EntityMap*, SerializedEntityDescriptor>> entityDescs;
        for ( auto pEntity : selectedEntities )
        {
            auto& serializedEntity = entityDescs.emplace_back();
            serializedEntity.first = m_pWorld->GetMapForEntity( pEntity );
            EE_ASSERT( serializedEntity.first != nullptr );

            if ( Serializer::SerializeEntity( *m_pToolsContext->m_pTypeRegistry, pEntity, serializedEntity.second ) )
            {
                // Clear the serialized ID so that we will generate a new ID for the duplicated entities
                serializedEntity.second.ClearAllSerializedIDs();
            }
            else
            {
                entityDescs.pop_back();
            }
        }

        // Duplicate the entities and add them to the map
        //-------------------------------------------------------------------------

        bool hasSpatialEntities = false;
        THashMap<StringID, StringID> nameRemap;
        int32_t const numEntitiesToDuplicate = (int32_t) entityDescs.size();
        for ( int32_t i = 0; i < numEntitiesToDuplicate; i++ )
        {
            // Create the duplicated entity
            auto pDuplicatedEntity = Serializer::CreateEntity( *m_pToolsContext->m_pTypeRegistry, entityDescs[i].second );
            duplicatedEntities.emplace_back( pDuplicatedEntity );

            // Add to map
            auto pMap = entityDescs[i].first;
            pMap->AddEntity( pDuplicatedEntity );
            hasSpatialEntities = hasSpatialEntities || pDuplicatedEntity->IsSpatialEntity();

            // Add to remap table
            nameRemap.insert( TPair<StringID, StringID>( entityDescs[i].second.m_name, pDuplicatedEntity->GetNameID() ) );
        }

        // Resolve entity names for parenting
        auto ResolveEntityName = [&nameRemap] ( StringID name )
        {
            StringID resolvedName = name;
            auto iter = nameRemap.find( name );
            if ( iter != nameRemap.end() )
            {
                resolvedName = iter->second;
            }

            return resolvedName;
        };

        // Restore spatial parenting
        if ( hasSpatialEntities )
        {
            for ( int32_t i = 0; i < numEntitiesToDuplicate; i++ )
            {
                if ( !duplicatedEntities[i]->IsSpatialEntity() )
                {
                    continue;
                }

                if ( entityDescs[i].second.HasSpatialParent() )
                {
                    auto pMap = entityDescs[i].first;

                    StringID const newParentID = ResolveEntityName( entityDescs[i].second.m_spatialParentName );
                    auto pParentEntity = pMap->FindEntityByName( newParentID );
                    EE_ASSERT( pParentEntity != nullptr );

                    StringID const newChildID = ResolveEntityName( entityDescs[i].second.m_name );
                    auto pChildEntity = pMap->FindEntityByName( newChildID );
                    EE_ASSERT( pChildEntity != nullptr );

                    pChildEntity->SetSpatialParent( pParentEntity, entityDescs[i].second.m_attachmentSocketID, Entity::SpatialAttachmentRule::KeepLocalTranform );
                }
            }
        }

        // Refresh Tree and Selection
        //-------------------------------------------------------------------------

        m_outlinerState = TreeState::NeedsRebuild;
        m_selectionSwitchRequest.Clear();
        for ( auto pEntity : duplicatedEntities )
        {
            m_selectionSwitchRequest.m_selectedEntities.emplace_back( pEntity->GetID() );
        }

        return duplicatedEntities;
    }

    void EntityEditorWorkspace::RenameEntity( Entity* pEntity, StringID newDesiredName )
    {
        EE_ASSERT( !m_outlinerTreeView.IsCurrentlyDrawingTree() ); // This function rebuilds the tree so it cannot be called during tree drawing

        auto pMap = m_pWorld->GetMapForEntity( pEntity );
        auto pActiveUndoableAction = EE::New<EntityUndoableAction>( this );
        pActiveUndoableAction->RecordBeginEdit( { pEntity } );

        pMap->RenameEntity( pEntity, newDesiredName );

        pActiveUndoableAction->RecordEndEdit();
        m_undoStack.RegisterAction( pActiveUndoableAction );

        m_outlinerState = TreeState::NeedsRebuildAndMaintainSelection;
    }

    void EntityEditorWorkspace::SetEntityParent( Entity* pEntity, Entity* pNewParent )
    {
        EE_ASSERT( !m_outlinerTreeView.IsCurrentlyDrawingTree() ); // This function rebuilds the tree so it cannot be called during tree drawing

        //-------------------------------------------------------------------------

        auto pActiveUndoableAction = EE::New<EntityUndoableAction>( this );
        pActiveUndoableAction->RecordBeginEdit( { pEntity } );

        pEntity->SetSpatialParent( pNewParent, StringID() );

        pActiveUndoableAction->RecordEndEdit();
        m_undoStack.RegisterAction( pActiveUndoableAction );

        m_outlinerState = TreeState::NeedsRebuildAndMaintainSelection;
    }

    void EntityEditorWorkspace::ClearEntityParent( Entity* pEntity )
    {
        EE_ASSERT( !m_outlinerTreeView.IsCurrentlyDrawingTree() ); // This function rebuilds the tree so it cannot be called during tree drawing

        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( pEntity->HasSpatialParent() );

        //-------------------------------------------------------------------------

        auto pActiveUndoableAction = EE::New<EntityUndoableAction>( this );
        pActiveUndoableAction->RecordBeginEdit( { pEntity } );

        //-------------------------------------------------------------------------

        pEntity->ClearSpatialParent();

        //-------------------------------------------------------------------------

        pActiveUndoableAction->RecordEndEdit();
        m_undoStack.RegisterAction( pActiveUndoableAction );

        m_outlinerState = TreeState::NeedsRebuildAndMaintainSelection;
    }

    void EntityEditorWorkspace::CreateSystem( TypeSystem::TypeInfo const* pSystemTypeInfo )
    {
        auto pEditedEntity = GetEditedEntity();
        EE_ASSERT( pSystemTypeInfo != nullptr );
        EE_ASSERT( pEditedEntity != nullptr );
        EE_ASSERT( pEditedEntity->IsAddedToMap() );

        auto pActiveUndoAction = EE::New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( { pEditedEntity } );

        //-------------------------------------------------------------------------

        pEditedEntity->CreateSystem( pSystemTypeInfo );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_undoStack.RegisterAction( pActiveUndoAction );
        m_structureEditorState = TreeState::NeedsRebuild;
    }

    void EntityEditorWorkspace::DestroySystem( TypeSystem::TypeID systemTypeID )
    {
        auto pEditedEntity = GetEditedEntity();
        EE_ASSERT( systemTypeID.IsValid() );
        EE_ASSERT( pEditedEntity != nullptr );

        auto pActiveUndoAction = EE::New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( { pEditedEntity } );

        //-------------------------------------------------------------------------

        pEditedEntity->DestroySystem( systemTypeID );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_undoStack.RegisterAction( pActiveUndoAction );
        m_structureEditorState = TreeState::NeedsRebuild;
    }

    void EntityEditorWorkspace::DestroySystem( EntitySystem* pSystem )
    {
        auto pEditedEntity = GetEditedEntity();
        EE_ASSERT( pSystem != nullptr );
        EE_ASSERT( pEditedEntity != nullptr );
        EE_ASSERT( VectorContains( pEditedEntity->GetSystems(), pSystem ) );

        auto pActiveUndoAction = EE::New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( { pEditedEntity } );

        //-------------------------------------------------------------------------

        pEditedEntity->DestroySystem( pSystem->GetTypeID() );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_undoStack.RegisterAction( pActiveUndoAction );
        m_structureEditorState = TreeState::NeedsRebuild;
    }

    void EntityEditorWorkspace::CreateComponent( TypeSystem::TypeInfo const* pComponentTypeInfo, ComponentID const& parentSpatialComponentID )
    {
        auto pEditedEntity = GetEditedEntity();
        EE_ASSERT( pComponentTypeInfo != nullptr );
        EE_ASSERT( pEditedEntity != nullptr );

        auto pActiveUndoAction = EE::New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( { pEditedEntity } );

        //-------------------------------------------------------------------------

        pEditedEntity->CreateComponent( pComponentTypeInfo, parentSpatialComponentID );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_undoStack.RegisterAction( pActiveUndoAction );
        m_structureEditorState = TreeState::NeedsRebuild;
    }

    void EntityEditorWorkspace::DestroyComponent( EntityComponent* pComponent )
    {
        auto pEditedEntity = GetEditedEntity();
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( pEditedEntity != nullptr );

        auto pActiveUndoAction = EE::New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( { pEditedEntity } );

        //-------------------------------------------------------------------------

        pEditedEntity->DestroyComponent( pComponent->GetID() );

        // Check if there are any other spatial components left
        if ( pEditedEntity->IsSpatialEntity() && IsOfType<SpatialEntityComponent>( pComponent ) )
        {
            bool isStillASpatialEntity = false;
            for ( auto pExistingComponent : pEditedEntity->GetComponents() )
            {
                if ( pExistingComponent == pComponent )
                {
                    continue;
                }

                if ( IsOfType<SpatialEntityComponent>( pExistingComponent ) )
                {
                    isStillASpatialEntity = true;
                    break;
                }
            }
            // If we are no longer a spatial entity, clear the parent
            if ( !isStillASpatialEntity && pEditedEntity->HasSpatialParent() )
            {
                pEditedEntity->ClearSpatialParent();
            }
        }

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_undoStack.RegisterAction( pActiveUndoAction );
        m_structureEditorState = TreeState::NeedsRebuild;
    }

    void EntityEditorWorkspace::RenameComponent( EntityComponent* pComponent, StringID newNameID )
    {
        auto pEditedEntity = GetEditedEntity();
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( pEditedEntity != nullptr );

        //-------------------------------------------------------------------------

        auto pActiveUndoAction = EE::New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( { pEditedEntity } );

        //-------------------------------------------------------------------------

        pEditedEntity->RenameComponent( pComponent, newNameID );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_undoStack.RegisterAction( pActiveUndoAction );
        m_structureEditorState = TreeState::NeedsRebuildAndMaintainSelection;
    }

    void EntityEditorWorkspace::ReparentComponent( SpatialEntityComponent* pComponent, SpatialEntityComponent* pNewParentComponent )
    {
        auto pEditedEntity = GetEditedEntity();
        EE_ASSERT( pEditedEntity != nullptr );
        EE_ASSERT( pComponent != nullptr && pNewParentComponent != nullptr );
        EE_ASSERT( !pComponent->IsRootComponent() );

        // Nothing to do, the new parent is the current parent
        if ( pComponent->HasSpatialParent() && pComponent->GetSpatialParentID() == pNewParentComponent->GetID() )
        {
            return;
        }

        // Create undo action
        auto pActiveUndoAction = EE::New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( { pEditedEntity } );
        m_pWorld->BeginComponentEdit( pEditedEntity );

        //-------------------------------------------------------------------------

        // Remove the component from its old parent
        pComponent->m_pSpatialParent->m_spatialChildren.erase_first( pComponent );
        pComponent->m_pSpatialParent = nullptr;

        // Add the component to its new parent
        pNewParentComponent->m_spatialChildren.emplace_back( pComponent );
        pComponent->m_pSpatialParent = pNewParentComponent;

        //-------------------------------------------------------------------------

        m_pWorld->EndComponentEdit( pEditedEntity );
        pActiveUndoAction->RecordEndEdit();
        m_undoStack.RegisterAction( pActiveUndoAction );
        m_structureEditorState = TreeState::NeedsRebuildAndMaintainSelection;
    }

    void EntityEditorWorkspace::MakeRootComponent( SpatialEntityComponent* pComponent )
    {
        auto pEditedEntity = GetEditedEntity();
        EE_ASSERT( pEditedEntity != nullptr );
        EE_ASSERT( pComponent != nullptr );

        // This component is already the root
        if ( pEditedEntity->GetRootSpatialComponentID() == pComponent->GetID() )
        {
            return;
        }

        // Create undo action
        auto pActiveUndoAction = EE::New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( { pEditedEntity } );
        m_pWorld->BeginComponentEdit( pEditedEntity );

        //-------------------------------------------------------------------------

        // Break any cross entity links
        bool recreateSpatialAttachment = pEditedEntity->m_isSpatialAttachmentCreated;
        if ( recreateSpatialAttachment )
        {
            pEditedEntity->DestroySpatialAttachment( Entity::SpatialAttachmentRule::KeepLocalTranform );
        }

        // Remove the component from its parent
        pComponent->m_pSpatialParent->m_spatialChildren.erase_first( pComponent );
        pComponent->m_pSpatialParent = nullptr;

        // Add the old root component as a child of the new root
        pComponent->m_spatialChildren.emplace_back( pEditedEntity->m_pRootSpatialComponent );
        pEditedEntity->m_pRootSpatialComponent->m_pSpatialParent = pComponent;

        // Make the new component the root
        pEditedEntity->m_pRootSpatialComponent = pComponent;

        // Recreate the cross entity links
        if ( recreateSpatialAttachment )
        {
            pEditedEntity->CreateSpatialAttachment();
        }

        //-------------------------------------------------------------------------

        m_pWorld->EndComponentEdit( pEditedEntity );
        pActiveUndoAction->RecordEndEdit();
        m_undoStack.RegisterAction( pActiveUndoAction );
        m_structureEditorState = TreeState::NeedsRebuildAndMaintainSelection;
    }

    //-------------------------------------------------------------------------
    // Outliner
    //-------------------------------------------------------------------------

    class OutlinerItem final : public TreeListViewItem
    {
    public:

        constexpr static char const* const s_dragAndDropID = "EntityItem";

        static void Create( TreeListViewItem* pParentItem, Entity* pEntity )
        {
            auto pEntityItem = pParentItem->CreateChild<OutlinerItem>( pEntity );
            if ( pEntity->HasAttachedEntities() )
            {
                for ( auto pChildEntity : pEntity->GetAttachedEntities() )
                {
                    Create( pEntityItem, pChildEntity );
                }

                pEntityItem->SetExpanded( true, true );
            }
        }

    public:

        OutlinerItem( TreeListViewItem* pParent, Entity* pEntity )
            : TreeListViewItem( pParent )
            , m_pEntity( pEntity )
            , m_entityID( pEntity->GetID() )
        {
            EE_ASSERT( m_pEntity != nullptr );
        }

        virtual uint64_t GetUniqueID() const override { return m_entityID.m_value; }
        virtual StringID GetNameID() const override { return m_pEntity->GetNameID(); }
        virtual bool HasContextMenu() const override { return true; }
        virtual bool IsDragAndDropTarget() const override { return m_pEntity->IsSpatialEntity(); }
        virtual bool IsDragAndDropSource() const override { return m_pEntity->IsSpatialEntity(); }

        virtual String GetDisplayName() const override
        {
            String displayName;

            if ( m_pEntity->IsSpatialEntity() )
            {
                displayName.sprintf( EE_ICON_AXIS_ARROW" %s", GetNameID().c_str() );
            }
            else
            {
                displayName.sprintf( EE_ICON_DATABASE" %s", GetNameID().c_str() );
            }

            return displayName;
        }

        virtual void SetDragAndDropPayloadData() const override
        {
            uintptr_t itemAddress = uintptr_t( this );
            ImGui::SetDragDropPayload( s_dragAndDropID, &itemAddress, sizeof( uintptr_t ) );
        }

    public:

        Entity*     m_pEntity = nullptr;
        EntityID    m_entityID; // Cached since we want to access this on rebuild without touching the entity memory which might have been invalidated
    };

    EntityMap* EntityEditorWorkspace::GetEditedMap()
    {
        return m_pWorld->GetFirstNonPersistentMap();
    }

    TVector<Entity*> EntityEditorWorkspace::GetSelectedSpatialEntities() const
    {
        TVector<Entity*> selectedSpatialEntities;
        for ( auto pItem : m_outlinerTreeView.GetSelection() )
        {
            auto pSelectedItem = static_cast<OutlinerItem*> ( pItem );
            if ( pSelectedItem->m_pEntity->IsSpatialEntity() )
            {
                selectedSpatialEntities.emplace_back( pSelectedItem->m_pEntity );
            }
        }

        return selectedSpatialEntities;
    }

    TVector<Entity*> EntityEditorWorkspace::GetSelectedEntities() const
    {
        TVector<Entity*> selectedEntities;
        for ( auto pItem : m_outlinerTreeView.GetSelection() )
        {
            auto pSelectedItem = static_cast<OutlinerItem*> ( pItem );
            selectedEntities.emplace_back( pSelectedItem->m_pEntity );
        }

        return selectedEntities;
    }

    void EntityEditorWorkspace::DrawOutliner( UpdateContext const& context, bool isFocused )
    {
        // Draw Tree
        //-------------------------------------------------------------------------

        EntityMap* pMap = GetEditedMap();

        if ( m_outlinerState != TreeState::UpToDate )
        {
            // Display nothing
        }
        else if ( pMap == nullptr )
        {
            ImGui::Text( "Nothing To Outline." );
        }
        else // Draw outliner
        {
            if ( !pMap->IsLoaded() )
            {
                ImGui::Indent();
                ImGuiX::DrawSpinner( "LoadIndicator" );
                ImGui::SameLine( 0, 10 );
                ImGui::Text( "Map Loading - Please Wait" );
                ImGui::Unindent();
            }
            else // Valid world and map
            {
                {
                    ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );
                    if ( ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, "CREATE NEW ENTITY", ImVec2( -1, 0 ) ) )
                    {
                        CreateEntity( pMap->GetID() );
                    }
                }

                //-------------------------------------------------------------------------

                if ( m_outlinerTreeView.UpdateAndDraw( m_outlinerContext ) )
                {
                    m_structureEditorState = TreeState::NeedsRebuild;
                }
            }
        }

        // Handle Input
        //-------------------------------------------------------------------------

        if ( isFocused )
        {
            auto const& selection = m_outlinerTreeView.GetSelection();

            if ( ImGui::IsKeyPressed( ImGuiKey_F2 ) )
            {
                if( selection.size() == 1 )
                {
                    auto pSelectedItem = static_cast<OutlinerItem*>( selection[0] );
                    StartEntityRename( pSelectedItem->m_pEntity );
                }
            }

            if ( ImGui::IsKeyPressed( ImGuiKey_Delete ) )
            {
                for( auto pItem : selection )
                {
                    auto pSelectedItem = static_cast<OutlinerItem*>( pItem );
                    RequestDestroyEntity( pSelectedItem->m_pEntity );
                }
            }
        }
    }

    void EntityEditorWorkspace::RebuildOutlinerTree( TreeListViewItem* pRootItem )
    {
        EntityMap* pMap = GetEditedMap();
        if ( pMap == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        for ( auto pEntity : pMap->GetEntities() )
        {
            // Children are handled as part of their parents
            if ( pEntity->HasSpatialParent() )
            {
                continue;
            }

            OutlinerItem::Create( pRootItem, pEntity );
        }
    }

    void EntityEditorWorkspace::DrawOutlinerContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus )
    {
        if ( selectedItemsWithContextMenus.size() == 1 )
        {
            auto pEntityItem = (OutlinerItem*) selectedItemsWithContextMenus[0];

            if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename" ) )
            {
                Printf( m_operationBuffer, 256, pEntityItem->m_pEntity->GetNameID().c_str() );
                m_activeOperation = Operation::RenameEntity;
            }

            if ( pEntityItem->m_pEntity->HasSpatialParent() )
            {
                if ( ImGui::MenuItem( EE_ICON_CLOSE" Detach From Parent" ) )
                {
                    RequestEntityReparent( pEntityItem->m_pEntity, nullptr );
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( ImGui::MenuItem( EE_ICON_DELETE" Delete" ) )
        {
            auto selection = m_outlinerTreeView.GetSelection();
            for ( auto pItem : selection )
            {
                auto pSelectedItem = static_cast<OutlinerItem*>( pItem );
                RequestDestroyEntity( pSelectedItem->m_pEntity );
            }
        }
    }

    void EntityEditorWorkspace::HandleOutlinerDragAndDrop( TreeListViewItem* pDropTarget )
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( OutlinerItem::s_dragAndDropID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( payload->IsDelivery() )
            {
                auto pRawData = (uintptr_t*) payload->Data;
                auto pSourceEntityItem = (OutlinerItem*) *pRawData;
                auto pTargetEntityItem = (OutlinerItem*) pDropTarget;

                // Same items, nothing to do
                if ( pSourceEntityItem == pTargetEntityItem )
                {
                    return;
                }

                // We cannot parent cross map
                if ( pTargetEntityItem->m_pEntity->GetMapID() != pSourceEntityItem->m_pEntity->GetMapID() )
                {
                    return;
                }

                // We cannot re-parent ourselves to one of our children
                if ( pTargetEntityItem->m_pEntity->IsSpatialChildOf( pSourceEntityItem->m_pEntity ) )
                {
                    return;
                }

                // Enqueue request
                RequestEntityReparent( pSourceEntityItem->m_pEntity, pTargetEntityItem->m_pEntity );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Structure Editor
    //-------------------------------------------------------------------------

    class StructureEditorItem final : public TreeListViewItem
    {

    public:

        constexpr static char const* const s_dragAndDropID = "SpatialComponentItem";
        constexpr static char const* const s_spatialComponentsHeaderID = EE_ICON_AXIS_ARROW" Spatial Components";
        constexpr static char const* const s_componentsHeaderID = EE_ICON_PACKAGE_VARIANT_CLOSED" Components";
        constexpr static char const* const s_systemsHeaderID = EE_ICON_PROGRESS_WRENCH" Systems";

        static void Create( StructureEditorItem* pParentItem, TInlineVector<SpatialEntityComponent*, 10> const& spatialComponents )
        {
            EE_ASSERT( pParentItem != nullptr && pParentItem->IsSpatialComponent() );

            for ( auto i = 0u; i < spatialComponents.size(); i++ )
            {
                if ( !spatialComponents[i]->HasSpatialParent() )
                {
                    continue;
                }

                if ( spatialComponents[i]->GetSpatialParentID() == pParentItem->m_pComponent->GetID() )
                {
                    auto pNewParentSpatialComponent = pParentItem->CreateChild<StructureEditorItem>( spatialComponents[i] );
                    pNewParentSpatialComponent->SetExpanded( true );
                    Create( pNewParentSpatialComponent, spatialComponents );
                }
            }
        }

    public:

        StructureEditorItem( TreeListViewItem* pParent, char const* const pLabel )
            : TreeListViewItem( pParent )
            , m_ID( pLabel )
            , m_tooltip( pLabel )
        {
            SetExpanded( true );
        }

        explicit StructureEditorItem( TreeListViewItem* pParent, SpatialEntityComponent* pComponent )
            : TreeListViewItem( pParent )
            , m_ID( pComponent->GetNameID() )
            , m_pComponent( pComponent )
            , m_pSpatialComponent( pComponent )
            , m_componentID( pComponent->GetID() )
        {
            m_tooltip.sprintf( "Type: %s", m_pComponent->GetTypeInfo()->GetFriendlyTypeName() );
            SetExpanded( true );
        }

        StructureEditorItem( TreeListViewItem* pParent, EntityComponent* pComponent )
            : TreeListViewItem( pParent )
            , m_ID( pComponent->GetNameID() )
            , m_pComponent( pComponent )
            , m_componentID( pComponent->GetID() )
        {
            EE_ASSERT( !IsOfType<SpatialEntityComponent>( pComponent ) );
            m_tooltip.sprintf( "Type: %s", m_pComponent->GetTypeInfo()->GetFriendlyTypeName() );
        }

        StructureEditorItem( TreeListViewItem* pParent, EntitySystem* pSystem )
            : TreeListViewItem( pParent )
            , m_ID( pSystem->GetTypeInfo()->GetFriendlyTypeName() )
            , m_pSystem( pSystem )
        {
            EE_ASSERT( pSystem != nullptr );
            m_tooltip.sprintf( "Type: %s", m_pSystem->GetTypeInfo()->GetFriendlyTypeName() );
        }

        //-------------------------------------------------------------------------

        virtual bool IsHeader() const { return m_pComponent == nullptr && m_pSystem == nullptr; }
        bool IsComponent() const { return m_pComponent != nullptr; }
        bool IsSpatialComponent() const { return m_pComponent != nullptr && m_pSpatialComponent != nullptr; }
        bool IsSystem() const { return m_pSystem == nullptr; }

        //-------------------------------------------------------------------------

        virtual StringID GetNameID() const override { return m_ID; }
        virtual uint64_t GetUniqueID() const override { return IsComponent() ? m_componentID.m_value : m_ID.ToUint(); }
        virtual bool HasContextMenu() const override { return true; }
        virtual char const* GetTooltipText() const override { return m_tooltip.c_str(); }
        virtual bool IsDragAndDropSource() const override { return IsSpatialComponent() && !m_pSpatialComponent->IsRootComponent(); }
        virtual bool IsDragAndDropTarget() const override { return IsSpatialComponent(); }

        virtual void SetDragAndDropPayloadData() const override
        {
            EE_ASSERT( IsDragAndDropSource() );
            uintptr_t itemAddress = uintptr_t( this );
            ImGui::SetDragDropPayload( s_dragAndDropID, &itemAddress, sizeof( uintptr_t ) );
        }

    public:

        StringID                    m_ID;
        String                      m_tooltip;

        // Components - if this is a component, the component ptr must be set, the spatial component ptr is optionally set for spatial components
        EntityComponent*            m_pComponent = nullptr;
        SpatialEntityComponent*     m_pSpatialComponent = nullptr;
        ComponentID                 m_componentID; // Cached since we want to access this on rebuild without touching the component memory which might have been invalidated

        // Systems
        EntitySystem*               m_pSystem = nullptr;
    };

    Entity* EntityEditorWorkspace::GetEditedEntity() const
    {
        Entity* pEditedEntity = nullptr;

        auto const& outlinerSelection = m_outlinerTreeView.GetSelection();
        if ( outlinerSelection.size() == 1 )
        {
            pEditedEntity = static_cast<OutlinerItem const*>( outlinerSelection.back() )->m_pEntity;
        }

        return pEditedEntity;
    }

    TVector<SpatialEntityComponent*> EntityEditorWorkspace::GetSelectedSpatialComponents() const
    {
        TVector<SpatialEntityComponent*> selectedSpatialComponents;
        for ( auto pItem : m_structureEditorTreeView.GetSelection() )
        {
            auto pSelectedItem = static_cast<StructureEditorItem*> ( pItem );
            if ( pSelectedItem->IsSpatialComponent() )
            {
                selectedSpatialComponents.emplace_back( pSelectedItem->m_pSpatialComponent );
            }
        }

        return selectedSpatialComponents;
    }

    void EntityEditorWorkspace::DrawStructureEditor( UpdateContext const& context, bool isFocused )
    {
        if ( m_outlinerTreeView.GetSelection().size() > 1 )
        {
            ImGui::Text( "Multiple Entities Selected..." );
            return;
        }

        //-------------------------------------------------------------------------

        Entity* pEditedEntity = GetEditedEntity();
        if ( pEditedEntity == nullptr )
        {
            return;
        }

        if ( m_structureEditorState != TreeState::UpToDate )
        {
            return;
        }

        // Draw Tree
        //-------------------------------------------------------------------------

        if ( pEditedEntity->HasStateChangeActionsPending() )
        {
            ImGui::Indent();
            ImGuiX::DrawSpinner( "LoadIndicator" );
            ImGui::SameLine( 0, 10 );
            ImGui::Text( "Entity Operations Pending - Please Wait" );
            ImGui::Unindent();
            m_structureEditorState = TreeState::NeedsRebuild;
        }
        else
        {
            if ( ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_PLUS" Add Component/System", ImVec2( -1, 0 ) ) )
            {
                StartEntityOperation( Operation::EntityAddSystemOrComponent );
            }

            m_structureEditorTreeView.UpdateAndDraw( m_structureEditorContext );
        }

        // Handle Input
        //-------------------------------------------------------------------------

        if ( isFocused )
        {
            auto const& selection = m_structureEditorTreeView.GetSelection();

            if ( ImGui::IsKeyPressed( ImGuiKey_F2 ) )
            {
                if ( selection.size() == 1 )
                {
                    auto pItem = static_cast<StructureEditorItem const*>( selection[0] );
                    if ( pItem->IsComponent() )
                    {
                        StartEntityOperation( Operation::EntityRenameComponent, pItem->m_pComponent );
                    }
                }
            }

            if ( ImGui::IsKeyPressed( ImGuiKey_Delete ) )
            {
                for ( auto pItem : selection )
                {
                    auto pSelectedItem = static_cast<StructureEditorItem const*>( pItem );
                    if ( pSelectedItem->IsComponent() )
                    {
                        DestroyComponent( pSelectedItem->m_pComponent );
                    }
                    else if ( pSelectedItem->IsSystem() )
                    {
                        DestroySystem( pSelectedItem->m_pSystem );
                    }
                }
            }
        }
    }

    void EntityEditorWorkspace::RebuildStructureEditorTree( TreeListViewItem* pRootItem )
    {
        Entity* pEditedEntity = GetEditedEntity();
        if ( pEditedEntity == nullptr )
        {
            return;
        }

        if ( pEditedEntity->HasStateChangeActionsPending() )
        {
            return;
        }

        // Split Components
        //-------------------------------------------------------------------------

        TInlineVector<SpatialEntityComponent*, 10> spatialComponents;
        TInlineVector<EntityComponent*, 10> nonSpatialComponents;

        for ( auto pComponent : pEditedEntity->GetComponents() )
        {
            if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent ) )
            {
                spatialComponents.emplace_back( pSpatialComponent );
            }
            else
            {
                nonSpatialComponents.emplace_back( pComponent );
            }
        }

        // Spatial Components
        //-------------------------------------------------------------------------

        auto pSpatialComponentsHeaderItem = pRootItem->CreateChild<StructureEditorItem>( StructureEditorItem::s_spatialComponentsHeaderID );
        if ( pEditedEntity->IsSpatialEntity() )
        {
            auto pRootComponentItem = pSpatialComponentsHeaderItem->CreateChild<StructureEditorItem>( pEditedEntity->GetRootSpatialComponent() );
            StructureEditorItem::Create( pRootComponentItem, spatialComponents );
        }

        pSpatialComponentsHeaderItem->SortChildren();

        // Non-Spatial Components
        //-------------------------------------------------------------------------

        auto pComponentsHeaderItem = pRootItem->CreateChild<StructureEditorItem>( StructureEditorItem::s_componentsHeaderID );
        for ( auto pComponent : nonSpatialComponents )
        {
            pComponentsHeaderItem->CreateChild<StructureEditorItem>( pComponent );
        }

        pComponentsHeaderItem->SortChildren();

        // Systems
        //-------------------------------------------------------------------------

        auto pSystemsHeaderItem = pRootItem->CreateChild<StructureEditorItem>( StructureEditorItem::s_systemsHeaderID );
        for ( auto pSystem : pEditedEntity->GetSystems() )
        {
            pSystemsHeaderItem->CreateChild<StructureEditorItem>( pSystem );
        }

        pSystemsHeaderItem->SortChildren();
    }

    void EntityEditorWorkspace::DrawStructureEditorContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus )
    {
        auto pItem = static_cast<StructureEditorItem const*>( selectedItemsWithContextMenus[0] );

        //-------------------------------------------------------------------------
        // Header Context Menu
        //-------------------------------------------------------------------------

        if ( pItem->IsHeader() )
        {
            if ( pItem->m_ID == StringID( StructureEditorItem::s_spatialComponentsHeaderID ) )
            {
                if ( ImGui::MenuItem( EE_ICON_PLUS" Add Component" ) )
                {
                    StartEntityOperation( Operation::EntityAddSpatialComponent );
                }
            }
            else if ( pItem->m_ID == StringID( StructureEditorItem::s_componentsHeaderID ) )
            {
                if ( ImGui::MenuItem( EE_ICON_PLUS" Add Component" ) )
                {
                    StartEntityOperation( Operation::EntityAddComponent, pItem->m_pComponent );
                }
            }
            else if ( pItem->m_ID == StringID( StructureEditorItem::s_systemsHeaderID ) )
            {
                if ( ImGui::MenuItem( EE_ICON_PLUS" Add System" ) )
                {
                    StartEntityOperation( Operation::EntityAddSystem, pItem->m_pComponent );
                }
            }
        }

        //-------------------------------------------------------------------------
        // Component Context Menu
        //-------------------------------------------------------------------------

        else if ( pItem->IsComponent() )
        {
            bool canRemoveItem = true;

            if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename Component" ) )
            {
                StartEntityOperation( Operation::EntityRenameComponent, pItem->m_pComponent );
            }

            //-------------------------------------------------------------------------

            if ( pItem->IsSpatialComponent() )
            {
                bool const isRootComponent = pItem->m_pSpatialComponent->IsRootComponent();
                if ( !isRootComponent )
                {
                    if ( ImGui::MenuItem( EE_ICON_STAR" Make Root Component" ) )
                    {
                        MakeRootComponent( pItem->m_pSpatialComponent );
                    }
                }

                if ( ImGui::MenuItem( EE_ICON_PLUS" Add Child Component" ) )
                {
                    StartEntityOperation( Operation::EntityAddSpatialComponent, pItem->m_pComponent );
                }

                canRemoveItem = !isRootComponent || pItem->GetChildren().size() <= 1;
            }

            if ( canRemoveItem )
            {
                if ( ImGui::MenuItem( EE_ICON_DELETE" Remove" ) )
                {
                    DestroyComponent( pItem->m_pComponent );
                }
            }
        }

        //-------------------------------------------------------------------------
        // System Context Menu
        //-------------------------------------------------------------------------

        else
        {
            if ( ImGui::MenuItem( EE_ICON_DELETE" Remove" ) )
            {
                DestroySystem( pItem->m_pSystem );
            }
        }
    }

    void EntityEditorWorkspace::HandleStructureEditorDragAndDrop( TreeListViewItem* pDropTarget )
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( StructureEditorItem::s_dragAndDropID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( payload->IsDelivery() )
            {
                auto pRawData = (uintptr_t*) payload->Data;
                auto pSourceComponentItem = (StructureEditorItem*) *pRawData;
                auto pTargetComponentItem = (StructureEditorItem*) pDropTarget;

                EE_ASSERT( pSourceComponentItem->IsSpatialComponent() && pTargetComponentItem->IsSpatialComponent() );

                // Same items, nothing to do
                if ( pSourceComponentItem == pTargetComponentItem )
                {
                    return;
                }

                // We cannot re-parent ourselves to one of our children
                if ( pTargetComponentItem->m_pSpatialComponent->IsSpatialChildOf( pSourceComponentItem->m_pSpatialComponent ) )
                {
                    return;
                }

                // Perform operation
                ReparentComponent( pSourceComponentItem->m_pSpatialComponent, pTargetComponentItem->m_pSpatialComponent );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Property Grid
    //-------------------------------------------------------------------------

    void EntityEditorWorkspace::DrawPropertyGrid( UpdateContext const& context, bool isFocused )
    {
        if ( m_pWorld->HasPendingMapChangeActions() )
        {
            ImGui::Indent();
            ImGuiX::DrawSpinner( "LoadIndicator" );
            ImGui::SameLine( 0, 10 );
            ImGui::Text( "Entity Operations Pending - Please Wait" );
            ImGui::Unindent();

            m_propertyGrid.SetTypeToEdit( nullptr );

            return;
        }
        else if ( m_structureEditorState != TreeState::UpToDate )
        {
            return;
        }
        else // No pending operations
        {
            auto const& selection = m_structureEditorTreeView.GetSelection();

            // Nothing selected
            if ( selection.empty() )
            {
                if ( m_outlinerTreeView.GetSelection().size() > 1 )
                {
                    ImGui::Text( "Multiple entities selected..." );
                }

                m_propertyGrid.SetTypeToEdit( nullptr );
                return;
            }

            // Multiple things selected
            if ( selection.size() > 1 )
            {
                ImGui::Text( "Multiple components selected..." );
                m_propertyGrid.SetTypeToEdit( nullptr );
                return;
            }

            // Check if we need to switch 
            auto pSelectedItem = static_cast<StructureEditorItem*>( selection[0] );
            if ( pSelectedItem->IsComponent() )
            {
                if ( m_propertyGrid.GetEditedType() != pSelectedItem->m_pComponent )
                {
                    m_propertyGrid.SetTypeToEdit( pSelectedItem->m_pComponent );
                }
            }
            else
            {
                m_propertyGrid.SetTypeToEdit( nullptr );
            }
        }

        //-------------------------------------------------------------------------

        auto const& style = ImGui::GetStyle();
        ImGui::PushStyleColor( ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg] );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, style.FrameRounding );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, style.FramePadding );
        if ( ImGui::BeginChild( "ST", ImVec2( -1, 24 ), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding ) )
        {
            if ( m_editedComponentID.IsValid() )
            {
                auto pEntity = m_pWorld->FindEntity( m_editedEntityID );
                auto pComponent = pEntity->FindComponent( m_editedComponentID );
                if ( pComponent != nullptr )
                {
                    ImGui::Text( "Entity: %s, Component: %s", pEntity->GetNameID().c_str(), pComponent->GetNameID().c_str() );
                }
            }
            else if ( m_editedEntityID.IsValid() )
            {
                auto pEntity = m_pWorld->FindEntity( m_editedEntityID );
                ImGui::Text( "Entity: %s", pEntity->GetNameID().c_str() );
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar( 3 );
        ImGui::PopStyleColor();

        //-------------------------------------------------------------------------

        m_propertyGrid.DrawGrid();
    }

    void EntityEditorWorkspace::PreEditProperty( PropertyEditInfo const& eventInfo )
    {
        EE_ASSERT( m_pActiveUndoableAction == nullptr );

        //-------------------------------------------------------------------------

        Entity* pEntity = nullptr;

        if ( auto pEditedEntity = TryCast<Entity>( eventInfo.m_pOwnerTypeInstance ) )
        {
            pEntity = pEditedEntity;
        }
        else if ( auto pComponent = TryCast<EntityComponent>( eventInfo.m_pOwnerTypeInstance ) )
        {
            pEntity = m_pWorld->FindEntity( pComponent->GetEntityID() );
        }

        //-------------------------------------------------------------------------

        auto pMap = m_pWorld->GetMap( pEntity->GetMapID() );
        EE_ASSERT( pMap != nullptr );
        auto pUndoAction = EE::New<EntityUndoableAction>( this );
        pUndoAction->RecordBeginEdit( { pEntity } );
        m_pActiveUndoableAction = pUndoAction;

        //-------------------------------------------------------------------------

        if ( auto pComponent = TryCast<EntityComponent>( eventInfo.m_pOwnerTypeInstance ) )
        {
            m_pWorld->BeginComponentEdit( pComponent );
        }
    }

    void EntityEditorWorkspace::PostEditProperty( PropertyEditInfo const& eventInfo )
    {
        EE_ASSERT( m_pActiveUndoableAction != nullptr );

        // Reset the local transform to ensure that the world transform is recalculated
        if ( eventInfo.m_pPropertyInfo->m_ID == StringID( "m_transform" ) )
        {
            if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( eventInfo.m_pOwnerTypeInstance ) )
            {
                pSpatialComponent->SetLocalTransform( pSpatialComponent->GetLocalTransform() );
            }
        }

        //-------------------------------------------------------------------------

        auto pUndoAction = (EntityUndoableAction*) m_pActiveUndoableAction;
        pUndoAction->RecordEndEdit();
        m_undoStack.RegisterAction( pUndoAction );
        m_pActiveUndoableAction = nullptr;

        //-------------------------------------------------------------------------

        if ( auto pComponent = TryCast<EntityComponent>( eventInfo.m_pOwnerTypeInstance ) )
        {
            m_pWorld->EndComponentEdit( pComponent );
        }
    }

    //-------------------------------------------------------------------------
    // Dialogs
    //-------------------------------------------------------------------------

    bool EntityEditorWorkspace::DrawRenameEntityDialog( UpdateContext const& context )
    {
        EE_ASSERT( m_activeOperation == Operation::RenameEntity );

        Entity* pEntityToRename = static_cast<OutlinerItem*>( m_outlinerTreeView.GetSelection()[0] )->m_pEntity;
        EntityMapID const& mapID = pEntityToRename->GetMapID();
        EE_ASSERT( mapID.IsValid() );
        EntityMap* pMap = GetEditedMap();
        EE_ASSERT( pMap != nullptr );

        //-------------------------------------------------------------------------

        auto ValidateName = [this, pMap] ()
        {
            bool isValidName = strlen( m_operationBuffer ) > 0;
            if ( isValidName )
            {
                StringID const desiredNameID = StringID( m_operationBuffer );
                auto uniqueNameID = pMap->GenerateUniqueEntityNameID( desiredNameID );
                isValidName = ( desiredNameID == uniqueNameID );
            }

            return isValidName;
        };

        //-------------------------------------------------------------------------

        bool isDialogOpen = true;
        if ( ImGuiX::BeginViewportPopupModal( "Rename Entity", &isDialogOpen, ImVec2( 400, 110 ), ImGuiCond_Always, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar ) )
        {
            bool applyRename = false;
            bool isValidName = ValidateName();

            // Draw UI
            //-------------------------------------------------------------------------

            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Name: " );
            ImGui::SameLine();

            ImGui::SetNextItemWidth( -1 );
            ImGui::PushStyleColor( ImGuiCol_Text, isValidName ? (uint32_t) ImGuiX::Style::s_colorText : Colors::Red.ToUInt32_ABGR() );
            applyRename = ImGui::InputText( "##Name", m_operationBuffer, 256, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CallbackCharFilter, ImGuiX::FilterNameIDChars );
            isValidName = ValidateName();
            ImGui::PopStyleColor();

            ImGui::NewLine();
            ImGui::SameLine( ImGui::GetContentRegionAvail().x - 120 - ImGui::GetStyle().ItemSpacing.x );

            ImGui::BeginDisabled( !isValidName );
            if ( ImGui::Button( "OK", ImVec2( 60, 0 ) ) )
            {
                applyRename = true;
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            if ( ImGui::Button( "Cancel", ImVec2( 60, 0 ) ) )
            {
                m_activeOperation = Operation::None;
                ImGui::CloseCurrentPopup();
            }

            //-------------------------------------------------------------------------

            if ( applyRename && isValidName )
            {
                StringID const desiredNameID = StringID( m_operationBuffer );
                RenameEntity( pEntityToRename, desiredNameID );
                m_activeOperation = Operation::None;
            }

            //-------------------------------------------------------------------------

            isDialogOpen = ImGuiX::CancelDialogViaEsc( isDialogOpen );

            ImGui::EndPopup();
        }

        return isDialogOpen;
    }

    bool EntityEditorWorkspace::DrawRenameComponentDialog( UpdateContext const& context )
    {
        Entity* pEditedEntity = GetEditedEntity();

        EE_ASSERT( m_activeOperation == Operation::EntityRenameComponent );
        EE_ASSERT( pEditedEntity != nullptr && m_pOperationTargetComponent != nullptr );

        //-------------------------------------------------------------------------

        auto ValidateName = [this, pEditedEntity] ()
        {
            bool isValidName = strlen( m_operationBuffer ) > 0;
            if ( isValidName )
            {
                StringID const desiredNameID = StringID( m_operationBuffer );
                auto uniqueNameID = pEditedEntity->GenerateUniqueComponentNameID( m_pOperationTargetComponent, desiredNameID );
                isValidName = ( desiredNameID == uniqueNameID );
            }

            return isValidName;
        };

        //-------------------------------------------------------------------------

        bool isDialogOpen = true;
        if ( ImGuiX::BeginViewportPopupModal( "Rename Component", &isDialogOpen, ImVec2( 400, 110 ), ImGuiCond_Always, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar ) )
        {
            bool applyRename = false;
            bool isValidName = ValidateName();

            // Draw UI
            //-------------------------------------------------------------------------

            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Name: " );
            ImGui::SameLine();

            ImGui::SetNextItemWidth( -1 );
            ImGui::PushStyleColor( ImGuiCol_Text, isValidName ? (uint32_t) ImGuiX::Style::s_colorText : Colors::Red.ToUInt32_ABGR() );
            applyRename = ImGui::InputText( "##Name", m_operationBuffer, 256, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CallbackCharFilter, ImGuiX::FilterNameIDChars );
            isValidName = ValidateName();
            ImGui::PopStyleColor();

            ImGui::NewLine();
            ImGui::SameLine( ImGui::GetContentRegionAvail().x - 120 - ImGui::GetStyle().ItemSpacing.x );

            ImGui::BeginDisabled( !isValidName );
            if ( ImGui::Button( "OK", ImVec2( 60, 0 ) ) )
            {
                applyRename = true;
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            if ( ImGui::Button( "Cancel", ImVec2( 60, 0 ) ) )
            {
                m_activeOperation = Operation::None;
                ImGui::CloseCurrentPopup();
            }

            //-------------------------------------------------------------------------

            if ( applyRename && isValidName )
            {
                StringID const desiredNameID = StringID( m_operationBuffer );
                RenameComponent( m_pOperationTargetComponent, desiredNameID );
                m_activeOperation = Operation::None;
            }

            //-------------------------------------------------------------------------

            isDialogOpen = ImGuiX::CancelDialogViaEsc( isDialogOpen );

            ImGui::EndPopup();
        }

        return isDialogOpen;
    }

    bool EntityEditorWorkspace::DrawAddComponentOrSystemDialog( UpdateContext const& context )
    {
        bool isDialogOpen = true;
        bool shouldExecuteOperation = false;

        //-------------------------------------------------------------------------

        char const* pDialogTitle = nullptr;

        switch ( m_activeOperation )
        {
            case Operation::EntityAddSystemOrComponent:
                pDialogTitle = "Add System Or Component##ASC";
            break;

            case Operation::EntityAddSystem:
                pDialogTitle = "Add System##ASC";
            break;

            case Operation::EntityAddSpatialComponent:
                pDialogTitle = "Add Spatial Component##ASC";
            break;

            case Operation::EntityAddComponent:
                pDialogTitle = "Add Component##ASC";
            break;

            default:
            break;
        }

        EE_ASSERT( pDialogTitle != nullptr );

        //-------------------------------------------------------------------------

        ImGui::SetNextWindowSizeConstraints( ImVec2( 400, 400 ), ImVec2( FLT_MAX, FLT_MAX ) );
        if ( ImGuiX::BeginViewportPopupModal( pDialogTitle, &isDialogOpen, ImVec2( 1000, 400 ), ImGuiCond_FirstUseEver, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavInputs ) )
        {
            ImVec2 const contentRegionAvailable = ImGui::GetContentRegionAvail();

            // Draw Filter
            //-------------------------------------------------------------------------

            if ( m_operationFilterWidget.DrawAndUpdate( -1, ImGuiX::FilterWidget::Flags::TakeInitialFocus ) )
            {
                if ( m_operationFilterWidget.HasFilterSet() )
                {
                    m_filteredOptions.clear();
                    for ( auto const& pTypeInfo : m_operationOptions )
                    {
                        if ( m_operationFilterWidget.MatchesFilter( pTypeInfo->GetTypeName() ) )
                        {
                            m_filteredOptions.emplace_back( pTypeInfo );
                        }
                    }
                }
                else
                {
                    m_filteredOptions = m_operationOptions;
                }

                m_pOperationSelectedOption = m_filteredOptions.empty() ? nullptr : m_filteredOptions.front();
            }

            // Draw results
            //-------------------------------------------------------------------------

            float const tableHeight = contentRegionAvailable.y - ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y;
            ImGui::PushStyleColor( ImGuiCol_Header, ImGuiX::Style::s_colorGray1.Value );
            if ( ImGui::BeginTable( "Options List", 1, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2( contentRegionAvailable.x, tableHeight ) ) )
            {
                ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthStretch, 1.0f );

                //-------------------------------------------------------------------------

                ImGuiListClipper clipper;
                clipper.Begin( (int32_t) m_filteredOptions.size() );
                while ( clipper.Step() )
                {
                    for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                    {
                        ImGui::TableNextRow();

                        ImGui::TableNextColumn();
                        bool const wasSelected = ( m_pOperationSelectedOption == m_filteredOptions[i] );
                        if ( wasSelected )
                        {
                            ImGui::PushStyleColor( ImGuiCol_Text, ImGuiX::Style::s_colorAccent0.Value );
                        }

                        bool isSelected = wasSelected;
                        if ( ImGui::Selectable( m_filteredOptions[i]->GetFriendlyTypeName(), &isSelected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns ) )
                        {
                            m_pOperationSelectedOption = m_filteredOptions[i];

                            if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                            {
                                shouldExecuteOperation = true;
                            }
                        }

                        if ( wasSelected )
                        {
                            ImGui::PopStyleColor();
                        }
                    }
                }

                ImGui::EndTable();
            }
            ImGui::PopStyleColor();

            //-------------------------------------------------------------------------

            auto AdjustSelection = [this] ( bool increment )
            {
                int32_t const numOptions = (int32_t) m_filteredOptions.size();
                if ( numOptions == 0 )
                {
                    return;
                }

                int32_t optionIdx = InvalidIndex;
                for ( auto i = 0; i < numOptions; i++ )
                {
                    if ( m_filteredOptions[i] == m_pOperationSelectedOption )
                    {
                        optionIdx = i;
                        break;
                    }
                }

                EE_ASSERT( optionIdx != InvalidIndex );

                optionIdx += ( increment ? 1 : -1 );
                if ( optionIdx < 0 )
                {
                    optionIdx += numOptions;
                }
                optionIdx = optionIdx % m_filteredOptions.size();
                m_pOperationSelectedOption = m_filteredOptions[optionIdx];
            };

            if ( ImGui::IsKeyReleased( ImGuiKey_Enter ) )
            {
                shouldExecuteOperation = true;
            }
            else if ( ImGui::IsKeyReleased( ImGuiKey_Escape ) )
            {
                ImGui::CloseCurrentPopup();
                isDialogOpen = false;
            }
            else if ( ImGui::IsKeyReleased( ImGuiKey_UpArrow ) )
            {
                if ( m_filteredOptions.size() > 0 )
                {
                    AdjustSelection( false );
                    m_initializeFocus = true;
                }
            }
            else if ( ImGui::IsKeyReleased( ImGuiKey_DownArrow ) )
            {
                if ( m_filteredOptions.size() > 0 )
                {
                    AdjustSelection( true );
                    m_initializeFocus = true;
                }
            }

            //-------------------------------------------------------------------------

            if ( shouldExecuteOperation && m_pOperationSelectedOption == nullptr )
            {
                shouldExecuteOperation = false;
            }

            if ( shouldExecuteOperation )
            {
                EE_ASSERT( m_activeOperation != Operation::None );
                EE_ASSERT( m_pOperationSelectedOption != nullptr );

                if ( VectorContains( m_allSystemTypes, m_pOperationSelectedOption ) )
                {
                    CreateSystem( m_pOperationSelectedOption );
                }
                else if ( VectorContains( m_allComponentTypes, m_pOperationSelectedOption ) )
                {
                    CreateComponent( m_pOperationSelectedOption );
                }
                else if ( VectorContains( m_allSpatialComponentTypes, m_pOperationSelectedOption ) )
                {
                    ComponentID parentComponentID;
                    if ( m_pOperationTargetComponent != nullptr )
                    {
                        EE_ASSERT( IsOfType<SpatialEntityComponent>( m_pOperationTargetComponent ) );
                        parentComponentID = m_pOperationTargetComponent->GetID();
                    }
                    CreateComponent( m_pOperationSelectedOption, parentComponentID );
                }

                //-------------------------------------------------------------------------

                m_activeOperation = Operation::None;
                m_pOperationSelectedOption = nullptr;
                m_pOperationTargetComponent = nullptr;
                isDialogOpen = false;
            }

            isDialogOpen = ImGuiX::CancelDialogViaEsc( isDialogOpen );

            ImGui::EndPopup();
        }

        //-------------------------------------------------------------------------

        return isDialogOpen;
    }

    //-------------------------------------------------------------------------
    // Transform editing
    //-------------------------------------------------------------------------

    void EntityEditorWorkspace::BeginTransformManipulation( Transform const& newTransform, bool duplicateSelection )
    {
        EE_ASSERT( m_outlinerTreeView.HasSelection() );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );

        // Should we duplicate the selection?
        if ( duplicateSelection )
        {
            m_manipulatedSpatialEntities = DuplicateSelectedEntities();
            m_manipulatedSpatialComponents.clear();
        }
        else // Record the manipulated entities
        {
            m_manipulatedSpatialEntities = GetSelectedSpatialEntities();
            m_manipulatedSpatialComponents = GetSelectedSpatialComponents();
        }

        // Create undo action
        auto pEntityUndoAction = EE::New<EntityUndoableAction>( this );
        pEntityUndoAction->RecordBeginTransformEdit( m_manipulatedSpatialEntities, duplicateSelection );
        m_pActiveUndoableAction = pEntityUndoAction;

        ApplyTransformManipulation( newTransform );
    }

    void EntityEditorWorkspace::ApplyTransformManipulation( Transform const& newTransform )
    {
        EE_ASSERT( m_pActiveUndoableAction != nullptr );

        // Update all selected components
        //-------------------------------------------------------------------------

        if ( m_manipulatedSpatialComponents.size() > 0 )
        {
            int32_t offsetIdx = 0;
            for ( auto pComponent : m_manipulatedSpatialComponents )
            {
                if ( pComponent->HasSpatialParent() )
                {
                    auto Comparator = [] ( SpatialEntityComponent const* pComponent, ComponentID const& ID )
                    {
                        return pComponent->GetID() == ID;
                    };

                    if ( VectorContains( m_manipulatedSpatialComponents, pComponent->GetSpatialParentID(), Comparator ) )
                    {
                        offsetIdx++;
                        continue;
                    }
                }

                pComponent->SetWorldTransform( m_selectionOffsetTransforms[offsetIdx] * newTransform );
                offsetIdx++;
            }
        }
        else // Update all selected entities
        {
            EE_ASSERT( !m_manipulatedSpatialEntities.empty() );

            int32_t offsetIdx = 0;
            for ( auto pSelectedEntity : m_manipulatedSpatialEntities )
            {
                if ( pSelectedEntity->HasSpatialParent() )
                {
                    if ( VectorContains( m_manipulatedSpatialEntities, pSelectedEntity->GetSpatialParent() ) )
                    {
                        offsetIdx++;
                        continue;
                    }
                }

                pSelectedEntity->SetWorldTransform( m_selectionOffsetTransforms[offsetIdx] * newTransform );
                offsetIdx++;
            }
        }
    }

    void EntityEditorWorkspace::EndTransformManipulation( Transform const& newTransform )
    {
        ApplyTransformManipulation( newTransform );

        auto pEntityUndoAction = Cast<EntityUndoableAction>( m_pActiveUndoableAction );
        pEntityUndoAction->RecordEndTransformEdit();
        m_undoStack.RegisterAction( m_pActiveUndoableAction );
        m_pActiveUndoableAction = nullptr;

        m_manipulatedSpatialEntities.clear();
        m_manipulatedSpatialComponents.clear();
    }

    //-------------------------------------------------------------------------
    // Update
    //-------------------------------------------------------------------------

    void EntityEditorWorkspace::Update( UpdateContext const& context, bool isFocused )
    {
        // Process deletions
        //-------------------------------------------------------------------------

        if ( !m_entityDeletionRequests.empty() )
        {
            auto pActiveUndoableAction = EE::New<EntityUndoableAction>( this );
            pActiveUndoableAction->RecordDelete( m_entityDeletionRequests );
            m_undoStack.RegisterAction( pActiveUndoableAction );

            for ( auto pEntity : m_entityDeletionRequests )
            {
                auto pMap = m_pWorld->GetMapForEntity( pEntity );
                EE_ASSERT( pMap != nullptr );
                pMap->DestroyEntity( pEntity->GetID() );
            }

            m_entityDeletionRequests.clear();
            m_outlinerState = TreeState::NeedsRebuild;
        }

        // Process re-parenting requests
        //-------------------------------------------------------------------------

        if ( !m_spatialParentingRequests.empty() )
        {
            m_undoStack.BeginCompoundAction();
            for ( auto const& request : m_spatialParentingRequests )
            {
                if ( request.m_pNewParent == nullptr )
                {
                    ClearEntityParent( request.m_pEntity );
                }
                else
                {
                    SetEntityParent( request.m_pEntity, request.m_pNewParent );
                }
            }
            m_undoStack.EndCompoundAction();

            m_spatialParentingRequests.clear();
        }

        // Update Outliner
        //-------------------------------------------------------------------------

        if ( m_pWorld->HasPendingMapChangeActions() )
        {
            m_outlinerState = TreeState::NeedsRebuildAndMaintainSelection;
        }
        else // Process rebuild requests
        {
            EntityMap* pMap = GetEditedMap();
            if ( m_outlinerState != TreeState::UpToDate && pMap != nullptr && pMap->IsLoaded() )
            {
                m_outlinerTreeView.RebuildTree( m_outlinerContext, m_outlinerState == TreeState::NeedsRebuildAndMaintainSelection );
                m_outlinerState = TreeState::UpToDate;
            }
        }

        if ( m_outlinerState == TreeState::UpToDate && m_selectionSwitchRequest.IsValid() )
        {
            m_outlinerTreeView.ClearSelection();
            for ( auto const& entityID : m_selectionSwitchRequest.m_selectedEntities )
            {
                if ( auto pItem = m_outlinerTreeView.FindItem( entityID.m_value ) )
                {
                    m_outlinerTreeView.AddToSelection( pItem );
                }
            }

            m_structureEditorState = TreeState::NeedsRebuild;
        }

        // Update Structure editor
        //-------------------------------------------------------------------------

        if ( m_outlinerState == TreeState::UpToDate && m_structureEditorState != TreeState::UpToDate )
        {
            m_structureEditorTreeView.RebuildTree( m_structureEditorContext, m_structureEditorState == TreeState::NeedsRebuildAndMaintainSelection );
            m_structureEditorState = TreeState::UpToDate;
        }

        // Update selection
        //-------------------------------------------------------------------------

        if ( m_outlinerState == TreeState::UpToDate && m_structureEditorState == TreeState::UpToDate && m_selectionSwitchRequest.IsValid() )
        {
            m_structureEditorTreeView.ClearSelection();
            for ( auto const& componentID : m_selectionSwitchRequest.m_selectedComponents )
            {
                if ( auto pItem = m_structureEditorTreeView.FindItem( componentID.m_value ) )
                {
                    m_structureEditorTreeView.AddToSelection( pItem );
                }
            }

            m_selectionSwitchRequest.Clear();
        }

        // Handle input
        //-------------------------------------------------------------------------

        if ( m_isViewportFocused || m_isViewportHovered )
        {
            if ( ImGui::IsKeyPressed( ImGuiKey_Space ) )
            {
                m_gizmo.SwitchToNextMode();
            }

            if ( ImGui::IsKeyPressed( ImGuiKey_Delete ) )
            {
                auto const& selection = m_outlinerTreeView.GetSelection();
                for ( auto pItem : selection )
                {
                    auto pSelectedItem = static_cast<OutlinerItem*>( pItem );
                    RequestDestroyEntity( pSelectedItem->m_pEntity );
                }
            }
        }
    }

    void EntityEditorWorkspace::DrawDialogs( UpdateContext const& context )
    {
        bool isDialogOpen = m_activeOperation != Operation::None;

        switch ( m_activeOperation )
        {
            case Operation::None:
            break;

            case Operation::RenameEntity:
            {
                isDialogOpen = DrawRenameEntityDialog( context );
            }
            break;

            case Operation::EntityRenameComponent:
            {
                isDialogOpen = DrawRenameComponentDialog( context );
            }
            break;

            default: // Add Component/System
            {
                isDialogOpen = DrawAddComponentOrSystemDialog( context );
            }
            break;
        }

        //-------------------------------------------------------------------------

        // If the dialog was closed (i.e. operation canceled)
        if ( !isDialogOpen )
        {
            m_activeOperation = Operation::None;
        }
    }
}