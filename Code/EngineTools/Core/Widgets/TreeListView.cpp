#include "TreeListView.h"
#include "EASTL/sort.h"
#include "System/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool TreeListViewItem::Compare( TreeListViewItem const* pItem ) const
    {
        char const* const pNameA = GetNameID().c_str();
        size_t const lengthA = strlen( pNameA );
        char const* const pNameB = pItem->GetNameID().c_str();
        size_t const lengthB = strlen( pNameB );

        int32_t const result = String::comparei( pNameA, pNameA + lengthA, pNameB, pNameB + lengthB );
        return result < 0;
    }

    ImVec4 TreeListViewItem::GetDisplayColor( ItemState state ) const
    {
        ImVec4 color = (ImVec4) ImGuiX::Style::s_colorText;

        switch ( state )
        {
            case ItemState::Selected:
            {
                color = ImGuiX::Style::s_colorAccent1;
            }
            break;

            case ItemState::Active:
            {
                color = ImGuiX::Style::s_colorAccent0;
            }
            break;

            default:
            break;
        }

        return color;
    }

    void TreeListViewItem::DestroyChild( uint64_t uniqueItemID )
    {
        for ( int32_t i = 0; i < (int32_t) m_children.size(); i++ )
        {
            if ( m_children[i]->GetUniqueID() == uniqueItemID )
            {
                m_children[i]->DestroyChildren();
                EE::Delete( m_children[i] );
                m_children.erase( m_children.begin() + i );
                break;
            }
        }
    }

    void TreeListViewItem::DestroyChildren()
    {
        for ( auto& pChild : m_children )
        {
            pChild->DestroyChildren();
            EE::Delete( pChild );
        }

        m_children.clear();
    }

    TreeListViewItem const* TreeListViewItem::FindChild( uint64_t uniqueID ) const 
    {
        for ( auto pChild : m_children )
        {
            if ( pChild->GetUniqueID() == uniqueID )
            {
                return pChild;
            }
        }

        return nullptr;
    }

    TreeListViewItem* TreeListViewItem::FindChild( TFunction<bool( TreeListViewItem const* )> const& searchPredicate )
    {
        TreeListViewItem* pFoundItem = nullptr;
        auto itemIter = eastl::find_if( m_children.begin(), m_children.end(), searchPredicate );
        if ( itemIter != m_children.end() )
        {
            pFoundItem = *itemIter;
        }
        return pFoundItem;
    }

    TreeListViewItem* TreeListViewItem::SearchChildren( TFunction<bool( TreeListViewItem const* )> const& searchPredicate )
    {
        TreeListViewItem* pFoundItem = nullptr;

        // Search immediate children
        //-------------------------------------------------------------------------

        auto itemIter = eastl::find_if( m_children.begin(), m_children.end(), searchPredicate );
        if ( itemIter != m_children.end() )
        {
            pFoundItem = *itemIter;
            return pFoundItem;
        }

        // Recursively search each child
        //-------------------------------------------------------------------------

        for ( auto pChild : m_children )
        {
            pFoundItem = pChild->SearchChildren( searchPredicate );
            if ( pFoundItem != nullptr )
            {
                return pFoundItem;
            }
        }

        return pFoundItem;
    }

    void TreeListViewItem::UpdateVisibility( TFunction<bool( TreeListViewItem const* pItem )> const& isVisibleFunction, bool showParentItemsWithNoVisibleChildren )
    {
        if ( HasChildren() )
        {
            // Always update child visibility before our own - this allows clients to affect our visibility based on our children's visibility
            bool hasVisibleChild = false;
            for ( auto& pChild : m_children )
            {
                pChild->UpdateVisibility( isVisibleFunction );
                if ( pChild->IsVisible() )
                {
                    hasVisibleChild = true;
                }
            }

            // If we have a visible child, we are always visible
            if ( hasVisibleChild )
            {
                m_isVisible = true;
            }
            else // Optional: run visibility check if requested
            {
                m_isVisible = showParentItemsWithNoVisibleChildren ? isVisibleFunction( this ) : false;
            }
        }
        else // Leaf nodes always run visibility check
        {
            m_isVisible = isVisibleFunction( this );
        }
    }

    void TreeListViewItem::SortChildren()
    {
        eastl::sort( m_children.begin(), m_children.end(), [&] ( TreeListViewItem const* pItemA, TreeListViewItem const* pItemB ) { return pItemA->Compare( pItemB ); } );

        //-------------------------------------------------------------------------

        for ( auto pChild : m_children )
        {
            pChild->SortChildren();
        }
    }

    //-------------------------------------------------------------------------

    TreeListView::~TreeListView()
    {
        m_rootItem.DestroyChildren();
    }

    void TreeListView::SetViewToSelection()
    {
        // Expand all parents
        for ( auto pSelectedItem : m_selection )
        {
            auto pParentItem = pSelectedItem->m_pParent;
            while (pParentItem != nullptr)
            {
                pParentItem->SetExpanded( true );
                pParentItem = pParentItem->m_pParent;
            }
        }

        m_visualTreeState = VisualTreeState::NeedsRebuildAndFocusSelection;
    }

    TreeListViewItem* TreeListView::FindItem( uint64_t uniqueID )
    {
        if ( m_rootItem.GetUniqueID() == uniqueID )
        {
            return &m_rootItem;
        }

        auto SearchPredicate = [uniqueID] ( TreeListViewItem const* pItem )
        {
            return pItem->GetUniqueID() == uniqueID;
        };

        auto pFoundItem = m_rootItem.SearchChildren( SearchPredicate );
        return pFoundItem;
    }

    void TreeListView::DestroyItem( uint64_t uniqueID )
    {
        EE_ASSERT( m_rootItem.GetUniqueID() != uniqueID );

        // Remove from visual tree
        //-------------------------------------------------------------------------

        for ( auto i = 0; i < m_visualTree.size(); i++ )
        {
            if ( m_visualTree[i].m_pItem->GetUniqueID() == uniqueID )
            {
                m_visualTree.erase( m_visualTree.begin() + i );
                break;
            }
        }

        // Remove for regular tree
        //-------------------------------------------------------------------------

        auto SearchPredicate = [uniqueID] ( TreeListViewItem const* pItem )
        {
            return pItem->FindChild( uniqueID ) != nullptr;
        };

        auto pFoundParentItem = m_rootItem.SearchChildren( SearchPredicate );
        EE_ASSERT( pFoundParentItem != nullptr );
        pFoundParentItem->DestroyChild( uniqueID );
    }

    void TreeListView::RebuildTree( TreeListViewContext context, bool maintainSelection )
    {
        EE_ASSERT( !IsCurrentlyDrawingTree() );

        // Record current state
        //-------------------------------------------------------------------------

        if ( maintainSelection )
        {
            CacheSelectionAndExpansionState();
        }

        // Reset tree state
        //-------------------------------------------------------------------------

        DestroyTree();

        // Rebuild Tree
        //-------------------------------------------------------------------------

        EE_ASSERT( context.m_rebuildTreeFunction != nullptr );
        context.m_rebuildTreeFunction( &m_rootItem );

        // Sort Tree
        //-------------------------------------------------------------------------

        if ( m_flags.IsFlagSet( Flags::SortTree ) )
        {
            m_rootItem.SortChildren();
        }

        // Restore original state
        //-------------------------------------------------------------------------

        if ( maintainSelection )
        {
            RestoreCachedSelectionAndExpansionState();
        }

        m_visualTreeState = VisualTreeState::NeedsRebuild;
        RebuildVisualTree();

        if ( m_flags.IsFlagSet( ViewTracksSelection ) )
        {
            SetViewToSelection();
        }
    }

    void TreeListView::DestroyTree()
    {
        m_selection.clear();
        m_pActiveItem = nullptr;
        m_rootItem.DestroyChildren();
        m_rootItem.SetExpanded( true );
        m_visualTree.clear();
    }

    //-------------------------------------------------------------------------

    void TreeListView::CacheSelectionAndExpansionState()
    {
        m_cachedActiveItemID = 0;
        if ( m_pActiveItem != nullptr )
        {
            m_cachedActiveItemID = m_pActiveItem->GetUniqueID();
        }

        //-------------------------------------------------------------------------

        m_selectedItemIDs.clear();
        for ( auto pSelectedItem : m_selection )
        {
            m_selectedItemIDs.emplace_back( pSelectedItem->GetUniqueID() );
        }

        //-------------------------------------------------------------------------

        m_originalExpandedItems.clear();
        m_originalExpandedItems.reserve( GetNumItems() );

        auto RecordItemState = [this] ( TreeListViewItem const* pItem )
        {
            if ( pItem->IsExpanded() )
            {
                m_originalExpandedItems.emplace_back( pItem->GetUniqueID() );
            }
        };

        ForEachItemConst( RecordItemState );
    }

    void TreeListView::RestoreCachedSelectionAndExpansionState()
    {
        // Restore Item State
        //-------------------------------------------------------------------------

        auto RestoreItemState = [this] ( TreeListViewItem* pItem )
        {
            if ( VectorContains( m_originalExpandedItems, pItem->GetUniqueID() ) )
            {
                pItem->SetExpanded( true );
            }

            pItem->m_isSelected = false;
        };

        ForEachItem( RestoreItemState );

        // Restore active item
        //-------------------------------------------------------------------------

        auto FindActiveItem = [this] ( TreeListViewItem const* pItem ) { return pItem->GetUniqueID() == m_cachedActiveItemID; };
        m_pActiveItem = m_rootItem.SearchChildren( FindActiveItem );

        // Restore selection
        //-------------------------------------------------------------------------

        m_selection.clear();
        for ( auto selectedItemID : m_selectedItemIDs )
        {
            auto FindSelectedItem = [selectedItemID] ( TreeListViewItem const* pItem )
            {
                return pItem->GetUniqueID() == selectedItemID;
            };

            auto pPreviouslySelectedItem = m_rootItem.SearchChildren( FindSelectedItem );
            if ( pPreviouslySelectedItem != nullptr )
            {
                m_selection.emplace_back( pPreviouslySelectedItem );
                pPreviouslySelectedItem->m_isSelected = true;
            }
        }
    }

    //-------------------------------------------------------------------------

    void TreeListView::TryAddItemToVisualTree( TreeListViewItem* pItem, int32_t hierarchyLevel )
    {
        EE_ASSERT( pItem != nullptr );
        EE_ASSERT( hierarchyLevel >= 0 );

        //-------------------------------------------------------------------------

        if ( !pItem->IsVisible() )
        {
            return;
        }

        m_visualTree.emplace_back( pItem, hierarchyLevel );

        //-------------------------------------------------------------------------

        if ( pItem->IsExpanded() )
        {
            // If we want to sort branches vs leaves, we need to run a two pass update
            if ( m_flags.IsFlagSet( ShowBranchesFirst ) )
            {
                // Always add branch items first
                for ( auto& pChildItem : pItem->m_children )
                {
                    if ( !pChildItem->IsLeaf() )
                    {
                        TryAddItemToVisualTree( pChildItem, hierarchyLevel + 1 );
                    }
                }

                // Add leaf items last
                for ( auto& pChildItem : pItem->m_children )
                {
                    if ( pChildItem->IsLeaf() )
                    {
                        TryAddItemToVisualTree( pChildItem, hierarchyLevel + 1 );
                    }
                }
            }
            else // Maintain sorted order irrespective of branch/leaf status
            {
                for ( auto& pChildItem : pItem->m_children )
                {
                    TryAddItemToVisualTree( pChildItem, hierarchyLevel + 1 );
                }
            }
        }
    }

    void TreeListView::RebuildVisualTree()
    {
        EE_ASSERT( m_visualTreeState != VisualTreeState::UpToDate );

        //-------------------------------------------------------------------------

        m_visualTree.clear();

        // If we want to sort branches vs leaves, we need to run a two pass update
        if ( m_flags.IsFlagSet( ShowBranchesFirst ) )
        {
            // Always add branch items first
            for ( auto& pChildItem : m_rootItem.m_children )
            {
                if ( !pChildItem->IsLeaf() )
                {
                    TryAddItemToVisualTree( pChildItem, 0 );
                }
            }

            // Add leaf items last
            for ( auto& pChildItem : m_rootItem.m_children )
            {
                if ( pChildItem->IsLeaf() )
                {
                    TryAddItemToVisualTree( pChildItem, 0 );
                }
            }
        }
        else // Maintain sorted order irrespective of branch/leaf status
        {
            for ( auto& pChildItem : m_rootItem.m_children )
            {
                TryAddItemToVisualTree( pChildItem, 0 );
            }
        }

        m_estimatedRowHeight = -1.0f;
        m_estimatedTreeHeight = -1.0f;

        //-------------------------------------------------------------------------

        // Set view to selection
        if ( m_visualTreeState == VisualTreeState::NeedsRebuildAndFocusSelection )
        {
            for ( auto i = 0; i < m_visualTree.size(); i++ )
            {
                if ( m_visualTree[i].m_pItem->m_isSelected )
                {
                    m_firstVisibleRowItemIdx = i;
                    m_maintainVisibleRowIdx = true;
                    break;
                }
            }
        }

        m_visualTreeState = VisualTreeState::UpToDate;
    }

    int32_t TreeListView::GetVisualTreeItemIndex( TreeListViewItem const* pBaseItem ) const
    {
        int32_t const numItems = (int32_t) m_visualTree.size();
        for ( auto i = 0; i < numItems; i++ )
        {
            if ( m_visualTree[i].m_pItem == pBaseItem )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    int32_t TreeListView::GetVisualTreeItemIndex( uint64_t uniqueID ) const
    {
        int32_t const numItems = (int32_t) m_visualTree.size();
        for ( auto i = 0; i < numItems; i++ )
        {
            if ( m_visualTree[i].m_pItem->GetUniqueID() == uniqueID )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    //-------------------------------------------------------------------------
    
    void TreeListView::ClearSelection()
    {
        if ( !m_selection.empty() )
        {
            for ( auto pItem : m_selection )
            {
                pItem->m_isSelected = false;
                pItem->OnSelectionStateChanged();
            }

            m_selection.clear();
        }
    }

    void TreeListView::AddToSelectionInternal( TreeListViewItem* pItem, bool updateView )
    {
        EE_ASSERT( pItem != nullptr );

        if ( !VectorContains( m_selection, pItem ) )
        {
            if ( !m_flags.IsFlagSet( MultiSelectionAllowed ) )
            {
                ClearSelection();
            }

            pItem->m_isSelected = true;
            pItem->OnSelectionStateChanged();
            m_selection.emplace_back( pItem );

            if ( updateView )
            {
                SetViewToSelection();
            }
        }
    }

    void TreeListView::SetSelectionInternal( TreeListViewItem* pItem, bool updateView )
    {
        EE_ASSERT( pItem != nullptr );

        // Check if we need to actually modify the selection
        if ( m_selection.size() == 1 && m_selection[0] == pItem )
        {
            return;
        }

        //-------------------------------------------------------------------------

        ClearSelection();
        AddToSelectionInternal( pItem, updateView );
    }

    void TreeListView::AddToSelectionInternal( TVector<TreeListViewItem*> const& itemRange, bool updateView )
    {
        if ( itemRange.empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        for ( auto pItem : itemRange )
        {
            EE_ASSERT( pItem != nullptr );
            AddToSelectionInternal( pItem, false );
        }

        if ( updateView )
        {
            SetViewToSelection();
        }
    }

    void TreeListView::RemoveFromSelectionInternal( TreeListViewItem* pItem, bool updateView )
    {
        EE_ASSERT( m_flags.IsFlagSet( MultiSelectionAllowed ) );
        EE_ASSERT( pItem != nullptr );

        if ( VectorContains( m_selection, pItem ) )
        {
            m_selection.erase_first( pItem );
            pItem->m_isSelected = false;
            pItem->OnSelectionStateChanged();
        }

        if ( updateView )
        {
            SetViewToSelection();
        }
    }

    void TreeListView::SetSelectionInternal( TVector<TreeListViewItem*> const& itemRange, bool updateView )
    {
        bool shouldModifySelection = true;

        if ( itemRange.empty() )
        {
            ClearSelection();
            return;
        }

        //-------------------------------------------------------------------------

        // Deselect items not in the specified range
        for( int32_t i = (int32_t) m_selection.size() - 1; i >=0; i-- )
        {
            if ( VectorContains( itemRange, m_selection[i] ) )
            {
                RemoveFromSelectionInternal( m_selection[i], false );
            }
        }

        // Select new items
        for ( auto pItem : itemRange )
        {
            EE_ASSERT( pItem != nullptr );

            if ( !VectorContains( m_selection, pItem ) )
            {
                AddToSelectionInternal( pItem, false );
            }
        }

        // Update final view
        if ( updateView )
        {
            SetViewToSelection();
        }
    }

    void TreeListView::HandleItemSelection( TreeListViewItem* pItem )
    {
        EE_ASSERT( pItem != nullptr );
        bool const isSelected = pItem->m_isSelected;

        // Left click follows regular selection logic
        if ( ImGui::IsItemClicked( ImGuiMouseButton_Left ) )
        {
            if ( m_flags.IsFlagSet( MultiSelectionAllowed ) && ImGui::GetIO().KeyShift )
            {
                if ( m_selection.empty() )
                {
                    AddToSelectionInternal( pItem, false );
                }
                else
                {
                    // Get the index of the clicked item
                    int32_t const clickedItemIdx = GetVisualTreeItemIndex( pItem );

                    // Get the index of the selection range start item
                    TreeListViewItem const* const pLastSelectedItem = m_selection.back();
                    int32_t const lastSelectedItemIdx = GetVisualTreeItemIndex( pLastSelectedItem );

                    // Get the items we want to add to the selection
                    TVector<TreeListViewItem*> itemsToSelect;
                    itemsToSelect.reserve( 20 );

                    // Ensure that we always keep the last selected item as last item in the range (i.e. the last selected)
                    if ( lastSelectedItemIdx <= clickedItemIdx )
                    {
                        for ( auto i = clickedItemIdx; i >= lastSelectedItemIdx; i-- )
                        {
                            itemsToSelect.emplace_back( m_visualTree[i].m_pItem );
                        }
                    }
                    else // Selection is upwards in tree
                    {
                        for ( auto i = clickedItemIdx; i <= lastSelectedItemIdx; i++ )
                        {
                            itemsToSelect.emplace_back( m_visualTree[i].m_pItem );
                        }
                    }

                    // Modify the selection
                    SetSelectionInternal( itemsToSelect, false );
                }
            }
            else  if ( m_flags.IsFlagSet( MultiSelectionAllowed ) && ImGui::GetIO().KeyCtrl )
            {
                if ( isSelected )
                {
                    RemoveFromSelectionInternal( pItem, false );
                }
                else
                {
                    AddToSelectionInternal( pItem, false );
                }
            }
            else
            {
                SetSelectionInternal( pItem, false );
            }
        }
        // Right click never deselects! Nor does it support multi-selection
        else if ( ImGui::IsItemClicked( ImGuiMouseButton_Right ) )
        {
            if ( !isSelected )
            {
                SetSelectionInternal( pItem, false );
            }
        }
    }

    //-------------------------------------------------------------------------

    void TreeListView::DrawVisualItem( TreeListViewContext context, VisualTreeItem& visualTreeItem )
    {
        EE_ASSERT( visualTreeItem.m_pItem != nullptr && visualTreeItem.m_hierarchyLevel >= 0 );

        TreeListViewItem* const pItem = visualTreeItem.m_pItem;
        bool const isSelectedItem = VectorContains( m_selection, pItem );
        EE_ASSERT( isSelectedItem == pItem->m_isSelected );
        bool const isActiveItem = ( pItem == m_pActiveItem );

        ImGui::PushID( pItem );
        ImGui::TableNextRow();

        // Draw label column
        //-------------------------------------------------------------------------

        ImGui::TableSetColumnIndex( 0 );

        // Set tree indent level
        float const indentLevel = visualTreeItem.m_hierarchyLevel * ImGui::GetStyle().IndentSpacing;
        bool const requiresIndent = indentLevel > 0;
        if ( requiresIndent )
        {
            ImGui::Indent( indentLevel );
        }

        // Set node flags
        uint32_t treeNodeflags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding;

        if ( pItem->IsHeader() )
        {
            treeNodeflags |= ( ImGuiTreeNodeFlags_Framed );
        }

        if ( m_flags.IsFlagSet( ExpandItemsOnlyViaArrow ) )
        {
            treeNodeflags |= ImGuiTreeNodeFlags_OpenOnArrow;
        }

        if ( isSelectedItem )
        {
            treeNodeflags |= ImGuiTreeNodeFlags_Selected;
        }

        // Leaf nodes
        if ( pItem->IsLeaf() )
        {
            treeNodeflags |= ImGuiTreeNodeFlags_Leaf;

            if ( m_flags.IsFlagSet( ShowBulletsOnLeaves ) )
            {
                treeNodeflags |= ImGuiTreeNodeFlags_Bullet;
            }
        }
        else // Branch nodes
        {
            ImGui::SetNextItemOpen( pItem->IsExpanded() );
        }

        // Draw label
        String const displayName = pItem->GetDisplayName();
        bool newExpansionState = false;
        TreeListViewItem::ItemState const state = isActiveItem ? TreeListViewItem::Active : isSelectedItem ? TreeListViewItem::Selected : TreeListViewItem::None;
        
        {
            ImGuiX::Font const activeItemFont = m_flags.IsFlagSet( UseSmallFont ) ? ImGuiX::Font::SmallBold : ImGuiX::Font::MediumBold;
            ImGuiX::Font const inactiveItemFont = m_flags.IsFlagSet( UseSmallFont ) ? ImGuiX::Font::Small : ImGuiX::Font::Medium;

            ImGuiX::ScopedFont font( isActiveItem ? activeItemFont : inactiveItemFont, pItem->GetDisplayColor( state ) );
            newExpansionState = ImGui::TreeNodeEx( displayName.c_str(), treeNodeflags);
        }

        // Tooltip
        auto const pTooltipText = pItem->GetTooltipText();
        if ( pTooltipText != nullptr )
        {
            ImGuiX::ItemTooltip( pTooltipText );
        }

        // Drag and Drop
        if ( pItem->IsDragAndDropSource() )
        {
            if ( ImGui::BeginDragDropSource( ImGuiDragDropFlags_None ) )
            {
                pItem->SetDragAndDropPayloadData();
                ImGui::Text( displayName.c_str() );
                ImGui::EndDragDropSource();
            }
        }

        if ( pItem->IsDragAndDropTarget() )
        {
            if ( ImGui::BeginDragDropTarget() )
            {
                if ( context.m_onDragAndDropFunction != nullptr )
                {
                    context.m_onDragAndDropFunction( pItem );
                }

                ImGui::EndDragDropTarget();
            }
        }

        // Handle expansion
        if ( newExpansionState )
        {
            ImGui::TreePop();
        }

        if ( !pItem->IsLeaf() && pItem->IsExpanded() != newExpansionState )
        {
            pItem->SetExpanded( newExpansionState );
            m_visualTreeState = VisualTreeState::NeedsRebuild;
        }

        // Handle selection
        HandleItemSelection( pItem );

        // Context Menu
        if ( pItem->HasContextMenu() )
        {
            if ( ImGui::BeginPopupContextItem( "ItemContextMenu" ) )
            {
                if ( !isSelectedItem )
                {
                    if ( m_flags.IsFlagSet( MultiSelectionAllowed ) && ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl )
                    {
                        AddToSelectionInternal( pItem, false );
                    }
                    else // Switch selection to this item
                    {
                        SetSelectionInternal( pItem, false );
                    }
                }

                if ( context.m_drawItemContextMenuFunction != nullptr )
                {
                    TVector<TreeListViewItem*> selectedItemsWithContextMenus;
                    for ( auto pSelectedItem : m_selection )
                    {
                        if ( pSelectedItem->HasContextMenu() )
                        {
                            selectedItemsWithContextMenus.emplace_back( pSelectedItem );
                        }
                    }

                    context.m_drawItemContextMenuFunction( selectedItemsWithContextMenus );
                    ImGui::EndPopup();
                }
            }
        }

        // Handle double click
        if ( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( 0 ) )
        {
            if ( pItem->OnDoubleClick() )
            {
                m_visualTreeState = VisualTreeState::NeedsRebuild;
            }

            // Activation
            //-------------------------------------------------------------------------

            if ( pItem->IsActivatable() )
            {
                if ( m_pActiveItem == pItem )
                {
                    m_pActiveItem = nullptr;
                    m_pActiveItem->m_isActivated = false;
                }
                else
                {
                    m_pActiveItem = pItem;
                    m_pActiveItem->m_isActivated = true;
                }

                pItem->OnActivationStateChanged();
            }
        }

        // Pop indent level if set
        if ( requiresIndent )
        {
            ImGui::Unindent( indentLevel );
        }

        // Draw extra columns
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < context.m_numExtraColumns; i++ )
        {
            EE_ASSERT( context.m_drawItemExtraColumnsFunction != nullptr );
            ImGui::TableSetColumnIndex( i + 1 );
            context.m_drawItemExtraColumnsFunction( pItem, i );
        }

        //-------------------------------------------------------------------------

        ImGui::PopID();
    }

    bool TreeListView::UpdateAndDraw( TreeListViewContext context, float listHeight )
    {
        if ( m_visualTreeState != VisualTreeState::UpToDate )
        {
            RebuildVisualTree();
        }

        if ( m_visualTree.empty() )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        TVector<TreeListViewItem*> previousSelection = m_selection;

        //-------------------------------------------------------------------------

        ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY;

        if ( m_flags.IsFlagSet( DrawRowBackground ) )
        {
            tableFlags |= ImGuiTableFlags_RowBg;
        }

        if ( m_flags.IsFlagSet( DrawBorders ) )
        {
            tableFlags |= ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_BordersV;
        }

        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 2, 2 ) );
        if ( ImGui::BeginTable( "TreeViewTable", context.m_numExtraColumns + 1, tableFlags, ImVec2( ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x / 2, -1 ) ) )
        {
            ImGui::TableSetupColumn( "Label", ImGuiTableColumnFlags_WidthStretch );
            if ( context.m_setupExtraColumnHeadersFunction != nullptr )
            {
                context.m_setupExtraColumnHeadersFunction();
            }

            m_isDrawingTree = true;

            ImGuiListClipper clipper;
            clipper.Begin( (int32_t) m_visualTree.size() );

            // If we want to maintain the current visible set of data, update the scroll bar position to keep the same first visible item
            if ( m_maintainVisibleRowIdx && m_estimatedRowHeight > 0 )
            {
                ImGui::SetScrollY( m_firstVisibleRowItemIdx * m_estimatedRowHeight );
                m_maintainVisibleRowIdx = false;
            }

            // Draw clipped list
            while ( clipper.Step() )
            {
                for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i )
                {
                    float const cursorPosY = ImGui::GetCursorPosY();
                    DrawVisualItem( context, m_visualTree[i] );
                    m_estimatedRowHeight = ImGui::GetCursorPosY() - cursorPosY;
                }
            }
            m_isDrawingTree = false;

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        ImGui::PopID();

        // Handle input
        //-------------------------------------------------------------------------

        if ( ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_DockHierarchy ) )
        {
            bool rebuildVisualTree = false;
            if ( ImGui::IsKeyReleased( ImGuiKey_Enter ) )
            {
                for ( auto pItem : m_selection )
                {
                    rebuildVisualTree |= pItem->OnEnterPressed();
                }
            }

            if ( rebuildVisualTree )
            {
                m_visualTreeState = VisualTreeState::NeedsRebuild;
            }
        }

        // Check selection state
        //-------------------------------------------------------------------------

        if ( previousSelection.size() != m_selection.size() )
        {
            return true;
        }

        for ( int32_t i = 0; i < (int32_t) m_selection.size(); i++ )
        {
            if ( previousSelection[i] != m_selection[i] )
            {
                return true;
            }
        }

        return false;
    }
}