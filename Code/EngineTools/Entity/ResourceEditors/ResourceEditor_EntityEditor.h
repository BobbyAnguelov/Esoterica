#pragma once

#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Widgets/TreeListView.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/Entity/Entity.h"
#include "Base/Imgui/ImguiGizmo.h"

//-------------------------------------------------------------------------
// Entity Editor
//-------------------------------------------------------------------------
// This is the base for all entity editors

namespace EE::EntityModel
{
    class EE_ENGINETOOLS_API EntityEditor : public EditorTool
    {
        friend class EntityUndoableAction;

    protected:

        enum class AddRequestType : uint8_t
        {
            Any,
            System,
            SpatialComponent,
            Component,
        };

        struct ReparentRequest
        {
            ReparentRequest( Entity* pEntity, Entity* pNewParent )
                : m_pEntity( pEntity )
                , m_pNewParent( pNewParent )
            {
                EE_ASSERT( m_pEntity != nullptr && m_pEntity->IsSpatialEntity() );

                if ( m_pNewParent != nullptr )
                {
                    EE_ASSERT( m_pNewParent->IsSpatialEntity() );
                }
            }

            Entity* m_pEntity = nullptr;
            Entity* m_pNewParent = nullptr;
        };

        struct SelectionSwitchRequest
        {
            inline bool IsValid() const { return !m_selectedEntities.empty(); }
            inline void Clear() { m_selectedEntities.clear(); m_selectedComponents.clear(); }

            TVector<EntityID>       m_selectedEntities;
            TVector<ComponentID>    m_selectedComponents;
        };

        enum class TreeState
        {
            UpToDate,
            NeedsRebuild,
            NeedsRebuildAndMaintainSelection
        };

    public:

        explicit EntityEditor( ToolsContext const* pToolsContext, String const& displayName, EntityWorld* pWorld );
        ~EntityEditor();

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;

    protected:

        virtual char const* GetUniqueTypeName() const override { return "Entity Editor"; }
        virtual bool ShouldLoadDefaultEditorMap() const override { return false; }
        virtual bool SupportsViewport() const override { return true; }
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;
        virtual void DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual bool SupportsSaving() const override { return true; }
        virtual bool AlwaysAllowSaving() const override { return true; }
        virtual void OnMousePick( Render::PickingID pickingID ) override;
        virtual void DropResourceInViewport( ResourceID const& resourceID, Vector const& worldPosition ) override;

        virtual void PreUndoRedo( UndoStack::Operation operation ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;

        // Operations/Requests
        //-------------------------------------------------------------------------
        // These start or request actions that are deferred to a subsequent update or require addition user input

        void RequestDestroyEntity( Entity* pEntity );

        void RequestDestroyEntities( TVector<Entity*> entities );

        void RequestEntityReparent( Entity* pEntity, Entity* pNewParentEntity );

        void StartAddComponentOrSystem( AddRequestType addRequestType, EntityComponent* pTargetComponent = nullptr );

        // Commands
        //-------------------------------------------------------------------------
        // Most of these operations cause an outliner rebuild so they shouldn't be called during outliner drawing
        // TODO: instead of a full tree rebuild, change these functions to modify the outliner tree in place and only rebuild the visual tree

        void CreateEntity( EntityMapID const& mapID );
        void RenameEntity( Entity* pEntity, StringID newDesiredName );
        TVector<Entity*> DuplicateSelectedEntities();
        void SetEntityParent( Entity* pEntity, Entity* pNewParent );
        void ClearEntityParent( Entity* pEntity );

        void CreateSystem( TypeSystem::TypeInfo const* pSystemTypeInfo );
        void DestroySystem( EntitySystem* pSystem );
        void DestroySystem( TypeSystem::TypeID systemTypeID );

        void CreateComponent( TypeSystem::TypeInfo const* pComponentTypeInfo, ComponentID const& parentSpatialComponentID = ComponentID() );
        void DestroyComponent( EntityComponent* pComponent );

        void ReparentComponent( SpatialEntityComponent* pComponent, SpatialEntityComponent* pNewParentComponent );
        void MakeRootComponent( SpatialEntityComponent* pComponent );

        void RenameComponent( EntityComponent* pComponent, StringID newNameID );

        // Events
        //-------------------------------------------------------------------------

        void UpdateSelectionSpatialInfo();

        // Outliner
        //-------------------------------------------------------------------------

        EntityMap* GetEditedMap(); // Temporary until we add multi-map editing
        TVector<Entity*> GetSelectedEntities() const;
        TVector<Entity*> GetSelectedSpatialEntities() const;
        void DrawOutliner( UpdateContext const& context, bool isFocused );
        void RebuildOutlinerTree( TreeListViewItem* pRootItem );
        void DrawOutlinerContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus );
        void HandleOutlinerDragAndDrop( TreeListViewItem* pDropTarget );

        // Entity Structure Editor
        //-------------------------------------------------------------------------

        Entity* GetEditedEntity() const;
        void DrawStructureEditor( UpdateContext const& context, bool isFocused );
        void RebuildStructureEditorTree( TreeListViewItem* pRootItem );
        void DrawStructureEditorContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus );
        void HandleStructureEditorDragAndDrop( TreeListViewItem* pDropTarget );
        TVector<SpatialEntityComponent*> GetSelectedSpatialComponents() const;

        // Property Grid
        //-------------------------------------------------------------------------

        void DrawPropertyGrid( UpdateContext const& context, bool isFocused );
        void PreEditProperty( PropertyEditInfo const& eventInfo );
        void PostEditProperty( PropertyEditInfo const& eventInfo );

        // Dialogs
        //-------------------------------------------------------------------------

        bool DrawRenameEntityDialog( UpdateContext const& context );
        bool DrawRenameComponentDialog( UpdateContext const& context );
        bool DrawAddComponentOrSystemDialog( UpdateContext const& context );

        // Transform
        //-------------------------------------------------------------------------

        void BeginTransformManipulation( Transform const& newTransform, bool duplicateSelection = false );
        void ApplyTransformManipulation( Transform const& newTransform );
        void EndTransformManipulation( Transform const& newTransform );

    protected:

        // All the entities requested for deletion in a frame, treated as a single operation
        TVector<Entity*>                                m_entityDeletionRequests;

        // Spatial parenting requests from drag and drop need to be deferred since this occurs while we draw the tree which means we cant rebuild the tree
        TVector<ReparentRequest>                        m_spatialParentingRequests;

        // Selection
        //-------------------------------------------------------------------------

        SelectionSwitchRequest                          m_selectionSwitchRequest;

        // Actions
        //-------------------------------------------------------------------------

        EntityComponent*                                m_pActionTargetComponent = nullptr;
        ImGuiX::FilterWidget                            m_operationFilterWidget;
        char                                            m_operationBuffer[256];
        TVector<TypeSystem::TypeInfo const*>            m_operationOptions;
        TVector<TypeSystem::TypeInfo const*>            m_filteredOptions;
        TypeSystem::TypeInfo const*                     m_pOperationSelectedOption = nullptr;
        bool                                            m_initializeFocus = false;

        // Transform manipulation
        //-------------------------------------------------------------------------

        ImGuiX::Gizmo                                   m_gizmo;
        OBB                                             m_selectionBounds;
        Transform                                       m_selectionTransform;
        TVector<Transform>                              m_selectionOffsetTransforms;
        TVector<Entity*>                                m_manipulatedSpatialEntities;
        TVector<SpatialEntityComponent*>                m_manipulatedSpatialComponents;
        bool                                            m_hasSpatialSelection = false;

        // Outliner
        //-------------------------------------------------------------------------

        TreeListView                                    m_outlinerTreeView;
        TreeListViewContext                             m_outlinerContext;
        TreeState                                       m_outlinerState = TreeState::NeedsRebuild;

        // Structure editor
        //-------------------------------------------------------------------------

        TreeListView                                    m_structureEditorTreeView;
        TreeListViewContext                             m_structureEditorContext;
        EntityID                                        m_editedEntityID;
        ComponentID                                     m_editedComponentID;
        TVector<TypeSystem::TypeInfo const*>            m_allSystemTypes;
        TVector<TypeSystem::TypeInfo const*>            m_allComponentTypes;
        TVector<TypeSystem::TypeInfo const*>            m_allSpatialComponentTypes;
        TreeState                                       m_structureEditorState = TreeState::NeedsRebuild;

        // Property Grid
        //-------------------------------------------------------------------------

        PropertyGrid                                    m_propertyGrid;
        PropertyGrid::VisualState                       m_propertyGridVisualState;
        EventBindingID                                  m_preEditPropertyBindingID;
        EventBindingID                                  m_postEditPropertyBindingID;

        // Visualization
        //-------------------------------------------------------------------------

        TVector<TypeSystem::TypeInfo const*>            m_volumeTypes;
        TVector<TypeSystem::TypeInfo const*>            m_visualizedVolumeTypes;
    };
}