#include "EntityWorldPropertyGrid.h"
#include "EntityUndoableAction.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Core/UndoStack.h"
#include "Engine/Entity/EntityWorld.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityWorldPropertyGrid::EntityWorldPropertyGrid( ToolsContext const* pToolsContext, UndoStack* pUndoStack, EntityWorld* pWorld )
        : m_pToolsContext( pToolsContext )
        , m_pUndoStack( pUndoStack )
        , m_pWorld( pWorld )
        , m_propertyGrid( pToolsContext)
    {
        EE_ASSERT( m_pUndoStack != nullptr );
        EE_ASSERT( m_pWorld != nullptr );
    }

    EntityWorldPropertyGrid::~EntityWorldPropertyGrid()
    {
        EE_ASSERT( m_pActiveUndoAction == nullptr );
    }

    void EntityWorldPropertyGrid::Initialize( UpdateContext const& context, uint32_t widgetUniqueID )
    {
        m_windowName.sprintf( "Properties##%u", widgetUniqueID );
        m_preEditPropertyBindingID = m_propertyGrid.OnPreEdit().Bind( [this] ( PropertyEditInfo const& eventInfo ) { PreEditProperty( eventInfo ); } );
        m_postEditPropertyBindingID = m_propertyGrid.OnPostEdit().Bind( [this] ( PropertyEditInfo const& eventInfo ) { PostEditProperty( eventInfo ); } );
    }

    void EntityWorldPropertyGrid::Shutdown( UpdateContext const& context )
    {
        m_propertyGrid.OnPreEdit().Unbind( m_preEditPropertyBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_postEditPropertyBindingID );
    }

    void EntityWorldPropertyGrid::PreEditProperty( PropertyEditInfo const& eventInfo )
    {
        EE_ASSERT( m_pActiveUndoAction == nullptr );

        //-------------------------------------------------------------------------

        Entity* pEntity = nullptr;

        if ( auto pEditedEntity = TryCast<Entity>( eventInfo.m_pEditedTypeInstance ) )
        {
            pEntity = pEditedEntity;
        }
        else if ( auto pComponent = TryCast<EntityComponent>( eventInfo.m_pEditedTypeInstance ) )
        {
            pEntity = m_pWorld->FindEntity( pComponent->GetEntityID() );
        }

        //-------------------------------------------------------------------------

        auto pMap = m_pWorld->GetMap( pEntity->GetMapID() );
        EE_ASSERT( pMap != nullptr );
        m_pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        m_pActiveUndoAction->RecordBeginEdit( { pEntity } );

        //-------------------------------------------------------------------------

        if ( auto pComponent = TryCast<EntityComponent>( eventInfo.m_pEditedTypeInstance ) )
        {
            m_pWorld->BeginComponentEdit( pComponent );
        }
    }

    void EntityWorldPropertyGrid::PostEditProperty( PropertyEditInfo const& eventInfo )
    {
        EE_ASSERT( m_pActiveUndoAction != nullptr );

        // Reset the local transform to ensure that the world transform is recalculated
        if ( eventInfo.m_pPropertyInfo->m_ID == StringID( "m_transform" ) )
        {
            if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( eventInfo.m_pEditedTypeInstance ) )
            {
                pSpatialComponent->SetLocalTransform( pSpatialComponent->GetLocalTransform() );
            }
        }

        //-------------------------------------------------------------------------

        m_pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( m_pActiveUndoAction );
        m_pActiveUndoAction = nullptr;

        //-------------------------------------------------------------------------

        if ( auto pComponent = TryCast<EntityComponent>( eventInfo.m_pEditedTypeInstance ) )
        {
            m_pWorld->EndComponentEdit( pComponent );
        }
    }

    //-------------------------------------------------------------------------

    void EntityWorldPropertyGrid::Clear()
    {
        m_editedEntityID.Clear();
        m_editedComponentID.Clear();
        m_propertyGrid.SetTypeToEdit( nullptr );
    }

    bool EntityWorldPropertyGrid::RefreshEditedTypeInstance()
    {
        // We have nothing to edit
        if ( !m_editedEntityID.IsValid() )
        {
            m_propertyGrid.SetTypeToEdit( nullptr );
            return true;
        }

        // Find entity
        //-------------------------------------------------------------------------

        auto pEntity = m_pWorld->FindEntity( m_editedEntityID );
        if ( pEntity == nullptr )
        {
            m_propertyGrid.SetTypeToEdit( nullptr );
            return false;
        }

        // Find Component
        //-------------------------------------------------------------------------

        if ( m_editedComponentID.IsValid() )
        {
            auto pComponent = pEntity->FindComponent( m_editedComponentID );
            if ( pComponent )
            {
                m_propertyGrid.SetTypeToEdit( pComponent );
            }
            else
            {
                m_propertyGrid.SetTypeToEdit( nullptr );
                return false;
            }
        }
        else
        {
            m_propertyGrid.SetTypeToEdit( pEntity );
        }

        return true;
    }

    void EntityWorldPropertyGrid::SetTypeInstanceToEdit( IRegisteredType* pTypeInstance )
    {
        if ( pTypeInstance == nullptr )
        {
            Clear();
        }
        else if ( auto pEntity = TryCast<Entity>( pTypeInstance ) )
        {
            auto pFoundEntity = m_pWorld->FindEntity( pEntity->GetID() );
            EE_ASSERT( pFoundEntity != nullptr );
            m_editedEntityID = pFoundEntity->GetID();
            m_editedComponentID.Clear();
            m_requestRefresh = true;
        }
        else if ( auto pComponent = TryCast<EntityComponent>( pTypeInstance ) )
        {
            auto pFoundEntity = m_pWorld->FindEntity( pComponent->GetEntityID() );
            EE_ASSERT( pFoundEntity != nullptr );
            m_editedEntityID = pFoundEntity->GetID();
            m_editedComponentID = pComponent->GetID();
            m_requestRefresh = true;
        }
        else
        {
            EE_UNREACHABLE_CODE();
        }
    }

    //-------------------------------------------------------------------------

    bool EntityWorldPropertyGrid::UpdateAndDraw( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        if ( m_pWorld->HasPendingMapChangeActions() )
        {
            m_requestRefresh = true;
            m_propertyGrid.SetTypeToEdit( nullptr );
        }
        else // No pending operations, safe to refresh
        {
            if ( m_requestRefresh )
            {
                if ( !RefreshEditedTypeInstance() )
                {
                    Clear();
                }
                m_requestRefresh = false;
            }
        }

        //-------------------------------------------------------------------------

        bool isFocused = false;
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_windowName.c_str() ) )
        {
            auto const& style = ImGui::GetStyle();
            ImGui::PushStyleColor( ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg] );
            ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, style.FrameRounding );
            ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, style.FramePadding );
            if ( ImGui::BeginChild( "ST", ImVec2( -1, 24 ), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding ) )
            {
                if ( m_editedComponentID.IsValid() )
                {
                    auto pEntity = m_pWorld->FindEntity( m_editedEntityID );
                    auto pComponent = pEntity->FindComponent( m_editedComponentID );
                    ImGui::Text( "Entity: %s, Component: %s", pEntity->GetNameID().c_str(), pComponent->GetNameID().c_str() );
                }
                else if ( m_editedEntityID.IsValid() )
                {
                    auto pEntity = m_pWorld->FindEntity( m_editedEntityID );
                    ImGui::Text( "Entity: %s", pEntity->GetNameID().c_str() );
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar( 3 );
            ImGui::PopStyleColor();

            //-------------------------------------------------------------------------

            m_propertyGrid.DrawGrid();

            isFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows );
        }
        ImGui::End();

        //-------------------------------------------------------------------------

        return isFocused;
    }
}