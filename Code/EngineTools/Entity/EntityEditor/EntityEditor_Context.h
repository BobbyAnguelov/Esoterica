#pragma once

#include "EngineTools/_Module/API.h"
#include "EntityEditor_EntityItem.h"
#include "EngineTools/Core/UndoStack.h"
#include "EngineTools/Entity/EntityToolTypes.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/TypeSystem/TypeInfo.h"
#include "Base/Types/Function.h"
#include "Engine/Imgui/ImguiGizmo.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
    class ToolsContext;
    class IUndoableAction;
    class UndoStack;
    class DialogManager;
    class ToolsCameraComponent;
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EE_ENGINETOOLS_API EditorContext final
    {
        friend struct ScopedChange;
        friend struct ScopedGroupChange;
        friend class EntityGroupsUndoableAction;

    public:

        EditorContext( ToolsContext const *pToolsContext, UndoStack* pUndoStack, DialogManager* pDialogManager, EntityWorld* pWorld, ToolsCameraComponent* pCamera );
        ~EditorContext();

        // Update / World State
        //-------------------------------------------------------------------------

        void Update( UpdateContext const& context );

        inline bool HasPendingEntityStateChangeActions() const { return m_pWorld->HasPendingEntityStateChangeActions(); }

        // World
        //-------------------------------------------------------------------------

        EntityWorld const* GetWorld() const { return m_pWorld; }
        EntityWorld* GetWorld() { return m_pWorld; }

        inline DebugDrawContext GetDebugDrawContext()
        {
            return m_pWorld->GetDebugDrawSystem()->GetDebugDrawContext();
        }

        // Map
        //-------------------------------------------------------------------------

        void SetEditedMap( EntityMapID ID );
        EntityMap* GetEditedMap();
        EntityMap const* GetEditedMap() const { return const_cast<EditorContext*>( this )->GetEditedMap(); }

        // Entity Groups
        //-------------------------------------------------------------------------

        // Sets all the groups for a given map - Note: this is not an undoable action!
        void SetEntityGroupsForMap( EntityMapID const& mapID, TVector<EntityGroup> const& groups );

        // Sets all the groups for a given map - Note: this is not an undoable action!
        void SetGroupsForMap( EntityMapID const& mapID, TVector<EntityEditorItemGroup> const& groups );

        TVector<EntityGroup> GetEntityGroupsForMap( EntityMapID const& mapID ) const;

        TVector<EntityEditorItemGroup> const* GetGroupsForMap( EntityMapID const& mapID ) const;
        TVector<EntityEditorItemGroup> const* GetGroupsForMap( EntityMap const* pMap ) const { return GetGroupsForMap( pMap->GetID() ); }

        void CreateGroup( EntityMapID const& mapID, StringID groupID );
        void DestroyGroup( EntityMapID const& mapID, StringID groupID );
        EntityEditorItemGroup* GetEntityGroup( EntityMapID const& mapID, StringID groupID );
        EntityEditorItemGroup const* GetEntityGroup( EntityMapID const& mapID, StringID groupID ) const { return const_cast<EditorContext*>( this )->GetEntityGroup( mapID, groupID ); }
        bool HasEntityGroup( EntityMapID const& mapID, StringID groupID ) const { return GetEntityGroup( mapID, groupID ) != nullptr; }
        void RenameEntityGroup( EntityMapID const& mapID, StringID oldGroupID, StringID newGroupID );
        void ReparentGroup( EntityMapID const& mapID, StringID groupID, StringID newParentID );
        bool DoesGroupContainEntity( EntityMapID const& mapID, StringID groupID, Entity const* pEntity, bool recursiveSearch = true ) const;
        void GetAllEntitiesInGroup( EntityMapID const& mapID, StringID parentGroupID, TVector<Entity*>& outEntities, bool recurse = true ) const;
        void GetAllGroupsInGroup( EntityMapID const& mapID, StringID parentGroupID, TVector<StringID>& outGroups, bool recurse = true ) const;

        inline void AddOrMoveEntitiesToGroup( EntityMapID const& mapID, StringID groupID, TVector<Entity*> const& entities ) { AddOrMoveEntitiesToGroup( mapID, groupID, entities.data(), entities.size() ); }
        template<size_t S> void AddOrMoveEntitiesToGroup( EntityMapID const& mapID, StringID groupID, TInlineVector<Entity*, S>& entities ) { AddOrMoveEntitiesToGroup( mapID, groupID, entities.data(), entities.size() ); }
        inline void AddOrMoveEntitiesToGroup( EntityMapID const& mapID, StringID groupID, TVector<EntityEditorItem> const& entities ) { AddOrMoveEntitiesToGroup( mapID, groupID, entities.data(), entities.size() ); }
        template<size_t S> void AddOrMoveEntitiesToGroup( EntityMapID const& mapID, StringID groupID, TInlineVector<EntityEditorItem, S>& entities ) { AddOrMoveEntitiesToGroup( mapID, groupID, entities.data(), entities.size() ); }
        inline void AddOrMoveEntityToGroup( EntityMapID const& mapID, StringID groupID, Entity* pEntity ) { AddOrMoveEntitiesToGroup( mapID, groupID, &pEntity, 1 ); }
        inline void AddOrMoveEntityToGroup( EntityMapID const& mapID, StringID groupID, EntityEditorItem const& entityItem ) { AddOrMoveEntitiesToGroup( mapID, groupID, &entityItem, 1 ); }

        inline void RemoveEntitiesFromAllGroups( TVector<Entity*> const& entities ) { RemoveEntitiesFromAllGroups( entities.data(), entities.size()); }
        template<size_t S> void RemoveEntitiesFromAllGroups( TInlineVector<Entity*, S>& entities ) { RemoveEntitiesFromAllGroups(entities.data(), entities.size() ); }
        inline void RemoveEntitiesFromAllGroups( TVector<EntityEditorItem> const& entities ) { RemoveEntitiesFromAllGroups( entities.data(), entities.size() ); }
        template<size_t S> void RemoveEntitiesFromAllGroups( TInlineVector<EntityEditorItem, S>& entities ) { RemoveEntitiesFromAllGroups( entities.data(), entities.size() ); }
        inline void RemoveEntitiesFromAllGroups( Entity* pEntity ) { RemoveEntitiesFromAllGroups( &pEntity, 1 ); }
        inline void RemoveEntitiesFromAllGroups( EntityEditorItem const& entityItem ) { RemoveEntitiesFromAllGroups( &entityItem, 1 ); }

        void ShowEntityGroupRenameDialog( EntityMapID const& mapID, StringID groupID );
        void ShowEntityGroupCreateDialog( EntityMapID const& mapID, StringID baseGroupID = StringID() );

        // Entity
        //-------------------------------------------------------------------------

        Entity* CreateEntity( EntityMap* pMap, StringID groupID = StringID() );
        Entity* CreateEntity( EntityMapID const& mapID, StringID groupID = StringID() ) { return CreateEntity( m_pWorld->GetMap( mapID ), groupID ); }
        void AddEntityToMap( EntityMap* pMap, Entity* pEntity );
        void DestroyEntity( EntityMap* pMap, Entity* pEntity );
        void DestroyEntity( Entity* pEntity );
        void DestroyEntities( TVector<Entity*>& entitiesToDelete );
        void RenameEntity( Entity* pEntity, StringID newNameID );
        void ReparentEntity( Entity* pEntity, Entity* pNewParentEntity = nullptr );
        void DeleteEntities( TVector<Entity*>& entitiesToDelete );

        TVector<Entity*> DuplicateEntities( TVector<Entity*> const& entities );

        void ShowEntityRenameDialog( Entity* pEntity );

        // Components
        //-------------------------------------------------------------------------

        EntityComponent* CreateComponent( Entity* pEntity, TypeSystem::TypeID const componentTypeID, SpatialEntityComponent* pParentComponent = nullptr );
        inline EntityComponent* CreateComponent( Entity* pEntity, TypeSystem::TypeInfo const* pComponentTypeInfo, SpatialEntityComponent* pParentComponent = nullptr ) { return CreateComponent( pEntity, pComponentTypeInfo->m_ID, pParentComponent ); }
        template<typename T> EntityComponent* CreateComponent( Entity* pEntity, SpatialEntityComponent* pParentComponent = nullptr ) { return CreateComponent( pEntity, T::GetStaticTypeID(), pParentComponent ); }

        void DestroyComponent( Entity* pEntity, EntityComponent* pComponent );
        void RenameComponent( Entity* pEntity, EntityComponent* pComponent, StringID newNameID );
        void ReparentComponent( Entity* pEntity, SpatialEntityComponent* pChildComponent, SpatialEntityComponent* pParentComponent );
        void SwitchComponentsInHierarchy( Entity* pEntity, SpatialEntityComponent* pComponentA, SpatialEntityComponent* pComponentB );
        inline void MakeRootComponent( Entity* pEntity, SpatialEntityComponent* pComponent ) { ReparentComponent( pEntity, pComponent, nullptr ); }
        EntityComponent* MoveComponent( Entity* pEntity, EntityComponent* pComponent, Entity* pNewParentEntity, SpatialEntityComponent* pNewParentComponent = nullptr );

        void ShowComponentRenameDialog( Entity* pEntity, EntityComponent* pComponent );

        // Systems
        //-------------------------------------------------------------------------

        EntitySystem* CreateSystem( Entity* pEntity, TypeSystem::TypeID const systemTypeID );
        inline EntitySystem* CreateSystem( Entity* pEntity, TypeSystem::TypeInfo const* pSystemTypeInfo ) { return CreateSystem( pEntity, pSystemTypeInfo->m_ID ); }
        template<typename T> EntitySystem* CreateSystem( Entity* pEntity ) { return CreateSystem( pEntity, T::GetStaticTypeID() ); }
        void DestroySystem( Entity* pEntity, EntitySystem* pSystem );
        EntitySystem* MoveSystem( Entity* pEntity, EntitySystem* pSystem, Entity* pNewParentEntity );

        // Type Info
        //-------------------------------------------------------------------------

        TVector<TypeSystem::TypeInfo const*> const& GetSystemTypes() const { return m_systemTypes; }
        TVector<TypeSystem::TypeInfo const*> const& GetComponentTypes() const { return m_componentTypes; }
        TVector<TypeSystem::TypeInfo const*> const& GetSpatialComponentTypes() const { return m_spatialComponentTypes; }
        TVector<TypeSystem::TypeInfo const*> const& GetNonSpatialComponentTypes() const { return m_nonSpatialComponentTypes; }

        TVector<CategorizedEntityItemTypeInfo> const& GetCategorizedSystemTypes() const { return m_categorizedSystemTypes; }
        TVector<CategorizedEntityItemTypeInfo> const& GetCategorizedComponentTypes() const { return m_categorizedComponentTypes; }
        TVector<CategorizedEntityItemTypeInfo> const& GetCategorizedSpatialComponentTypes() const { return m_categorizedSpatialComponentTypes; }
        TVector<CategorizedEntityItemTypeInfo> const& GetCategorizedNonSpatialComponentTypes() const { return m_categorizedNonSpatialComponentTypes; }

        // Selected Entities
        //-------------------------------------------------------------------------

        inline TVector<EntityEditorItem> const& GetSelection() const { return m_selection; }
        void ClearSelection();

        inline bool IsSelected( EntityEditorItem const& data ) const { EE_ASSERT( data.IsEntity() ); return VectorContains( m_selection, data ); }
        inline bool IsSelected( Entity* pEntity ) const { return IsSelected( EntityEditorItem( pEntity ) ); }
        inline bool IsSelected( Entity* pEntity, EntityComponent* pComponent ) const { return IsSelected( EntityEditorItem( pEntity, pComponent ) ); }
        inline bool IsSelected( Entity* pEntity, EntitySystem* pSystem ) const { return IsSelected( EntityEditorItem( pEntity, pSystem ) ); }

        inline void SetSelection( EntityEditorItem const& data ) { SetSelection( &data, 1 ); }
        inline void SetSelection( Entity* pEntity ) { SetSelection( EntityEditorItem( pEntity ) ); }
        inline void SetSelection( Entity* pEntity, EntityComponent* pComponent ) { SetSelection( EntityEditorItem( pEntity, pComponent ) ); }
        inline void SetSelection( Entity* pEntity, EntitySystem* pSystem ) { SetSelection( EntityEditorItem( pEntity, pSystem ) ); }
        inline void SetSelection( TVector<EntityEditorItem> const& selection ) { SetSelection( selection.data(), selection.size() ); }
        template<size_t S> void SetSelection( TInlineVector<EntityEditorItem, S>& selection ) { SetSelection( selection.data(), selection.size() ); }

        inline void AddToSelection( EntityEditorItem const& data ) { EE_ASSERT( data.IsEntity() ); AddToSelection( &data, 1 ); }
        inline void AddToSelection( Entity* pEntity ) { AddToSelection( EntityEditorItem( pEntity ) ); }
        inline void AddToSelection( Entity* pEntity, EntityComponent* pComponent ) { AddToSelection( EntityEditorItem( pEntity, pComponent ) ); }
        inline void AddToSelection( Entity* pEntity, EntitySystem* pSystem ) { AddToSelection( EntityEditorItem( pEntity, pSystem ) ); }
        inline void AddToSelection( TVector<EntityEditorItem> const& selection ) { AddToSelection( selection.data(), selection.size() ); }
        template<size_t S> void AddToSelection( TInlineVector<EntityEditorItem, S>& selection ) { AddToSelection( selection.data(), selection.size() ); }

        void RemoveFromSelection( EntityEditorItem const& data );
        inline void RemoveFromSelection( Entity* pEntity ) { RemoveFromSelection( EntityEditorItem( pEntity ) ); }
        inline void RemoveFromSelection( Entity* pEntity, EntityComponent* pComponent ) { RemoveFromSelection( EntityEditorItem( pEntity, pComponent ) ); }
        inline void RemoveFromSelection( Entity* pEntity, EntitySystem* pSystem ) { RemoveFromSelection( EntityEditorItem( pEntity, pSystem ) ); }

        // Spatial Selection + Manipulation
        //-------------------------------------------------------------------------

        bool HasSpatialSelection() const { return !m_spatialSelection.empty(); }
        TVector<EntityEditorItem> const& GetSpatialSelection() const { return m_spatialSelection; }
        bool DoesSpatialSelectionSupportNonUniformScale() const;
        bool DoesSpatialSelectionContainComponents() const;
        void ClearSpatialSelection();

        TVector<OBB> const& GetSpatialSelectionBounds() const { return m_selectedItemBounds; }
        void GetSpatialSelectionWorldTransforms( TInlineVector<Transform, 10>& outTransforms ) const;
        OBB const& GetSpatialSelectionCombinedBounds() const { return m_selectionCombinedBounds; }
        Transform const GetSpatialSelectionTransform() const { return m_selectionTransform; }

        void BeginManipulatingSpatialSelection( ImGuiX::Gizmo::Result const& result, bool duplicateSelectionIfPossible );
        void ManipulateSpatialSelection( ImGuiX::Gizmo::Result const& result );
        void EndManipulatingSpatialSelection( ImGuiX::Gizmo::Result const& result );

        // Moves the current spatial selection to this new transform
        void PositionSpatialSelection( Transform const& newTransform );

        // Applies scale to the current spatial selection
        void ScaleSpatialSelection( Float3 const& scale );

        // Camera
        //-------------------------------------------------------------------------

        void FocusCamera( Entity* pEntity );
        void FocusCamera( SpatialEntityComponent* pComponent );
        void FocusCamera( TInlineVector<Entity*, 5> const& entities );
        void FocusCamera( TInlineVector<SpatialEntityComponent*, 5> const& components );

        // Copy/Paste/Duplication
        //-------------------------------------------------------------------------

        TVector<Entity*> CreateEntities( EntityMap* pMap, TVector<EntityDescriptor> const &entityDescs );

        // Modifications/Undo/Redo
        //-------------------------------------------------------------------------

        void PreUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction );
        void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction );

        void BeginCompoundUndoableAction() { m_pUndoStack->BeginCompoundAction(); }
        void EndCompoundUndoableAction() { m_pUndoStack->EndCompoundAction(); }

        void BeginEdit( TVector<EntityEditorItem>& itemsToBeModified );
        void BeginEdit( TVector<Entity*> entitiesToBeModified );
        void EndEdit();

        void BeginSingleComponentEdit( Entity* pEntity, EntityComponent* pComponent );
        void EndSingleComponentEdit( EntityComponent* pComponent );

        void BeginTransformEdit( TVector<EntityEditorItem>& itemsToBeModified );
        void BeginTransformEdit( TVector<Entity*>& entitiesToBeModified );
        void EndTransformEdit();

        // Events
        //-------------------------------------------------------------------------

        // Event fired after something has changed the world state (added/removed an entity/component/system)
        // Note: Not fired for transform changes
        inline TEventHandle<> OnPreWorldChange() { return m_preWorldChangeEvent; }

        // Event fired after something has changed the world state (added/removed an entity/component/system)
        // Note: Not fired for transform changes
        inline TEventHandle<> OnPostWorldChange() { return m_postWorldChangeEvent; }

        // Event fired when the selection changes
        inline TEventHandle<> OnSelectionChanged() { return m_selectionChangedEvent; }

        // Event fired when the spatial selection or spatial data changes
        inline TEventHandle<> OnSpatialSelectionChanged() { return m_spatialSelectionChangedEvent; }

    private:

        void FixUpSelectionPtrs();
        void SetSelection( EntityEditorItem const* pItemsToSelect, size_t numEntities );
        void AddToSelection( EntityEditorItem const* pItemsToSelect, size_t numEntities );
        void OnSelectionChangedInternal();

        void FixUpGroupPtrs();
        void AddOrMoveEntitiesToGroup( EntityMapID const& mapID, StringID groupID, Entity* const* ppEntities, size_t numEntities );
        void AddOrMoveEntitiesToGroup( EntityMapID const& mapID, StringID groupID, EntityEditorItem const* pEntityItems, size_t numItem );
        void RemoveEntitiesFromAllGroups( Entity* const* ppEntities, size_t numEntities );
        void RemoveEntitiesFromAllGroups( EntityEditorItem const* pEntityItems, size_t numEntities );

        THashMap<EntityMapID, TVector<EntityEditorItemGroup>> const& GetEntityGroups() const { return m_entityGroups; }
        void SetEntityGroups( THashMap<EntityMapID, TVector<EntityEditorItemGroup>> const& entityGroups );

        void UpdateSpatialSelectionData();
        void SetSpatialSelection( EntityEditorItem const* pItemsToSelect, size_t numItems );

        // Destroys a component and fixes up any spatial hierarchy
        // Note: Does NOT register an undo action
        void DestroyComponentInternal( Entity* pEntity, EntityComponent* pComponent );

        void SerializeEntities( TVector<Entity*> const& entities, TVector<EntityDescriptor>& outDescs );
        void SerializeComponents( TVector<EntityComponent*> const& components, TVector<ComponentDescriptor>& outDescs );

    public:

        ToolsContext const*                                                 m_pToolsContext = nullptr;

    private:

        UndoStack*                                                          m_pUndoStack = nullptr;
        DialogManager*                                                      m_pDialogManager = nullptr;
        IUndoableAction*                                                    m_pActiveUndoableAction = nullptr;
        EntityWorld*                                                        m_pWorld = nullptr;
        ToolsCameraComponent*                                               m_pCamera = nullptr;

        TVector<TypeSystem::TypeInfo const*>                                m_systemTypes;
        TVector<TypeSystem::TypeInfo const*>                                m_componentTypes;
        TVector<TypeSystem::TypeInfo const*>                                m_spatialComponentTypes;
        TVector<TypeSystem::TypeInfo const*>                                m_nonSpatialComponentTypes;
        TVector<CategorizedEntityItemTypeInfo>                              m_categorizedSystemTypes;
        TVector<CategorizedEntityItemTypeInfo>                              m_categorizedComponentTypes;
        TVector<CategorizedEntityItemTypeInfo>                              m_categorizedSpatialComponentTypes;
        TVector<CategorizedEntityItemTypeInfo>                              m_categorizedNonSpatialComponentTypes;

        TEvent<>                                                            m_preWorldChangeEvent;
        TEvent<>                                                            m_postWorldChangeEvent;
        TEvent<>                                                            m_selectionChangedEvent;
        TEvent<>                                                            m_spatialSelectionChangedEvent;

        TVector<EntityEditorItem>                                           m_selection;

        TVector<EntityEditorItem>                                           m_spatialSelection;
        TVector<OBB>                                                        m_selectedItemBounds;
        OBB                                                                 m_selectionCombinedBounds;
        Transform                                                           m_selectionTransform;
        TVector<Transform>                                                  m_selectionOffsetTransforms;
        bool                                                                m_isManipulatingSpatialSelection = false;
        bool                                                                m_spatialManipulationCreatedCompoundAction = false;

        EntityMapID                                                         m_editedMapID;
        Transform                                                           m_cameraStartTransform;
        THashMap<EntityMapID, TVector<EntityEditorItemGroup>>               m_entityGroups;
    };
}