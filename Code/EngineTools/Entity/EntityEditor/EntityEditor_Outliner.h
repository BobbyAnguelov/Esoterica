#pragma once

#include "EntityEditor_Context.h"
#include "EngineTools/Widgets/TreeListView.h"
#include "EngineTools/PropertyGrid/PropertyGrid.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Imgui/ImguiCommandStack.h"
#include "Base/Utils/CategoryTree.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class OutlinerItem;
    class MapItem;
    class GroupItem;
    class EntityItem;

    //-------------------------------------------------------------------------

    class EntityOutliner
    {
    public:

        enum class InternalState
        {
            UpToDate,
            NeedsRebuild,
            NeedsRebuildAndSelectionUpdate
        };

    public:

        explicit EntityOutliner( EditorContext& editorContext );
        ~EntityOutliner();

        void SetReadOnly( bool isReadOnly ) { m_isReadOnly = isReadOnly; }

        void UpdateAndDraw( UpdateContext const& context, bool isFocused );

    private:

        void PreWorldChange();
        void PostWorldChange();

        // Tree
        //-------------------------------------------------------------------------

        void DrawTree( bool isFocused );
        void DrawExtraColumns( TreeListViewItem* pBaseItem, int32_t extraColumnIdx );
        void RebuildOutlinerTree( TreeListViewItem* pRootItem );
        void CreateEntityItemRecursive( TreeListViewItem* pParentItem, Entity* pEntity );
        void CreateEntityGroupRecursive( EntityMap* pMap, TreeListViewItem* pParentItem, Category<EntityItem*> const& category );

        // Selection
        //-------------------------------------------------------------------------

        void OnSelectionChanged();
        void UpdateSelection();
        void SplitSelection( TInlineVector<EntityItem*, 20>& outSelectedEntities, TInlineVector<GroupItem*, 20>& outSelectedGroups );

        // Drag and Drop
        //-------------------------------------------------------------------------

        void HandleDragAndDrop( TreeListViewItem* pDropTarget );
        void ExecuteDragAndDrop( OutlinerItem* pTargetItem );
        void DropOnMap( MapItem* pDropTarget, TInlineVector<EntityItem*, 20> const& draggedEntities, TInlineVector<GroupItem*, 20>const& draggedGroups );
        void DropOnGroup( GroupItem* pDropTarget, TInlineVector<EntityItem*, 20> const& draggedEntities, TInlineVector<GroupItem*, 20>const& draggedGroups );
        void DropOnEntity( EntityItem* pDropTarget, TInlineVector<EntityItem*, 20> const& draggedEntities );

        // Context Menus
        //-------------------------------------------------------------------------

        void DrawContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus );
    
        // Input Handling
        //-------------------------------------------------------------------------

        void HandleInput();

        // Commands
        //-------------------------------------------------------------------------

        void DeleteSelected();
        void CopySelected();
        void Paste();

    private:

        bool                                            m_isReadOnly = false;

        EditorContext&                                  m_editorContext;
        EventBindingID                                  m_preWorldChangedEventID;
        EventBindingID                                  m_postWorldChangedEventID;
        EventBindingID                                  m_selectionChangedEventID;
        InternalState                                   m_state = InternalState::NeedsRebuild;

        ImGuiX::CommandStack                            m_commandStack;

        ImGuiX::FilterWidget                            m_filter;
        TreeListView                                    m_treeView;
        TreeListViewContext                             m_treeContext;
    };
}