#include "TreeListView.h"
#include "System/Imgui/ImguiStyle.h"
#include "EASTL/sort.h"
#include "System/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE
{
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

    //-------------------------------------------------------------------------

    TreeListView::~TreeListView()
    {
        m_rootItem.DestroyChildren();
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

    void TreeListView::SortItemChildren( TreeListViewItem* pItem )
    {
        EE_ASSERT( pItem != nullptr );

        eastl::sort( pItem->m_children.begin(), pItem->m_children.end(), [this] ( TreeListViewItem const* pItemA, TreeListViewItem const* pItemB ) { return ItemSortComparator( pItemA, pItemB ); } );

        //-------------------------------------------------------------------------

        for ( auto pChild : pItem->m_children )
        {
            SortItemChildren( pChild );
        }
    }

    bool TreeListView::ItemSortComparator( TreeListViewItem const* pItemA, TreeListViewItem const* pItemB ) const
    {
        char const* const pNameA = pItemA->GetNameID().c_str();
        size_t const lengthA = strlen( pNameA );
        char const* const pNameB = pItemB->GetNameID().c_str();
        size_t const lengthB = strlen( pNameB );

        int32_t const result = String::comparei( pNameA, pNameA + lengthA, pNameB, pNameB + lengthB );
        return result < 0;
    }

    void TreeListView::RebuildTree( bool maintainExpansionAndSelection )
    {
        // Record current state
        //-------------------------------------------------------------------------

        if ( maintainExpansionAndSelection )
        {
            CacheSelectionAndExpansionState();
        }

        // Reset tree state
        //-------------------------------------------------------------------------

        DestroyTree();

        // Rebuild Tree
        //-------------------------------------------------------------------------

        RebuildTreeUserFunction();

        // Sort Tree
        //-------------------------------------------------------------------------

        if ( ShouldSortTree() )
        {
            SortItemChildren( &m_rootItem );
        }

        // Restore original state
        //-------------------------------------------------------------------------

        if ( maintainExpansionAndSelection )
        {
            RestoreCachedSelectionAndExpansionState();
        }

        RefreshVisualState();

        // Always notify selection change on rebuild
        NotifyActiveItemChanged( ChangeReason::TreeRebuild );
        NotifySelectionChanged( ChangeReason::TreeRebuild );
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
        // Restore Expansion
        //-------------------------------------------------------------------------

        auto RestoreItemState = [this] ( TreeListViewItem* pItem )
        {
            if ( VectorContains( m_originalExpandedItems, pItem->GetUniqueID() ) )
            {
                pItem->SetExpanded( true );
            }
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
            if ( m_prioritizeBranchesOverLeavesInVisualTree )
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
        if ( m_prioritizeBranchesOverLeavesInVisualTree )
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

        // Reset view
        if ( m_visualTreeState == VisualTreeState::NeedsRebuildAndViewReset )
        {
            m_firstVisibleRowItemIdx = 0;
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

    void TreeListView::OnItemDoubleClickedInternal( TreeListViewItem* pItem )
    {
        // Double click
        //-------------------------------------------------------------------------

        m_onItemDoubleClicked.Execute( pItem );

        // Activation
        //-------------------------------------------------------------------------

        if ( !pItem->IsActivatable() )
        {
            return;
        }

        if ( m_pActiveItem == pItem )
        {
            m_pActiveItem = nullptr;
        }
        else
        {
            m_pActiveItem = pItem;
        }

        NotifySelectionChanged( ChangeReason::UserInput );
    }

    //-------------------------------------------------------------------------
    
    void TreeListView::ClearSelection()
    {
        if ( !m_selection.empty() )
        {
            m_selection.clear();
            NotifySelectionChanged( ChangeReason::CodeRequest );
        }
    }

    bool TreeListView::IsItemSelected( TreeListViewItem const* pItem ) const
    {
        EE_ASSERT( pItem != nullptr );
        for ( auto pSelectedItem : m_selection )
        {
            if ( pSelectedItem == pItem )
            {
                return true;
            }
        }

        return false;
    }

    void TreeListView::SetSelectionInternal( TreeListViewItem* pItem, TreeListView::ChangeReason reason )
    {
        EE_ASSERT( pItem != nullptr );

        // Check if we need to actually modify the selection
        if ( m_selection.size() == 1 && m_selection[0] == pItem )
        {
            return;
        }

        //-------------------------------------------------------------------------

        m_selection.clear();
        m_selection.emplace_back( pItem );
        NotifySelectionChanged( reason );
    }

    void TreeListView::AddToSelectionInternal( TreeListViewItem* pItem, TreeListView::ChangeReason reason )
    {
        EE_ASSERT( m_multiSelectionAllowed );
        EE_ASSERT( pItem != nullptr );

        if ( !VectorContains( m_selection, pItem ) )
        {
            m_selection.emplace_back( pItem );
            NotifySelectionChanged( reason );
        }
    }

    void TreeListView::AddToSelectionInternal( TVector<TreeListViewItem*> const& itemRange, TreeListView::ChangeReason reason )
    {
        if ( itemRange.empty() )
        {
            return;
        }

        for ( auto pItem : itemRange )
        {
            EE_ASSERT( pItem != nullptr );
        }

        //-------------------------------------------------------------------------

        bool selectionModified = false;
        for ( auto pItem : itemRange )
        {
            if ( !VectorContains( m_selection, pItem ) )
            {
                m_selection.emplace_back( pItem );
                selectionModified = true;
            }
        }

        if ( selectionModified )
        {
            NotifySelectionChanged( reason );
        }
    }

    void TreeListView::SetSelectionInternal( TVector<TreeListViewItem*> const& itemRange, TreeListView::ChangeReason reason )
    {
        bool shouldModifySelection = true;

        if ( itemRange.empty() )
        {
            ClearSelection();
        }

        for ( auto pItem : itemRange )
        {
            EE_ASSERT( pItem != nullptr );
        }

        //-------------------------------------------------------------------------

        if ( itemRange.size() == m_selection.size() )
        {
            shouldModifySelection = false;
            for ( auto pItem : itemRange )
            {
                if ( !VectorContains( m_selection, pItem ) )
                {
                    shouldModifySelection = true;
                    break;
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( shouldModifySelection )
        {
            m_selection = itemRange;
            NotifySelectionChanged( reason );
        }
    }

    void TreeListView::RemoveFromSelectionInternal( TreeListViewItem* pItem, TreeListView::ChangeReason reason )
    {
        EE_ASSERT( m_multiSelectionAllowed );
        EE_ASSERT( pItem != nullptr );

        if ( VectorContains( m_selection, pItem ) )
        {
            m_selection.erase_first( pItem );
            NotifySelectionChanged( reason );
        }
    }

    void TreeListView::HandleItemSelection( TreeListViewItem* pItem, bool isSelectedItem )
    {
        EE_ASSERT( pItem != nullptr );

        // Left click follows regular selection logic
        if ( ImGui::IsItemClicked( ImGuiMouseButton_Left ) )
        {
            if ( m_multiSelectionAllowed && ImGui::GetIO().KeyShift )
            {
                if ( m_selection.empty() )
                {
                    AddToSelectionInternal( pItem, ChangeReason::UserInput );
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
                    SetSelectionInternal( itemsToSelect, ChangeReason::UserInput );
                }
            }
            else  if ( m_multiSelectionAllowed && ImGui::GetIO().KeyCtrl )
            {
                if ( isSelectedItem )
                {
                    RemoveFromSelectionInternal( pItem, ChangeReason::UserInput );
                }
                else
                {
                    AddToSelectionInternal( pItem, ChangeReason::UserInput );
                }
            }
            else
            {
                SetSelectionInternal( pItem, ChangeReason::UserInput );
            }
        }
        // Right click never deselects! Nor does it support multi-selection
        else if ( ImGui::IsItemClicked( ImGuiMouseButton_Right ) )
        {
            if ( !isSelectedItem )
            {
                SetSelectionInternal( pItem, ChangeReason::UserInput );
            }
        }
        else if ( ImGui::IsItemFocused() )
        {
            if ( !isSelectedItem )
            {
                SetSelectionInternal( pItem, ChangeReason::UserInput );
            }
        }
    }

    //-------------------------------------------------------------------------

    void TreeListView::DrawVisualItem( VisualTreeItem& visualTreeItem )
    {
        EE_ASSERT( visualTreeItem.m_pItem != nullptr && visualTreeItem.m_hierarchyLevel >= 0 );

        TreeListViewItem* const pItem = visualTreeItem.m_pItem;
        bool const isSelectedItem = VectorContains( m_selection, pItem );
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

        if ( m_expandItemsOnlyViaArrow )
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

            if ( m_showBulletsOnLeaves )
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
            ImGuiX::Font const activeItemFont = m_useSmallFont ? ImGuiX::Font::SmallBold : ImGuiX::Font::MediumBold;
            ImGuiX::Font const inactiveItemFont = m_useSmallFont ? ImGuiX::Font::Small : ImGuiX::Font::Medium;

            ImGuiX::ScopedFont font( isActiveItem ? activeItemFont : inactiveItemFont, pItem->GetDisplayColor( state ) );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 0, 4 ) );
            newExpansionState = ImGui::TreeNodeEx( displayName.c_str(), treeNodeflags);
            ImGui::PopStyleVar();
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
                HandleDragAndDropOnItem( pItem );
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
        HandleItemSelection( pItem, isSelectedItem );

        // Context Menu
        if ( pItem->HasContextMenu() )
        {
            if ( ImGui::BeginPopupContextItem( "ItemContextMenu" ) )
            {
                // is this needed?
                if ( !isSelectedItem )
                {
                    if ( m_multiSelectionAllowed && ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl )
                    {
                        AddToSelectionInternal( pItem, ChangeReason::UserInput );
                    }
                    else // Switch selection to this item
                    {
                        SetSelectionInternal( pItem, ChangeReason::UserInput );
                    }
                }

                TVector<TreeListViewItem*> selectedItemsWithContextMenus;
                for ( auto pSelectedItem : m_selection )
                {
                    if ( pSelectedItem->HasContextMenu() )
                    {
                        selectedItemsWithContextMenus.emplace_back( pSelectedItem );
                    }
                }

                ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( ImGui::GetStyle().ItemSpacing.x, ImGui::GetStyle().ItemSpacing.x ) ); // Undo the table item spacing changes
                DrawItemContextMenu( selectedItemsWithContextMenus );
                ImGui::PopStyleVar();

                ImGui::EndPopup();
            }
        }

        // Handle double click
        if ( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( 0 ) )
        {
            OnItemDoubleClickedInternal( pItem );
        }

        // Pop indent level if set
        if ( requiresIndent )
        {
            ImGui::Unindent( indentLevel );
        }

        // Draw extra columns
        //-------------------------------------------------------------------------

        for ( uint32_t i = 0u; i < GetNumExtraColumns(); i++ )
        {
            ImGui::TableSetColumnIndex( i + 1 );
            DrawItemExtraColumns( pItem, i );
        }

        //-------------------------------------------------------------------------

        ImGui::PopID();
    }

    void TreeListView::UpdateAndDraw( float listHeight )
    {
        if ( m_visualTreeState != VisualTreeState::UpToDate )
        {
            RebuildVisualTree();
        }

        if ( m_visualTree.empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        ImGuiTableFlags tableFlags = ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_Resizable;

        if ( m_drawRowBackground )
        {
            tableFlags |= ImGuiTableFlags_RowBg;
        }

        if ( m_drawBorders )
        {
            tableFlags |= ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_BordersV;
        }

        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( ImGui::GetStyle().ItemSpacing.x, 0 ) ); // Ensure table border and scrollbar align
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 0, 0 ) );
        ImGui::PushStyleColor( ImGuiCol_Header, ImGuiX::Style::s_colorGray2.Value ); // Why does 'header' control selected table row BG?!
        if( ImGui::BeginChild( "TVC", ImVec2( -1, listHeight ), false ) )
        {
            float const totalVerticalSpaceAvailable = ImGui::GetContentRegionAvail().y;
            float const maxVerticalScrollPosition = ImGui::GetScrollMaxY();

            //-------------------------------------------------------------------------

            // Calculate the required height in pixels for all rows based on the height of the first row. 
            // We only render a single row here (so we dont bust the imgui index buffer limits for a single tree) - At low frame rates this will flicker
            if ( m_estimatedTreeHeight < 0 )
            {
                if ( ImGui::BeginTable( "TreeViewTable", GetNumExtraColumns() + 1, tableFlags, ImVec2( ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x / 2, -1 ) ) )
                {
                    ImGui::TableSetupColumn( "Label", ImGuiTableColumnFlags_NoHide );
                    SetupExtraColumnHeaders();

                    ImVec2 const prevCursorPos = ImGui::GetCursorPos();
                    DrawVisualItem( m_visualTree[0] );
                    ImGui::TableNextRow();
                    ImVec2 const cursorPos = ImGui::GetCursorPos();
                    m_estimatedRowHeight = cursorPos.y - prevCursorPos.y;

                    ImGui::EndTable();
                }

                // Draw a dummy to fake the real size so as to not completely reset scrollbar position
                //-------------------------------------------------------------------------

                m_estimatedTreeHeight = float( m_visualTree.size() ) * m_estimatedRowHeight;
                ImGui::Dummy( ImVec2( -1, m_estimatedTreeHeight - ImGui::GetCursorPos().y ) );

                // Update scrollbar position
                //-------------------------------------------------------------------------

                m_firstVisibleRowItemIdx = Math::Clamp( m_firstVisibleRowItemIdx, 0, int32_t( m_visualTree.size() - 1 ) );
                m_maintainVisibleRowIdx = true;
            }
            else // Draw clipped table
            {
                int32_t const maxNumDrawableRows = (int32_t) Math::Floor( totalVerticalSpaceAvailable / m_estimatedRowHeight );
                float const numRowIndices = float( m_visualTree.size() ) - 1;
                float currentVerticalScrollPosition = ImGui::GetScrollY();

                // If we want to maintain the current visible set of data, update the scroll bar position to keep the same first visible item
                if ( m_maintainVisibleRowIdx )
                {
                    currentVerticalScrollPosition = ( m_firstVisibleRowItemIdx / numRowIndices * maxVerticalScrollPosition );
                    ImGui::SetScrollY( currentVerticalScrollPosition );
                    m_maintainVisibleRowIdx = false;
                }

                // Update visible item based on scrollbar position
                // Assumption is that when we are at max scrolling we should show the last item at the bottom of the visible area
                int32_t const scrollItemOffset = (int32_t) Math::Round( currentVerticalScrollPosition / m_estimatedRowHeight );
                m_firstVisibleRowItemIdx = ( scrollItemOffset == 0 ) ? 0 : scrollItemOffset + 1; // Ensure the last item is always fully visible
                m_firstVisibleRowItemIdx = Math::Clamp( m_firstVisibleRowItemIdx, 0, (int32_t) m_visualTree.size() - 1 );

                // Calculate draw range
                bool shouldDrawDummyRow = false;
                int32_t const itemsToDrawStartIdx = m_firstVisibleRowItemIdx;
                int32_t itemsToDrawEndIdx = Math::Min( itemsToDrawStartIdx + maxNumDrawableRows, (int32_t) m_visualTree.size() - 1 );

                // Draw initial dummy to adjust scrollbar position
                if ( currentVerticalScrollPosition != 0 )
                {
                    ImGui::Dummy( ImVec2( -1, currentVerticalScrollPosition ) );
                }

                // Draw table rows
                if ( ImGui::BeginTable( "Tree View Browser", GetNumExtraColumns() + 1, tableFlags, ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x / 2, -1 ) ) )
                {
                    ImGui::TableSetupColumn( "Label", ImGuiTableColumnFlags_WidthStretch );
                    SetupExtraColumnHeaders();

                    m_isDrawingTree = true;
                    for ( int32_t i = itemsToDrawStartIdx; i <= itemsToDrawEndIdx; i++ )
                    {
                        DrawVisualItem( m_visualTree[i] );
                    }
                    m_isDrawingTree = false;

                    ImGui::EndTable();
                }

                // Draw final dummy to maintain scrollbar position
                ImGui::Dummy( ImVec2( -1, m_estimatedTreeHeight - ImGui::GetCursorPos().y ) );
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar( 2 );
        ImGui::PopStyleColor();

        //-------------------------------------------------------------------------

        ImGui::PopID();
    }
}