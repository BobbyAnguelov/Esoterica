#pragma once

#include "EngineTools/_Module/API.h"
#include "System/Imgui/ImguiX.h"
#include "System/Types/Function.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Types/StringID.h"
#include "System/Types/Event.h"
#include "System/Types/HashMap.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINETOOLS_API TreeListViewItem
    {
        friend class TreeListView;

    public:

        enum ItemState
        {
            None,
            Selected,
            Active
        };

    public:

        TreeListViewItem() = default;
        virtual ~TreeListViewItem() = default;

        //-------------------------------------------------------------------------

        // The unique ID is need to be able to ID, record and restore tree state
        virtual uint64_t GetUniqueID() const = 0;

        // The name ID is the name of the item relative to its parent. This is not guaranteed to be unique per item
        // Used by default for sorting
        virtual StringID GetNameID() const = 0;

        // The friendly display name printed in the UI (generally the same as the nameID)
        // This is separate from the name since we might want to add an icon or other decoration to the display name without changing the name
        virtual String GetDisplayName() const 
        {
            StringID const nameID = GetNameID();
            return nameID.IsValid() ? nameID.c_str() : "!!! Invalid Name !!!";
        }
        
        // The color that the display name should be printed in
        virtual ImVec4 GetDisplayColor( ItemState state ) const;

        // Does this item have a context menu?
        virtual bool HasContextMenu() const { return false; }

        // Can this item be set as the active item (note: this is different from the selected item)
        virtual bool IsActivatable() const { return false; }

        // Get the tooltip for this item if it has one
        virtual char const* GetTooltipText() const { return nullptr; }

        // Expansion
        //-------------------------------------------------------------------------

        // Is this item a leaf node (i.e., should we draw the expansion arrow)
        virtual bool IsLeaf() const { return m_children.empty(); }

        inline void SetExpanded( bool isExpanded ) { m_isExpanded = isExpanded; }

        inline bool IsExpanded() const { return m_isExpanded; }

        // Visibility
        //-------------------------------------------------------------------------

        inline bool IsVisible() const { return m_isVisible; }

        inline bool HasVisibleChildren() const { return !m_children.empty(); }

        // Update visibility for this branch based on a user-supplied delegate
        void UpdateVisibility( TFunction<bool( TreeListViewItem const* pItem )> const& isVisibleFunction, bool showParentItemsWithNoVisibleChildren = false );

        // Drag and drop
        //-------------------------------------------------------------------------

        // Can we be dragged somewhere?
        virtual bool IsDragAndDropSource() const { return false; }

        // Are we a valid target for drag and drop operations?
        virtual bool IsDragAndDropTarget() const { return false; }

        // Set the ImGui payload data for the drag and drop operation
        virtual void SetDragAndDropPayloadData() const {}

        // Hierarchy
        //-------------------------------------------------------------------------

        inline bool HasChildren() const { return !m_children.empty(); }
        inline TVector<TreeListViewItem*> const& GetChildren() { return m_children; }
        inline TVector<TreeListViewItem*> const& GetChildren() const { return m_children; }

        // Create a new child tree view item (memory is owned by this item)
        template< typename T, typename ... ConstructorParams >
        T* CreateChild( ConstructorParams&&... params )
        {
            static_assert( std::is_base_of<TreeListViewItem, T>::value, "T must derive from TreeViewItem" );
            TreeListViewItem* pAddedItem = m_children.emplace_back( EE::New<T>( std::forward<ConstructorParams>( params )... ) );
            EE_ASSERT( pAddedItem->GetUniqueID() != 0 );
            return static_cast<T*>( pAddedItem );
        }

        // Destroy specific child - be careful when calling this, you need to make sure the visual tree is kept in sync
        void DestroyChild( uint64_t uniqueItemID );

        // Destroys all child for this branch - be careful when calling this, you need to make sure the visual tree is kept in sync
        void DestroyChildren();

        // Find a specific child
        TreeListViewItem const* FindChild( uint64_t uniqueID ) const;

        // Find a specific child
        TreeListViewItem* FindChild( uint64_t uniqueID ) { return const_cast<TreeListViewItem*>( const_cast<TreeListViewItem const*>( this )->FindChild( uniqueID ) ); }

        // Find a specific child
        TreeListViewItem* FindChild( TFunction<bool( TreeListViewItem const* )> const& searchPredicate );

        // Recursively search all children
        TreeListViewItem* SearchChildren( TFunction<bool( TreeListViewItem const* )> const& searchPredicate );

        // Apply some operation for all elements in this branch
        inline void ForEachChild( TFunction<void( TreeListViewItem* pItem )> const& function )
        {
            for ( auto& pChild : m_children )
            {
                function( pChild );
                pChild->ForEachChild( function );
            }
        }

        // Apply some operation for all elements in this branch
        inline void ForEachChildConst( TFunction<void( TreeListViewItem const* pItem )> const& function ) const
        {
            for ( auto& pChild : m_children )
            {
                function( pChild );
                pChild->ForEachChildConst( function );
            }
        }

    private:

        // Disable copies/moves
        TreeListViewItem& operator=( TreeListViewItem const& ) = delete;
        TreeListViewItem& operator=( TreeListViewItem&& ) = delete;

    protected:

        TVector<TreeListViewItem*>              m_children;
        bool                                    m_isVisible = true;
        bool                                    m_isExpanded = false;
    };

    //-------------------------------------------------------------------------
    // Tree List View 
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API TreeListView
    {

    public:

        enum class ChangeReason
        {
            UserInput,
            CodeRequest,
            TreeRebuild
        };

    protected:

        class TreeRootItem final : public TreeListViewItem
        {
        public:

            TreeRootItem()
            {
                m_isExpanded = true;
            }

            virtual StringID GetNameID() const { return m_ID; }
            virtual uint64_t GetUniqueID() const { return 0; }

        private:

            StringID m_ID = StringID( "Root" );
        };

    private:

        enum class VisualTreeState
        {
            None,
            UpToDate,
            NeedsRebuild,
            NeedsRebuildAndViewReset
        };

        struct VisualTreeItem
        {
            VisualTreeItem() = default;

            VisualTreeItem( TreeListViewItem* pItem, int32_t hierarchyLevel )
                : m_pItem( pItem )
                , m_hierarchyLevel( hierarchyLevel )
            {
                EE_ASSERT( pItem != nullptr && hierarchyLevel >= 0 );
            }

        public:

            TreeListViewItem*                   m_pItem = nullptr;
            int32_t                             m_hierarchyLevel = -1;
        };

    public:

        TreeListView() = default;
        virtual ~TreeListView();

        // Visual
        //-------------------------------------------------------------------------

        void Draw( float listHeight = 0.0f );

        // Selection, Activation and Events
        //-------------------------------------------------------------------------

        // Clear the current selection
        void ClearSelection();

        // Get the current selection
        inline TVector<TreeListViewItem*> GetSelection() const { return m_selection; }

        // Fire whenever the selection changes
        inline TEventHandle<ChangeReason> OnSelectedChanged() { return m_onSelectionChanged; }

        // Clear active item
        inline void ClearActiveItem() { m_pActiveItem = nullptr; m_onActiveItemChanged.Execute( ChangeReason::CodeRequest ); }

        inline TreeListViewItem* GetActiveItem() const { return m_pActiveItem; }

        // Fires whenever the active item changes, parameter is the new active item (can be null)
        inline TEventHandle<ChangeReason> OnActiveItemChanged() { return m_onActiveItemChanged; }

        // Fires whenever an item is double clicked, parameter is the item that was double clicked (cant be null)
        inline TEventHandle<TreeListViewItem*> OnItemDoubleClicked() { return m_onItemDoubleClicked; }

        // Bulk Item Operations
        //-------------------------------------------------------------------------

        void ForEachItem( TFunction<void( TreeListViewItem* pItem )> const& function, bool refreshVisualState = true )
        {
            m_rootItem.ForEachChild( function );

            if ( refreshVisualState )
            {
                RefreshVisualState();
            }
        }

        void ForEachItemConst( TFunction<void( TreeListViewItem const* pItem )> const& function ) const
        {
            m_rootItem.ForEachChildConst( function );
        }

        void UpdateItemVisibility( TFunction<bool( TreeListViewItem const* pItem )> const& isVisibleFunction, bool showParentItemsWithNoVisibleChildren = false )
        {
            m_rootItem.UpdateVisibility( isVisibleFunction, showParentItemsWithNoVisibleChildren );
            RefreshVisualState();
        }

    protected:

        inline void RefreshVisualState() { m_visualTreeState = VisualTreeState::NeedsRebuild; }

        inline int32_t GetNumItems() const { return (int32_t) m_visualTree.size(); }

        TreeListViewItem* FindItem( uint64_t uniqueID );

        inline TreeListViewItem const* FindItem( uint64_t uniqueID ) const { return const_cast<TreeListView*>( this )->FindItem( uniqueID ); }

        void DestroyItem( uint64_t uniqueID );

        //-------------------------------------------------------------------------

        virtual void HandleDragAndDropOnItem( TreeListViewItem* pDragAndDropTargetItem ) {}

        // Get the number of extra columns needed
        virtual uint32_t GetNumExtraColumns() const { return 0; }

        // Get the number of extra columns needed
        virtual void SetupExtraColumnHeaders() const {}

        // Draw any custom item controls you might need
        virtual void DrawItemExtraColumns( TreeListViewItem* pBaseItem, int32_t extraColumnIdx ) {}

        // Draw any custom item context menus you might need
        virtual void DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) {}

        // Call this function to rebuild the tree contents - This will in turn call the user supplied "RebuildTreeInternal" function
        void RebuildTree( bool maintainExpansionAndSelection = true );

        // Tears down the entire tree
        void DestroyTree();

        // Implement this to rebuild the tree, the root item will have already been destroyed at this point!
        // DO NOT CALL THIS DIRECTLY!
        virtual void RebuildTreeUserFunction() = 0;

        // Is a given item selected?
        bool IsItemSelected( TreeListViewItem const* pItem ) const;

        // Set the selection to a single item - Notification will only be fired if the selection actually changes
        inline void SetSelection( TreeListViewItem* pItem ){ SetSelectionInternal( pItem, ChangeReason::CodeRequest ); }

        // Add to the current selection - Notification will only be fired if the selection actually changes
        inline void AddToSelection( TreeListViewItem* pItem ){ AddToSelectionInternal( pItem, ChangeReason::CodeRequest ); }

        // Add an item range to the selection - Notification will only be fired if the selection actually changes
        inline void AddToSelection( TVector<TreeListViewItem*> const& itemRange ){ AddToSelectionInternal( itemRange, ChangeReason::CodeRequest ); }

        // Add an item range to the selection - Notification will only be fired if the selection actually changes
        inline void SetSelection( TVector<TreeListViewItem*> const& itemRange ){ SetSelectionInternal( itemRange, ChangeReason::CodeRequest ); }

        // Remove an item from the current selection - Notification will only be fired if the selection actually changes
        inline void RemoveFromSelection( TreeListViewItem* pItem ) { RemoveFromSelectionInternal( pItem, ChangeReason::CodeRequest ); }

        // Override this to handle selection changes
        virtual void HandleActiveItemChanged( ChangeReason reason ){}

        // Override this to handle selection changes
        virtual void HandleSelectionChanged( ChangeReason reason ) {}

        // Caches the current selection and expansion state
        void CacheSelectionAndExpansionState();

        // Tries to restore the current selection and expansion state
        void RestoreCachedSelectionAndExpansionState();

        // Override this to disable tree sorting
        virtual bool ShouldSortTree() const { return true; }

        // Sorts an item's children using the virtual ItemSortComparator function
        void SortItemChildren( TreeListViewItem* pItem );

        // The function used in the sort
        virtual bool ItemSortComparator( TreeListViewItem const* pItemA, TreeListViewItem const* pItemB ) const;

    private:

        void HandleItemSelection( TreeListViewItem* pItem, bool isSelected );
        void SetSelectionInternal( TreeListViewItem* pItem, ChangeReason reason );
        void AddToSelectionInternal( TreeListViewItem* pItem, ChangeReason reason );
        void AddToSelectionInternal( TVector<TreeListViewItem*> const& itemRange, ChangeReason reason );
        void SetSelectionInternal( TVector<TreeListViewItem*> const& itemRange, ChangeReason reason );
        void RemoveFromSelectionInternal( TreeListViewItem* pItem, ChangeReason reason );

        void DrawVisualItem( VisualTreeItem& visualTreeItem );
        void TryAddItemToVisualTree( TreeListViewItem* pItem, int32_t hierarchyLevel );

        void RebuildVisualTree();
        void OnItemDoubleClickedInternal( TreeListViewItem* pItem );

        int32_t GetVisualTreeItemIndex( TreeListViewItem const* pBaseItem ) const;
        int32_t GetVisualTreeItemIndex( uint64_t uniqueID ) const;

        void NotifySelectionChanged( ChangeReason reason )
        {
            HandleSelectionChanged( reason );
            m_onSelectionChanged.Execute( reason );
        }

        void NotifyActiveItemChanged( ChangeReason reason )
        {
            HandleActiveItemChanged( reason );
            m_onActiveItemChanged.Execute( reason );
        }

    protected:

        // The root of the tree - fill this with your items
        TreeRootItem                                            m_rootItem;

        TEvent<ChangeReason>                                    m_onSelectionChanged;
        TEvent<ChangeReason>                                    m_onActiveItemChanged;
        TEvent<TreeListViewItem*>                               m_onItemDoubleClicked;

        // The active item is an item that is activated (and deactivated) via a double click
        TreeListViewItem*                                       m_pActiveItem = nullptr;

        // Control tree view behavior
        bool                                                    m_prioritizeBranchesOverLeavesInVisualTree = true;
        bool                                                    m_expandItemsOnlyViaArrow = false;
        bool                                                    m_multiSelectionAllowed = false;
        bool                                                    m_drawRowBackground = true;
        bool                                                    m_drawBorders = true;
        bool                                                    m_useSmallFont = true;
        bool                                                    m_showBulletsOnLeaves = false;

    private:

        TVector<VisualTreeItem>                                 m_visualTree;
        VisualTreeState                                         m_visualTreeState = VisualTreeState::None;
        float                                                   m_estimatedRowHeight = -1.0f;
        float                                                   m_estimatedTreeHeight = -1.0f;
        int32_t                                                 m_firstVisibleRowItemIdx = 0;
        float                                                   m_itemControlColumnWidth = 0;
        bool                                                    m_maintainVisibleRowIdx = false;

        // The currently selected item (changes frequently due to clicks/focus/etc...) - In order of selection time, first is oldest
        TVector<TreeListViewItem*>                              m_selection;
        uint64_t                                                m_cachedActiveItemID = 0;
        TVector<uint64_t>                                       m_selectedItemIDs;
        TVector<uint64_t>                                       m_originalExpandedItems;
    };
}