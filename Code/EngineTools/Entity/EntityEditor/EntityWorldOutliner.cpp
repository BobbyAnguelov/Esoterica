#include "EntityWorldOutliner.h"
#include "EntityUndoableAction.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntitySerialization.h"
#include "System/Imgui/ImguiX.h"
#include "System/Math/MathRandom.h"
#include "System/Imgui/ImguiStyle.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityItem final : public TreeListViewItem
    {
    public:

        constexpr static char const* const s_dragAndDropID = "EntityItem";

    public:

        EntityItem( Entity* pEntity )
            : m_pEntity( pEntity )
            , m_entityID( pEntity->GetID() )
        {
            EE_ASSERT( m_pEntity != nullptr );
        }

        virtual uint64_t GetUniqueID() const override
        {
            return m_entityID.m_value;
        }

        virtual StringID GetNameID() const override
        {
            return m_pEntity->GetNameID();
        }

        virtual String GetDisplayName() const override
        {
            String displayName;

            if ( m_pEntity->IsSpatialEntity() )
            {
                displayName.sprintf( EE_ICON_AXIS_ARROW" %s", GetNameID().c_str() );
            }
            else
            {
                displayName.sprintf( EE_ICON_DATABASE" %s", GetNameID().c_str() );
            }

            return displayName;
        }

        virtual bool IsDragAndDropTarget() const override { return m_pEntity->IsSpatialEntity(); }

        virtual bool IsDragAndDropSource() const override { return m_pEntity->IsSpatialEntity(); }

        virtual void SetDragAndDropPayloadData() const override
        {
            uintptr_t itemAddress = uintptr_t( this );
            ImGui::SetDragDropPayload( s_dragAndDropID, &itemAddress, sizeof( uintptr_t ) );
        }

        virtual bool HasContextMenu() const override { return true; }

    public:

        Entity*     m_pEntity = nullptr;
        EntityID    m_entityID; // Cached since we want to access this on rebuild without touching the entity memory which might have been invalidated
    };

    //-------------------------------------------------------------------------

    EntityWorldOutliner::EntityWorldOutliner( ToolsContext const* pToolsContext, UndoStack* pUndoStack, EntityWorld* pWorld )
        : m_pToolsContext( pToolsContext )
        , m_pUndoStack( pUndoStack )
        , m_pWorld( pWorld )
    {
        EE_ASSERT( m_pToolsContext != nullptr );
        EE_ASSERT( m_pUndoStack != nullptr );
        EE_ASSERT( m_pWorld != nullptr );

        m_multiSelectionAllowed = true;
        m_expandItemsOnlyViaArrow = true;
        m_prioritizeBranchesOverLeavesInVisualTree = false;
    }

    void EntityWorldOutliner::Initialize( UpdateContext const& context, uint32_t widgetUniqueID )
    {
        m_windowName.sprintf( "Outliner##%u", widgetUniqueID );
    }

    void EntityWorldOutliner::Shutdown( UpdateContext const& context )
    {
        // Do Nothing
    }

    //-------------------------------------------------------------------------

    bool EntityWorldOutliner::UpdateAndDraw( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        // Process deletions
        //-------------------------------------------------------------------------

        if ( !m_entityDeletionRequests.empty() )
        {
            auto pActiveUndoableAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
            pActiveUndoableAction->RecordDelete( m_entityDeletionRequests );
            m_pUndoStack->RegisterAction( pActiveUndoableAction );

            for ( auto pEntity : m_entityDeletionRequests )
            {
                auto pMap = GetMapForEntity( pEntity );
                EE_ASSERT( pMap != nullptr );
                pMap->DestroyEntity( pEntity->GetID() );
            }

            m_entityDeletionRequests.clear();
            ClearSelection();
            m_requestTreeRebuild = true;
        }

        // Update Tree
        //-------------------------------------------------------------------------

        if ( m_pWorld->HasPendingMapChangeActions() )
        {
            m_requestTreeRebuild = true;
        }
        else
        {
            EntityMap* pMap = GetEditedMap();
            if ( m_requestTreeRebuild && pMap != nullptr && pMap->IsActivated() )
            {
                // The rebuild has to come first so that we can return the correct selected entities once the tree rebuild and sends out the "selection changed event"
                m_requestTreeRebuild = false;
                RebuildTree();
            }
        }

        // Process selection requests
        //-------------------------------------------------------------------------

        if ( !m_requestTreeRebuild && !m_deferredSelectionRequests.empty() )
        {
            TVector<TreeListViewItem*> newSelection;
            for ( auto pEntity : m_deferredSelectionRequests )
            {
                auto pItem = FindItem( pEntity->GetID().m_value );
                if ( pItem != nullptr )
                {
                    newSelection.emplace_back( pItem );
                }
            }

            TreeListView::SetSelection( newSelection );
            m_deferredSelectionRequests.clear();
        }

        // Draw Tree
        //-------------------------------------------------------------------------

        bool isFocused = false;
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_windowName.c_str() ) )
        {
            if ( m_requestTreeRebuild )
            {
                // Display nothing
            }
            else if ( GetEditedMap() == nullptr )
            {
                ImGui::Text( "Nothing To Outline." );
            }
            else // Draw outliner
            {
                EntityMap* pMap = GetEditedMap();
                if ( !pMap->IsActivated() )
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
                        if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, "CREATE NEW ENTITY", ImVec2( -1, 0 ) ) )
                        {
                            CreateEntity( pMap->GetID() );
                        }
                    }

                    TreeListView::Draw();
                }
            }

            // Handle Input
            isFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows );
            if ( isFocused )
            {
                if ( ImGui::IsKeyPressed( ImGuiKey_F2 ) && GetSelection().size() == 1 )
                {
                    Printf( m_renameBuffer, 256, GetSelection()[0]->GetNameID().c_str() );
                    m_activeOperation = Operation::Rename;
                }

                if ( ImGui::IsKeyPressed( ImGuiKey_Delete ) )
                {
                    DestroySelectedEntities();
                }
            }
        }
        ImGui::End();

        //-------------------------------------------------------------------------

        DrawDialogs();

        //-------------------------------------------------------------------------

        return isFocused;
    }

    void EntityWorldOutliner::DrawDialogs()
    {
        bool isDialogOpen = m_activeOperation != Operation::None;
        if ( isDialogOpen )
        {
            if ( m_activeOperation == Operation::Rename )
            {
                Entity* pEntityToRename = static_cast<EntityItem*>( GetSelection()[0] )->m_pEntity;
                EntityMapID const& mapID = pEntityToRename->GetMapID();
                EE_ASSERT( mapID.IsValid() );
                EntityMap* pMap = GetEditedMap();
                EE_ASSERT( pMap != nullptr );

                //-------------------------------------------------------------------------

                auto ValidateName = [this, pMap] ()
                {
                    bool isValidName = strlen( m_renameBuffer ) > 0;
                    if ( isValidName )
                    {
                        StringID const desiredNameID = StringID( m_renameBuffer );
                        auto uniqueNameID = pMap->GenerateUniqueEntityNameID( desiredNameID );
                        isValidName = ( desiredNameID == uniqueNameID );
                    }

                    return isValidName;
                };

                constexpr static char const* const dialogName = "Rename Entity";
                ImGui::OpenPopup( dialogName );
                ImGui::SetNextWindowSize( ImVec2( 400, 90 ) );
                if ( ImGui::BeginPopupModal( dialogName, &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar ) )
                {
                    bool applyRename = false;
                    bool isValidName = ValidateName();

                    // Draw UI
                    //-------------------------------------------------------------------------

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( "Name: " );
                    ImGui::SameLine();

                    ImGui::SetNextItemWidth( -1 );
                    ImGui::PushStyleColor( ImGuiCol_Text, isValidName ? (uint32_t) ImGuiX::Style::s_colorText : Colors::Red.ToUInt32_ABGR() );
                    applyRename = ImGui::InputText( "##Name", m_renameBuffer, 256, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CallbackCharFilter, ImGuiX::FilterNameIDChars );
                    isValidName = ValidateName();
                    ImGui::PopStyleColor();

                    ImGui::NewLine();
                    ImGui::SameLine( ImGui::GetContentRegionAvail().x - 120 - ImGui::GetStyle().ItemSpacing.x );

                    ImGui::BeginDisabled( !isValidName );
                    if ( ImGui::Button( "OK", ImVec2( 60, 0 ) ) )
                    {
                        applyRename = true;
                    }
                    ImGui::EndDisabled();

                    ImGui::SameLine();
                    if ( ImGui::Button( "Cancel", ImVec2( 60, 0 ) ) )
                    {
                        m_activeOperation = Operation::None;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();

                    //-------------------------------------------------------------------------

                    if ( applyRename && isValidName )
                    {
                        StringID const desiredNameID = StringID( m_renameBuffer );
                        RenameEntity( pEntityToRename, desiredNameID );
                        m_activeOperation = Operation::None;
                    }
                }
            }
        }

        // If the dialog was closed (i.e. operation canceled)
        if ( !isDialogOpen )
        {
            m_activeOperation = Operation::None;
        }
    }

    void EntityWorldOutliner::CreateEntityTreeItem( TreeListViewItem* pParentItem, Entity* pEntity )
    {
        auto pEntityItem = pParentItem->CreateChild<EntityItem>( pEntity );
        if ( pEntity->HasAttachedEntities() )
        {
            for ( auto pChildEntity : pEntity->GetAttachedEntities() )
            {
                CreateEntityTreeItem( pEntityItem, pChildEntity );
            }
        }
    }

    void EntityWorldOutliner::RebuildTreeUserFunction()
    {
        EntityMap* pMap = GetEditedMap();
        if ( pMap == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        for ( auto pEntity : pMap->GetEntities() )
        {
            // Children are handled as part of their parents
            if ( pEntity->HasSpatialParent() )
            {
                continue;
            }

            CreateEntityTreeItem( &m_rootItem, pEntity );
        }
    }

    void EntityWorldOutliner::HandleDragAndDropOnItem( TreeListViewItem* pDragAndDropTargetItem )
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( EntityItem::s_dragAndDropID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( payload->IsDelivery() )
            {
                auto pRawData = (uintptr_t*) payload->Data;
                auto pSourceEntityItem = (EntityItem*) *pRawData;
                auto pTargetEntityItem = (EntityItem*) pDragAndDropTargetItem;

                // Same items, nothing to do
                if ( pSourceEntityItem == pTargetEntityItem )
                {
                    return;
                }

                // We cannot parent cross map
                if ( pTargetEntityItem->m_pEntity->GetMapID() != pSourceEntityItem->m_pEntity->GetMapID() )
                {
                    return;
                }

                // We cannot re-parent ourselves to one of our children
                if ( pTargetEntityItem->m_pEntity->IsSpatialChildOf( pSourceEntityItem->m_pEntity ) )
                {
                    return;
                }

                SetEntityParent( pSourceEntityItem->m_pEntity, pTargetEntityItem->m_pEntity );
            }
        }
    }

    void EntityWorldOutliner::DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus )
    {
        if ( selectedItemsWithContextMenus.size() == 1 )
        {
            auto pEntityItem = (EntityItem*) selectedItemsWithContextMenus[0];

            if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename" ) )
            {
                Printf( m_renameBuffer, 256, pEntityItem->m_pEntity->GetNameID().c_str() );
                m_activeOperation = Operation::Rename;
            }

            if ( pEntityItem->m_pEntity->HasSpatialParent() )
            {
                if ( ImGui::MenuItem( EE_ICON_CLOSE" Detach From Parent" ) )
                {
                    ClearEntityParent( pEntityItem->m_pEntity );
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( ImGui::MenuItem( EE_ICON_DELETE" Delete" ) )
        {
            DestroySelectedEntities();
        }
    }

    //-------------------------------------------------------------------------
    // Map
    //-------------------------------------------------------------------------

    EntityMap* EntityWorldOutliner::GetEditedMap()
    {
        return m_pWorld->GetFirstNonPersistentMap();
    }

    EntityMap* EntityWorldOutliner::GetMapForEntity( Entity const* pEntity ) const
    {
        EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
        auto const& mapID = pEntity->GetMapID();
        auto pMap = m_pWorld->GetMap( mapID );
        EE_ASSERT( pMap != nullptr );
        return pMap;
    }

    //-------------------------------------------------------------------------
    // Selection
    //-------------------------------------------------------------------------

    TVector<Entity*> EntityWorldOutliner::GetSelectedEntities() const
    {
        TVector<Entity*> selectedEntities;

        // Do not return anything if we require a rebuild, this means the item data is stale
        if ( !m_requestTreeRebuild )
        {
            for ( auto pItem : GetSelection() )
            {
                auto pEntityItem = static_cast<EntityItem*>( pItem );
                selectedEntities.emplace_back( pEntityItem->m_pEntity );
            }
        }

        return selectedEntities;
    }

    bool EntityWorldOutliner::IsEntitySelected( Entity* pEntity ) const
    {
        if ( m_requestTreeRebuild )
        {
            return false;
        }

        EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
        auto pItem = FindItem( pEntity->GetID().m_value );
        EE_ASSERT( pItem != nullptr );
        return IsItemSelected( pItem );
    }

    void EntityWorldOutliner::SetSelection( Entity* pEntity )
    {
        EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
        TreeListViewItem* pItem = FindItem( pEntity->GetID().m_value );
        if ( pItem != nullptr )
        {
            TreeListView::SetSelection( pItem );
        }
        else // Defer selection change
        {
            m_deferredSelectionRequests.emplace_back( pEntity );
        }
    }

    void EntityWorldOutliner::AddToSelection( Entity* pEntity )
    {
        EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
        TreeListViewItem* pItem = FindItem( pEntity->GetID().m_value );
        if ( pItem != nullptr )
        {
            TreeListView::AddToSelection( pItem );
        }
        else // Defer selection change
        {
            m_deferredSelectionRequests = GetSelectedEntities();
            m_deferredSelectionRequests.emplace_back( pEntity );
        }
    }

    void EntityWorldOutliner::RemoveFromSelection( Entity* pEntity )
    {
        EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
        TreeListViewItem* pItem = FindItem( pEntity->GetID().m_value );
        if ( pItem != nullptr )
        {
            TreeListView::RemoveFromSelection( pItem );
        }
    }

    //-------------------------------------------------------------------------
    // World Operations
    //-------------------------------------------------------------------------

    void EntityWorldOutliner::CreateEntity( EntityMapID const& mapID )
    {
        auto pMap = m_pWorld->GetMap( mapID );
        EE_ASSERT( pMap != nullptr && pMap->IsActivated() );

        StringID const uniqueNameID = pMap->GenerateUniqueEntityNameID( "Entity" );
        Entity* pEntity = EE::New<Entity>( StringID( uniqueNameID ) );

        // Add entity to map
        pMap->AddEntity( pEntity );

        // Record undo action
        auto pActiveUndoableAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        pActiveUndoableAction->RecordCreate( { pEntity } );
        m_pUndoStack->RegisterAction( pActiveUndoableAction );

        // Update selection
        m_deferredSelectionRequests.emplace_back( pEntity );
        m_requestTreeRebuild = true;
    }

    void EntityWorldOutliner::DuplicateSelectedEntities()
    {
        if ( GetSelection().empty() )
        {
            return;
        }

        // Duplicate the entities and add them to the map
        //-------------------------------------------------------------------------

        auto const selectedEntities = GetSelectedEntities();
        TVector<Entity*> duplicatedEntities;
        for ( auto pEntity : selectedEntities )
        {
            auto pMap = GetMapForEntity( pEntity );

            SerializedEntityDescriptor entityDesc;
            if ( Serializer::SerializeEntity( *m_pToolsContext->m_pTypeRegistry, pEntity, entityDesc ) )
            {
                auto pDuplicatedEntity = Serializer::CreateEntity( *m_pToolsContext->m_pTypeRegistry, entityDesc );
                duplicatedEntities.emplace_back( pDuplicatedEntity );
                pMap->AddEntity( pDuplicatedEntity );
            }
        }

        // Update selection
        //-------------------------------------------------------------------------

        for ( auto pDuplicatedEntity : duplicatedEntities )
        {
            m_deferredSelectionRequests.emplace_back( pDuplicatedEntity );
        }
    }

    void EntityWorldOutliner::DestroyEntity( Entity* pEntity )
    {
        EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() && m_pWorld->FindEntity( pEntity->GetID() ) != nullptr );
        m_entityDeletionRequests.emplace_back( pEntity );
    }

    void EntityWorldOutliner::DestroySelectedEntities()
    {
        if ( GetSelection().empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto const selectedEntities = GetSelectedEntities();
        for ( auto pEntity : selectedEntities )
        {
            m_entityDeletionRequests.emplace_back( pEntity );
        }
    }

    void EntityWorldOutliner::RenameEntity( Entity* pEntity, StringID newDesiredName )
    {
        auto pMap = GetMapForEntity( pEntity );
        auto pActiveUndoableAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        pActiveUndoableAction->RecordBeginEdit( { pEntity } );

        pMap->RenameEntity( pEntity, newDesiredName );

        pActiveUndoableAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoableAction );

        m_deferredSelectionRequests.emplace_back( pEntity );
        m_requestTreeRebuild = true;
    }

    void EntityWorldOutliner::SetEntityParent( Entity* pEntity, Entity* pNewParent )
    {
        EE_ASSERT( pEntity != nullptr );

        //-------------------------------------------------------------------------

        auto pMap = GetMapForEntity( pEntity );
        auto pActiveUndoableAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        pActiveUndoableAction->RecordBeginEdit( { pEntity } );

        //-------------------------------------------------------------------------

        pEntity->SetSpatialParent( pNewParent, StringID() );

        //-------------------------------------------------------------------------

        pActiveUndoableAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoableAction );

        m_deferredSelectionRequests.emplace_back( pEntity );
        m_requestTreeRebuild = true;
    }

    void EntityWorldOutliner::ClearEntityParent( Entity* pEntity )
    {
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( pEntity->HasSpatialParent() );

        //-------------------------------------------------------------------------

        auto pMap = GetMapForEntity( pEntity );
        auto pActiveUndoableAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        pActiveUndoableAction->RecordBeginEdit( { pEntity } );

        //-------------------------------------------------------------------------

        pEntity->ClearSpatialParent();

        //-------------------------------------------------------------------------

        pActiveUndoableAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoableAction );

        m_deferredSelectionRequests.emplace_back( pEntity );
        m_requestTreeRebuild = true;
    }
}