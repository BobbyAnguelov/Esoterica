#pragma once

#include "EntityEditor_Context.h"
#include "Base/Imgui/ImguiCommandStack.h"
#include "EngineTools/Widgets/TreeListView.h"
#include "EngineTools/PropertyGrid/PropertyGrid.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class InspectorItem;

    //-------------------------------------------------------------------------

    class EntityInspector
    {
        enum class InternalState
        {
            UpToDate,
            NeedsRebuild,
        };

    public:

        EntityInspector( EditorContext& editorContext );
        ~EntityInspector();

        void UpdateAndDraw( UpdateContext const& context, bool isFocused );
        
        void SetReadOnly( bool isReadOnly ) { m_isReadOnly = isReadOnly; }

    private:

        void PreWorldChange();
        void PostWorldChange();

        // Selection
        //-------------------------------------------------------------------------

        void GetSplitSelection( TInlineVector<EntityEditorItem, 10>& outSelectedEntities, TInlineVector<EntityEditorItem, 10>& outSelectedComponentAndSystems ) const;
        void OnEditorSelectionChanged();
        void UpdateEditorSelectionFromTree();
        void ReflectSelectionToTree();

        // Tree
        //-------------------------------------------------------------------------

        void DrawTree( bool isFocused );
        void DrawExtraColumns( TreeListViewItem* pBaseItem, int32_t extraColumnIdx );
        void RebuildTree( TreeListViewItem* pRootItem );

        // Property Grid
        //-------------------------------------------------------------------------

        void DrawPropertyGrid( bool isFocused );
        void UpdatePropertyGridFromSelection();
        void PreEditProperty( PropertyEditInfo const& eventInfo );
        void PostEditProperty( PropertyEditInfo const& eventInfo );

        // Context Menu
        //-------------------------------------------------------------------------

        void DrawContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus );
        void DrawAddComponentOrSystemMenu( Entity* pEntity );

        void DrawAddSystemMenu( Entity* pEntity, TVector<TypeSystem::TypeInfo const*> const& systemTypes );
        void DrawAddComponentMenu( Entity* pEntity, TVector<TypeSystem::TypeInfo const*> const& componentTypes, SpatialEntityComponent* pParentComponent = nullptr );

        // Drag and Drop
        //-------------------------------------------------------------------------

        void HandleDragAndDrop( TreeListViewItem* pDropTarget );
        void ExecuteDragAndDrop( EntityEditorItem& draggedItem, EntityEditorItem& dropTargetItem );

        // Commands
        //-------------------------------------------------------------------------

        void DeleteSelectedItems();

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

        EntityEditorItem                                m_inspectedEntity;

        PropertyGrid                                    m_propertyGrid;
        PropertyGrid::VisualState                       m_propertyGridVisualState;
        EventBindingID                                  m_preEditPropertyBindingID;
        EventBindingID                                  m_postEditPropertyBindingID;

        ImGuiX::FilterWidget                            m_componentContextMenufilter;
        ImGuiX::FilterWidget                            m_systemContextMenufilter;
    };
}