#pragma once
#include "EngineTools/Core/Widgets/TreeListView.h"
#include "EngineTools/Core/UndoStack.h"
#include "Engine/Entity/EntityIDs.h"
#include "System/Types/String.h"

//-------------------------------------------------------------------------

struct ImGuiWindowClass;

namespace EE
{
    class ToolsContext;
    class UpdateContext;
    class UndoStack;
    class EntityWorld;
    class Entity;
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityMap;
    class EntityEditorContext;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API EntityWorldOutliner final : public TreeListView
    {
        enum class Operation
        {
            None,
            Rename,
        };

    public:

        EntityWorldOutliner( ToolsContext const* pToolsContext, UndoStack* pUndoStack, EntityWorld* pWorld );

        void Initialize( UpdateContext const& context, uint32_t widgetUniqueID );
        void Shutdown( UpdateContext const& context );

        inline char const* GetWindowName() const { return m_windowName.c_str(); }

        void Reset() { m_requestTreeRebuild = true; }

        // Update and draw this widget - Returns true if it has focus
        bool UpdateAndDraw( UpdateContext const& context, ImGuiWindowClass* pWindowClass );

        // Selection
        //-------------------------------------------------------------------------

        // Get the currently selected entities
        TVector<Entity*> GetSelectedEntities() const;

        bool IsEntitySelected( Entity* pEntity ) const;

        void SetSelection( Entity* pEntity );
        void AddToSelection( Entity* pEntity );
        void RemoveFromSelection( Entity* pEntity );

        // World Operations
        //-------------------------------------------------------------------------

        void CreateEntity( EntityMapID const& mapID );
        void RenameEntity( Entity* pEntity, StringID newDesiredName );
        void DuplicateSelectedEntities();
        void DestroyEntity( Entity* pEntity );
        void DestroySelectedEntities();
        void SetEntityParent( Entity* pEntity, Entity* pNewParent );
        void ClearEntityParent( Entity* pEntity );

    private:

        void CreateEntityTreeItem( TreeListViewItem* pParentItem, Entity* pEntity );
        virtual void RebuildTreeUserFunction() override;
        virtual void HandleDragAndDropOnItem( TreeListViewItem* pDragAndDropTargetItem ) override;
        virtual void DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) override;

        void DrawDialogs();

        // Map operations
        //-------------------------------------------------------------------------
        
        // Temporary until we add multi-map editing
        EntityMap* GetEditedMap();

        EntityMap* GetMapForEntity( Entity const* pEntity ) const;

    private:

        ToolsContext const*                 m_pToolsContext = nullptr;
        UndoStack*                          m_pUndoStack = nullptr;
        EntityWorld*                        m_pWorld = nullptr;
        String                              m_windowName;
        Operation                           m_activeOperation = Operation::None;
        char                                m_renameBuffer[256];
        bool                                m_requestTreeRebuild = true;

        TVector<Entity*>                    m_entityDeletionRequests;       // All the entities requested for deletion in a frame, treated as a single operation
        TVector<Entity*>                    m_deferredSelectionRequests;    // Selection requests for entities that might be added to the map yet
    };
}