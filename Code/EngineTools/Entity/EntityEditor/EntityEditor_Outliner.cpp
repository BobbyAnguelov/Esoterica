#include "EntityEditor_Outliner.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityMap.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class OutlinerItem : public TreeListViewItem
    {
    public:

        enum class Type
        {
            Invalid,
            MapItem,
            GroupItem,
            EntityItem
        };

        constexpr static char const* const s_dragAndDropID = "OutlinerItem";

    public:

        inline bool IsMap() const { return m_type == Type::MapItem; }
        inline bool IsGroup() const { return m_type == Type::GroupItem; }
        inline bool IsEntity() const { return m_type == Type::EntityItem; }
        virtual bool HasContextMenu() const override { return true; }

        virtual void SetDragAndDropPayloadData() const override
        {
            uintptr_t itemAddress = uintptr_t( this );
            ImGui::SetDragDropPayload( s_dragAndDropID, &itemAddress, sizeof( uintptr_t ) );
        }

    protected:

        OutlinerItem( TreeListViewItem* pParent, Type type ) : TreeListViewItem( pParent ), m_type( type ) {}

    public:

        String                  m_sortingKey;

    private:

        Type                    m_type = Type::Invalid;
    };

    //-------------------------------------------------------------------------

    class MapItem : public OutlinerItem
    {

    public:

        MapItem( TreeListViewItem* pParent, EntityMapID mapID, StringID name )
            : OutlinerItem( pParent, OutlinerItem::Type::MapItem )
            , m_ID( mapID )
            , m_name( name )
        {
            m_uniqueID = Hash::GetHash64( &mapID, sizeof( mapID ) );
            m_sortingKey.sprintf( "_/%s", m_name.c_str() );
        }

        virtual uint64_t GetUniqueID() const override { return m_uniqueID; }
        virtual char const* GetSortingKey() const override { return m_sortingKey.c_str(); }
        virtual bool HasIcon() const override { return true; }
        virtual char const* GetIcon() const override { return EE_ICON_MAP; }
        virtual char const* GetDisplayName() const override { return m_name.c_str(); }
        virtual bool IsDragAndDropTarget() const override { return true; }
        virtual bool IsDragAndDropSource() const override { return false; }

    public:

        uint64_t                m_uniqueID = 0;
        EntityMapID             m_ID;
        StringID                m_name;
        String                  m_sortingKey;
    };

    //-------------------------------------------------------------------------

    class GroupItem : public OutlinerItem
    {
    public:

        GroupItem( TreeListViewItem* pParent, EntityMapID mapID, StringID groupID, String const& name )
            : OutlinerItem( pParent, OutlinerItem::Type::GroupItem )
            , m_mapID( mapID )
            , m_groupID( groupID )
            , m_name( name )
        {
            m_sortingKey.sprintf( "A/%s", groupID.c_str() );
        }

        virtual uint64_t GetUniqueID() const override { return m_groupID.ToUint(); }
        virtual char const* GetSortingKey() const override { return m_sortingKey.c_str(); }
        virtual bool HasIcon() const override { return true; }
        virtual char const* GetIcon() const override { return EE_ICON_FOLDER; }
        virtual char const* GetDisplayName() const override { return m_name.c_str(); }
        virtual bool IsDragAndDropTarget() const override { return true; }
        virtual bool IsDragAndDropSource() const override { return true; }

    public:

        EntityMapID             m_mapID;
        StringID                m_groupID;
        String                  m_name;
        String                  m_sortingKey;
    };

    //-------------------------------------------------------------------------

    class EntityItem : public OutlinerItem
    {

    public:

        EntityItem( TreeListViewItem* pParent, Entity* pEntity )
            : OutlinerItem( pParent, OutlinerItem::Type::EntityItem )
            , m_data( pEntity )
        {
            m_sortingKey.sprintf( "X/%s", m_data.m_pEntity->GetNameID().c_str() );
        }

        inline bool IsSpatialEntity() const { return IsEntity() && m_data.IsSpatialEntity(); }

        virtual uint64_t GetUniqueID() const override { return m_data.GetUniqueID(); }
        virtual char const* GetSortingKey() const override { return m_sortingKey.c_str(); }
        virtual bool HasIcon() const override { return true; }
        virtual char const* GetIcon() const override { return m_data.IsSpatialEntity() ? EE_ICON_AXIS_ARROW : EE_ICON_SPHERE; }
        virtual Color GetIconColor() const override { return m_data.IsSpatialEntity() ? Colors::Lime : Colors::LightBlue; }
        virtual char const* GetDisplayName() const override { return m_data.m_pEntity->GetNameID().c_str(); }
        virtual bool IsDragAndDropTarget() const override { return true; }
        virtual bool IsDragAndDropSource() const override { return true; }

    public:

        EntityEditorItem        m_data;
        String                  m_sortingKey;
    };

    //-------------------------------------------------------------------------

    EntityOutliner::EntityOutliner( EditorContext& editorContext )
        : m_editorContext( editorContext )
    {
        // Tree
        //-------------------------------------------------------------------------

        m_treeView.SetFlag( TreeListView::Flags::ExpandItemsOnlyViaArrow );
        m_treeView.SetFlag( TreeListView::Flags::MultiSelectionAllowed );
        m_treeView.SetFlag( TreeListView::Flags::SortTree );
        m_treeView.SetFlag( TreeListView::Flags::TrackSelection );

        m_treeContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildOutlinerTree( pRootItem ); };
        m_treeContext.m_drawItemContextMenuFunction = [this] ( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) { DrawContextMenu( selectedItemsWithContextMenus ); };
        m_treeContext.m_onDragAndDropFunction = [this] ( TreeListViewItem* pDropTarget ) { HandleDragAndDrop( pDropTarget ); };

        //-------------------------------------------------------------------------

        auto SetupColumnHeaders = []
        {
            ImGui::TableSetupColumn( "##Override", ImGuiTableColumnFlags_WidthFixed, 75 );
        };

        m_treeContext.m_numExtraColumns = 1;
        m_treeContext.m_drawItemExtraColumnsFunction = [this] ( TreeListViewItem* pBaseItem, int32_t extraColumnIdx ) { DrawExtraColumns( pBaseItem, extraColumnIdx ); };
        m_treeContext.m_setupExtraColumnHeadersFunction = SetupColumnHeaders;

        // Events
        //-------------------------------------------------------------------------

        m_preWorldChangedEventID = m_editorContext.OnPreWorldChange().Bind( [this] () { PreWorldChange(); } );
        m_postWorldChangedEventID = m_editorContext.OnPostWorldChange().Bind( [this] () { PostWorldChange(); } );
        m_selectionChangedEventID = m_editorContext.OnSelectionChanged().Bind( [this] () { OnSelectionChanged(); } );
    }

    EntityOutliner::~EntityOutliner()
    {
        m_editorContext.OnPreWorldChange().Unbind( m_preWorldChangedEventID );
        m_editorContext.OnPostWorldChange().Unbind( m_postWorldChangedEventID );
        m_editorContext.OnSelectionChanged().Unbind( m_selectionChangedEventID );
    }

    void EntityOutliner::PreWorldChange()
    {}

    void EntityOutliner::PostWorldChange()
    {
        if ( m_state == InternalState::UpToDate )
        {
            m_state = InternalState::NeedsRebuild;
        }
    }

    void EntityOutliner::UpdateAndDraw( UpdateContext const& context, bool isFocused )
    {
        // Update
        //-------------------------------------------------------------------------

        if ( m_state == InternalState::UpToDate && m_editorContext.HasPendingEntityStateChangeActions() )
        {
            m_state = InternalState::NeedsRebuild;
        }

        if ( !m_editorContext.HasPendingEntityStateChangeActions() )
        {
            EntityMap* pMap = m_editorContext.GetEditedMap();
            if ( pMap == nullptr )
            {
                m_state = InternalState::UpToDate;
            }
            else if ( m_state != InternalState::UpToDate && pMap->IsMapLoaded() )
            {
                m_treeView.RebuildTree( m_treeContext, m_state == InternalState::NeedsRebuild );

                if ( m_state == InternalState::NeedsRebuildAndSelectionUpdate )
                {
                    UpdateSelection();
                }

                m_state = InternalState::UpToDate;
            }
        }

        // Draw
        //-------------------------------------------------------------------------

        DrawTree( isFocused );

        // Handle Input
        //-------------------------------------------------------------------------

        if ( isFocused )
        {
            HandleInput();
        }

        // Process deferred commands
        //-------------------------------------------------------------------------

        m_commandStack.ExecuteCommands();
    }

    //-------------------------------------------------------------------------
    // Drag and Drop
    //-------------------------------------------------------------------------

    void EntityOutliner::HandleDragAndDrop( TreeListViewItem* pTarget )
    {
        if ( m_isReadOnly )
        {
            return;
        }

        OutlinerItem* pDropTargetItem = (OutlinerItem*) pTarget;

        // Dragged a group
        //-------------------------------------------------------------------------

        if ( ImGuiPayload const* pPayload = ImGui::AcceptDragDropPayload( OutlinerItem::s_dragAndDropID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            bool isValidDropOperation = true;

            OutlinerItem* pDraggedItem = (OutlinerItem*) *( (uintptr_t*) pPayload->Data );
            if ( pDraggedItem == pDropTargetItem )
            {
                isValidDropOperation = false;
            }

            //-------------------------------------------------------------------------

            TInlineVector<EntityItem*, 20> draggedEntities;
            TInlineVector<GroupItem*, 20> draggedGroups;
            SplitSelection( draggedEntities, draggedGroups );

            if ( pDropTargetItem->IsEntity() && !draggedGroups.empty() )
            {
                isValidDropOperation = false;
            }

            //-------------------------------------------------------------------------

            if ( isValidDropOperation )
            {
                if ( pPayload->IsDelivery() )
                {
                    m_commandStack.PushCommand( [this, pDropTargetItem] () { ExecuteDragAndDrop( pDropTargetItem ); } );
                }
            }
            else
            {
                ImGui::SetMouseCursor( ImGuiMouseCursor_NotAllowed );
                ImGui::SetTooltip( "Cannot drop here!" );
            }
        }
    }

    void EntityOutliner::ExecuteDragAndDrop( OutlinerItem* pTargetItem )
    {
        TInlineVector<EntityItem*, 20> draggedEntities;
        TInlineVector<GroupItem*, 20> draggedGroups;
        SplitSelection( draggedEntities, draggedGroups );

        if ( draggedEntities.empty() && draggedGroups.empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( pTargetItem->IsMap() )
        {
            auto pTargetMapItem = static_cast<MapItem*>( pTargetItem );
            DropOnMap( pTargetMapItem, draggedEntities, draggedGroups );
        }
        else if ( pTargetItem->IsGroup() )
        {
            auto pTargetGroupItem = static_cast<GroupItem*>( pTargetItem );
            DropOnGroup( pTargetGroupItem, draggedEntities, draggedGroups );
        }
        else if ( pTargetItem->IsEntity() )
        {
            auto pTargetEntityItem = static_cast<EntityItem*>( pTargetItem );
            EE_ASSERT( draggedGroups.empty() );
            DropOnEntity( pTargetEntityItem, draggedEntities );
        }
    }

    void EntityOutliner::DropOnMap( MapItem* pDropTarget, TInlineVector<EntityItem*, 20> const& draggedEntities, TInlineVector<GroupItem*, 20>const& draggedGroups )
    {
        TInlineVector<Entity*, 10> validEntityParentingRequests;
        for ( auto& pItem : draggedEntities )
        {
            validEntityParentingRequests.emplace_back( pItem->m_data.m_pEntity );
        }

        // Reparent
        //-------------------------------------------------------------------------

        if ( ( draggedGroups.size() + validEntityParentingRequests.size() ) > 0 )
        {
            m_editorContext.BeginCompoundUndoableAction();
            for ( auto& pGroupItem : draggedGroups )
            {
                m_editorContext.ReparentGroup( pDropTarget->m_ID, pGroupItem->m_groupID, StringID() );
            }

            m_editorContext.AddOrMoveEntitiesToGroup( pDropTarget->m_ID, StringID(), validEntityParentingRequests );
            m_editorContext.EndCompoundUndoableAction();
        }
    }

    void EntityOutliner::DropOnGroup( GroupItem* pTargetGroupItem, TInlineVector<EntityItem*, 20> const& draggedEntities, TInlineVector<GroupItem*, 20>const& draggedGroups )
    {
        // Filter list
        //-------------------------------------------------------------------------

        TInlineVector<StringID, 10> validGroupParentingRequests;
        for ( GroupItem const* pNewChildItem : draggedGroups )
        {
            // Already a child
            if ( EntityEditorItemGroup::IsDirectParentOf( pTargetGroupItem->m_groupID, pNewChildItem->m_groupID ) )
            {
                continue;
            }

            // Cant parent to a child
            if ( EntityEditorItemGroup::IsParentOf( pNewChildItem->m_groupID, pTargetGroupItem->m_groupID ) )
            {
                continue;
            }

            validGroupParentingRequests.emplace_back( pNewChildItem->m_groupID );
        }

        TInlineVector<Entity*, 10> validEntityParentingRequests;
        for ( auto& pItem : draggedEntities )
        {
            Entity* pNewChild = pItem->m_data.m_pEntity;

            // Already a child
            if ( m_editorContext.DoesGroupContainEntity( pTargetGroupItem->m_mapID, pTargetGroupItem->m_groupID, pNewChild, false ) )
            {
                continue;
            }

            validEntityParentingRequests.emplace_back( pNewChild );
        }

        // Reparent
        //-------------------------------------------------------------------------

        if ( ( validGroupParentingRequests.size() + validEntityParentingRequests.size() ) > 0 )
        {
            m_editorContext.BeginCompoundUndoableAction();
            for ( auto& groupID : validGroupParentingRequests )
            {
                m_editorContext.ReparentGroup( pTargetGroupItem->m_mapID, groupID, pTargetGroupItem->m_groupID );
            }

            m_editorContext.AddOrMoveEntitiesToGroup( pTargetGroupItem->m_mapID, pTargetGroupItem->m_groupID, validEntityParentingRequests );
            m_editorContext.EndCompoundUndoableAction();
        }
    }

    void EntityOutliner::DropOnEntity( EntityItem* pTargetEntityItem, TInlineVector<EntityItem*, 20> const& draggedEntities )
    {
        Entity* pParentEntity = pTargetEntityItem->m_data.m_pEntity;

        // Filter list
        //-------------------------------------------------------------------------

        TInlineVector<Entity*, 10> validParentingRequests;

        for ( auto& pItem : draggedEntities )
        {
            Entity* pNewChild = pItem->m_data.m_pEntity;

            // Already a child
            if ( pParentEntity->IsDirectSpatialParentOf( pNewChild ) )
            {
                continue;
            }

            // Cant parent to a child
            if ( pParentEntity->IsSpatialChildOf( pNewChild ) )
            {
                continue;
            }

            validParentingRequests.emplace_back( pNewChild );
        }

        // Reparent
        //-------------------------------------------------------------------------

        if ( !validParentingRequests.empty() )
        {
            m_editorContext.BeginCompoundUndoableAction();
            for ( auto& pEntity : validParentingRequests )
            {
                m_editorContext.ReparentEntity( pEntity, pParentEntity );
            }
            m_editorContext.EndCompoundUndoableAction();
        }
    }

    //-------------------------------------------------------------------------
    // Selection
    //-------------------------------------------------------------------------

    void EntityOutliner::OnSelectionChanged()
    {
        // We are gonna rebuild the tree, so no need to switch selection atm
        if ( m_state != InternalState::UpToDate )
        {
            m_state = InternalState::NeedsRebuildAndSelectionUpdate;
            return;
        }

        //-------------------------------------------------------------------------

        UpdateSelection();
    }

    void EntityOutliner::UpdateSelection()
    {
        TVector<EntityEditorItem> const& selection = m_editorContext.GetSelection();

        // Update Tree
        //-------------------------------------------------------------------------

        TVector<TreeListViewItem*> treeSelection;
        for ( EntityEditorItem const& selectedData : selection )
        {
            TreeListViewItem* pFoundItem = m_treeView.FindItem( selectedData.GetUniqueID() );
            if ( pFoundItem != nullptr )
            {
                treeSelection.emplace_back( pFoundItem );
            }
        }

        m_treeView.SetSelection( treeSelection );
    }

    void EntityOutliner::SplitSelection( TInlineVector<EntityItem*, 20>& outSelectedEntities, TInlineVector<GroupItem*, 20>& outSelectedGroups )
    {
        outSelectedEntities.clear();
        outSelectedGroups.clear();

        auto const& selection = m_treeView.GetSelection();
        if ( selection.empty() )
        {
            return;
        }

        int32_t const numItems = (int32_t) selection.size();
        for ( int32_t i = 0; i < numItems; i++ )
        {
            OutlinerItem* pItem = static_cast<OutlinerItem*>( selection[i] );
            if ( pItem->IsEntity() )
            {
                EntityItem* pEntityItem = static_cast<EntityItem*>( selection[i] );
                outSelectedEntities.emplace_back( pEntityItem );
            }
            else if ( pItem->IsGroup() )
            {
                GroupItem* pGroupItem = static_cast<GroupItem*>( selection[i] );
                outSelectedGroups.emplace_back( pGroupItem );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Tree
    //-------------------------------------------------------------------------

    void EntityOutliner::RebuildOutlinerTree( TreeListViewItem* pRootItem )
    {
        EntityMap* pMap = m_editorContext.GetEditedMap();
        if ( pMap == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto pMapItem = pRootItem->CreateChild<MapItem>( pMap->GetID(), pMap->GetName() );
        pMapItem->SetExpanded( true );

        //-------------------------------------------------------------------------

        CategoryTree<EntityItem*> categoryTree;
        TInlineVector<Entity*,200> alreadyAddedEntities;

        auto pGroups = m_editorContext.GetGroupsForMap( pMap );
        if ( pGroups != nullptr )
        {
            for ( auto const& group : *pGroups )
            {
                EE_ASSERT( group.m_ID.IsValid() );
                categoryTree.AddCategory( group.m_ID.c_str() );

                for ( auto const& item : group.m_items )
                {
                    EntityItem* pEntityItem = EE::New<EntityItem>( nullptr, item.m_pEntity );
                    categoryTree.AddItem( group.m_ID.c_str(), item.m_pEntity->GetNameID().c_str(), pEntityItem, false );
                    alreadyAddedEntities.emplace_back( item.m_pEntity );
                }
            }
        }

        for ( auto pEntity : pMap->GetEntities() )
        {
            // Children are handled as part of their parents
            if ( pEntity->HasSpatialParent() )
            {
                continue;
            }

            if ( VectorContains( alreadyAddedEntities, pEntity ) )
            {
                continue;
            }

            EntityItem* pEntityItem = EE::New<EntityItem>( nullptr, pEntity );
            categoryTree.AddItem( "", pEntity->GetNameID().c_str(), pEntityItem, false );
        }

        CreateEntityGroupRecursive( pMap, pMapItem, categoryTree.m_rootCategory );
    }

    void EntityOutliner::CreateEntityGroupRecursive( EntityMap* pMap, TreeListViewItem* pParentItem, Category<EntityItem*> const& category )
    {
        EE_ASSERT( pMap != nullptr );

        InlineString fullGroupID( category.m_fullPath.c_str() );
        if ( !fullGroupID.empty() )
        {
            fullGroupID.pop_back();
        }

        auto pGroupItem = category.m_name.empty() ? pParentItem : pParentItem->CreateChild<GroupItem>( pMap->GetID(), StringID( fullGroupID.c_str() ), category.m_name );
        pGroupItem->SetExpanded( true, true );

        // Create sub-groups
        for ( auto const& subCategory : category.m_childCategories )
        {
            CreateEntityGroupRecursive( pMap, pGroupItem, subCategory );
        }

        // Add entities
        for ( CategoryItem<EntityItem*> const& item : category.m_items )
        {
            EntityItem* pEntityItem = item.m_data;
            pGroupItem->AddNewChild( pEntityItem );

            if ( pEntityItem->m_data.m_pEntity->HasAttachedEntities() )
            {
                for ( auto pChildEntity : pEntityItem->m_data.m_pEntity->GetAttachedEntities() )
                {
                    CreateEntityItemRecursive( pEntityItem, pChildEntity );
                }

                pEntityItem->SetExpanded( true, true );
            }
        }
    }

    void EntityOutliner::CreateEntityItemRecursive( TreeListViewItem* pParentItem, Entity* pEntity )
    {
        auto pEntityItem = pParentItem->CreateChild<EntityItem>( pEntity );

        // Entities
        //-------------------------------------------------------------------------

        if ( pEntity->HasAttachedEntities() )
        {
            for ( auto pChildEntity : pEntity->GetAttachedEntities() )
            {
                CreateEntityItemRecursive( pEntityItem, pChildEntity );
            }

            pEntityItem->SetExpanded( true, true );
        }
    }

    void EntityOutliner::DrawTree( bool isFocused )
    {
        EntityMap* pMap = m_editorContext.GetEditedMap();

        if ( m_state != InternalState::UpToDate )
        {
            ImGuiX::DrawSpinner( "Map Actions Still Pending..." );
        }
        else if ( pMap == nullptr )
        {
            ImGui::Text( "Nothing To Outline." );
        }
        else // Draw outliner
        {
            if ( !pMap->IsMapLoaded() )
            {
                ImGui::Indent();
                ImGuiX::DrawSpinner( "LoadIndicator" );
                ImGui::SameLine( 0, 10 );
                ImGui::Text( "Map Loading - Please Wait" );
                ImGui::Unindent();
            }
            else // Valid world and map
            {
                {
                    ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );
                    ImGui::BeginDisabled( m_isReadOnly );
                    if ( ImGuiX::ButtonColored( "CREATE NEW ENTITY", Colors::Green, Colors::White ) )
                    {
                        auto pCreatedEntity = m_editorContext.CreateEntity( pMap );
                        m_editorContext.SetSelection( pCreatedEntity );
                    }
                    ImGui::EndDisabled();
                }

                {
                    ImGui::SameLine();
                    if ( m_filter.UpdateAndDraw() )
                    {
                        auto UpdateVisibility = [this] ( TreeListViewItem const* pItem ) -> bool
                        {
                            if ( m_filter.HasFilterSet() )
                            {
                                auto pOutlinerItem = static_cast<OutlinerItem const*>( pItem );
                                return m_filter.MatchesFilter( pOutlinerItem->GetDisplayName() );
                            }

                            return true;
                        };

                        m_treeView.UpdateItemVisibility( UpdateVisibility );
                    }
                }

                //-------------------------------------------------------------------------

                if ( m_treeView.UpdateAndDraw( m_treeContext ) )
                {
                    if ( m_treeView.GetSelection().empty() )
                    {
                        m_editorContext.ClearSelection();
                    }
                    else // Set selection to match tree
                    {
                        TInlineVector<EntityEditorItem, 5> selectedTypes;
                        auto const& selection = m_treeView.GetSelection();
                        for ( auto pItem : selection )
                        {
                            auto pOutlinerItem = static_cast<OutlinerItem const*>( pItem );
                            if ( pOutlinerItem->IsEntity() )
                            {
                                auto pEntityItem = static_cast<EntityItem*>( pItem );
                                EE_ASSERT( pEntityItem->m_data.m_pEntity != nullptr );
                                selectedTypes.emplace_back( pEntityItem->m_data.m_pEntity );
                            }
                        }

                        m_editorContext.SetSelection( selectedTypes );
                    }
                }
            }
        }
    }

    void EntityOutliner::DrawExtraColumns( TreeListViewItem* pBaseItem, int32_t extraColumnIdx )
    {
        auto pOutlinerItem = (OutlinerItem*) pBaseItem;
        if ( extraColumnIdx == 0 )
        {
            // Do Nothing
        }
    }

    //-------------------------------------------------------------------------
    // Context Menu
    //-------------------------------------------------------------------------

    void EntityOutliner::DrawContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus )
    {
        if ( m_isReadOnly )
        {
            return;
        }

        //-------------------------------------------------------------------------

        bool const multipleSelected = selectedItemsWithContextMenus.size() > 1;
        if ( !multipleSelected )
        {
            auto pItem = (OutlinerItem*) selectedItemsWithContextMenus[0];
            if ( pItem->IsEntity() )
            {
                auto pEntityItem = static_cast<EntityItem*>( pItem );
                EE_ASSERT( pEntityItem->m_data.m_pEntity != nullptr );

                if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename" ) )
                {
                    m_editorContext.ShowEntityRenameDialog( pEntityItem->m_data.m_pEntity );
                }

                if ( pEntityItem->m_data.m_pEntity->HasSpatialParent() )
                {
                    if ( ImGui::MenuItem( EE_ICON_CLOSE" Detach From Parent" ) )
                    {
                        m_editorContext.ReparentEntity( pEntityItem->m_data.m_pEntity, nullptr );
                    }
                }
            }
            else if ( pItem->IsGroup() )
            {
                auto pGroupItem = static_cast<GroupItem*>( pItem );
                EE_ASSERT( pGroupItem->m_groupID.IsValid() );

                if ( ImGui::MenuItem( EE_ICON_SPHERE" Create Entity" ) )
                {
                    m_editorContext.CreateEntity( pGroupItem->m_mapID, pGroupItem->m_groupID );
                }

                if ( ImGui::MenuItem( EE_ICON_FOLDER" Create Group" ) )
                {
                    m_editorContext.ShowEntityGroupCreateDialog( pGroupItem->m_mapID, pGroupItem->m_groupID );
                }

                if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename" ) )
                {
                    m_editorContext.ShowEntityGroupRenameDialog( pGroupItem->m_mapID, pGroupItem->m_groupID );
                }
            }
            else if ( pItem->IsMap() )
            {
                auto pMapItem = static_cast<MapItem*>( pItem );

                if ( ImGui::MenuItem( EE_ICON_FOLDER" Create Group" ) )
                {
                    m_editorContext.ShowEntityGroupCreateDialog( pMapItem->m_ID );
                }
            }
        }

        if ( ImGui::MenuItem( multipleSelected ? EE_ICON_DELETE" Delete Selected" : EE_ICON_DELETE" Delete" ) )
        {
            m_commandStack.PushCommand( [this] () { DeleteSelected(); } );
        }
    }

    //-------------------------------------------------------------------------

    void EntityOutliner::HandleInput()
    {
        auto const& selection = m_treeView.GetSelection();

        if ( selection.empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( ImGui::IsKeyPressed( ImGuiKey_F2 ) )
        {
            if ( !m_isReadOnly )
            {
                auto pSelectedItem = static_cast<OutlinerItem*>( selection.back() );
                if ( pSelectedItem->IsEntity() )
                {
                    auto pEntityItem = static_cast<EntityItem*>( pSelectedItem );
                    EE_ASSERT( pEntityItem->m_data.m_pEntity != nullptr );
                    m_editorContext.ShowEntityRenameDialog( pEntityItem->m_data.m_pEntity );
                }
                else if ( pSelectedItem->IsGroup() )
                {
                    auto pGroupItem = static_cast<GroupItem*>( pSelectedItem );
                    EE_ASSERT( pGroupItem->m_groupID.IsValid() );
                    m_editorContext.ShowEntityGroupRenameDialog( pGroupItem->m_mapID, pGroupItem->m_groupID );
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( ImGui::IsKeyPressed( ImGuiKey_Delete ) )
        {
            if ( !m_isReadOnly )
            {
                m_commandStack.PushCommand( [this] () { DeleteSelected(); } );
            }
        }

        //-------------------------------------------------------------------------

        ImGuiKeyChord const copyChord = ImGuiMod_Ctrl | ImGuiKey_C;
        ImGuiKeyChord const pasteChord = ImGuiMod_Ctrl | ImGuiKey_V;

        if ( ImGui::IsKeyChordPressed( copyChord ) )
        {
            if ( !m_isReadOnly )
            {
                m_commandStack.PushCommand( [this] () { CopySelected(); } );
            }
        }

        if ( ImGui::IsKeyChordPressed( pasteChord ) )
        {
            if ( !m_isReadOnly )
            {
                m_commandStack.PushCommand( [this] () { Paste(); } );
            }
        }

        //-------------------------------------------------------------------------

        if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
        {
            auto pSelectedItem = static_cast<OutlinerItem*>( selection.back() );
            if ( pSelectedItem->IsEntity() )
            {
                auto pEntityItem = static_cast<EntityItem*>( pSelectedItem );
                EE_ASSERT( pEntityItem->m_data.m_pEntity != nullptr );
                m_editorContext.FocusCamera( pEntityItem->m_data.m_pEntity );
            }
        }
    }

    //-------------------------------------------------------------------------

    void EntityOutliner::DeleteSelected()
    {
        TInlineVector<EntityItem*, 20> draggedEntities;
        TInlineVector<GroupItem*, 20> draggedGroups;
        SplitSelection( draggedEntities, draggedGroups );

        if ( draggedEntities.empty() && draggedGroups.empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        m_editorContext.BeginCompoundUndoableAction();

        TVector<Entity*> entitiesToDelete;
        for ( EntityItem* pItem : draggedEntities )
        {
            entitiesToDelete.emplace_back( pItem->m_data.m_pEntity );
        }
        m_editorContext.DestroyEntities( entitiesToDelete );

        for ( GroupItem* pItem : draggedGroups )
        {
            m_editorContext.DestroyGroup( pItem->m_mapID, pItem->m_groupID );
        }

        m_editorContext.EndCompoundUndoableAction();
    }

    void EntityOutliner::CopySelected()
    {
        // TODO
    }

    void EntityOutliner::Paste()
    {
        // TODO
    }
}