#include "ResourceEditor_EntityCollectionEditor.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_Utils.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "EngineTools/Entity/ResourceDescriptors/ResourceDescriptor_EntityCollection.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/Threading/TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EE_RESOURCE_EDITOR_FACTORY( EntityCollectionEditorFactory, EntityCollection, EntityCollectionEditor );

    //-------------------------------------------------------------------------

    EntityCollectionEditor::EntityCollectionEditor( ToolsContext const* pToolsContext, ResourceID const& collectionResourceID, EntityWorld* pWorld )
        : EditorTool( pToolsContext, collectionResourceID.GetFilenameWithoutExtension(), pWorld )
        , m_editorContext( pToolsContext, &m_undoStack, pToolsContext->m_pDialogManager, m_pWorld, m_pCamera )
        , m_outliner( m_editorContext )
        , m_inspector( m_editorContext )
        , m_collection( collectionResourceID )
    {
        auto dropHandlers = m_pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( ViewportResourceDropHandler::GetStaticTypeID(), true, false );
        for ( auto pHandlerTypeInfo : dropHandlers )
        {
            m_pViewportDropHandlers.emplace_back( pHandlerTypeInfo->GetDefaultInstance<ViewportResourceDropHandler>() );
        }
    }

    void EntityCollectionEditor::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        CreateToolWindow( "Outliner", [this] ( UpdateContext const& context, bool isFocused ) { m_outliner.UpdateAndDraw( context, isFocused ); }, ImVec2( -1, -1 ) );
        CreateToolWindow( "Inspector", [this] ( UpdateContext const& context, bool isFocused ) { m_inspector.UpdateAndDraw( context, isFocused ); }, ImVec2( -1, -1 ), true );

        SetFloorVisibility( false );

        LoadResource( &m_collection );
        m_collectionInstantiated = false;
    }

    void EntityCollectionEditor::Shutdown( UpdateContext const& context )
    {
        if ( m_collection.IsLoaded() )
        {
            UnloadResource( &m_collection );
        }

        m_collectionInstantiated = false;

        EditorTool::Shutdown( context );
    }

    void EntityCollectionEditor::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID leftDockID = 0, topRightDockID = 0, bottomRightDockID = 0;
        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Right, 0.25f, &topRightDockID, &leftDockID );
        ImGui::DockBuilderSplitNode( topRightDockID, ImGuiDir_Up, 0.75f, &topRightDockID, &bottomRightDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Outliner" ).c_str(), topRightDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Inspector" ).c_str(), bottomRightDockID );
    }

    //-------------------------------------------------------------------------

    String EntityCollectionEditor::GetFilenameForSave() const
    {
        return GetFileSystemPath( m_collection.GetDataPath() ).ToString();
    }

    bool EntityCollectionEditor::SaveData()
    {
        auto pEditedMap = m_pWorld->GetFirstNonPersistentMap();
        if ( pEditedMap == nullptr || !pEditedMap->IsMapLoaded() )
        {
            return false;
        }

        EntityCollectionResourceDescriptor resourceDescriptor;

        Log log( LogCategory::Entity, "Save Entity Collection" );
        if ( !CreateEntityMapDescriptor( *m_pToolsContext->m_pTypeRegistry, log, pEditedMap, resourceDescriptor.m_collection ) )
        {
            return false;
        }

        resourceDescriptor.m_entityGroups = m_editorContext.GetEntityGroupsForMap( pEditedMap->GetID() );
        resourceDescriptor.m_editorCameraTransform = GetCameraTransform();

        FileSystem::Path const filePath = GetFileSystemPath( m_collection.GetDataPath() );
        if ( !Resource::ResourceDescriptor::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, log, filePath, &resourceDescriptor ) )
        {
            return false;
        }

        ClearDirty();
        return true;
    }

    void EntityCollectionEditor::PreUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        EditorTool::PreUndoRedo( operation, pAction );
        m_editorContext.PreUndoRedo( operation, pAction );
    }

    void EntityCollectionEditor::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        EditorTool::PostUndoRedo( operation, pAction );
        m_editorContext.PostUndoRedo( operation, pAction );
    }

    //-------------------------------------------------------------------------

    void EntityCollectionEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        if ( !m_collectionInstantiated )
        {
            if ( m_collection.IsLoaded() )
            {
                // Create transient map for the collection editing
                auto pMap = m_pWorld->CreateTransientMap();
                pMap->AddEntityCollection( context.GetSystem<TaskSystem>(), *m_pToolsContext->m_pTypeRegistry, *m_collection.GetPtr() );

                // Load descriptor
                Log log;
                EntityCollectionResourceDescriptor collectionDesc;
                if ( EntityCollectionResourceDescriptor::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, log, GetFileSystemPath( m_collection.GetDataPath() ), collectionDesc ) )
                {
                    m_editorContext.SetEntityGroupsForMap( pMap->GetID(), collectionDesc.m_entityGroups );
                }

                m_editorContext.SetEditedMap( pMap->GetID() );
                SetCameraTransform( collectionDesc.m_editorCameraTransform );

                // Unload the collection resource
                m_collectionInstantiated = true;
                UnloadResource( &m_collection );
            }
        }

        m_editorContext.Update( context );

        // Handle input
        //-------------------------------------------------------------------------

        if ( m_isViewportFocused || m_isViewportHovered )
        {
            if ( ImGui::IsKeyPressed( ImGuiKey_Space ) )
            {
                m_gizmo.SwitchToNextMode();
            }
        }

        //-------------------------------------------------------------------------

        if ( m_drawGrid )
        {
            DrawGrid();
        }

        auto drawingCtx = GetDebugDrawContext();

        // Draw selection bounds
        //-------------------------------------------------------------------------

        if ( m_editorContext.HasSpatialSelection() )
        {
            drawingCtx.DrawWireBox( m_editorContext.GetSpatialSelectionCombinedBounds(), Colors::Yellow, 3.0f, DebugDrawLayer::World );

            if ( m_editorContext.GetSpatialSelectionBounds().size() > 1 )
            {
                for ( OBB const& bounds : m_editorContext.GetSpatialSelectionBounds() )
                {
                    drawingCtx.DrawWireBox( bounds, Colors::Cyan, 1.0f, DebugDrawLayer::World );
                }
            }

            TInlineVector<Transform, 10> transforms;
            m_editorContext.GetSpatialSelectionWorldTransforms( transforms );
            for ( Transform const& transform : transforms )
            {
                drawingCtx.DrawAxis( transform, 0.25f );
            }
        }
    }

    void EntityCollectionEditor::DrawMenu( UpdateContext const& context )
    {
        EditorTool::DrawMenu( context );

        if ( ImGui::BeginMenu( EE_ICON_TUNE" Options" ) )
        {
            ImGuiX::Checkbox( "Draw Grid", &m_drawGrid );
            ImGui::EndMenu();
        }
    }

    void EntityCollectionEditor::ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport )
    {
        ImGui::SameLine();
        ImGui::BeginDisabled( !m_editorContext.HasSpatialSelection() );
        float const requiredGizmoControlsWidth = GizmoViewportControls::GetRequiredSize( m_gizmo );
        if ( BeginViewportToolbarGroup( "SpatialControls", ImVec2( requiredGizmoControlsWidth, 0 ), ImVec2( 0, 0 ) ) )
        {
            GizmoViewportControls::Draw( m_gizmo );
        }
        EndViewportToolbarGroup();
        ImGui::EndDisabled();
    }

    void EntityCollectionEditor::DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused )
    {
        // Update Gizmo
        //-------------------------------------------------------------------------

        if ( m_gizmo.GetMode() == ImGuiX::Gizmo::Mode::Scale )
        {
            m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowNonUniformScale, m_editorContext.DoesSpatialSelectionSupportNonUniformScale() );
        }

        Transform const& selectionTransform = m_editorContext.GetSpatialSelectionTransform();
        auto const gizmoResult = m_gizmo.Draw( selectionTransform.GetTranslation(), selectionTransform.GetRotation(), *pViewport );
        switch ( gizmoResult.m_state )
        {
            case ImGuiX::GizmoState::StartedManipulating:
            {
                m_editorContext.BeginManipulatingSpatialSelection( gizmoResult, ImGui::GetIO().KeyAlt );
            }
            break;

            case ImGuiX::GizmoState::Manipulating:
            {
                m_editorContext.ManipulateSpatialSelection( gizmoResult );
            }
            break;

            case ImGuiX::GizmoState::StoppedManipulating:
            {
                m_editorContext.EndManipulatingSpatialSelection( gizmoResult );
            }
            break;

            default:
            break;
        }
    }

    void EntityCollectionEditor::DropResourceInViewport( ResourceID const& resourceID, Vector const& worldPosition )
    {
        EE_ASSERT( resourceID.IsValid() );

        auto pMap = m_pWorld->GetFirstNonPersistentMap();
        if ( pMap == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        for ( auto pHandler : m_pViewportDropHandlers )
        {
            if ( pHandler->CanCreateEntitiesFromResourceType( resourceID.GetResourceTypeID() ) )
            {
                pHandler->CreateEntitiesFromResource( m_editorContext, pMap, resourceID, worldPosition );
            }
        }
    }

    void EntityCollectionEditor::HandleViewportInteractions()
    {
        if ( !ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
        {
            return;
        }

        if ( m_gizmo.IsManipulating() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        PickingID pickingID = GetPickingID();
        if ( !pickingID.IsSet() )
        {
            m_editorContext.ClearSelection();
            return;
        }

        //-------------------------------------------------------------------------

        EntityID const pickedEntityID( pickingID.m_primaryID );
        auto pEntity = m_pWorld->FindEntity( pickedEntityID );
        if ( pEntity == nullptr )
        {
            m_editorContext.ClearSelection();
            return;
        }

        //-------------------------------------------------------------------------

        // If we have alt-held, select the individual component
        if ( ImGui::GetIO().KeyAlt )
        {
            if ( pickingID.HasSecondaryID() )
            {
                ComponentID const pickedComponentID( pickingID.m_secondaryID );
                auto pComponent = pEntity->FindComponent( pickedComponentID );
                EE_ASSERT( pComponent != nullptr );

                EntityEditorItem const item( pEntity, pComponent );
                m_editorContext.SetSelection( item );
            }
            else
            {
                EntityEditorItem const item( pEntity );
                m_editorContext.SetSelection( item );
            }
        }
        // Else just select the entire entity
        else if ( ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl )
        {
            EntityEditorItem selectedEntity( pEntity );

            if ( m_editorContext.IsSelected( selectedEntity ) )
            {
                m_editorContext.RemoveFromSelection( selectedEntity );
            }
            else
            {
                m_editorContext.AddToSelection( selectedEntity );
            }
        }
        else // No modifier so just set selection
        {
            m_editorContext.SetSelection( pEntity );
        }
    }
}