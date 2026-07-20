#include "EntityEditor_EntityInspector.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class InspectorItem final : public TreeListViewItem
    {
    public:

        constexpr static char const* const s_dragAndDropID = "InspectorItem";

        static void Create( TreeListViewItem* pParentItem, Entity* pEntity )
        {
            auto pEntityItem = pParentItem->CreateChild<InspectorItem>( pEntity );

            // Components
            //-------------------------------------------------------------------------

            if ( !pEntity->GetComponents().empty() )
            {
                TInlineVector<SpatialEntityComponent*, 20> spatialComponents;

                for ( auto pComponent : pEntity->GetComponents() )
                {
                    if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent ) )
                    {
                        spatialComponents.emplace_back( pSpatialComponent );
                    }
                    else
                    {
                        pEntityItem->CreateChild<InspectorItem>( pEntity, pComponent );
                    }
                }

                if ( !spatialComponents.empty() )
                {
                    // Create items
                    TInlineVector<InspectorItem*, 20> spatialComponentItems;
                    for ( auto pComponent : spatialComponents )
                    {
                        spatialComponentItems.emplace_back( EE::New<InspectorItem>( nullptr, pEntity, pComponent ) );
                        spatialComponentItems.back()->SetExpanded( true, true );
                    }

                    // Fix parenting
                    auto FindIndex = [&] ( ComponentID const& ID )
                    {
                        for ( int32_t i = 0; i < (int32_t) spatialComponents.size(); i++ )
                        {
                            if ( spatialComponents[i]->GetID() == ID )
                            {
                                return i;
                            }
                        }

                        return InvalidIndex;
                    };

                    for ( int32_t i = 0; i < (int32_t) spatialComponents.size(); i++ )
                    {
                        if ( !spatialComponents[i]->IsRootComponent() && spatialComponents[i]->HasSpatialParent() )
                        {
                            ComponentID const parentID = spatialComponents[i]->GetSpatialParentID();
                            int32_t const parentIdx = FindIndex( parentID );
                            EE_ASSERT( parentIdx != InvalidIndex );
                            spatialComponentItems[parentIdx]->AddNewChild( spatialComponentItems[i] );
                        }
                        else
                        {
                            pEntityItem->AddNewChild( spatialComponentItems[i] );
                        }
                    }
                }
            }

            // Systems
            //-------------------------------------------------------------------------

            if ( !pEntity->GetSystems().empty() )
            {
                for ( auto pSystem : pEntity->GetSystems() )
                {
                    pEntityItem->CreateChild<InspectorItem>( pEntity, pSystem );
                }
            }
        }

    public:

        InspectorItem( TreeListViewItem* pParent, Entity* pEntity )
            : TreeListViewItem( pParent )
            , m_data( pEntity )
        {
            m_sortingKey.sprintf( "X/%s", m_data.m_pEntity->GetNameID().c_str() );
        }

        InspectorItem( TreeListViewItem* pParent, Entity* pEntity, EntityComponent* pEntityComponent )
            : TreeListViewItem( pParent )
            , m_data( pEntity, pEntityComponent )
        {
            m_sortingKey.sprintf( "Components/%s/%s", m_data.m_isSpatialComponent ? "0" : "1", m_data.m_pComponent->GetNameID().c_str() );
        }

        InspectorItem( TreeListViewItem* pParent, Entity* pEntity, EntitySystem* pEntitySystem )
            : TreeListViewItem( pParent )
            , m_data( pEntity, pEntitySystem )
        {
            m_sortingKey.sprintf( "Systems/%s", m_data.m_pSystem->GetTypeInfo()->m_friendlyName.c_str() );
        }

        inline bool IsEntity() const { return m_data.IsEntity(); }
        inline bool IsSpatialEntity() const { return m_data.IsSpatialEntity(); }
        inline bool IsComponent() const { return m_data.IsComponent(); }
        inline bool IsSpatialComponent() const { return m_data.IsSpatialComponent(); }
        inline bool IsSystem() const { return m_data.IsSystem(); }

        virtual uint64_t GetUniqueID() const override { return m_data.GetUniqueID(); }

        virtual char const* GetSortingKey() const override { return m_sortingKey.c_str(); }

        virtual bool HasIcon() const override { return true; }

        virtual char const* GetIcon() const override
        {
            if ( IsEntity() )
            {
                return m_data.m_pEntity->IsSpatialEntity() ? EE_ICON_AXIS_ARROW : EE_ICON_SPHERE;
            }
            else if ( IsComponent() )
            {
                return m_data.m_isSpatialComponent ? EE_ICON_PUZZLE : EE_ICON_PUZZLE_OUTLINE;
            }
            else if ( IsSystem() )
            {
                return EE_ICON_COG;
            }

            return "";
        }

        virtual Color GetIconColor() const override
        {
            if ( IsEntity() )
            {
                return m_data.m_pEntity->IsSpatialEntity() ? Colors::Lime : Colors::LightBlue;
            }
            else if ( IsComponent() )
            {
                return m_data.m_isSpatialComponent ? Colors::DarkOrange : Colors::Khaki;
            }
            else if ( IsSystem() )
            {
                return Colors::DarkTurquoise;
            }

            return ImGuiX::Style::s_colorText;
        }

        virtual char const* GetDisplayName() const override
        {
            if ( IsEntity() )
            {
                return m_data.m_pEntity->GetNameID().c_str();
            }
            else if ( IsComponent() )
            {
                return m_data.m_pComponent->GetNameID().c_str();
            }
            else if ( IsSystem() )
            {
                return m_data.m_pSystem->GetTypeInfo()->m_friendlyName.c_str();
            }

            return "";
        }

        virtual bool HasContextMenu() const override { return true; }
        virtual bool IsDragAndDropTarget() const override { return IsSpatialComponent() || IsSpatialEntity(); }
        virtual bool IsDragAndDropSource() const override { return IsSpatialComponent(); }

        virtual void SetDragAndDropPayloadData() const override
        {
            uintptr_t itemAddress = uintptr_t( this );
            ImGui::SetDragDropPayload( s_dragAndDropID, &itemAddress, sizeof( uintptr_t ) );
        }

    public:

        EntityEditorItem        m_data;
        String                  m_sortingKey;
    };

    //-------------------------------------------------------------------------

    EntityInspector::EntityInspector( EditorContext& editorContext )
        : m_editorContext( editorContext )
        , m_propertyGrid( editorContext.m_pToolsContext )
    {
        // Tree
        //-------------------------------------------------------------------------

        m_treeView.SetFlag( TreeListView::Flags::ExpandItemsOnlyViaArrow );
        m_treeView.SetFlag( TreeListView::Flags::MultiSelectionAllowed );
        m_treeView.SetFlag( TreeListView::Flags::SortTree );
        m_treeView.SetFlag( TreeListView::Flags::TrackSelection );

        m_treeContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildTree( pRootItem ); };
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
        m_selectionChangedEventID = m_editorContext.OnSelectionChanged().Bind( [this] () { OnEditorSelectionChanged(); } );

        // Property Grid
        //-------------------------------------------------------------------------

        m_preEditPropertyBindingID = m_propertyGrid.OnPreEdit().Bind( [this] ( PropertyEditInfo const& eventInfo ) { PreEditProperty( eventInfo ); } );
        m_postEditPropertyBindingID = m_propertyGrid.OnPostEdit().Bind( [this] ( PropertyEditInfo const& eventInfo ) { PostEditProperty( eventInfo ); } );
    }

    EntityInspector::~EntityInspector()
    {
        m_editorContext.OnPreWorldChange().Unbind( m_preWorldChangedEventID );
        m_editorContext.OnPostWorldChange().Unbind( m_postWorldChangedEventID );
        m_editorContext.OnSelectionChanged().Unbind( m_selectionChangedEventID );

        m_propertyGrid.OnPreEdit().Unbind( m_preEditPropertyBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_postEditPropertyBindingID );
    }

    void EntityInspector::PreWorldChange()
    {
        m_propertyGrid.GetCurrentVisualState( m_propertyGridVisualState );
    }

    void EntityInspector::PostWorldChange()
    {
        if ( m_state != InternalState::NeedsRebuild )
        {
            m_state = InternalState::NeedsRebuild;
        }
    }

    void EntityInspector::UpdateAndDraw( UpdateContext const& context, bool isFocused )
    {
        EntityMap* pMap = m_editorContext.GetEditedMap();

        // Update
        //-------------------------------------------------------------------------

        if ( m_state == InternalState::UpToDate && m_editorContext.HasPendingEntityStateChangeActions() )
        {
            m_state = InternalState::NeedsRebuild;
        }

        if ( !m_editorContext.HasPendingEntityStateChangeActions() )
        {
            if ( pMap == nullptr )
            {
                m_state = InternalState::UpToDate;
            }
            else if( m_state != InternalState::UpToDate && pMap->IsMapLoaded() )
            {
                m_treeView.RebuildTree( m_treeContext, m_state == InternalState::NeedsRebuild );
                ReflectSelectionToTree();
                m_state = InternalState::UpToDate;
            }
        }

        // Draw
        //-------------------------------------------------------------------------

        if ( pMap == nullptr || !m_inspectedEntity.IsValid() )
        {
            ImGui::Text( "Nothing To Inspect." );
        }
        else
        {
            ImGui::SetNextWindowSizeConstraints( ImVec2( 200, 200 ), ImVec2( FLT_MAX, ImGui::GetContentRegionAvail().y - 200 ) );
            if ( ImGui::BeginChild( "Tree", ImVec2( 0, ImGui::GetContentRegionAvail().y / 2 ), ImGuiChildFlags_ResizeY, ImGuiWindowFlags_NoSavedSettings ) )
            {
                DrawTree( isFocused );
            }
            ImGui::EndChild();

            if ( ImGui::BeginChild( "PropertyGrid", ImVec2( 0, 0 ), ImGuiChildFlags_None, ImGuiWindowFlags_NoSavedSettings ) )
            {
                DrawPropertyGrid( isFocused );
            }
            ImGui::EndChild();
        }

        // Process deferred commands
        //-------------------------------------------------------------------------

        m_commandStack.ExecuteCommands();
    }

    //-------------------------------------------------------------------------

    void EntityInspector::GetSplitSelection( TInlineVector<EntityEditorItem, 10>& outSelectedEntities, TInlineVector<EntityEditorItem, 10>& outSelectedComponentAndSystems ) const
    {
        auto const& selection = m_editorContext.GetSelection();

        outSelectedEntities.clear();
        outSelectedComponentAndSystems.clear();

        for ( EntityEditorItem const& item : selection )
        {
            EE_ASSERT( item.IsValid() );

            if ( item.IsEntity() )
            {
                VectorEmplaceBackUnique( outSelectedEntities, item );
            }
            else if ( item.IsComponent() )
            {
                VectorEmplaceBackUnique( outSelectedComponentAndSystems, item );
            }
            else if ( item.IsSystem() )
            {
                VectorEmplaceBackUnique( outSelectedComponentAndSystems, item );
            }
        }
    }

    void EntityInspector::OnEditorSelectionChanged()
    {
        TInlineVector<EntityEditorItem, 10> selectedEntities;
        TInlineVector<EntityEditorItem, 10> selectedComponentsAndSystems;
        GetSplitSelection( selectedEntities, selectedComponentsAndSystems );

        //-------------------------------------------------------------------------

        if ( selectedEntities.size() == 0 || selectedEntities.size() > 1 )
        {
            m_inspectedEntity.Reset();
        }
        else
        {
            m_inspectedEntity = selectedEntities[0];
        }

        m_state = InternalState::NeedsRebuild;
    }

    void EntityInspector::UpdateEditorSelectionFromTree()
    {
        TInlineVector<EntityEditorItem, 10> selection;
        selection.emplace_back( m_inspectedEntity );

        auto const& treeSelection = m_treeView.GetSelection();
        for ( auto pItem : treeSelection )
        {
            auto pInspectorItem = static_cast<InspectorItem const*>( pItem );
            if ( pInspectorItem->IsComponent() )
            {
                selection.emplace_back( pInspectorItem->m_data.m_pEntity, pInspectorItem->m_data.m_pComponent );
            }
            else if ( pInspectorItem->IsSystem() )
            {
                selection.emplace_back( pInspectorItem->m_data.m_pEntity, pInspectorItem->m_data.m_pSystem );
            }
        }

        m_editorContext.SetSelection( selection );

        // Update property grid
        //-------------------------------------------------------------------------

        UpdatePropertyGridFromSelection();
    }

    void EntityInspector::ReflectSelectionToTree()
    {
        m_inspectedEntity.UpdateEntityPtrs( m_editorContext.GetWorld() );

        // Update Tree
        //-------------------------------------------------------------------------

        TInlineVector<EntityEditorItem, 10> selectedEntities;
        TInlineVector<EntityEditorItem, 10> selectedComponentsAndSystems;
        GetSplitSelection( selectedEntities, selectedComponentsAndSystems );

        TVector<TreeListViewItem*> treeSelection;
        for ( EntityEditorItem const& selectedData : selectedComponentsAndSystems )
        {
            TreeListViewItem* pFoundItem = m_treeView.FindItem( selectedData.GetUniqueID() );
            if ( pFoundItem != nullptr )
            {
                treeSelection.emplace_back( pFoundItem );
            }
        }

        if ( treeSelection.empty() )
        {
            TreeListViewItem* pFoundItem = m_treeView.FindItem( m_inspectedEntity.GetUniqueID() );
            if ( pFoundItem != nullptr )
            {
                treeSelection.emplace_back( pFoundItem );
            }
        }

        m_treeView.SetSelection( treeSelection );

        // Update Property Grid
        //-------------------------------------------------------------------------

        UpdatePropertyGridFromSelection();
    }

    //-------------------------------------------------------------------------

    void EntityInspector::DrawTree( bool isFocused )
    {
        EntityMap* pMap = m_editorContext.GetEditedMap();
        EE_ASSERT( pMap != nullptr );

        if ( m_state != InternalState::UpToDate )
        {
            ImGuiX::DrawSpinner( "Updating..." );
        }
        else // Draw component tree
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
                    ImGui::BeginDisabled( m_isReadOnly || !m_inspectedEntity.IsValid() );

                    auto CreateMenu = [this] ()
                    {
                        DrawAddComponentOrSystemMenu( m_inspectedEntity.m_pEntity );
                    };

                    ImGuiX::DropDownIconButtonColored( EE_ICON_PLUS, "ADD COMPONENT/SYSTEM", CreateMenu, Colors::Green, Colors::White, Colors::White );
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
                                auto pInspectorItem = static_cast<InspectorItem const*>( pItem );

                                if ( m_filter.MatchesFilter( pInspectorItem->GetDisplayName() ) )
                                {
                                    return true;
                                }

                                if ( m_filter.MatchesFilter( pInspectorItem->m_data.GetTypeFriendlyName() ) )
                                {
                                    return true;
                                }

                                return false;
                            }

                            return true;
                        };

                        m_treeView.UpdateItemVisibility( UpdateVisibility );
                    }
                }

                {
                    if ( !m_inspectedEntity.IsValid() )
                    {
                        constexpr static char const * const pText = "Nothing selected...";
                        ImVec2 const textSize = ImGui::CalcTextSize( pText );
                        ImGui::SetCursorPos( ImGui::GetCursorPos() + ( ImGui::GetContentRegionAvail() - textSize ) / 2 );
                        ImGui::Text( pText );
                    }
                }

                //-------------------------------------------------------------------------

                if ( m_treeView.UpdateAndDraw( m_treeContext ) )
                {
                    UpdateEditorSelectionFromTree();
                }
            }
        }

        // Handle Input
        //-------------------------------------------------------------------------

        if ( isFocused )
        {
            auto const& selection = m_treeView.GetSelection();

            if ( selection.empty() )
            {
                return;
            }

            //-------------------------------------------------------------------------

            if ( !m_isReadOnly )
            {
                if ( ImGui::IsKeyPressed( ImGuiKey_F2 ) )
                {
                    auto pSelectedItem = static_cast<InspectorItem*>( selection.back() );
                    if ( pSelectedItem->IsEntity() )
                    {
                        EE_ASSERT( pSelectedItem->m_data.m_pEntity != nullptr );
                        m_editorContext.ShowEntityRenameDialog( pSelectedItem->m_data.m_pEntity );
                    }
                    else if ( pSelectedItem->IsComponent() )
                    {
                        EE_ASSERT( pSelectedItem->m_data.m_pEntity != nullptr );
                        m_editorContext.ShowComponentRenameDialog( pSelectedItem->m_data.m_pEntity, pSelectedItem->m_data.m_pComponent );
                    }
                }

                //-------------------------------------------------------------------------

                /*if ( ImGui::IsKeyPressed( ImGuiKey_Delete ) )
                {
                    DeleteSelectedItems();
                }*/
            }

            //-------------------------------------------------------------------------

            if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
            {
                auto pSelectedItem = static_cast<InspectorItem*>( selection.back() );
                m_editorContext.FocusCamera( pSelectedItem->m_data.m_pEntity );
            }
        }
    }

    void EntityInspector::DrawExtraColumns( TreeListViewItem* pBaseItem, int32_t extraColumnIdx )
    {
        auto pInspectorItem = (InspectorItem*) pBaseItem;

        if ( extraColumnIdx == 0 )
        {
            if ( pInspectorItem->IsComponent() )
            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
                ImGui::Indent( 4.0f );
                ImGui::Text( pInspectorItem->m_data.m_pComponent->GetTypeInfo()->m_friendlyName.c_str() );
                ImGui::Unindent( 4.0f );
            }
        }
    }

    void EntityInspector::RebuildTree( TreeListViewItem* pRootItem )
    {
        auto const& selection = m_editorContext.GetSelection();

        OnEditorSelectionChanged();

        if ( m_inspectedEntity.IsValid() )
        {
            InspectorItem::Create( pRootItem, m_inspectedEntity.m_pEntity );
        }

        m_treeView.ExpandAll();
    }

    //-------------------------------------------------------------------------

    void EntityInspector::DrawPropertyGrid( bool isFocused )
    {
        if ( m_state != InternalState::UpToDate )
        {
            return;
        }

        //-------------------------------------------------------------------------

        m_propertyGrid.SetReadOnly( m_isReadOnly );
        m_propertyGrid.UpdateAndDraw();
    }

    void EntityInspector::UpdatePropertyGridFromSelection()
    {
        TInlineVector<EntityEditorItem, 10> selectedEntities;
        TInlineVector<EntityEditorItem, 10> selectedComponentsAndSystems;
        GetSplitSelection( selectedEntities, selectedComponentsAndSystems );

        if ( selectedEntities.size() != 1 )
        {
            m_propertyGrid.SetTypeToEdit( nullptr );
            return;
        }

        if ( selectedEntities[0] != m_inspectedEntity )
        {
            m_propertyGrid.SetTypeToEdit( nullptr );
            return;
        }

        //-------------------------------------------------------------------------

        if ( selectedComponentsAndSystems.empty() )
        {
            m_propertyGrid.SetTypeToEdit( m_inspectedEntity.m_pEntity );
        }
        else if ( selectedComponentsAndSystems.size() == 1 )
        {
            EntityEditorItem const& selectedData = selectedComponentsAndSystems.back();
            if ( selectedData.m_pComponent != nullptr )
            {
                m_propertyGrid.SetTypeToEdit( selectedData.m_pComponent, &m_propertyGridVisualState );
            }
            else if ( selectedData.m_pSystem != nullptr )
            {
                m_propertyGrid.SetTypeToEdit( selectedData.m_pSystem, &m_propertyGridVisualState );
            }
        }
        else
        {
            m_propertyGrid.SetTypeToEdit( nullptr );
        }

        m_propertyGridVisualState.Clear();
    }

    void EntityInspector::PreEditProperty( PropertyEditInfo const& eventInfo )
    {
        Entity* pEntity = nullptr;

        if ( auto pEditedEntity = TryCast<Entity>( eventInfo.m_pOwnerTypeInstance ) )
        {
            pEntity = pEditedEntity;
            m_editorContext.BeginEdit( { pEntity } );
        }
        else if ( auto pComponent = TryCast<EntityComponent>( eventInfo.m_pOwnerTypeInstance ) )
        {
            pEntity = m_editorContext.GetWorld()->FindEntity( pComponent->GetEntityID() );
            m_editorContext.BeginSingleComponentEdit( pEntity, pComponent );
        }
    }

    void EntityInspector::PostEditProperty( PropertyEditInfo const& eventInfo )
    {
        // Reset the local transform to ensure that the world transform is recalculated
        if ( eventInfo.m_pPropertyInfo->m_ID == StringID( "m_transform" ) )
        {
            if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( eventInfo.m_pOwnerTypeInstance ) )
            {
                pSpatialComponent->SetLocalTransform( pSpatialComponent->GetLocalTransform() );
            }
        }

        //-------------------------------------------------------------------------

        if ( auto pComponent = TryCast<EntityComponent>( eventInfo.m_pOwnerTypeInstance ) )
        {
            m_editorContext.EndSingleComponentEdit( pComponent );
        }
        else
        {
            m_editorContext.EndEdit();
        }
    }

    //-------------------------------------------------------------------------

    void EntityInspector::DeleteSelectedItems()
    {
        TVector<EntityEditorItem> const& selection = m_editorContext.GetSelection();
        if ( selection.empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        m_editorContext.BeginCompoundUndoableAction();

        for ( EntityEditorItem const& item : selection )
        {
            if ( item.IsComponent() )
            {
                m_editorContext.DestroyComponent( item.m_pEntity, item.m_pComponent );
            }
            else if ( item.IsSystem() )
            {
                m_editorContext.DestroySystem( item.m_pEntity, item.m_pSystem );
            }
        }

        m_editorContext.EndCompoundUndoableAction();
    }

    //-------------------------------------------------------------------------

    static bool CategoryContainsValidComponentEntries( Entity* pEntity, CategorizedEntityItemTypeInfo const& category )
    {
        for ( auto pTypeInfo : category.m_typeInfos )
        {
            if ( pEntity->CanCreateComponent( pTypeInfo ) )
            {
                return true;
            }
        }

        return false;
    }

    static bool CategoryContainsValidSystemEntries( Entity* pEntity, CategorizedEntityItemTypeInfo const& category )
    {
        for ( auto pTypeInfo : category.m_typeInfos )
        {
            if ( pEntity->CanCreateSystem( pTypeInfo ) )
            {
                return true;
            }
        }

        return false;
    }

    void EntityInspector::DrawContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus )
    {
        if ( m_isReadOnly )
        {
            return;
        }

        if ( selectedItemsWithContextMenus.empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        TVector<EntityEditorItem> const& selection = m_editorContext.GetSelection();
        if ( selection.empty() || selection.size() > 2 || selection[0] != m_inspectedEntity )
        {
            return;
        }

        bool onlySingleItemSelected = selection.size() == 1;
        if ( onlySingleItemSelected )
        {
            auto pItem = (InspectorItem*) selectedItemsWithContextMenus[0];
            if ( pItem->IsEntity() )
            {
                if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename" ) )
                {
                    m_editorContext.ShowEntityRenameDialog( pItem->m_data.m_pEntity );
                }

                DrawAddComponentOrSystemMenu( pItem->m_data.m_pEntity );
            }

            //-------------------------------------------------------------------------

            else if ( pItem->IsComponent() )
            {
                if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename" ) )
                {
                    m_editorContext.ShowComponentRenameDialog( pItem->m_data.m_pEntity, pItem->m_data.m_pComponent );
                }

                //-------------------------------------------------------------------------

                if ( pItem->IsSpatialComponent() )
                {
                    if ( pItem->m_data.m_pEntity->GetRootSpatialComponent() != pItem->m_data.m_pComponent )
                    {
                        if ( ImGui::MenuItem( EE_ICON_MAP_MARKER" Make Root Component" ) )
                        {
                            m_editorContext.ReparentComponent( pItem->m_data.m_pEntity, static_cast<SpatialEntityComponent*>( pItem->m_data.m_pComponent ), nullptr );
                        }
                    }

                    if ( ImGui::BeginMenu( EE_ICON_PLUS" Add Child Component" ) )
                    {
                        m_componentContextMenufilter.UpdateAndDraw();

                        if ( m_componentContextMenufilter.HasFilterSet() )
                        {
                            DrawAddComponentMenu( pItem->m_data.m_pEntity, m_editorContext.GetSpatialComponentTypes(), pItem->m_data.GetSpatialComponent() );
                        }
                        else
                        {
                            for ( auto const& category : m_editorContext.GetCategorizedSpatialComponentTypes() )
                            {
                                if ( !CategoryContainsValidComponentEntries( pItem->m_data.m_pEntity, category ) )
                                {
                                    continue;
                                }

                                if ( category.m_name.IsValid() )
                                {
                                    if ( ImGui::BeginMenu( category.m_name.c_str() ) )
                                    {
                                        DrawAddComponentMenu( pItem->m_data.m_pEntity, category.m_typeInfos, pItem->m_data.GetSpatialComponent() );
                                        ImGui::EndMenu();
                                    }
                                }
                                else
                                {
                                    ImGui::Separator();
                                    DrawAddComponentMenu( pItem->m_data.m_pEntity, category.m_typeInfos, pItem->m_data.GetSpatialComponent() );
                                }
                            }
                        }

                        ImGui::EndMenu();
                    }
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( ImGui::MenuItem( onlySingleItemSelected ? EE_ICON_DELETE" Delete" : EE_ICON_DELETE" Delete Selected" ) )
        {
            m_commandStack.PushCommand( [this] () { DeleteSelectedItems(); } );
        }
    }

    void EntityInspector::DrawAddComponentOrSystemMenu( Entity* pEntity )
    {
        if ( ImGui::BeginMenu( EE_ICON_PUZZLE" Add Component" ) )
        {
            m_componentContextMenufilter.UpdateAndDraw();

            if ( m_componentContextMenufilter.HasFilterSet() )
            {
                DrawAddComponentMenu( pEntity, m_editorContext.GetComponentTypes() );
            }
            else
            {
                for ( auto const& category : m_editorContext.GetCategorizedComponentTypes() )
                {
                    if ( !CategoryContainsValidComponentEntries( pEntity, category ) )
                    {
                        continue;
                    }

                    if ( category.m_name.IsValid() )
                    {
                        if ( ImGui::BeginMenu( category.m_name.c_str() ) )
                        {
                            DrawAddComponentMenu( pEntity, category.m_typeInfos );
                            ImGui::EndMenu();
                        }
                    }
                    else
                    {
                        ImGui::Separator();
                        DrawAddComponentMenu( pEntity, category.m_typeInfos );
                    }
                }
            }

            ImGui::EndMenu();
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginMenu( EE_ICON_COG" Add System" ) )
        {
            m_systemContextMenufilter.UpdateAndDraw();

            if ( m_systemContextMenufilter.HasFilterSet() )
            {
                DrawAddSystemMenu( pEntity, m_editorContext.GetSystemTypes() );
            }
            else
            {
                for ( auto const& category : m_editorContext.GetCategorizedSystemTypes() )
                {
                    if ( !CategoryContainsValidSystemEntries( pEntity, category ) )
                    {
                        continue;
                    }

                    if ( category.m_name.IsValid() )
                    {
                        if ( ImGui::BeginMenu( category.m_name.c_str() ) )
                        {
                            DrawAddSystemMenu( pEntity, category.m_typeInfos );
                            ImGui::EndMenu();
                        }
                    }
                    else
                    {
                        ImGui::Separator();
                        DrawAddSystemMenu( pEntity, category.m_typeInfos );
                    }
                }
            }

            ImGui::EndMenu();
        }
    }

    void EntityInspector::DrawAddSystemMenu( Entity* pEntity, TVector<TypeSystem::TypeInfo const*> const& systemTypes )
    {
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( !m_isReadOnly );

        for ( auto pTypeInfo : systemTypes )
        {
            if ( !m_systemContextMenufilter.MatchesFilter( pTypeInfo->m_friendlyName ) )
            {
                continue;
            }

            if ( pEntity->CanCreateSystem( pTypeInfo ) )
            {
                if ( ImGui::MenuItem( pTypeInfo->m_friendlyName.c_str() ) )
                {
                    auto pCreatedSystem = m_editorContext.CreateSystem( pEntity, pTypeInfo );
                    m_editorContext.SetSelection( pEntity, pCreatedSystem );
                }
            }
        }
    }

    void EntityInspector::DrawAddComponentMenu( Entity* pEntity, TVector<TypeSystem::TypeInfo const*> const& componentTypes, SpatialEntityComponent* pParentComponent )
    {
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( !m_isReadOnly );

        for ( auto pTypeInfo : componentTypes )
        {
            if ( !m_componentContextMenufilter.MatchesFilter( pTypeInfo->m_friendlyName ) )
            {
                continue;
            }

            EntityComponent const* pDefaultInstance = Cast<EntityComponent>( pTypeInfo->GetDefaultInstance() );
            if ( pDefaultInstance->IsTransientComponent() )
            {
                continue;
            }

            if ( pEntity->CanCreateComponent( pTypeInfo ) )
            {
                if ( ImGui::MenuItem( pTypeInfo->m_friendlyName.c_str() ) )
                {
                    auto pCreatedComponent = m_editorContext.CreateComponent( pEntity, pTypeInfo, pParentComponent );
                    m_editorContext.SetSelection( pEntity, pCreatedComponent );
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    void EntityInspector::HandleDragAndDrop( TreeListViewItem* pDropTarget )
    {
        if ( m_isReadOnly )
        {
            return;
        }

        //-------------------------------------------------------------------------

        InspectorItem* pDropTargetItem = (InspectorItem*) pDropTarget;
        InspectorItem* pDraggedItem = nullptr;

        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( InspectorItem::s_dragAndDropID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            pDraggedItem = (InspectorItem*) *( (uintptr_t*) payload->Data );

            if ( pDraggedItem == pDropTargetItem )
            {
                return;
            }

            if ( !payload->IsDelivery() )
            {
                pDraggedItem = nullptr;
            }
        }

        //-------------------------------------------------------------------------

        if ( pDraggedItem == nullptr )
        {
            return;
        }

        auto PerformDragAndDrop = [this, pDraggedItem, pDropTargetItem]
        {
            ExecuteDragAndDrop( pDraggedItem->m_data, pDropTargetItem->m_data );
        };

        m_commandStack.PushCommand( PerformDragAndDrop );
    }

    void EntityInspector::ExecuteDragAndDrop( EntityEditorItem& draggedItem, EntityEditorItem& dropTargetItem )
    {
        EE_ASSERT( draggedItem.IsSpatialComponent() );

        if ( dropTargetItem.IsSpatialComponent() )
        {
            auto pDraggedComponent = draggedItem.GetSpatialComponent();
            auto pTargetComponent = dropTargetItem.GetSpatialComponent();

            // If we drag a component to it's direct parent just exchange those components in the hierarchy
            if ( pDraggedComponent->HasSpatialParent() && pDraggedComponent->GetSpatialParentID() == pTargetComponent->GetID() )
            {
                m_editorContext.SwitchComponentsInHierarchy( draggedItem.m_pEntity, pDraggedComponent, pTargetComponent );
            }
            else
            {
                m_editorContext.ReparentComponent( draggedItem.m_pEntity, pDraggedComponent, pTargetComponent );
            }
        }
        else // Move under root or make the root
        {
            m_editorContext.ReparentComponent( draggedItem.m_pEntity, draggedItem.GetSpatialComponent(), nullptr );
        }
    }
}