#include "TreeListView.h"
#include "EASTL/sort.h"
#include "Base/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool TreeListViewItem::Compare( TreeListViewItem const* pItem ) const
    {
        char const* const pNameA = GetSortingKey();
        size_t const lengthA = strlen( pNameA );
        char const* const pNameB = pItem->GetSortingKey();
        size_t const lengthB = strlen( pNameB );

        int32_t const result = String::comparei( pNameA, pNameA + lengthA, pNameB, pNameB + lengthB );
        return result < 0;
    }

    Color TreeListViewItem::GetDisplayColor( ItemState state ) const
    {
        Color color = ImGuiX::Style::s_colorText;

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

        if ( IsLeaf() )
        {
            m_isExpanded = true;
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
        m_isExpanded = true;
    }

    int32_t TreeListViewItem::GetNumChildren( bool recurse ) const
    {
        int32_t numChildren = (int32_t) m_children.size();

        if ( recurse )
        {
            for ( auto& pChildItem : m_children )
            {
                numChildren += pChildItem->GetNumChildren( true );
            }
        }

        return numChildren;
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
                pChild->UpdateVisibility( isVisibleFunction, showParentItemsWithNoVisibleChildren );
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

    void TreeListView::RebuildTree( TreeListViewContext context, bool maintainExpansionAndSelection )
    {
        EE_ASSERT( !IsCurrentlyDrawingTree() );

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

        if ( maintainExpansionAndSelection )
        {
            RestoreCachedSelectionAndExpansionState();
        }

        m_visualTreeState = VisualTreeState::NeedsRebuild;
        RebuildVisualTree();

        if ( m_flags.IsFlagSet( TrackSelection ) )
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

        // Set view to selection
        //-------------------------------------------------------------------------

        if ( m_visualTreeState == VisualTreeState::NeedsRebuildAndFocusSelection )
        {
            for ( auto i = 0; i < m_visualTree.size(); i++ )
            {
                if ( m_visualTree[i].m_pItem->m_isSelected )
                {
                    if ( i < m_currentFirstVisibleRowIdx || i > m_currentLastVisibleRowIdx )
                    {
                        m_requestedFirstVisibleRowItemIdx = i;
                    }

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

    void TreeListView::AddToSelectionInternal( TVector<TreeListViewItem*> const& newSelection, bool updateView )
    {
        if ( newSelection.empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        for ( auto pItem : newSelection )
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

    void TreeListView::SetSelectionInternal( TVector<TreeListViewItem*> const& newSelection, bool updateView )
    {
        if ( newSelection.empty() )
        {
            ClearSelection();
            return;
        }

        //-------------------------------------------------------------------------

        // Deselect items not in the new selection
        for( int32_t i = (int32_t) m_selection.size() - 1; i >=0; i-- )
        {
            if ( !VectorContains( newSelection, m_selection[i] ) )
            {
                RemoveFromSelectionInternal( m_selection[i], false );
            }
        }

        // Select new items
        for ( auto pItem : newSelection )
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

    //-------------------------------------------------------------------------

    void TreeListView::DrawVisualItem( TreeListViewContext context, int32_t visualItemIdx )
    {
        VisualTreeItem& visualTreeItem = m_visualTree[visualItemIdx];
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
        uint32_t treeNodeflags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAllColumns;

        if ( pItem->IsHeader() )
        {
            treeNodeflags |= ( ImGuiTreeNodeFlags_Framed );
        }

        if ( m_flags.IsFlagSet( ExpandItemsOnlyViaArrow ) )
        {
            treeNodeflags |= ImGuiTreeNodeFlags_OpenOnArrow;
        }

        if ( m_flags.IsFlagSet( ExpandItemsWithDoubleClick ) )
        {
            treeNodeflags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
        }

        if ( isSelectedItem )
        {
            treeNodeflags |= ImGuiTreeNodeFlags_Selected;
        }

        // Leaf nodes
        if ( pItem->IsLeaf() || !pItem->HasVisibleChildren() )
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

        //-------------------------------------------------------------------------

        InlineString const displayName = pItem->GetDisplayName();
        bool isTreeNodeExpanded = false;
        TreeListViewItem::ItemState const state = isActiveItem ? TreeListViewItem::Active : isSelectedItem ? TreeListViewItem::Selected : TreeListViewItem::None;
        
        {
            ImGuiX::Font const activeItemFont = m_flags.IsFlagSet( UseSmallFont ) ? ImGuiX::Font::SmallBold : ImGuiX::Font::MediumBold;
            ImGuiX::Font const inactiveItemFont = m_flags.IsFlagSet( UseSmallFont ) ? ImGuiX::Font::Small : ImGuiX::Font::Medium;

            ImGuiX::ScopedFont font( isActiveItem ? activeItemFont : inactiveItemFont );

            // Create tree item
            InlineString treeItemLabel;

            if ( pItem->HasIcon() )
            {
                treeItemLabel.sprintf( "##%s", pItem->GetIcon(), displayName.c_str() );
            }
            else
            {
                treeItemLabel = displayName.c_str();
            }

            ImGui::SetNextItemSelectionUserData( visualItemIdx );
            isTreeNodeExpanded = ImGui::TreeNodeEx( treeItemLabel.c_str(), treeNodeflags | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanFullWidth );

            if ( pItem->IsLeaf() )
            {
                EE_ASSERT( isTreeNodeExpanded );
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
                if ( ImGui::BeginDragDropSource( ImGuiDragDropFlags_SourceAllowNullID ) )
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
                    }

                    ImGui::EndPopup();
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

            // Draw icon and label
            //-------------------------------------------------------------------------

            if ( pItem->HasIcon() )
            {
                ImGui::SameLine();

                ImGui::PushStyleColor( ImGuiCol_Text, pItem->GetIconColor() );
                ImGui::Text( pItem->GetIcon() );
                ImGui::PopStyleColor();

                ImGui::SameLine();

                ImGui::PushStyleColor( ImGuiCol_Text, pItem->GetDisplayColor( state ) );
                ImGui::Text( displayName.c_str() );
                ImGui::PopStyleColor();
            }
        }

        // Pop Tree and Indent
        if ( isTreeNodeExpanded )
        {
            ImGui::TreePop();
        }

        if ( requiresIndent )
        {
            ImGui::Unindent( indentLevel );
        }

        // Handle expansion
        if ( !pItem->IsLeaf() && pItem->IsExpanded() != isTreeNodeExpanded )
        {
            bool const applyToChildren = ImGui::IsKeyDown( ImGuiMod_Ctrl );

            pItem->SetExpanded( isTreeNodeExpanded, applyToChildren );
            m_visualTreeState = VisualTreeState::NeedsRebuild;
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

        // Start Selection
        //-------------------------------------------------------------------------

        TVector<TreeListViewItem*> previousSelection = m_selection;

        ImGuiMultiSelectFlags selectionFlags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ClearOnClickVoid;
        if ( m_flags.IsFlagCleared( MultiSelectionAllowed ) )
        {
            selectionFlags |= ImGuiMultiSelectFlags_SingleSelect;
        }

        int32_t const numVisualItems = (int32_t) m_visualTree.size();
        ImGuiMultiSelectIO* msIO = ImGui::BeginMultiSelect( selectionFlags, -1, numVisualItems );

        // Draw table
        //-------------------------------------------------------------------------

        ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY;

        if ( m_flags.IsFlagSet( DrawRowBackground ) )
        {
            tableFlags |= ImGuiTableFlags_RowBg;
        }

        if ( m_flags.IsFlagSet( DrawBorders ) )
        {
            tableFlags |= ImGuiTableFlags_Borders;
        }

        ImGui::PushID( this );
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 2, 2 ) );
        if ( ImGui::BeginTable( "TreeViewTable", context.m_numExtraColumns + 1, tableFlags, ImVec2( ImGui::GetContentRegionAvail().x, -1 ) ) )
        {
            ImGui::TableSetupColumn( "Label", ImGuiTableColumnFlags_WidthStretch );
            if ( context.m_setupExtraColumnHeadersFunction != nullptr )
            {
                context.m_setupExtraColumnHeadersFunction();
            }

            m_isDrawingTree = true;

            ImGuiListClipper clipper;
            clipper.Begin( numVisualItems );

            // If we want to maintain the current visible set of data, update the scroll bar position to keep the same first visible item
            if ( m_requestedFirstVisibleRowItemIdx != InvalidIndex && m_estimatedRowHeight > 0 )
            {
                ImGui::SetScrollY( m_requestedFirstVisibleRowItemIdx * m_estimatedRowHeight );
                m_requestedFirstVisibleRowItemIdx = InvalidIndex;
            }

            // Draw clipped list
            m_currentFirstVisibleRowIdx = InvalidIndex;
            m_currentLastVisibleRowIdx = InvalidIndex;
            while ( clipper.Step() )
            {
                m_currentFirstVisibleRowIdx = clipper.DisplayStart;
                m_currentLastVisibleRowIdx = Math::Max( clipper.DisplayStart, clipper.DisplayEnd - 1 );

                for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i )
                {
                    float const cursorPosY = ImGui::GetCursorPosY();
                    DrawVisualItem( context, i );
                    m_estimatedRowHeight = ImGui::GetCursorPosY() - cursorPosY;
                }
            }
            m_isDrawingTree = false;

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        ImGui::PopID();

        // Handle selection requests
        //-------------------------------------------------------------------------

        msIO = ImGui::EndMultiSelect();

        for ( ImGuiSelectionRequest const& req : msIO->Requests )
        {
            if ( req.Type == ImGuiSelectionRequestType_SetAll )
            {
                // If we need to select all then just update the selection state for all unselected items
                if ( req.Selected )
                {
                    m_selection.clear();

                    for ( auto& vi : m_visualTree )
                    {
                        m_selection.emplace_back( vi.m_pItem );

                        if ( !vi.m_pItem->m_isSelected )
                        {
                            vi.m_pItem->m_isSelected = true;
                            vi.m_pItem->OnSelectionStateChanged();
                        }
                    }
                }
                else // Just clear the selection
                {
                    ClearSelection();
                }
            }
            else if ( req.Type == ImGuiSelectionRequestType_SetRange )
            {
                if ( req.Selected )
                {
                    for ( int64_t i = req.RangeFirstItem; i <= req.RangeLastItem; i++ )
                    {
                        AddToSelectionInternal( m_visualTree[i].m_pItem, false );
                    }
                }
                else
                {
                    for ( int64_t i = req.RangeFirstItem; i <= req.RangeLastItem; i++ )
                    {
                        RemoveFromSelectionInternal( m_visualTree[i].m_pItem, false );
                    }
                }
            }
        }

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

        // Check selection state for the return value
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