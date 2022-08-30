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
        virtual StringID GetNameID() const = 0;

        // The friendly display name printed in the UI (generally the same as the nameID)
        virtual char const* GetDisplayName() const 
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

        virtual bool SupportsDragAndDrop() { return false; }
        virtual char const* GetDragAndDropPayloadID() { return nullptr; }
        virtual TPair<void*, size_t> GetDragAndDropPayload() const { return TPair<void*, size_t>( nullptr, 0 ); }

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

    public:

        TreeListView() = default;
        virtual ~TreeListView();

        // Visual
        //-------------------------------------------------------------------------

        void Draw();
        inline void RefreshVisualState() { m_visualTreeState = VisualTreeState::NeedsRebuild; }

        // Selection, Activation and Events
        //-------------------------------------------------------------------------

        // Clear the current selection
        inline void ClearSelection() { m_selection.clear(); m_onSelectionChanged.Execute(); }

        // Get the current selection
        inline TVector<TreeListViewItem*> GetSelection() const { return m_selection; }

        // Fire whenever the selection changes
        inline TEventHandle<> OnSelectedChanged() { return m_onSelectionChanged; }

        // Clear active item
        inline void ClearActiveItem() { m_pActiveItem = nullptr; m_onActiveItemChanged.Execute(); }

        inline TreeListViewItem* GetActiveItem() const { return m_pActiveItem; }

        // Fires whenever the active item changes, parameter is the new active item (can be null)
        inline TEventHandle<> OnActiveItemChanged() { return m_onActiveItemChanged; }

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

        inline int32_t GetNumItems() const { return (int32_t) m_visualTree.size(); }

        TreeListViewItem* FindItem( uint64_t uniqueID );

        void DestroyItem( uint64_t uniqueID );

        //-------------------------------------------------------------------------

        // User overrideable function to draw any addition windows/dialogs that might be needed
        virtual void DrawAdditionalUI() {}

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

        // Implement this to rebuild the tree, the root item will have already been destroyed at this point!
        // DO NOT CALL THIS DIRECTLY!
        virtual void RebuildTreeInternal() = 0;

        // Select just this item
        void SelectItem( TreeListViewItem* pItem );

        // Add to the current selection
        void AddToSelection( TreeListViewItem* pItem );

        // Remove an item from the current selection
        void RemoveFromSelection( TreeListViewItem* pItem );

        // Override this to handle selection changes
        virtual void OnSelectionChangedInternal() {}

        // Override this to disable tree sorting
        virtual bool ShouldSortTree() const { return true; }

        // Override this to change the default sorting approach (by item name)
        virtual void SortItemChildren( TreeListViewItem* pItem );

    private:

        void HandleItemSelection( TreeListViewItem* pItem, bool isSelected );

        void DrawVisualItem( VisualTreeItem& visualTreeItem );
        void TryAddItemToVisualTree( TreeListViewItem* pItem, int32_t hierarchyLevel );

        void RebuildVisualTree();
        void OnItemDoubleClickedInternal( TreeListViewItem* pItem );

        void NotifySelectionChanged()
        {
            OnSelectionChangedInternal();
            m_onSelectionChanged.Execute();
        }

    protected:

        // The root of the tree - fill this with your items
        TreeRootItem                                            m_rootItem;

        TEvent<>                                                m_onSelectionChanged;
        TEvent<>                                                m_onActiveItemChanged;
        TEvent<TreeListViewItem*>                               m_onItemDoubleClicked;

        // The active item is an item that is activated (and deactivated) via a double click
        TreeListViewItem*                                       m_pActiveItem = nullptr;

        // The currently selected item (changes frequently due to clicks/focus/etc...)
        TVector<TreeListViewItem*>                              m_selection;

        // Control tree view behavior
        bool                                                    m_expandItemsOnlyViaArrow = false;
        bool                                                    m_multiSelectionAllowed = false;

    private:

        TVector<VisualTreeItem>                                 m_visualTree;
        VisualTreeState                                         m_visualTreeState = VisualTreeState::None;
        float                                                   m_estimatedRowHeight = -1.0f;
        float                                                   m_estimatedTreeHeight = -1.0f;
        int32_t                                                 m_firstVisibleRowItemIdx = 0;
        float                                                   m_itemControlColumnWidth = 0;
        bool                                                    m_maintainVisibleRowIdx = false;
    };
}