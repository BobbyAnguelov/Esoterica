#pragma once
#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/Widgets/TreeListView.h"
#include "EngineTools/Core/PropertyGrid/PropertyGrid.h"
#include "EngineTools/Core/UndoStack.h"
#include "Engine/Entity/EntityIDs.h"
#include "System/TypeSystem/TypeID.h"

//-------------------------------------------------------------------------

struct ImGuiWindowClass;

namespace EE
{
    class Entity;
    class EntityComponent;
    class SpatialEntityComponent;
    class EntitySystem;
    class EntityWorld;
    class UpdateContext;
    class UndoStack;

    namespace TypeSystem
    { 
        class TypeRegistry;
        class TypeInfo;
    }
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityMap;
    class EntityUndoableAction;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API EntityStructureEditor final : public TreeListView
    {
        enum class Operation
        {
            None = -1,
            AddSystem = 0,
            AddSpatialComponent,
            AddComponent,
            RenameComponent
        };

    public:

        EntityStructureEditor( ToolsContext const* pToolsContext, UndoStack* pUndoStack, EntityWorld* pWorld );
        ~EntityStructureEditor();

        void Initialize( UpdateContext const& context, uint32_t widgetUniqueID );
        void Shutdown( UpdateContext const& context );

        inline char const* GetWindowName() const { return m_windowName.c_str(); }

        // Update and draw the window - returns true if the window is focused
        bool UpdateAndDraw( UpdateContext const& context, ImGuiWindowClass* pWindowClass );

        // Set the entity we should be editing
        void SetEntityToEdit( Entity* pEntity, EntityComponent* pInitiallySelectedComponent = nullptr );

        // Set multiple entities to edit
        void SetEntitiesToEdit( TVector<Entity*> const& entities );

        // Get all the selected spatial components
        TVector<SpatialEntityComponent*> const& GetSelectedSpatialComponents() const { return m_selectedSpatialComponents; }

    private:

        void EntityUpdateDetected( Entity* pEntityChanged );
        void DrawDialogs();
        void ClearEditedEntityState();

        // Tree View
        //-------------------------------------------------------------------------

        virtual void RebuildTreeUserFunction() override;
        virtual void DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) override;
        virtual void HandleSelectionChanged( TreeListView::ChangeReason reason ) override;
        virtual void HandleDragAndDropOnItem( TreeListViewItem* pDragAndDropTargetItem ) override;

        // Entity Operations
        //-------------------------------------------------------------------------

        void StartOperation( Operation operation, EntityComponent* pTargetComponent = nullptr );

        void CreateSystem( TypeSystem::TypeInfo const* pSystemTypeInfo );
        void DestroySystem( EntitySystem* pSystem );
        void DestroySystem( TypeSystem::TypeID systemTypeID );

        void CreateComponent( TypeSystem::TypeInfo const* pComponentTypeInfo, ComponentID const& parentSpatialComponentID = ComponentID() );
        void DestroyComponent( EntityComponent* pComponent );

        void ReparentComponent( SpatialEntityComponent* pComponent, SpatialEntityComponent* pNewParentComponent );
        void MakeRootComponent( SpatialEntityComponent* pComponent );

        void RenameComponent( EntityComponent* pComponent, StringID newNameID );

    private:

        ToolsContext const*                             m_pToolsContext = nullptr;
        UndoStack*                                      m_pUndoStack = nullptr;
        EntityWorld*                                    m_pWorld = nullptr;
        EventBindingID                                  m_entityStateChangedBindingID;
        String                                          m_windowName;

        TVector<TypeSystem::TypeInfo const*>            m_allSystemTypes;
        TVector<TypeSystem::TypeInfo const*>            m_allComponentTypes;
        TVector<TypeSystem::TypeInfo const*>            m_allSpatialComponentTypes;

        // Edition and selection state
        TVector<EntityID>                               m_editedEntities;
        EntityID                                        m_editedEntityID;
        Entity*                                         m_pEditedEntity = nullptr;
        bool                                            m_shouldRefreshEditorState = false;
        StringID                                        m_initiallySelectedComponentNameID;
        TVector<SpatialEntityComponent*>                m_selectedSpatialComponents;
        TVector<EntityComponent*>                       m_selectedComponents;

        // Cached filtered lists of addable options

        // Operations
        Operation                                       m_activeOperation = Operation::None;
        EntityComponent*                                m_pOperationTargetComponent = nullptr;
        char                                            m_operationBuffer[256];
        TVector<TypeSystem::TypeInfo const*>            m_operationOptions;
        TVector<TypeSystem::TypeInfo const*>            m_filteredOptions;
        TypeSystem::TypeInfo const*                     m_pOperationSelectedOption = nullptr;
        bool                                            m_initializeFocus = false;
        EntityUndoableAction*                           m_pActiveUndoAction = nullptr;
        Entity*                                         m_pActiveUndoActionEntity = nullptr; // We need to cache the entity ptr, since the structure editor can have it edited entity changed before the active operation can complete
    };
}