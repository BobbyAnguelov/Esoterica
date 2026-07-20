#include "EntityEditor_Context.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "EntityEditor_UndoableAction.h"
#include "EntityEditor_Dialogs.h"
#include "Engine/Entity/EntityMap.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityComponent.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Camera/Components/Component_ToolsCamera.h"
#include "Engine/Entity/Entity.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Core/UndoStack.h"
#include "Base/Serialization/TypeSerialization.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    struct [[nodiscard]] ScopedChange
    {
        ScopedChange( EditorContext* pCtx ) : m_context( *pCtx )
        {
            EE_ASSERT( pCtx != nullptr );
            m_context.m_preWorldChangeEvent.Execute();
        };

        ~ScopedChange()
        {
            m_context.FixUpSelectionPtrs();
            m_context.FixUpGroupPtrs();
            m_context.UpdateSpatialSelectionData();
            m_context.m_postWorldChangeEvent.Execute();
        };

    private:

        EditorContext& m_context;
    };

    /*struct [[nodiscard]] ScopedGroupChange
    {
        ScopedGroupChange( EditorContext* pCtx ) : m_context( *pCtx )
        {
            EE_ASSERT( pCtx != nullptr );
            m_context.m_preWorldChangeEvent.Execute();

            m_pAction = New<EntityGroupsUndoableAction>( &m_context );
            m_pAction->Begin();
        };

        ~ScopedGroupChange()
        {
            m_pAction->End();
            m_context.m_pUndoStack->RegisterAction( m_pAction );

            m_context.FixUpSelectionPtrs();
            m_context.FixUpGroupPtrs();
            m_context.UpdateSpatialSelectionData();
            m_context.m_postWorldChangeEvent.Execute();
        };

    private:

        EditorContext&              m_context;
        EntityGroupsUndoableAction* m_pAction = nullptr;
    };*/

    //-------------------------------------------------------------------------

    EditorContext::EditorContext( ToolsContext const* pToolsContext, UndoStack* pUndoStack, DialogManager* pDialogManager, EntityWorld* pWorld, ToolsCameraComponent* pCamera )
        : m_pToolsContext( pToolsContext )
        , m_pUndoStack( pUndoStack )
        , m_pDialogManager( pDialogManager )
        , m_pWorld( pWorld )
        , m_pCamera( pCamera )
        , m_systemTypes( pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( EntitySystem::GetStaticTypeID(), false, false, true ) )
        , m_componentTypes( pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( EntityComponent::GetStaticTypeID(), false, false, true ) )
    {
        EE_ASSERT( m_pToolsContext != nullptr );
        EE_ASSERT( m_pUndoStack != nullptr );
        EE_ASSERT( m_pDialogManager != nullptr );
        EE_ASSERT( m_pWorld != nullptr );
        EE_ASSERT( m_pCamera != nullptr );

        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < m_componentTypes.size(); i++ )
        {
            if ( m_componentTypes[i]->IsDerivedFrom( SpatialEntityComponent::GetStaticTypeID() ) )
            {
                m_spatialComponentTypes.emplace_back( m_componentTypes[i] );
            }
            else
            {
                m_nonSpatialComponentTypes.emplace_back( m_componentTypes[i] );
            }
        }

        auto FindOrCreateCategory = [] ( TVector<CategorizedEntityItemTypeInfo>& categories, char const* pCategoryName )
        {
            StringID const categoryName = ( pCategoryName == nullptr || strlen( pCategoryName ) == 0 ) ? StringID() : StringID( pCategoryName );

            for ( auto& category : categories )
            {
                if ( category.m_name == categoryName )
                {
                    return &category;
                }
            }

            CategorizedEntityItemTypeInfo category;
            category.m_name = categoryName;

            auto SortPredicate = [] ( CategorizedEntityItemTypeInfo const& a, CategorizedEntityItemTypeInfo const& b )
            {
                if ( !a.m_name.IsValid() && !b.m_name.IsValid() )
                {
                    return false;
                }

                if ( a.m_name.IsValid() && !b.m_name.IsValid() )
                {
                    return true;
                }

                if ( !a.m_name.IsValid() && b.m_name.IsValid() )
                {
                    return false;
                }

                return StringUtils::Stricmp( a.m_name.c_str(), b.m_name.c_str() ) < 0;
            };

            return &VectorInsertSorted( categories, category, SortPredicate );
        };

        auto TypeSortPredicate = [] ( TypeSystem::TypeInfo const* const& pA, TypeSystem::TypeInfo const* const& pB )
        {
            return pA->m_friendlyName.comparei( pA->m_friendlyName ) < 0;
        };

        for ( TypeSystem::TypeInfo const* pSystemTypeInfo : m_systemTypes )
        {
            CategorizedEntityItemTypeInfo* pCategory = FindOrCreateCategory( m_categorizedSystemTypes, pSystemTypeInfo->GetCategoryName() );
            EE_ASSERT( pCategory != nullptr );
            VectorInsertSorted( pCategory->m_typeInfos, pSystemTypeInfo, TypeSortPredicate );
        }

        for ( TypeSystem::TypeInfo const* pComponentTypeInfo : m_componentTypes )
        {
            CategorizedEntityItemTypeInfo* pCategory = FindOrCreateCategory( m_categorizedComponentTypes, pComponentTypeInfo->GetCategoryName() );
            EE_ASSERT( pCategory != nullptr );
            VectorInsertSorted( pCategory->m_typeInfos, pComponentTypeInfo, TypeSortPredicate );
        }

        for ( TypeSystem::TypeInfo const* pComponentTypeInfo : m_spatialComponentTypes )
        {
            CategorizedEntityItemTypeInfo* pCategory = FindOrCreateCategory( m_categorizedSpatialComponentTypes, pComponentTypeInfo->GetCategoryName() );
            EE_ASSERT( pCategory != nullptr );
            VectorInsertSorted( pCategory->m_typeInfos, pComponentTypeInfo, TypeSortPredicate );
        }

        for ( TypeSystem::TypeInfo const* pComponentTypeInfo : m_nonSpatialComponentTypes )
        {
            CategorizedEntityItemTypeInfo* pCategory = FindOrCreateCategory( m_categorizedNonSpatialComponentTypes, pComponentTypeInfo->GetCategoryName() );
            EE_ASSERT( pCategory != nullptr );
            VectorInsertSorted( pCategory->m_typeInfos, pComponentTypeInfo, TypeSortPredicate );
        }
    }

    EditorContext::~EditorContext()
    {
        EE::Delete( m_pActiveUndoableAction );
    }

    void EditorContext::SetEditedMap( EntityMapID ID )
    {
        m_editedMapID = ID;
        EE_ASSERT( m_pWorld->GetMap( m_editedMapID ) != nullptr );
    }

    EntityMap* EditorContext::GetEditedMap()
    {
        if ( m_editedMapID.IsValid() )
        {
            return m_pWorld->GetMap( m_editedMapID );
        }

        return nullptr;
    }

    void EditorContext::Update( UpdateContext const& context )
    {

    }

    //-------------------------------------------------------------------------

    TVector<EntityGroup> EditorContext::GetEntityGroupsForMap( EntityMapID const& mapID ) const
    {
        TVector<EntityGroup> entityGroups;

        TVector<EntityEditorItemGroup> const* pGroups = GetGroupsForMap( mapID );
        if ( pGroups != nullptr )
        {
            for ( EntityEditorItemGroup const& group : *pGroups )
            {
                EntityGroup& addedGroup = entityGroups.emplace_back();
                addedGroup.m_ID = group.m_ID;

                for ( EntityEditorItem const& item : group.m_items )
                {
                    addedGroup.m_names.emplace_back( item.m_pEntity->GetNameID() );
                }
            }
        }

        return entityGroups;
    }

    TVector<EntityEditorItemGroup> const* EditorContext::GetGroupsForMap( EntityMapID const& mapID ) const
    {
        auto iter = m_entityGroups.find( mapID );
        if ( iter != m_entityGroups.end() )
        {
            return &iter->second;
        }

        return nullptr;
    }

    void EditorContext::SetEntityGroupsForMap( EntityMapID const& mapID, TVector<EntityGroup> const& groups )
    {
        EE_ASSERT( mapID.IsValid() );
        auto pMap = m_pWorld->GetMap( mapID );
        EE_ASSERT( pMap != nullptr );

        TVector<EntityEditorItemGroup>& itemGroups = m_entityGroups[mapID];
        itemGroups.clear();

        for ( auto const& group : groups )
        {
            EntityEditorItemGroup& itemGroup = itemGroups.emplace_back();
            itemGroup.m_ID = group.m_ID;

            for ( StringID entityName : group.m_names )
            {
                auto pEntity = pMap->FindEntityByName( entityName );
                if ( pEntity != nullptr )
                {
                    VectorEmplaceBackUnique( itemGroup.m_items, EntityEditorItem( pEntity ) );
                }
            }
        }

        m_postWorldChangeEvent.Execute();
    }

    void EditorContext::SetGroupsForMap( EntityMapID const& mapID, TVector<EntityEditorItemGroup> const& groups )
    {
        EE_ASSERT( mapID.IsValid() );
        m_entityGroups[mapID] = groups;

        for ( auto& group : m_entityGroups[mapID] )
        {
            group.UpdateEntityPtrs( m_pWorld );
        }

        m_postWorldChangeEvent.Execute();
    }

    void EditorContext::SetEntityGroups( THashMap<EntityMapID, TVector<EntityEditorItemGroup>> const& entityGroups )
    {
        m_entityGroups = entityGroups;

        for ( auto& pair : m_entityGroups )
        {
            for ( auto& group : pair.second )
            {
                group.UpdateEntityPtrs( m_pWorld );
            }
        }

        m_postWorldChangeEvent.Execute();
    }

    void EditorContext::CreateGroup( EntityMapID const& mapID, StringID groupID )
    {
        EE_ASSERT( mapID.IsValid() );
        EE_ASSERT( groupID.IsValid() );

        bool groupFound = false;
        TVector<EntityEditorItemGroup>& groups = m_entityGroups[mapID];
        for ( int32_t i = int32_t( groups.size() ) - 1; i >= 0; i-- )
        {
            if ( groups[i].m_ID == groupID )
            {
                groupFound = true;
                break;
            }
        }

        if ( !groupFound )
        {
            ScopedChange const sc( this );

            auto pAction = New<EntityGroupsUndoableAction>( this );
            pAction->Begin();

            groups.emplace_back( groupID );

            pAction->End();
            m_pUndoStack->RegisterAction( pAction );
        }
    }

    void EditorContext::DestroyGroup( EntityMapID const& mapID, StringID groupID )
    {
        EE_ASSERT( mapID.IsValid() );
        EE_ASSERT( groupID.IsValid() );

        // Get all entities and groups under this one
        //-------------------------------------------------------------------------

        TVector<Entity*> entitiesToDelete;
        GetAllEntitiesInGroup( mapID, groupID, entitiesToDelete );

        TVector<StringID> groupsToDelete;
        GetAllGroupsInGroup( mapID, groupID, groupsToDelete );
        groupsToDelete.emplace_back( groupID );

        // Delete groups and entities
        //-------------------------------------------------------------------------

        bool const shouldStartCompoundAction = !m_pUndoStack->IsCompoundActionActive();
        if ( shouldStartCompoundAction )
        {
            m_pUndoStack->BeginCompoundAction();
        }

        // Destroy groups first so that when we undo we will restore the grouping
        {
            ScopedChange const sc( this );
            auto pAction = New<EntityGroupsUndoableAction>( this );
            pAction->Begin();

            TVector<EntityEditorItemGroup>& groups = m_entityGroups[mapID];
            for ( int32_t i = int32_t( groups.size() ) - 1; i >= 0; i-- )
            {
                if ( VectorContains( groupsToDelete, groups[i].m_ID ) )
                {
                    groups.erase( groups.begin() + i );
                }
            }

            pAction->End();
            m_pUndoStack->RegisterAction( pAction );
        }

        DestroyEntities( entitiesToDelete );

        if ( shouldStartCompoundAction )
        {
            m_pUndoStack->EndCompoundAction();
        }
    }

    EntityEditorItemGroup* EditorContext::GetEntityGroup( EntityMapID const& mapID, StringID groupID )
    {
        EE_ASSERT( mapID.IsValid() );
        EE_ASSERT( groupID.IsValid() );

        auto iter = m_entityGroups.find( mapID );
        if ( iter != m_entityGroups.end() )
        {
            for ( auto& group : iter->second )
            {
                if ( group.m_ID == groupID )
                {
                    return &group;
                }
            }
        }

        return nullptr;
    }

    void EditorContext::RenameEntityGroup( EntityMapID const& mapID, StringID oldGroupID, StringID newGroupID )
    {
        EE_ASSERT( mapID.IsValid() );
        EE_ASSERT( oldGroupID.IsValid() && newGroupID.IsValid() );

        if ( oldGroupID == newGroupID )
        {
            return;
        }

        TVector<EntityEditorItemGroup>& groups = m_entityGroups[mapID];
        for ( int32_t i = int32_t( groups.size() ) - 1; i >= 0; i-- )
        {
            if ( groups[i].m_ID == oldGroupID )
            {
                ScopedChange const sc( this );

                auto pAction = New<EntityGroupsUndoableAction>( this );
                pAction->Begin();
                groups[i].m_ID = newGroupID;
                pAction->End();
                m_pUndoStack->RegisterAction( pAction );

                return;
            }
        }
    }

    void EditorContext::AddOrMoveEntitiesToGroup( EntityMapID const& mapID, StringID groupID, Entity* const* ppEntities, size_t numEntities )
    {
        EE_ASSERT( mapID.IsValid() );

        if ( groupID.IsValid() )
        {
            EE_ASSERT( HasEntityGroup( mapID, groupID ) );
        }

        if ( numEntities == 0 )
        {
            return;
        }

        //-------------------------------------------------------------------------

        ScopedChange const sc( this );
        auto pAction = New<EntityGroupsUndoableAction>( this );
        pAction->Begin();

        for ( int32_t i = 0; i < numEntities; i++ )
        {
            Entity* pEntity = ppEntities[i];
            EE_ASSERT( pEntity->GetMapID() == mapID );
            EntityEditorItem const item( pEntity );

            TVector<EntityEditorItemGroup>& groups = m_entityGroups[mapID];
            for ( EntityEditorItemGroup& group : groups )
            {
                if ( group.m_ID == groupID )
                {
                    VectorEmplaceBackUnique( group.m_items, item );
                }
                else
                {
                    group.m_items.erase_first( item );
                }
            }
        }

        pAction->End();
        m_pUndoStack->RegisterAction( pAction );
    }

    void EditorContext::AddOrMoveEntitiesToGroup( EntityMapID const& mapID, StringID groupID, EntityEditorItem const* pEntityItems, size_t numItems )
    {
        if ( numItems == 0 )
        {
            return;
        }

        TInlineVector<Entity*, 10> entities;
        for ( int32_t i = 0; i < numItems; i++ )
        {
            if ( pEntityItems[i].IsEntity() )
            {
                entities.emplace_back( pEntityItems[i].m_pEntity );
            }
        }

        AddOrMoveEntitiesToGroup( mapID, groupID, entities );
    }

    void EditorContext::RemoveEntitiesFromAllGroups( Entity* const* ppEntities, size_t numEntities )
    {
        if ( numEntities == 0 )
        {
            return;
        }

        //-------------------------------------------------------------------------

        ScopedChange const sc( this );

        auto pAction = New<EntityGroupsUndoableAction>( this );
        pAction->Begin();

        for ( int32_t i = 0; i < numEntities; i++ )
        {
            Entity* pEntity = ppEntities[i];
            EE_ASSERT( m_pWorld->HasMap( pEntity->GetMapID() ) );

            auto iter = m_entityGroups.find( pEntity->GetMapID() );
            EE_ASSERT( iter != m_entityGroups.end() );

            EntityEditorItem const item( pEntity );
            for ( EntityEditorItemGroup& group : iter->second )
            {
                group.m_items.erase_first( item );
            }
        }

        pAction->End();
        m_pUndoStack->RegisterAction( pAction );
    }

    void EditorContext::RemoveEntitiesFromAllGroups( EntityEditorItem const* pEntityItems, size_t numEntities )
    {
        if ( numEntities == 0 )
        {
            return;
        }

        TInlineVector<Entity*, 10> entities;
        for ( int32_t i = 0; i < numEntities; i++ )
        {
            if ( pEntityItems[i].IsEntity() )
            {
                entities.emplace_back( pEntityItems[i].m_pEntity );
            }
        }

        RemoveEntitiesFromAllGroups( entities );
    }

    void EditorContext::ShowEntityGroupRenameDialog( EntityMapID const& mapID, StringID groupID )
    {
        EE_ASSERT( mapID.IsValid() );
        EE_ASSERT( groupID.IsValid() && HasEntityGroup( mapID, groupID ) );
        m_pDialogManager->StartModalDialog<CreateOrRenameGroupDialog>( *this, CreateOrRenameGroupDialog::Type::Rename, mapID, groupID );
    }

    void EditorContext::ShowEntityGroupCreateDialog( EntityMapID const& mapID, StringID baseGroupID )
    {
        EE_ASSERT( mapID.IsValid() );
        m_pDialogManager->StartModalDialog<CreateOrRenameGroupDialog>( *this, CreateOrRenameGroupDialog::Type::Create, mapID, baseGroupID );
    }

    void EditorContext::ReparentGroup( EntityMapID const& mapID, StringID groupID, StringID newParentID )
    {
        EE_ASSERT( mapID.IsValid() );
        EE_ASSERT( groupID.IsValid() );

        if ( newParentID.IsValid() )
        {
            EE_ASSERT( HasEntityGroup( mapID, newParentID ) );

            // Cant reparent to  ourselves
            if ( groupID == newParentID )
            {
                return;
            }

            // Cant reparent to one of our children
            if ( EntityEditorItemGroup::IsParentOf( groupID, newParentID ) )
            {
                return;
            }

            // Already correctly parented
            if ( EntityEditorItemGroup::IsDirectParentOf( newParentID, groupID ) )
            {
                return;
            }
        }

        //-------------------------------------------------------------------------

        ScopedChange const sc( this );

        auto pGroup = GetEntityGroup( mapID, groupID );
        EE_ASSERT( pGroup != nullptr );

        auto pAction = New<EntityGroupsUndoableAction>( this );
        pAction->Begin();

        pGroup->ReplaceParent( newParentID );

        pAction->End();
        m_pUndoStack->RegisterAction( pAction );
    }

    bool EditorContext::DoesGroupContainEntity( EntityMapID const& mapID, StringID groupID, Entity const* pEntity, bool recursiveSearch ) const
    {
        EE_ASSERT( mapID.IsValid() && groupID.IsValid() );

        EntityEditorItem item( const_cast<Entity*>( pEntity ) );

        if ( recursiveSearch )
        {
            auto iter = m_entityGroups.find( mapID );
            EE_ASSERT( iter != m_entityGroups.end() );

            TVector<EntityEditorItemGroup> const& groups = iter->second;
            for ( EntityEditorItemGroup const& group : groups )
            {
                if ( group.m_ID == groupID || group.IsChildOf( groupID ) )
                {
                    if ( VectorContains( group.m_items, item ) )
                    {
                        return true;
                    }
                }
            }
        }
        else
        {
            auto pGroup = GetEntityGroup( mapID, groupID );
            EE_ASSERT( pGroup != nullptr );
            return VectorContains( pGroup->m_items, item );
        }

        return false;
    }

    void EditorContext::GetAllEntitiesInGroup( EntityMapID const& mapID, StringID parentGroupID, TVector<Entity*>& outEntities, bool recurse ) const
    {
        EE_ASSERT( mapID.IsValid() && parentGroupID.IsValid() );
       
        auto iter = m_entityGroups.find( mapID );
        EE_ASSERT( iter != m_entityGroups.end() );

        outEntities.clear();

        TVector<EntityEditorItemGroup> const& groups = iter->second;
        for ( EntityEditorItemGroup const& childGroup : groups )
        {
            if ( childGroup.m_ID == parentGroupID || ( recurse && childGroup.IsChildOf( parentGroupID ) ) )
            {
                for ( auto& item : childGroup.m_items )
                {
                    outEntities.emplace_back( item.m_pEntity );
                }
            }
        }
    }

    void EditorContext::GetAllGroupsInGroup( EntityMapID const& mapID, StringID parentGroupID, TVector<StringID>& outGroups, bool recurse ) const
    {
        EE_ASSERT( mapID.IsValid() && parentGroupID.IsValid() );

        auto iter = m_entityGroups.find( mapID );
        EE_ASSERT( iter != m_entityGroups.end() );

        outGroups.clear();

        TVector<EntityEditorItemGroup> const& groups = iter->second;
        for ( EntityEditorItemGroup const& childGroup : groups )
        {
            if( childGroup.m_ID == parentGroupID )
            {
                continue;
            }

            if ( recurse )
            {
                if (  childGroup.IsChildOf( parentGroupID ) )
                {
                    outGroups.emplace_back( childGroup.m_ID );
                }
            }
            else
            {
                if ( childGroup.IsDirectChildOf( parentGroupID ) )
                {
                    outGroups.emplace_back( childGroup.m_ID );
                }
            }
        }
    }

    void EditorContext::FixUpGroupPtrs()
    {
        for ( auto& mapGroupPair : m_entityGroups )
        {
            for ( auto& group : mapGroupPair.second )
            {
                group.UpdateEntityPtrs( m_pWorld );
            }
        }
    }

    //-------------------------------------------------------------------------

    Entity* EditorContext::CreateEntity( EntityMap* pMap, StringID groupID )
    {
        EE_ASSERT( pMap != nullptr && pMap->IsMapLoaded() );
        EE_ASSERT( m_pWorld->HasMap( pMap->GetID() ) );

        bool const shouldStartCompoundAction = !m_pUndoStack->IsCompoundActionActive();
        if ( groupID.IsValid() )
        {
            if ( shouldStartCompoundAction )
            {
                m_pUndoStack->BeginCompoundAction();
            }
        }

        // Create Entity
        //-------------------------------------------------------------------------

        ScopedChange const sc( this );

        StringID const uniqueNameID = pMap->GenerateUniqueEntityNameID( "Entity" );
        Entity* pEntity = New<Entity>( StringID( uniqueNameID ) );

        // Add entity to map
        pMap->AddEntity( pEntity );

        // Record undo action
        auto pActiveUndoableAction = New<EntityUndoableAction>( this );
        pActiveUndoableAction->RecordCreate( pEntity );
        m_pUndoStack->RegisterAction( pActiveUndoableAction );

        // Add to group
        //-------------------------------------------------------------------------

        AddOrMoveEntityToGroup( pMap->GetID(), groupID, pEntity );

        //-------------------------------------------------------------------------

        if ( groupID.IsValid() )
        {
            if ( shouldStartCompoundAction )
            {
                m_pUndoStack->EndCompoundAction();
            }
        }

        return pEntity;
    }

    void EditorContext::AddEntityToMap( EntityMap* pMap, Entity* pEntity )
    {
        EE_ASSERT( pMap != nullptr && pMap->IsMapLoaded() );
        EE_ASSERT( m_pWorld->HasMap( pMap->GetID() ) );
        EE_ASSERT( pEntity != nullptr && !pEntity->IsAddedToMap() );

        ScopedChange const sc( this );

        pMap->AddEntity( pEntity );

        auto pActiveUndoableAction = New<EntityUndoableAction>( this );
        pActiveUndoableAction->RecordCreate( { pEntity } );
        m_pUndoStack->RegisterAction( pActiveUndoableAction );
    }

    void EditorContext::DestroyEntity( EntityMap* pMap, Entity* pEntity )
    {
        EE_ASSERT( pMap != nullptr );
        EE_ASSERT( pEntity != nullptr );

        ScopedChange const sc( this );

        bool shouldStartCompoundAction = !m_pUndoStack->IsCompoundActionActive();
        if ( shouldStartCompoundAction )
        {
            m_pUndoStack->BeginCompoundAction();
        }

        // Remove grouping
        RemoveEntitiesFromAllGroups( pEntity );

        // Record delete action
        auto pActiveUndoableAction = New<EntityUndoableAction>( this );
        pActiveUndoableAction->RecordDelete( { pEntity } );
        m_pUndoStack->RegisterAction( pActiveUndoableAction );

        if ( shouldStartCompoundAction )
        {
            m_pUndoStack->EndCompoundAction();
        }

        // Destroy and deselect entity
        RemoveFromSelection( pEntity );
        pMap->DestroyEntity( pEntity->GetID() );
    }

    void EditorContext::DestroyEntity( Entity* pEntity )
    {
        EntityMap* pMap = m_pWorld->FindMapForEntity( pEntity );
        EE_ASSERT( pMap != nullptr );
        DestroyEntity( pMap, pEntity );
    }

    void EditorContext::DestroyEntities( TVector<Entity*>& entitiesToDelete )
    {
        ScopedChange const sc( this );

        bool shouldStartCompoundAction = !m_pUndoStack->IsCompoundActionActive();
        if ( shouldStartCompoundAction)
        {
            m_pUndoStack->BeginCompoundAction();
        }

        // Remove grouping
        RemoveEntitiesFromAllGroups( entitiesToDelete );

        // Record delete action
        auto pActiveUndoableAction = New<EntityUndoableAction>( this );
        pActiveUndoableAction->RecordDelete( entitiesToDelete );
        m_pUndoStack->RegisterAction( pActiveUndoableAction );

        if ( shouldStartCompoundAction )
        {
            m_pUndoStack->EndCompoundAction();
        }

        // Destroy and deselect entity
        for ( auto pEntity : entitiesToDelete )
        {
            EntityMap* pMap = m_pWorld->FindMapForEntity( pEntity );
            EE_ASSERT( pMap != nullptr );
            RemoveFromSelection( pEntity );
            pMap->DestroyEntity( pEntity->GetID() );
        }
    }

    void EditorContext::RenameEntity( Entity* pEntity, StringID newNameID )
    {
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( newNameID.IsValid() );

        if ( pEntity->GetNameID() == newNameID )
        {
            return;
        }

        ScopedChange const sc( this );

        auto pMap = m_pWorld->FindMapForEntity( pEntity );
        auto pActiveUndoableAction = New<EntityUndoableAction>( this );
        pActiveUndoableAction->RecordBeginEdit( pEntity );

        pMap->RenameEntity( pEntity, newNameID );

        pActiveUndoableAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoableAction );
    }

    void EditorContext::ReparentEntity( Entity* pEntity, Entity* pNewParentEntity )
    {
        EE_ASSERT( pEntity != nullptr );

        if ( pNewParentEntity != nullptr )
        {
            if ( pEntity->GetSpatialParent() == pNewParentEntity )
            {
                return;
            }
        }
        else // No parent
        {
            if ( !pEntity->HasSpatialParent() )
            {
                return;
            }
        }

        //-------------------------------------------------------------------------

        ScopedChange const sc( this );
        auto pActiveUndoableAction = New<EntityUndoableAction>( this );

        // Change parent
        if ( pNewParentEntity != nullptr )
        {
            TInlineVector<EntityEditorItem, 2> entitiesToEdit = { pEntity, pNewParentEntity };
            pActiveUndoableAction->RecordBeginEdit( entitiesToEdit );
            pEntity->SetSpatialParent( pNewParentEntity, StringID() );
            pActiveUndoableAction->RecordEndEdit();
        }
        // Remove parent
        else
        {
            EE_ASSERT( pEntity->HasSpatialParent() );
            TInlineVector<EntityEditorItem, 2> entitiesToEdit = { pEntity, pEntity->GetSpatialParent() };
            pActiveUndoableAction->RecordBeginEdit( entitiesToEdit );
            pEntity->ClearSpatialParent();
            pActiveUndoableAction->RecordEndEdit();
        }

        m_pUndoStack->RegisterAction( pActiveUndoableAction );
    }

    void EditorContext::ShowEntityRenameDialog( Entity* pEntity )
    {
        EE_ASSERT( pEntity != nullptr );
        m_pDialogManager->StartModalDialog<RenameEntityDialog>( *this, pEntity );
    }

    TVector<Entity*> EditorContext::DuplicateEntities( TVector<Entity*> const& entities )
    {
        TVector<Entity*> duplicatedEntities;

        //-------------------------------------------------------------------------

        if ( entities.empty() )
        {
            return duplicatedEntities;
        }

        // Get Map
        //-------------------------------------------------------------------------

        EntityMap* pMap = nullptr;

        for ( auto pEntity : entities )
        {
            EE_ASSERT( pEntity != nullptr );

            if ( pMap != nullptr )
            {
                EE_ASSERT( pEntity->GetMapID() == pMap->GetID() );
            }
            else
            {
                pMap = m_pWorld->FindMapForEntity( pEntity );
            }
        }

        EE_ASSERT( pMap != nullptr );

        // Serialized all selected entities
        //-------------------------------------------------------------------------

        TVector<EntityDescriptor> entityDescs;
        SerializeEntities( entities, entityDescs );

        if ( entityDescs.empty() )
        {
            return duplicatedEntities;
        }

        // Duplicate the entities and add them to the map
        //-------------------------------------------------------------------------

        duplicatedEntities = CreateEntities( pMap, entityDescs );

        return duplicatedEntities;
    }

    void EditorContext::DeleteEntities( TVector<Entity*>& entitiesToDelete )
    {
        if ( entitiesToDelete.empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        ScopedChange const sc( this );

        auto pUndoAction = New<EntityUndoableAction>( this );
        pUndoAction->RecordDelete( entitiesToDelete );
        m_pUndoStack->RegisterAction( pUndoAction );

        for ( auto pEntity : entitiesToDelete )
        {
            auto pMap = m_pWorld->FindMapForEntity( pEntity );
            RemoveFromSelection( pEntity );
            pMap->DestroyEntity( pEntity->GetID() );
        }
    }

    //-------------------------------------------------------------------------

    EntityComponent* EditorContext::CreateComponent( Entity* pEntity, TypeSystem::TypeID const componentTypeID, SpatialEntityComponent* pParentComponent )
    {
        EE_ASSERT( componentTypeID.IsValid() );
        EE_ASSERT( pEntity != nullptr );

        ScopedChange const sc( this );

        auto pActiveUndoAction = New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( { pEntity } );

        //-------------------------------------------------------------------------

        auto pComponentTypeInfo = m_pToolsContext->m_pTypeRegistry->GetTypeInfo( componentTypeID );
        EE_ASSERT( pComponentTypeInfo != nullptr && pComponentTypeInfo->IsDerivedFrom( EntityComponent::GetStaticTypeID() ) );
        EE_ASSERT( !pComponentTypeInfo->GetDefaultInstance<EntityComponent>()->IsTransientComponent() );

        EntityComponent* pEntityComponent = nullptr;
        if ( pParentComponent != nullptr )
        {
            EE_ASSERT( pComponentTypeInfo->IsDerivedFrom( SpatialEntityComponent::GetStaticTypeID() ) );
            EE_ASSERT( VectorContains( pEntity->m_components, pParentComponent ) );
            pEntityComponent = pEntity->CreateComponent( pComponentTypeInfo, pParentComponent->GetID() );
        }
        else
        {
            pEntityComponent = pEntity->CreateComponent( pComponentTypeInfo );
        }

        EE_ASSERT( pEntityComponent != nullptr );

        // Record modified world state
        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );

        return pEntityComponent;
    }

    void EditorContext::DestroyComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( VectorContains( pEntity->m_components, pComponent ) );

        ScopedChange const sc( this );

        auto pActiveUndoAction = New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( pEntity );

        //-------------------------------------------------------------------------

        RemoveFromSelection( EntityEditorItem( pEntity, pComponent ) );
        DestroyComponentInternal( pEntity, pComponent );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );
    }

    void EditorContext::DestroyComponentInternal( Entity* pEntity, EntityComponent* pComponent )
    {
        pEntity->DestroyComponent( pComponent->GetID() );

        // Check if there are any other spatial components left
        if ( pEntity->IsSpatialEntity() && IsOfType<SpatialEntityComponent>( pComponent ) )
        {
            bool isStillASpatialEntity = false;
            for ( auto pExistingComponent : pEntity->GetComponents() )
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
            if ( !isStillASpatialEntity && pEntity->HasSpatialParent() )
            {
                pEntity->ClearSpatialParent();
            }
        }
    }

    void EditorContext::RenameComponent( Entity* pEntity, EntityComponent* pComponent, StringID newNameID )
    {
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( VectorContains( pEntity->m_components, pComponent ) );

        //-------------------------------------------------------------------------

        auto pActiveUndoAction = New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( pEntity );

        //-------------------------------------------------------------------------

        pEntity->RenameComponent( pComponent, newNameID );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );
    }

    void EditorContext::ReparentComponent( Entity* pEntity, SpatialEntityComponent* pComponent, SpatialEntityComponent* pNewParentComponent )
    {
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( VectorContains( pEntity->m_components, pComponent ) );

        // Ignore re-parent requests to our current parent
        //-------------------------------------------------------------------------

        if ( pNewParentComponent == nullptr )
        {
            if ( !pComponent->HasSpatialParent() )
            {
                return;
            }
        }
        else // New parent specified
        {
            if ( pComponent->HasSpatialParent() && pComponent->GetSpatialParentID() == pNewParentComponent->GetID() )
            {
                return;
            }
        }

        //-------------------------------------------------------------------------

        if ( pNewParentComponent != nullptr )
        {
            EE_ASSERT( VectorContains( pEntity->m_components, pNewParentComponent ) );

            // Nothing to do, the new parent is the current parent
            if ( pComponent->HasSpatialParent() && pComponent->GetSpatialParentID() == pNewParentComponent->GetID() )
            {
                return;
            }

            ScopedChange const sc( this );

            // Create undo action
            auto pActiveUndoAction = New<EntityUndoableAction>( this );
            pActiveUndoAction->RecordBeginEdit( { pEntity } );
            m_pWorld->BeginComponentEdit( pEntity );

            //-------------------------------------------------------------------------

            // Remove the component from its old parent
            Transform const componentWorldTransform = pComponent->GetWorldTransform();
            if ( pComponent->IsRootComponent() )
            {
                EE_ASSERT( pComponent->HasSpatialChildren() );

                // Make the first child of the root the new root
                SpatialEntityComponent* pNewRootComponent = pComponent->m_spatialChildren[0];
                Transform const newRootWorldTransform = pNewRootComponent->GetWorldTransform();
                pComponent->m_spatialChildren.erase_first( pNewRootComponent );
                pNewRootComponent->m_pSpatialParent = nullptr;
                pNewRootComponent->SetLocalTransform( newRootWorldTransform );

                // Move all other children of the root to the new root
                for ( auto pComponentToMove : pComponent->m_spatialChildren )
                {
                    Transform const componentToMoveWorldTransform = pComponentToMove->GetWorldTransform();
                    pNewRootComponent->m_spatialChildren.emplace_back( pComponentToMove );
                    pComponentToMove->m_pSpatialParent = pNewRootComponent;
                    pComponent->SetLocalTransform( componentToMoveWorldTransform.GetDeltaFromOther( newRootWorldTransform ) );
                }

                // Clear all the root's children
                pComponent->m_spatialChildren.clear();
            }
            else // Just remove component from its parent
            {
                EE_ASSERT( pComponent->HasSpatialParent() );
                pComponent->m_pSpatialParent->m_spatialChildren.erase_first( pComponent );
                pComponent->m_pSpatialParent = nullptr;
            }

            // Add the component to its new parent
            Transform const parentWorldTransform = pNewParentComponent->GetWorldTransform();
            pNewParentComponent->m_spatialChildren.emplace_back( pComponent );
            pComponent->m_pSpatialParent = pNewParentComponent;
            pComponent->SetLocalTransform( componentWorldTransform.GetDeltaFromOther( parentWorldTransform ) );

            //-------------------------------------------------------------------------

            m_pWorld->EndComponentEdit( pEntity );
            pActiveUndoAction->RecordEndEdit();
            m_pUndoStack->RegisterAction( pActiveUndoAction );
        }
        else
        {
            // This component is already the root
            if ( pEntity->GetRootSpatialComponentID() == pComponent->GetID() )
            {
                return;
            }

            ScopedChange const sc( this );

            // Create undo action
            auto pActiveUndoAction = New<EntityUndoableAction>( this );
            pActiveUndoAction->RecordBeginEdit( { pEntity } );
            m_pWorld->BeginComponentEdit( pEntity );

            //-------------------------------------------------------------------------

            // Break any cross entity links
            bool recreateSpatialAttachment = pEntity->m_isSpatialAttachmentCreated;
            if ( recreateSpatialAttachment )
            {
                pEntity->DestroySpatialAttachment( Entity::SpatialAttachmentRule::KeepLocalTransform );
            }

            // Remove the component from its parent
            pComponent->m_pSpatialParent->m_spatialChildren.erase_first( pComponent );
            pComponent->m_pSpatialParent = nullptr;

            // Add the old root component as a child of the new root
            pComponent->m_spatialChildren.emplace_back( pEntity->m_pRootSpatialComponent );
            pEntity->m_pRootSpatialComponent->m_pSpatialParent = pComponent;

            // Make the new component the root
            pEntity->m_pRootSpatialComponent = pComponent;

            // Recreate the cross entity links
            if ( recreateSpatialAttachment )
            {
                pEntity->CreateSpatialAttachment();
            }

            //-------------------------------------------------------------------------

            m_pWorld->EndComponentEdit( pEntity );
            pActiveUndoAction->RecordEndEdit();
            m_pUndoStack->RegisterAction( pActiveUndoAction );
        }
    }

    void EditorContext::SwitchComponentsInHierarchy( Entity* pEntity, SpatialEntityComponent* pComponentA, SpatialEntityComponent* pComponentB )
    {
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( pComponentA != nullptr );
        EE_ASSERT( pComponentB != nullptr );
        EE_ASSERT( VectorContains( pEntity->m_components, pComponentA ) );
        EE_ASSERT( VectorContains( pEntity->m_components, pComponentB ) );

        //-------------------------------------------------------------------------

        ScopedChange const sc( this );

        // Create undo action
        auto pActiveUndoAction = New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( { pEntity } );
        m_pWorld->BeginComponentEdit( pEntity );

        // Directly parented to one another so we need to be more careful
        if ( pComponentA->IsSpatialChildOf( pComponentB ) || pComponentB->IsSpatialChildOf( pComponentA ) )
        {
            SpatialEntityComponent* pNewParent = nullptr;
            SpatialEntityComponent* pNewChild = nullptr;
            SpatialEntityComponent* pOldParent = nullptr;
            SpatialEntityComponent* pOldChild = nullptr;

            if ( pComponentA->IsSpatialChildOf( pComponentB ) )
            {
                pOldParent = pComponentB;
                pOldChild = pComponentA;
                pNewParent = pComponentA;
                pNewChild = pComponentB;
            }
            else
            {
                pOldParent = pComponentA;
                pOldChild = pComponentB;
                pNewParent = pComponentB;
                pNewChild = pComponentA;
            }

            //-------------------------------------------------------------------------

            Transform tempTransform = pOldChild->m_transform;
            TInlineVector<SpatialEntityComponent*, 2> tempChildren = pOldChild->m_spatialChildren;
            StringID tempParentAttachmentSocketID = pOldChild->m_parentAttachmentSocketID;

            // New Parent
            pNewParent->m_transform = pOldParent->m_transform;
            pNewParent->m_pSpatialParent = pOldParent->m_pSpatialParent;
            pNewParent->m_spatialChildren = pOldParent->m_spatialChildren;
            pNewParent->m_parentAttachmentSocketID = pOldParent->m_parentAttachmentSocketID;
            pNewParent->m_spatialChildren.erase_first( pOldChild );
            pNewParent->m_spatialChildren.emplace_back( pOldParent );

            // New Child
            pNewChild->m_transform = tempTransform;
            pNewChild->m_pSpatialParent = pNewParent;
            pNewChild->m_spatialChildren = tempChildren;
            pNewChild->m_parentAttachmentSocketID = tempParentAttachmentSocketID;
            pNewChild->m_spatialChildren.erase_first( pOldParent );
        }
        else // Not directly parented to one another so just exchange data
        {
            Transform tempTransform = pComponentA->m_transform;
            SpatialEntityComponent* pTempParent = pComponentA->m_pSpatialParent;
            TInlineVector<SpatialEntityComponent*, 2> tempChildren = pComponentA->m_spatialChildren;
            StringID tempParentAttachmentSocketID = pComponentA->m_parentAttachmentSocketID;

            pComponentA->m_transform = pComponentB->m_transform;
            pComponentA->m_pSpatialParent = pComponentB->m_pSpatialParent;
            pComponentA->m_spatialChildren = pComponentB->m_spatialChildren;
            pComponentA->m_parentAttachmentSocketID = pComponentB->m_parentAttachmentSocketID;

            pComponentB->m_transform = tempTransform;
            pComponentB->m_pSpatialParent = pTempParent;
            pComponentB->m_spatialChildren = tempChildren;
            pComponentB->m_parentAttachmentSocketID = tempParentAttachmentSocketID;
        }

        m_pWorld->EndComponentEdit( pEntity );
        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );
    }

    EntityComponent* EditorContext::MoveComponent( Entity* pEntity, EntityComponent* pComponent, Entity* pNewParentEntity, SpatialEntityComponent* pNewParentComponent )
    {
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( pNewParentEntity != nullptr );
        EE_ASSERT( VectorContains( pEntity->m_components, pComponent ) );

        auto pSpatialEntityComponent = TryCast<SpatialEntityComponent>( pComponent );

        if ( pNewParentComponent != nullptr )
        {
            EE_ASSERT( pSpatialEntityComponent != nullptr ); // Cant parent a non-spatial component
            EE_ASSERT( VectorContains( pNewParentEntity->m_components, pNewParentComponent ) );
        }

        // Ignore re-parent requests to our current parent
        //-------------------------------------------------------------------------

        if ( pEntity == pNewParentEntity )
        {
            if ( pSpatialEntityComponent != nullptr )
            {
                if ( pNewParentComponent == nullptr )
                {
                    if ( !pSpatialEntityComponent->HasSpatialParent() )
                    {
                        return pComponent;
                    }
                }
                else
                {
                    if ( pSpatialEntityComponent->GetSpatialParentID() == pNewParentComponent->GetID() )
                    {
                        return pComponent;
                    }
                }
            }
            else // We are already under this entity
            {
                return pComponent;
            }
        }

        // Move Spatial components with the same parent
        //-------------------------------------------------------------------------

        if ( pEntity == pNewParentEntity )
        {
            ReparentComponent( pEntity, pSpatialEntityComponent, pNewParentComponent );
            return pSpatialEntityComponent;
        }

        // Move Component
        //-------------------------------------------------------------------------

        ScopedChange const sc( this );

        auto pActiveUndoAction = New<EntityUndoableAction>( this );
        TInlineVector<EntityEditorItem, 2> entitiesToEdit = { pEntity, pNewParentEntity };
        pActiveUndoAction->RecordBeginEdit( entitiesToEdit );

        //-------------------------------------------------------------------------

        TypeSystem::TypeInfo const* pComponentTypeInfo = pComponent->GetTypeInfo();

        // Write source component to xml
        pugi::xml_document doc;
        Serialization::WriteType( *m_pToolsContext->m_pTypeRegistry, pComponent, doc );
        pEntity->DestroyComponent( pComponent->GetID() );

        // Create new component
        EntityComponent* pNewComponent = nullptr;
        if ( pNewParentComponent )
        {
            pNewComponent = pNewParentEntity->CreateComponent( pComponentTypeInfo, pNewParentComponent->GetID() );
        }
        else
        {
            pNewComponent = pNewParentEntity->CreateComponent( pComponentTypeInfo );
        }

        // Transfer component state
        Log log( LogCategory::Entity, "Entity Editor - Move Component" );
        Serialization::ReadType( *m_pToolsContext->m_pTypeRegistry, log, doc, pNewComponent );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );

        return pNewComponent;
    }

    void EditorContext::ShowComponentRenameDialog( Entity* pEntity, EntityComponent* pComponent )
    {
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( pComponent != nullptr );
        m_pDialogManager->StartModalDialog<RenameComponentDialog>( *this, pEntity, pComponent );
    }

    //-------------------------------------------------------------------------

    EntitySystem* EditorContext::CreateSystem( Entity* pEntity, TypeSystem::TypeID const systemTypeID )
    {
        EE_ASSERT( systemTypeID.IsValid() );
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( pEntity->IsAddedToMap() );

        ScopedChange const sc( this );

        auto pActiveUndoAction = New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( pEntity );

        //-------------------------------------------------------------------------

        auto pSystemTypeInfo = m_pToolsContext->m_pTypeRegistry->GetTypeInfo( systemTypeID );
        EE_ASSERT( pSystemTypeInfo != nullptr && pSystemTypeInfo->IsDerivedFrom( EntitySystem::GetStaticTypeID() ) );
        pEntity->CreateSystem( pSystemTypeInfo );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );

        return pEntity->GetSystem( systemTypeID );
    }

    void EditorContext::DestroySystem( Entity* pEntity, EntitySystem* pSystem )
    {
        EE_ASSERT( pSystem != nullptr );
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( VectorContains( pEntity->m_systems, pSystem ) );

        ScopedChange const sc( this );

        auto pActiveUndoAction = New<EntityUndoableAction>( this );
        pActiveUndoAction->RecordBeginEdit( { pEntity } );

        //-------------------------------------------------------------------------

        RemoveFromSelection( EntityEditorItem( pEntity, pSystem ) );

        pEntity->DestroySystem( pSystem->GetTypeID() );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );
    }

    EntitySystem* EditorContext::MoveSystem( Entity* pEntity, EntitySystem* pSystem, Entity* pNewParentEntity )
    {
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( pSystem != nullptr );
        EE_ASSERT( pNewParentEntity != nullptr );
        EE_ASSERT( pEntity->HasSystem( pSystem->GetTypeID() ) );

        if ( pEntity == pNewParentEntity )
        {
            return pSystem;
        }

        ScopedChange const sc( this );

        auto pActiveUndoAction = New<EntityUndoableAction>( this );
        TInlineVector<EntityEditorItem, 2> entitiesToEdit = { pEntity, pNewParentEntity };
        pActiveUndoAction->RecordBeginEdit( entitiesToEdit );

        //-------------------------------------------------------------------------

        TypeSystem::TypeInfo const* pSystemTypeInfo = pSystem->GetTypeInfo();
        pEntity->DestroySystem( pSystemTypeInfo );
        pNewParentEntity->CreateSystem( pSystemTypeInfo );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );

        return pNewParentEntity->GetSystem( pSystemTypeInfo->m_ID );
    }

    //-------------------------------------------------------------------------

    void EditorContext::ClearSelection()
    {
        if ( !m_selection.empty() )
        {
            m_selection.clear();
            OnSelectionChangedInternal();
        }
    }

    void EditorContext::SetSelection( EntityEditorItem const* pItemsToSelect, size_t numItems )
    {
        if ( numItems == 0 )
        {
            return ClearSelection();
        }

        //-------------------------------------------------------------------------

        TVector<EntityEditorItem> newSelection;
        for ( size_t i = 0; i < numItems; i++ )
        {
            EE_ASSERT( pItemsToSelect[i].IsValid() );

            // Always create a separate selection entry for the entity and the entity element
            if ( !pItemsToSelect[i].IsEntity() )
            {
                VectorEmplaceBackUnique( newSelection, EntityEditorItem( pItemsToSelect[i].m_pEntity ) );
            }

            VectorEmplaceBackUnique( newSelection, pItemsToSelect[i] );
        }

        bool updateSelection = ( m_selection.size() != newSelection.size() );
        if ( !updateSelection )
        {
            for ( size_t i = 0; i < numItems; i++ )
            {
                if ( m_selection[i] != newSelection[i] )
                {
                    updateSelection = true;
                    break;
                }
            }
        }

        if ( updateSelection )
        {
            m_selection.swap( newSelection );
        }

        FixUpSelectionPtrs();

        if ( updateSelection )
        {
            OnSelectionChangedInternal();
        }
    }

    void EditorContext::AddToSelection( EntityEditorItem const* pItemsToSelect, size_t numItems )
    {
        if ( numItems == 0 )
        {
            return;
        }

        bool selectionChanged = false;
        for ( size_t i = 0; i < numItems; i++ )
        {
            EE_ASSERT( pItemsToSelect[i].IsValid() );

            if ( !VectorContains( m_selection, pItemsToSelect[i] ) )
            {
                // Always create a separate selection entry for the entity and the entity element
                if ( !pItemsToSelect[i].IsEntity() )
                {
                    VectorEmplaceBackUnique( m_selection, EntityEditorItem( pItemsToSelect[i].m_pEntity ) );
                }

                m_selection.emplace_back( pItemsToSelect[i] );
                selectionChanged = true;
            }
        }

        if ( selectionChanged )
        {
            OnSelectionChangedInternal();
        }
    }

    void EditorContext::RemoveFromSelection( EntityEditorItem const& data )
    {
        if ( VectorContains( m_selection, data ) )
        {
            m_selection.erase_first( data );
            OnSelectionChangedInternal();
        }
    }

    void EditorContext::FixUpSelectionPtrs()
    {
        for ( int32_t i = (int32_t) m_selection.size() - 1; i >= 0; i-- )
        {
            m_selection[i].UpdateEntityPtrs( m_pWorld );
            if ( !m_selection[i].IsValid() )
            {
                m_selection.erase( m_selection.begin() + i );
            }
        }

        for ( int32_t i = (int32_t) m_spatialSelection.size() - 1; i >= 0; i-- )
        {
            m_spatialSelection[i].UpdateEntityPtrs( m_pWorld );
            if ( !m_spatialSelection[i].IsValid() )
            {
                m_spatialSelection.erase( m_spatialSelection.begin() + i );
            }
        }
    }

    void EditorContext::OnSelectionChangedInternal()
    {
        // Check if we have spatial selection
        //-------------------------------------------------------------------------

        TInlineVector<EntityEditorItem, 20> newSpatialSelection;

        // Get all selected spatial entities
        for ( auto const& item : m_selection )
        {
            if ( item.IsEntity() && item.IsSpatialEntity() )
            {
                newSpatialSelection.emplace_back( item );
            }
        }

        // Add any loose components we have selected
        for ( auto const& item : m_selection )
        {
            if ( item.IsSpatialComponent() )
            {
                newSpatialSelection.emplace_back( item );
            }
        }

        SetSpatialSelection( newSpatialSelection.data(), newSpatialSelection.size() );

        // Notify Selection Changed
        //-------------------------------------------------------------------------

        m_selectionChangedEvent.Execute();
    }

    //-------------------------------------------------------------------------

    bool EditorContext::DoesSpatialSelectionSupportNonUniformScale() const
    {
        if ( m_spatialSelection.empty() )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        for ( EntityEditorItem const& item : m_spatialSelection )
        {
            if ( item.IsSpatialComponent() )
            {
                if ( !item.GetSpatialComponent()->SupportsNonUniformScale() )
                {
                    return false;
                }
            }
            else if ( item.IsSpatialEntity() )
            {
                if ( !item.m_pEntity->GetRootSpatialComponent()->SupportsNonUniformScale() )
                {
                    return false;
                }
            }
        }

        return true;
}

    bool EditorContext::DoesSpatialSelectionContainComponents() const
    {
        for ( EntityEditorItem const& item : m_spatialSelection )
        {
            if ( item.IsSpatialComponent() )
            {
                return true;
            }
        }

        return false;
    }

    void EditorContext::ClearSpatialSelection()
    {
        m_selectedItemBounds.clear();
        m_selectionCombinedBounds.Reset();
        m_selectionTransform = Transform::Identity;
        m_selectionOffsetTransforms.clear();
        m_spatialSelection.clear();
    }

    void EditorContext::GetSpatialSelectionWorldTransforms( TInlineVector<Transform, 10>& outTransforms ) const
    {
        outTransforms.clear();

        for ( auto const& item : m_spatialSelection )
        {
            if ( item.IsEntity() && item.IsSpatialEntity() )
            {
                outTransforms.emplace_back( item.m_pEntity->GetWorldTransform() );
            }
            else if ( item.IsSpatialComponent() )
            {
                outTransforms.emplace_back( item.GetSpatialComponent()->GetWorldTransform() );
            }
            else
            {
                EE_UNREACHABLE_CODE();
            }
        }
    }

    void EditorContext::UpdateSpatialSelectionData()
    {
        m_selectedItemBounds.clear();
        m_selectionCombinedBounds.Reset();
        m_selectionTransform = Transform::Identity;
        m_selectionOffsetTransforms.clear();

        //-------------------------------------------------------------------------

        if ( !m_spatialSelection.empty() )
        {
            // Fix up pointers
            //-------------------------------------------------------------------------

            for ( int32_t i = (int32_t) m_spatialSelection.size() - 1; i >= 0; i-- )
            {
                m_spatialSelection[i].UpdateEntityPtrs( m_pWorld );
                if ( !m_spatialSelection[i].IsValid() )
                {
                    m_spatialSelection.erase( m_spatialSelection.begin() + i );
                }
            }

            // Parse selection
            //-------------------------------------------------------------------------
            // Record the the transforms of all found spatial elements in the offset array
            // Get all bounds for selected items

            TInlineVector<Vector, 100> boundsPointCloud;

            for ( auto const& item : m_spatialSelection )
            {
                if ( item.IsEntity() && item.IsSpatialEntity() )
                {
                    m_selectionOffsetTransforms.emplace_back( item.m_pEntity->GetWorldTransform() );
                    m_selectedItemBounds.emplace_back( item.m_pEntity->GetCombinedWorldBounds() );
                    m_selectedItemBounds.back().GetCorners( boundsPointCloud );
                }
                else if ( item.IsSpatialComponent() )
                {
                    m_selectionOffsetTransforms.emplace_back( item.GetSpatialComponent()->GetWorldTransform() );
                    m_selectedItemBounds.emplace_back( item.GetSpatialComponent()->GetWorldBounds() );
                    m_selectedItemBounds.back().GetCorners( boundsPointCloud );
                }
                else
                {
                    EE_UNREACHABLE_CODE();
                }
            }

            // Update selection transforms and bounds
            //-------------------------------------------------------------------------

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

            m_selectionCombinedBounds = OBB( boundsPointCloud.data(), (uint32_t) boundsPointCloud.size() );
        }

        m_spatialSelectionChangedEvent.Execute();
    }

    void EditorContext::SetSpatialSelection( EntityEditorItem const* pItemsToSelect, size_t numItems )
    {
        ClearSpatialSelection();

        //-------------------------------------------------------------------------

        TInlineVector<EntityEditorItem, 10> entitiesToModify;
        TInlineVector<EntityEditorItem, 10> componentsToModify;

        for ( int32_t i = 0; i < numItems; i++ )
        {
            EE_ASSERT( pItemsToSelect[i].IsSpatialEntity() || pItemsToSelect[i].IsSpatialComponent() );
            if ( pItemsToSelect[i].IsSpatialEntity() )
            {
                VectorEmplaceBackUnique( entitiesToModify, pItemsToSelect[i] );
            }
            else
            {
                VectorEmplaceBackUnique( componentsToModify, pItemsToSelect[i] );
            }
        }

        for ( EntityEditorItem const& componentItem : componentsToModify )
        {
            entitiesToModify.erase_first( EntityEditorItem( componentItem.m_pEntity ) );
        }

        m_spatialSelection.insert( m_spatialSelection.end(), entitiesToModify.begin(), entitiesToModify.end() );
        m_spatialSelection.insert( m_spatialSelection.end(), componentsToModify.begin(), componentsToModify.end() );

        //-------------------------------------------------------------------------

        UpdateSpatialSelectionData();
    }

    void EditorContext::BeginManipulatingSpatialSelection( ImGuiX::Gizmo::Result const& result, bool duplicateSelectionIfPossible )
    {
        EE_ASSERT( !m_isManipulatingSpatialSelection );
        EE_ASSERT( result.m_state == ImGuiX::GizmoState::StartedManipulating );

        if ( duplicateSelectionIfPossible && result.IsTranslation() && !DoesSpatialSelectionContainComponents() )
        {
            if ( !m_pUndoStack->IsCompoundActionActive() )
            {
                m_pUndoStack->BeginCompoundAction();
                m_spatialManipulationCreatedCompoundAction = true;
            }

            TVector<Entity*> entitiesToDuplicate;
            for ( EntityEditorItem const& item : GetSpatialSelection() )
            {
                EE_ASSERT( item.IsEntity() );
                VectorEmplaceBackUnique( entitiesToDuplicate, item.m_pEntity );
            }

            DuplicateEntities( entitiesToDuplicate );
        }

        BeginTransformEdit( m_spatialSelection );
        m_isManipulatingSpatialSelection = true;
        ManipulateSpatialSelection( result );
    }

    void EditorContext::ManipulateSpatialSelection( ImGuiX::Gizmo::Result const& result )
    {
        if ( !result.IsScale() )
        {
            Transform const newTransform = result.GetModifiedTransform( GetSpatialSelectionTransform() );
            PositionSpatialSelection( newTransform );
        }
        else
        {
            ScaleSpatialSelection( result.m_delta.ToFloat3() );
        }
    }

    void EditorContext::EndManipulatingSpatialSelection( ImGuiX::Gizmo::Result const& result )
    {
        EE_ASSERT( m_isManipulatingSpatialSelection );

        ManipulateSpatialSelection( result );
        EndTransformEdit();
        m_isManipulatingSpatialSelection = false;

        if ( m_spatialManipulationCreatedCompoundAction )
        {
            EE_ASSERT( m_pUndoStack->IsCompoundActionActive() );
            m_pUndoStack->EndCompoundAction();
            m_spatialManipulationCreatedCompoundAction = false;
        }
    }

    void EditorContext::PositionSpatialSelection( Transform const& newTransform )
    {
        EE_ASSERT( m_pActiveUndoableAction != nullptr );

        bool createdUndoAction = false;
        if ( !m_isManipulatingSpatialSelection )
        {
            BeginTransformEdit( m_spatialSelection );
            createdUndoAction = true;
        }

        //-------------------------------------------------------------------------

        int32_t const numItems = (int32_t) m_spatialSelection.size();
        for ( int32_t i = 0; i < numItems; i++ )
        {
            if ( m_spatialSelection[i].IsSpatialEntity() )
            {
                m_spatialSelection[i].m_pEntity->SetWorldTransform( m_selectionOffsetTransforms[i] * newTransform );
            }
            else // Component
            {
                EE_ASSERT( m_spatialSelection[i].IsSpatialComponent() );
                m_spatialSelection[i].GetSpatialComponent()->SetWorldTransform( m_selectionOffsetTransforms[i] * newTransform );
            }
        }

        UpdateSpatialSelectionData();

        //-------------------------------------------------------------------------

        if ( createdUndoAction )
        {
            EndTransformEdit();
        }
    }

    void EditorContext::ScaleSpatialSelection( Float3 const& scaleDelta )
    {
        EE_ASSERT( m_pActiveUndoableAction != nullptr );

        bool const supportsNonUniformScale = DoesSpatialSelectionSupportNonUniformScale();
        if ( !supportsNonUniformScale && ( ( scaleDelta.m_x != scaleDelta.m_y ) || ( scaleDelta.m_y != scaleDelta.m_z ) ) )
        {
            EE_LOG_WARNING( LogCategory::Entity, "Entity Editor Context", "Attempting to apply non-uniform scale to a spatial selection that doesnt support it" );
        }

        //-------------------------------------------------------------------------

        bool createdUndoAction = false;
        if ( !m_isManipulatingSpatialSelection )
        {
            BeginTransformEdit( m_spatialSelection );
            createdUndoAction = true;
        }

        //-------------------------------------------------------------------------

        int32_t const numItems = (int32_t) m_spatialSelection.size();
        for ( int32_t i = 0; i < numItems; i++ )
        {
            if ( m_spatialSelection[i].IsSpatialEntity() )
            {
                auto pRootSpatialComponent = m_spatialSelection[i].m_pEntity->GetRootSpatialComponent();
                Transform transform = pRootSpatialComponent->GetLocalTransform();
                transform.SetScale( transform.GetScale() + scaleDelta.m_x );
                pRootSpatialComponent->SetLocalTransform( transform );
            }
            else // Component
            {
                EE_ASSERT( m_spatialSelection[i].IsSpatialComponent() );

                SpatialEntityComponent* pSpatialComponent = m_spatialSelection[i].GetSpatialComponent();
                Transform transform = pSpatialComponent->GetLocalTransform();
                transform.SetScale( transform.GetScale() + scaleDelta.m_x );
                pSpatialComponent->SetLocalTransform( transform );
            }
        }

        UpdateSpatialSelectionData();

        //-------------------------------------------------------------------------

        if ( createdUndoAction )
        {
            EndTransformEdit();
        }
    }

    //-------------------------------------------------------------------------

    void EditorContext::FocusCamera( Entity* pEntity )
    {
        if ( !pEntity->IsSpatialEntity() )
        {
            return;
        }

        EE_ASSERT( m_pCamera != nullptr );
        AABB const worldBounds = pEntity->GetCombinedWorldBounds();
        m_pCamera->FocusOn( worldBounds );
    }

    void EditorContext::FocusCamera( SpatialEntityComponent* pComponent )
    {
        EE_ASSERT( m_pCamera != nullptr );
        OBB const worldBounds = pComponent->GetWorldBounds();
        m_pCamera->FocusOn( worldBounds );
    }

    void EditorContext::FocusCamera( TInlineVector<Entity*, 5> const& entities )
    {
        EE_ASSERT( m_pCamera != nullptr );
        AABB worldBounds;

        for ( auto pEntity : entities )
        {
            if ( !pEntity->IsSpatialEntity() )
            {
                continue;
            }

            AABB const entityWorldBounds = pEntity->GetCombinedWorldBounds();
            if ( worldBounds.IsValid() )
            {
                worldBounds = worldBounds.GetCombinedBox( worldBounds, entityWorldBounds );
            }
            else
            {
                worldBounds = entityWorldBounds;
            }
        }

        m_pCamera->FocusOn( worldBounds );
    }

    void EditorContext::FocusCamera( TInlineVector<SpatialEntityComponent*, 5> const& components )
    {
        EE_ASSERT( m_pCamera != nullptr );
        AABB worldBounds;

        for ( auto pComponent : components )
        {
            AABB const componentBounds = pComponent->GetWorldBounds().GetAABB();
            if ( worldBounds.IsValid() )
            {
                worldBounds = worldBounds.GetCombinedBox( worldBounds, componentBounds );
            }
            else
            {
                worldBounds = componentBounds;
            }
        }

        m_pCamera->FocusOn( worldBounds );
    }

    //-------------------------------------------------------------------------

    TVector<Entity*> EditorContext::CreateEntities( EntityMap* pMap, TVector<EntityDescriptor> const &entityDescs )
    {
        EE_ASSERT( pMap != nullptr );

        ScopedChange const sc( this );

        TVector<Entity*> createdEntities;

        // Duplicate the entities and add them to the map
        //-------------------------------------------------------------------------

        bool hasSpatialEntities = false;
        THashMap<StringID, StringID> nameRemap;
        int32_t const numEntitiesToDuplicate = (int32_t) entityDescs.size();
        for ( int32_t i = 0; i < numEntitiesToDuplicate; i++ )
        {
            // Create the entity
            auto pNewEntity = entityDescs[i].CreateEntity( *m_pToolsContext->m_pTypeRegistry );
            createdEntities.emplace_back( pNewEntity );

            // Add to map
            pMap->AddEntity( pNewEntity );
            hasSpatialEntities = hasSpatialEntities || pNewEntity->IsSpatialEntity();

            // Add to remap table
            nameRemap.insert( TPair<StringID, StringID>( entityDescs[i].m_name, pNewEntity->GetNameID() ) );
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
                if ( !createdEntities[i]->IsSpatialEntity() )
                {
                    continue;
                }

                if ( entityDescs[i].HasSpatialParent() )
                {
                    StringID const newParentID = ResolveEntityName( entityDescs[i].m_spatialParentName );
                    auto pParentEntity = pMap->FindEntityByName( newParentID );
                    EE_ASSERT( pParentEntity != nullptr );

                    StringID const newChildID = ResolveEntityName( entityDescs[i].m_name );
                    auto pChildEntity = pMap->FindEntityByName( newChildID );
                    EE_ASSERT( pChildEntity != nullptr );

                    pChildEntity->SetSpatialParent( pParentEntity, entityDescs[i].m_attachmentSocketID, Entity::SpatialAttachmentRule::KeepLocalTransform );
                }
            }
        }

        // Record undo action
        auto pActiveUndoableAction = New<EntityUndoableAction>( this );
        pActiveUndoableAction->RecordCreate( createdEntities );
        m_pUndoStack->RegisterAction( pActiveUndoableAction );

        return createdEntities;
    }

    void EditorContext::SerializeEntities( TVector<Entity*> const& entities, TVector<EntityDescriptor>& outDescs )
    {
        TInlineVector<Entity*, 20> processedEntities;

        for ( auto pEntity : entities )
        {
            EE_ASSERT( pEntity != nullptr );

            if ( VectorContains( processedEntities, pEntity ) )
            {
                continue;
            }

            Log log( LogCategory::Entity, "Entity Editor Context" );
            auto& serializedEntity = outDescs.emplace_back();
            if ( CreateEntityDescriptor( *m_pToolsContext->m_pTypeRegistry, log, pEntity, serializedEntity ) )
            {
                // Clear the serialized ID so that we will generate a new ID for any created entities from this desc
                serializedEntity.ClearAllSerializedIDs();
            }
            else
            {
                log.LogWarning( "Failed to serialize entity to entity descriptor" );
                outDescs.pop_back();
            }

            processedEntities.emplace_back( pEntity );
        }
    }

    void EditorContext::SerializeComponents( TVector<EntityComponent*> const& components, TVector<ComponentDescriptor>& outDescs )
    {
        TInlineVector<EntityComponent*, 20> processedComponents;

        for ( auto pComponent : components )
        {
            EE_ASSERT( pComponent != nullptr );

            if ( VectorContains( processedComponents, pComponent ) )
            {
                continue;
            }

            Log log( LogCategory::Entity, "Entity Editor Context" );
            auto& serializedComponent = outDescs.emplace_back();
            if ( !CreateComponentDescriptor( *m_pToolsContext->m_pTypeRegistry, log, pComponent, serializedComponent ) )
            {
                log.LogWarning( "Failed to serialize entity to entity component: %s", pComponent->GetNameID().c_str() );
                outDescs.pop_back();
            }

            processedComponents.emplace_back( pComponent );
        }
    }

    //-------------------------------------------------------------------------

    void EditorContext::PreUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        EntityUndoableAction const* pUndoableAction = static_cast<EntityUndoableAction const*>( pAction );
        if ( pUndoableAction->GetActionType() != EntityUndoableAction::TransformUpdate )
        {
            m_preWorldChangeEvent.Execute();
        }
    }

    void EditorContext::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        bool triggerPostWorldChangeEvent = true;
        TVector<EntityEditorItem> newSelection;

        if ( CompoundStackAction const* pCompoundAction = TryCast<CompoundStackAction>( pAction ) )
        {
            for ( auto pSubAction : pCompoundAction->GetActions() )
            {
                if ( EntityUndoableAction const* pUndoableAction = TryCast<EntityUndoableAction>( pSubAction ) )
                {
                    TVector<EntityEditorItem> const& recordedSelection = pUndoableAction->GetRecordedSelection( operation );
                    newSelection.insert( newSelection.end(), recordedSelection.begin(), recordedSelection.end() );
                }
            }
        }
        else if ( EntityUndoableAction const* pUndoableAction = TryCast<EntityUndoableAction>( pAction ) )
        {
            newSelection = pUndoableAction->GetRecordedSelection( operation );
            triggerPostWorldChangeEvent = !( pUndoableAction->GetActionType() == EntityUndoableAction::TransformUpdate );
        }

        // Update editor
        //-------------------------------------------------------------------------

        if ( triggerPostWorldChangeEvent )
        {
            m_postWorldChangeEvent.Execute();
        }

        // Fix up selection ptrs
        //-------------------------------------------------------------------------

        for ( int32_t i = (int32_t) newSelection.size() - 1; i >= 0; i-- )
        {
            newSelection[i].UpdateEntityPtrs( m_pWorld );
            if ( !newSelection[i].IsValid() )
            {
                newSelection.erase( newSelection.begin() + i );
            }
        }

        // Update selection to the recorded state
        //-------------------------------------------------------------------------

        SetSelection( newSelection.data(), newSelection.size() );
        FixUpGroupPtrs();
        UpdateSpatialSelectionData();
    }

    void EditorContext::BeginEdit( TVector<Entity*> entitiesToBeModified )
    {
        TVector<EntityEditorItem> itemsToBeModified;
        for ( auto const& pEntity : entitiesToBeModified )
        {
            EE_ASSERT( pEntity != nullptr );
            VectorEmplaceBackUnique( itemsToBeModified, EntityEditorItem( pEntity ) );
        }

        auto pEntityUndoAction = New<EntityUndoableAction>( this );
        pEntityUndoAction->RecordBeginEdit( itemsToBeModified );
        m_pActiveUndoableAction = pEntityUndoAction;
    }

    void EditorContext::BeginEdit( TVector<EntityEditorItem>& itemsToBeModified )
    {
        EE_ASSERT( m_pActiveUndoableAction == nullptr );

        auto pEntityUndoAction = New<EntityUndoableAction>( this );
        pEntityUndoAction->RecordBeginEdit( itemsToBeModified );
        m_pActiveUndoableAction = pEntityUndoAction;
    }

    void EditorContext::EndEdit()
    {
        EE_ASSERT( m_pActiveUndoableAction != nullptr );

        auto pEntityUndoAction = Cast<EntityUndoableAction>( m_pActiveUndoableAction );
        pEntityUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pEntityUndoAction );
        m_pActiveUndoableAction = nullptr;
    }

    void EditorContext::BeginSingleComponentEdit( Entity* pEntity, EntityComponent* pComponent )
    {
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        EE_ASSERT( pComponent != nullptr );

        auto pUndoAction = New<EntityUndoableAction>( this );
        pUndoAction->RecordBeginEdit( EntityEditorItem( pEntity, pComponent ) );
        m_pActiveUndoableAction = pUndoAction;

        //-------------------------------------------------------------------------

        m_pWorld->BeginComponentEdit( pComponent );
    }

    void EditorContext::EndSingleComponentEdit( EntityComponent* pComponent )
    {
        EE_ASSERT( m_pActiveUndoableAction != nullptr );
        EE_ASSERT( pComponent != nullptr );

        auto pUndoAction = (EntityUndoableAction*) m_pActiveUndoableAction;
        pUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pUndoAction );
        m_pActiveUndoableAction = nullptr;

        //-------------------------------------------------------------------------

        m_pWorld->EndComponentEdit( pComponent );
    }

    void EditorContext::BeginTransformEdit( TVector<EntityEditorItem>& itemsToBeModified )
    {
        EE_ASSERT( m_pActiveUndoableAction == nullptr );

        auto pEntityUndoAction = New<EntityUndoableAction>( this );
        pEntityUndoAction->RecordBeginTransformEdit( itemsToBeModified );
        m_pActiveUndoableAction = pEntityUndoAction;
    }

    void EditorContext::BeginTransformEdit( TVector<Entity*>& entitiesToBeModified )
    {
        TVector<EntityEditorItem> itemsToBeModified;
        for ( auto const& pEntity : entitiesToBeModified )
        {
            EE_ASSERT( pEntity != nullptr );
            VectorEmplaceBackUnique( itemsToBeModified, EntityEditorItem( pEntity ) );
        }

        BeginTransformEdit( itemsToBeModified );
    }

    void EditorContext::EndTransformEdit()
    {
        EE_ASSERT( m_pActiveUndoableAction != nullptr );

        auto pEntityUndoAction = Cast<EntityUndoableAction>( m_pActiveUndoableAction );
        pEntityUndoAction->RecordEndTransformEdit();
        m_pUndoStack->RegisterAction( m_pActiveUndoableAction );
        m_pActiveUndoableAction = nullptr;
    }
}