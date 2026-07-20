#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Types/Function.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Types/StringID.h"
#include "Base/Types/Event.h"
#include "Base/Types/HashMap.h"
#include "Base/Types/BitFlags.h"

//-------------------------------------------------------------------------

namespace EE
{
    //-------------------------------------------------------------------------
    // Tree List Item
    //-------------------------------------------------------------------------

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

        TreeListViewItem( TreeListViewItem* pParent ) : m_pParent( pParent ) {}
        virtual ~TreeListViewItem() = default;

        //-------------------------------------------------------------------------

        // The unique ID is need to be able to ID, record and restore tree state
        virtual uint64_t GetUniqueID() const = 0;

        // The sorting key for the items
        virtual char const* GetSortingKey() const { return GetDisplayName(); }

        // Does this item have a context menu?
        virtual bool HasContextMenu() const { return false; }

        // Can this item be set as the active item (note: this is different from the selected item)
        virtual bool IsActivatable() const { return false; }

        // Sort comparator function
        virtual bool Compare( TreeListViewItem const* pItem ) const;

        // Signals
        //-------------------------------------------------------------------------

        // Called whenever this item is activated/deactivated
        virtual void OnActivationStateChanged() {}

        // Called whenever this item is selected/unselected
        virtual void OnSelectionStateChanged() {}

        // Called whenever we double click an item - return true if you require a visual tree rebuild
        virtual bool OnDoubleClick() { return false; }

        // Called whenever we hit enter on a selected item - return true if you require a visual tree rebuild
        virtual bool OnEnterPressed() { return false; }

        // Visuals
        //-------------------------------------------------------------------------

        virtual bool HasIcon() const { return false; }

        virtual char const* GetIcon() const { return ""; }

        virtual Color GetIconColor() const { return ImGuiX::Style::s_colorText; }

        // The friendly display name printed in the UI
        virtual char const* GetDisplayName() const = 0;

        // The color that the display name should be printed in
        virtual Color GetDisplayColor( ItemState state ) const;

        // Is this a header item (i.e. should be framed)
        virtual bool IsHeader() const { return false; }

        // Get the tooltip for this item if it has one
        virtual char const* GetTooltipText() const { return nullptr; }

        // Expansion
        //-------------------------------------------------------------------------

        // Is this item a leaf node (i.e. should we draw the expansion arrow)
        virtual bool IsLeaf() const { return m_children.empty(); }

        // Set node expansion state. Note: Leaf nodes are always expanded
        inline void SetExpanded( bool isExpanded, bool applyToChildren = false ) 
        {
            if ( IsLeaf() )
            {
                m_isExpanded = true;
            }
            else
            {
                m_isExpanded = isExpanded;

                if ( applyToChildren )
                {
                    for ( auto pChild : m_children )
                    {
                        pChild->SetExpanded( isExpanded, applyToChildren );
                    }
                }
            }
        }

        // Is this node expanded. Note: Leaf nodes are always expanded
        inline bool IsExpanded() const
        {
            if ( IsLeaf() ) { EE_ASSERT( m_isExpanded ); }
            return m_isExpanded;
        }

        // Visibility
        //-------------------------------------------------------------------------

        inline bool IsVisible() const { return m_isVisible; }

        inline bool HasVisibleChildren() const
        {
            for ( auto const& pChild : m_children )
            {
                if ( pChild->IsVisible() )
                {
                    return true;
                }
            }

            return false;
        }

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

        int32_t GetNumChildren( bool recurse ) const;

        // Create a new child tree view item (memory is owned by this item)
        template< typename T, typename ... ConstructorParams >
        T* CreateChild( ConstructorParams&&... params )
        {
            static_assert( std::is_base_of<TreeListViewItem, T>::value, "T must derive from TreeViewItem" );
            TreeListViewItem* pAddedItem = m_children.emplace_back( EE::New<T>( this, eastl::forward<ConstructorParams>( params )... ) );
            EE_ASSERT( pAddedItem->GetUniqueID() != 0 );
            return static_cast<T*>( pAddedItem );
        }

        // Add a new child tree view item (memory is now owned by this item)
        void AddNewChild( TreeListViewItem* pChildItem )
        {
            EE_ASSERT( pChildItem != nullptr && pChildItem->m_pParent == nullptr );
            EE_ASSERT( pChildItem->GetUniqueID() != 0 );
            m_children.emplace_back( pChildItem );
            pChildItem->m_pParent = this;
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

        // Sorts the children of this item, Note: this will not be reflected into the tree view unless you rebuild the visual tree
        void SortChildren();

    private:

        // Disable copies/moves
        TreeListViewItem& operator=( TreeListViewItem const& ) = delete;
        TreeListViewItem& operator=( TreeListViewItem&& ) = delete;

    protected:

        TreeListViewItem*                       m_pParent = nullptr;
        TVector<TreeListViewItem*>              m_children;
        bool                                    m_isVisible = true;
        bool                                    m_isExpanded = true;
        bool                                    m_isActivated = false;
        bool                                    m_isSelected = false;
    };

    //-------------------------------------------------------------------------
    // Tree List View Context
    //-------------------------------------------------------------------------

    struct TreeListViewContext
    {
        // This function is called to rebuild the tree, you are expected to fill the root item
        TFunction<void( TreeListViewItem* /*rootItem*/ )>                                   m_rebuildTreeFunction;

        // Called when we have a drag and drop operation on a target item
        TFunction<void( TreeListViewItem* /*dropTarget*/ )>                                 m_onDragAndDropFunction;

        // Set up any headers for your extra columns
        TFunction<void()>                                                                   m_setupExtraColumnHeadersFunction;

        // Draw any custom item controls you might need for a given item
        TFunction<void( TreeListViewItem* /*pBaseItem*/, int32_t /*extraColumnIdx*/ )>      m_drawItemExtraColumnsFunction;

        // Draw any custom item context menus you might need
        TFunction<void( TVector<TreeListViewItem*> const& /*items*/ )>                      m_drawItemContextMenuFunction;

        // The number of extra columns to draw
        int32_t                                                                             m_numExtraColumns = 0;
    };

    //-------------------------------------------------------------------------
    // Tree List View 
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API TreeListView final
    {
        class TreeRootItem final : public TreeListViewItem
        {
        public:

            TreeRootItem()
                : TreeListViewItem( nullptr )
            {
                m_isExpanded = true;
            }

            virtual char const* GetDisplayName() const { return m_ID.c_str(); }
            virtual uint64_t GetUniqueID() const { return 0; }

        private:

            StringID m_ID = StringID( "Root" );
        };

        enum class VisualTreeState
        {
            UpToDate,
            NeedsRebuild,
            NeedsRebuildAndFocusSelection
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

        enum Flags : uint8_t
        {
            ShowBranchesFirst = 0,
            ExpandItemsOnlyViaArrow,
            ExpandItemsWithDoubleClick,
            MultiSelectionAllowed,
            DrawRowBackground,
            DrawBorders,
            UseSmallFont,
            ShowBulletsOnLeaves,
            SortTree, // Sort all items in the tree
            TrackSelection, // Update the viewed items to match the current selection
        };

    public:

        TreeListView( TBitFlags<Flags> flags = TBitFlags<Flags>( ShowBranchesFirst, DrawRowBackground, DrawBorders, UseSmallFont ) )
            : m_flags( flags )
        {}

        template<typename... Args, class Enable = std::enable_if_t<( ... && std::is_convertible_v<Args, Flags> )>>
        TreeListView( Args&&... args )
            : m_flags( args... )
        {}

        ~TreeListView();

        // Rebuilds the tree's internal data
        void RebuildTree( TreeListViewContext context, bool maintainExpansionAndSelection = false );

        // Returns true if the selection has changed
        bool UpdateAndDraw( TreeListViewContext context, float listHeight = 0.0f );

        // Visual
        //-------------------------------------------------------------------------

        // Refreshes the visual tree and reflects any changes to expansion, visibility or sorting
        inline void RefreshVisualState() { m_visualTreeState = VisualTreeState::NeedsRebuild; }

        // Check if we are currently drawing the tree - useful to assert about when certain external operations are performed
        inline bool IsCurrentlyDrawingTree() const { return m_isDrawingTree; }

        // Set option flag
        void SetFlag( Flags flag, bool value = true ) { m_flags.SetFlag( flag, value ); }

        // Set all option flags
        void SetFlags( TBitFlags<Flags> flags ) { m_flags = flags; }

        // Expand all items
        void ExpandAll() { m_rootItem.SetExpanded( true, true ); RefreshVisualState(); }

        // Collapse all items
        void CollapseAll() { m_rootItem.SetExpanded( false, true ); RefreshVisualState(); }

        // Activation
        //-------------------------------------------------------------------------

        // Do we have anything selected?
        inline bool HasActiveItem() const { return m_pActiveItem != nullptr; }

        // Clear active item
        inline void ClearActiveItem() { m_pActiveItem = nullptr; }

        // Get the currently active item
        inline TreeListViewItem* GetActiveItem() const { return m_pActiveItem; }

        // Selection
        //-------------------------------------------------------------------------

        // Do we have anything selected?
        inline bool HasSelection() const { return !m_selection.empty(); }

        // Clear the current selection
        void ClearSelection();

        // Get the current selection
        inline TVector<TreeListViewItem*> const& GetSelection() const { return m_selection; }

        // Set the view to the selected item
        void SetViewToSelection();

        // Find an item based on its unique ID
        TreeListViewItem* FindItem( uint64_t uniqueID );

        // Find an item based on its unique ID
        inline TreeListViewItem const* FindItem( uint64_t uniqueID ) const { return const_cast<TreeListView*>( this )->FindItem( uniqueID ); }

        // Is this item selected?
        inline bool IsItemSelected( uint64_t uniqueID ) const { auto pItem = FindItem( uniqueID ); return pItem == nullptr ? false : pItem->m_isSelected; }

        // Set the selection to a single item - Notification will only be fired if the selection actually changes
        inline void SetSelection( TreeListViewItem* pItem ){ SetSelectionInternal( pItem, m_flags.IsFlagSet( TrackSelection ) ); }

        // Add to the current selection - Notification will only be fired if the selection actually changes
        inline void AddToSelection( TreeListViewItem* pItem ){ AddToSelectionInternal( pItem, m_flags.IsFlagSet( TrackSelection ) ); }

        // Add an item range to the selection - Notification will only be fired if the selection actually changes
        inline void AddToSelection( TVector<TreeListViewItem*> const& itemRange ){ AddToSelectionInternal( itemRange, m_flags.IsFlagSet( TrackSelection ) ); }

        // Add an item range to the selection - Notification will only be fired if the selection actually changes
        inline void SetSelection( TVector<TreeListViewItem*> const& itemRange ){ SetSelectionInternal( itemRange, m_flags.IsFlagSet( TrackSelection ) ); }

        // Remove an item from the current selection - Notification will only be fired if the selection actually changes
        inline void RemoveFromSelection( TreeListViewItem* pItem ) { RemoveFromSelectionInternal( pItem, m_flags.IsFlagSet( TrackSelection ) ); }

        // Set the selection to a single item - Notification will only be fired if the selection actually changes
        inline void SetSelection( uint64_t itemID ) { SetSelectionInternal( FindItem( itemID ), m_flags.IsFlagSet( TrackSelection ) ); }

        // Add to the current selection - Notification will only be fired if the selection actually changes
        inline void AddToSelection( uint64_t itemID ) { AddToSelectionInternal( FindItem( itemID ), m_flags.IsFlagSet( TrackSelection ) ); }

        // Remove an item from the current selection - Notification will only be fired if the selection actually changes
        inline void RemoveFromSelection( uint64_t itemID ) { RemoveFromSelectionInternal( FindItem( itemID ), m_flags.IsFlagSet( TrackSelection ) ); }

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

        // Update the visibility of all items using a delegate, if the delegate returns true then the item is visible, false means invisible
        void UpdateItemVisibility( TFunction<bool( TreeListViewItem const* pItem )> const& isVisibleFunction, bool showParentItemsWithNoVisibleChildren = false )
        {
            m_rootItem.UpdateVisibility( isVisibleFunction, showParentItemsWithNoVisibleChildren );
            RefreshVisualState();
        }

    private:

        inline int32_t GetNumItems() const { return (int32_t) m_visualTree.size(); }

        void DestroyItem( uint64_t uniqueID );

        //-------------------------------------------------------------------------

        // Tears down the entire tree
        void DestroyTree();

        // Caches the current selection and expansion state
        void CacheSelectionAndExpansionState();

        // Tries to restore the current selection and expansion state
        void RestoreCachedSelectionAndExpansionState();

        void SetSelectionInternal( TreeListViewItem* pItem, bool updateView );
        void AddToSelectionInternal( TreeListViewItem* pItem, bool updateView );
        void AddToSelectionInternal( TVector<TreeListViewItem*> const& newSelection, bool updateView );
        void SetSelectionInternal( TVector<TreeListViewItem*> const& newSelection, bool updateView );
        void RemoveFromSelectionInternal( TreeListViewItem* pItem, bool updateView );

        void DrawVisualItem( TreeListViewContext context, int32_t visualItemIdx );
        void TryAddItemToVisualTree( TreeListViewItem* pItem, int32_t hierarchyLevel );

        void RebuildVisualTree();

        int32_t GetVisualTreeItemIndex( TreeListViewItem const* pBaseItem ) const;
        int32_t GetVisualTreeItemIndex( uint64_t uniqueID ) const;

    private:

        // The root of the tree - fill this with your items
        TreeRootItem                                            m_rootItem;

        // The active item is an item that is activated (and deactivated) via a double click
        TreeListViewItem*                                       m_pActiveItem = nullptr;

        // Control tree view behavior
        TBitFlags<Flags>                                        m_flags;

        TVector<VisualTreeItem>                                 m_visualTree;
        VisualTreeState                                         m_visualTreeState = VisualTreeState::NeedsRebuild;
        float                                                   m_estimatedRowHeight = -1.0f;
        float                                                   m_estimatedTreeHeight = -1.0f;
        int32_t                                                 m_currentFirstVisibleRowIdx = InvalidIndex;
        int32_t                                                 m_currentLastVisibleRowIdx = InvalidIndex;
        int32_t                                                 m_requestedFirstVisibleRowItemIdx = 0;
        float                                                   m_itemControlColumnWidth = 0;
        bool                                                    m_isDrawingTree = false;

        // The currently selected item (changes frequently due to clicks/focus/etc...) - In order of selection time, first is oldest
        TVector<TreeListViewItem*>                              m_selection;
        uint64_t                                                m_cachedActiveItemID = 0;
        TVector<uint64_t>                                       m_selectedItemIDs;
        TVector<uint64_t>                                       m_originalExpandedItems;
    };
}