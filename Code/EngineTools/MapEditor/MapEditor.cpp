#include "MapEditor.h"
#include "MapEditorMode.h"
#include "EngineTools/Entity/ResourceDescriptors/ResourceDescriptor_EntityMap.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_Utils.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "EngineTools/Core/SystemDialogs.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    MapEditor::MapEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld )
        : EditorTool( pToolsContext, "Map Editor", pWorld )
        , m_editorContext( pToolsContext, &m_undoStack, pToolsContext->m_pDialogManager, m_pWorld, m_pCamera )
        , m_outliner( m_editorContext )
        , m_inspector( m_editorContext )
    {
        auto dropHandlers = m_pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( ViewportResourceDropHandler::GetStaticTypeID(), true, false );
        for ( auto pHandlerTypeInfo : dropHandlers )
        {
            m_pViewportDropHandlers.emplace_back( pHandlerTypeInfo->GetDefaultInstance<ViewportResourceDropHandler>() );
        }

        m_editModeTypeInfos = m_pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( MapEditorMode::GetStaticTypeID(), false, false, true );
    }

    MapEditor::~MapEditor()
    {
        EE_ASSERT( m_pActiveEditMode == nullptr );
    }

    void MapEditor::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        CreateToolWindow( "Outliner", [this] ( UpdateContext const& context, bool isFocused ) { m_outliner.UpdateAndDraw( context, isFocused ); }, ImVec2( -1, -1 ) );
        CreateToolWindow( "Inspector", [this] ( UpdateContext const& context, bool isFocused ) { m_inspector.UpdateAndDraw( context, isFocused ); }, ImVec2( -1, -1 ), true );
        CreateToolWindow( "Edit Mode", [this] ( UpdateContext const& context, bool isFocused ) { DrawEditModeWindow( context, isFocused ); }, ImVec2( -1, -1 ) );

        SetCameraSpeed( 15.0f );
    }

    void MapEditor::Shutdown( UpdateContext const& context )
    {
        ClearEditMode();
        EditorTool::Shutdown( context );
    }

    void MapEditor::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID leftDockID = 0, rightDockID = 0, centerDockID = 0, outlinerDockID = 0, inspectorDockID = 0;
        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Right, 0.33f, &rightDockID, &leftDockID );
        ImGui::DockBuilderSplitNode( leftDockID, ImGuiDir_Left, 0.25f, &leftDockID, &centerDockID );
        ImGui::DockBuilderSplitNode( rightDockID, ImGuiDir_Left, 0.5f, &outlinerDockID, &inspectorDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Edit Mode" ).c_str(), leftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Outliner" ).c_str(), outlinerDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Inspector" ).c_str(), inspectorDockID );
    }

    //-------------------------------------------------------------------------

    void MapEditor::CreateNewFile() const
    {
        // Get new map filename
        //-------------------------------------------------------------------------

        FileDialog::Result const result = FileDialog::SaveResourceOrDataFile( m_pToolsContext, EntityMapResourceDescriptor::GetStaticTypeID(), m_pToolsContext->m_pFileRegistry->GetSourceDataDirectoryPath().c_str() );
        if ( !result )
        {
            return;
        }

        FileSystem::Path const mapFilePath( result.m_filePaths[0].c_str() );
        if ( FileSystem::Exists( mapFilePath ) )
        {
            if ( !MessageDialog::Confirmation( Severity::Warning, "Confirm Overwrite", "Map file exists, should we overwrite it?" ) )
            {
                return;
            }
        }

        ResourceID const newMapResourceID = DataPath( mapFilePath, m_pToolsContext->GetSourceDataDirectory() );
        if ( newMapResourceID == m_loadedMap )
        {
            MessageDialog::Error( "Error", "Cant override currently loaded map!" );
            return;
        }

        if ( newMapResourceID.GetResourceTypeID() != EntityMapDescriptor::GetStaticResourceTypeID() )
        {
            MessageDialog::Error( "Error", "Invalid map extension provided! Maps need to have the .map extension!" );
            return;
        }

        // Write the map out to a new path and load it
        //-------------------------------------------------------------------------

        EntityMapResourceDescriptor mapResourceDesc;
        Log log;
        if ( Resource::ResourceDescriptor::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, log, mapFilePath, &mapResourceDesc ) )
        {
            m_pToolsContext->TryOpenResource( newMapResourceID );
        }
        else
        {
            MessageDialog::Error( "Error", "Failed to create new map!" );
        }
    }

    void MapEditor::SelectAndLoadMap()
    {
        FileDialog::Result result = FileDialog::LoadResourceOrDataFile( m_pToolsContext, EntityMapResourceDescriptor::GetStaticTypeID() );
        if ( !result )
        {
            return;
        }

        ResourceID const mapToLoad = DataPath( result, m_pToolsContext->GetSourceDataDirectory() );
        LoadMap( mapToLoad );
    }

    void MapEditor::LoadMap( ResourceID const& mapToLoad )
    {
        EE_ASSERT( mapToLoad.IsValid() );
        EE_ASSERT( mapToLoad.GetResourceTypeID() == EntityMapDescriptor::GetStaticResourceTypeID() );

        if ( mapToLoad != m_loadedMap )
        {
            m_editorContext.ClearSelection();
            m_editorContext.ClearSpatialSelection();

            // Should we save the current map before unloading?
            if ( IsDirty() )
            {
                if ( MessageDialog::Confirmation( Severity::Warning, "Unsaved Changes", "You have unsaved changes! Do you want to save map?" ) )
                {
                    Save();
                }
            }

            m_undoStack.Reset();

            // Unload current map
            if ( m_loadedMap.IsValid() && m_pWorld->HasMap( m_loadedMap ) )
            {
                m_pWorld->UnloadMap( m_loadedMap );
            }

            // Load map
            m_loadedMap = mapToLoad;
            m_editedMapID = m_pWorld->LoadMap( m_loadedMap );
            m_editorContext.SetEditedMap( m_editedMapID );
            SetDisplayName( m_loadedMap.GetDataPath().GetFilenameWithoutExtension() );

            // Descriptor
            //-------------------------------------------------------------------------

            EntityMapResourceDescriptor mapDesc;
            Log log;
            if ( EntityMapResourceDescriptor::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, log, GetFileSystemPath( m_loadedMap.GetDataPath() ), mapDesc ) )
            {
                m_editorContext.SetEntityGroupsForMap( m_editedMapID, mapDesc.m_entityGroups );
                SetCameraTransform( mapDesc.m_editorCameraTransform );
            }
        }
    }

    String MapEditor::GetFilenameForSave() const
    {
        if ( !m_loadedMap.IsValid() )
        {
            return String();
        }

       return GetFileSystemPath( m_loadedMap ).ToString();
    }

    void MapEditor::SaveMap()
    {
        Save();
    }

    void MapEditor::SaveMapAs()
    {
        auto pEditedMap = m_editorContext.GetEditedMap();
        if ( pEditedMap == nullptr || !( pEditedMap->IsMapLoaded() ) )
        {
            return;
        }

        // Get new map filename
        //-------------------------------------------------------------------------

        FileDialog::Result const result = FileDialog::SaveResourceOrDataFile( m_pToolsContext, EntityMapResourceDescriptor::GetStaticTypeID(), GetFileSystemPath( m_loadedMap ) );
        if ( !result )
        {
            return;
        }

        // Write the map out to a new path and load it
        //-------------------------------------------------------------------------

        EntityMapResourceDescriptor resourceDescriptor;
        Log log( LogCategory::Entity, "Save Map" );
        if ( !CreateEntityMapDescriptor( *m_pToolsContext->m_pTypeRegistry, log, pEditedMap, resourceDescriptor.m_mapDescriptor ) )
        {
            MessageDialog::Error( "Error", "Failed to save file!" );
            return;
        }

        if ( Resource::ResourceDescriptor::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, log, result.m_filePaths[0], &resourceDescriptor ) )
        {
            ResourceID const mapResourcePath = DataPath( result.m_filePaths[0], m_pToolsContext->GetSourceDataDirectory() );
            LoadMap( mapResourcePath );
        }
        else
        {
            MessageDialog::Error( "Error", "Failed to save file!" );
        }
    }

    bool MapEditor::SaveData()
    {
        auto pEditedMap = m_editorContext.GetEditedMap();
        if ( pEditedMap == nullptr || !( pEditedMap->IsMapLoaded() ) )
        {
            return false;
        }

        EntityMapResourceDescriptor resourceDescriptor;
        Log log( LogCategory::Entity, "Save Map" );
        if ( !CreateEntityMapDescriptor( *m_pToolsContext->m_pTypeRegistry, log, pEditedMap, resourceDescriptor.m_mapDescriptor ) )
        {
            MessageDialog::Error( "Error", "Failed to save file!" );
            return false;
        }

        resourceDescriptor.m_entityGroups = m_editorContext.GetEntityGroupsForMap( m_editedMapID );
        resourceDescriptor.m_editorCameraTransform = GetCameraTransform();

        FileSystem::Path const mapFilePath = GetFileSystemPath( m_loadedMap );
        if ( !Resource::ResourceDescriptor::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, log, mapFilePath, &resourceDescriptor ) )
        {
            return false;
        }

        ClearDirty();
        return true;
    }

    //-------------------------------------------------------------------------

    void MapEditor::DrawMenu( UpdateContext const& context )
    {
        EditorTool::DrawMenu( context );
    }

    void MapEditor::DrawHelpMenu() const
    {
        DrawHelpTextRow( "Switch Gizmo Mode", "Spacebar" );
        DrawHelpTextRow( "Multi Select", "Ctrl/Shift + Left Click" );
        DrawHelpTextRow( "Directly Select Component", "Alt + Left Click" );
        DrawHelpTextRow( "Duplicate Selected Entities", "Alt + translate" );
    }

    void MapEditor::DrawToolbar( UpdateContext const& context )
    {
        float toolbarWidth = ImGui::GetContentRegionAvail().x;

        // Mode Selector
        //-------------------------------------------------------------------------

        constexpr static float const modeSelectorWidth = 250;

        InlineString currentModeStr;
        if ( m_pActiveEditMode == nullptr )
        {
            currentModeStr = "No Edit Mode Selected";
        }
        else
        {
            currentModeStr = m_pActiveEditMode->GetName();
        }

        ImGui::SetNextItemWidth( modeSelectorWidth );
        if ( ImGui::BeginCombo( "##ModeSelector", currentModeStr.c_str(), ImGuiComboFlags_HeightLargest ) )
        {
            if ( ImGui::MenuItem( "No Mode" ) )
            {
                ClearEditMode();
            }

            for ( auto pEditModeTypeInfo : m_editModeTypeInfos )
            {
                MapEditorMode const* pDefaultInstance = pEditModeTypeInfo->GetDefaultInstance<MapEditorMode>();
                if ( ImGui::MenuItem( pDefaultInstance->GetName() ) )
                {
                    SwitchEditMode( pEditModeTypeInfo );
                }
            }

            ImGui::EndCombo();
        }

        ImGui::BeginDisabled( m_pActiveEditMode == nullptr );
        ImGui::SameLine();
        if ( ImGuiX::IconButton( EE_ICON_CANCEL, "##EditModeCancel", Colors::Red, ImVec2( ImGuiX::CalculateButtonWidth( EE_ICON_CANCEL ), 0 ) ) )
        {
            ClearEditMode();
        }
        ImGui::EndDisabled();

        // Play Pause
        //-------------------------------------------------------------------------

        ImGuiX::ScopedFont const sf( ImGuiX::Font::MediumBold );
        constexpr static float const previewButtonWidth = 150;

        ImGui::SameLine( ( toolbarWidth - previewButtonWidth ) / 2.0f, 0.0f );

        if ( !m_isGamePreviewRunning )
        {
            if ( ImGuiX::IconButton( EE_ICON_PLAY, "Play Map", Colors::Lime, ImVec2( previewButtonWidth, 0 ), true ) )
            {
                if ( IsDirty() )
                {
                    Save();
                }
                m_requestStartGamePreview.Execute( context );
            }
        }
        else
        {
            if ( ImGuiX::IconButton( EE_ICON_STOP, "Stop Playing", Colors::Red, ImVec2( previewButtonWidth, 0 ), true ) )
            {
                m_requestStopGamePreview.Execute( context );
            }
        }
    }

    bool MapEditor::ExtendViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport )
    {
        return false;
    }

    void MapEditor::ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport )
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

    void MapEditor::DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused )
    {
        auto drawingCtx = GetDebugDrawContext();

        // Edit Mode
        //-------------------------------------------------------------------------

        if ( m_pActiveEditMode != nullptr )
        {
            m_pActiveEditMode->DrawViewportOverlayElements( context, pViewport, m_isViewportHovered, m_isViewportFocused  );

            if ( !m_pActiveEditMode->AllowDefaultViewportOverlayElements() )
            {
                return;
            }
        }

        // Selection and Manipulation
        //-------------------------------------------------------------------------

        if ( m_editorContext.HasSpatialSelection() )
        {
            // Draw selection bounds
            //-------------------------------------------------------------------------

            drawingCtx.DrawWireBox( m_editorContext.GetSpatialSelectionCombinedBounds(), Colors::Yellow, 3.0f );

            if ( m_editorContext.GetSpatialSelectionBounds().size() > 1 )
            {
                for ( OBB const& bounds : m_editorContext.GetSpatialSelectionBounds() )
                {
                    drawingCtx.DrawWireBox( bounds, Colors::Cyan, 1.0f );
                }
            }

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
    }

    void MapEditor::HandleViewportInteractions()
    {
        if ( m_pActiveEditMode != nullptr )
        {
            if ( !m_pActiveEditMode->AllowDefaultViewportInteractions() )
            {
                return;
            }
        }

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

    void MapEditor::DropResourceInViewport( ResourceID const& resourceID, Vector const& worldPosition )
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

    void MapEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        m_editorContext.Update( context );

        // Handle input
        //-------------------------------------------------------------------------

        if ( ( m_isViewportFocused || m_isViewportHovered ) && m_editorContext.HasSpatialSelection() )
        {
            if ( ImGui::IsKeyPressed( ImGuiKey_Space ) )
            {
                m_gizmo.SwitchToNextMode();
            }
        }
    }

    //-------------------------------------------------------------------------

    void MapEditor::PreUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        EditorTool::PreUndoRedo( operation, pAction );
        m_editorContext.PreUndoRedo( operation, pAction );
    }

    void MapEditor::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        EditorTool::PostUndoRedo( operation, pAction );
        m_editorContext.PostUndoRedo( operation, pAction );
    }

    //-------------------------------------------------------------------------

    void MapEditor::NotifyGamePreviewStarted()
    {
        m_pWorld->SuspendUpdates();
        m_isGamePreviewRunning = true;
    }

    void MapEditor::NotifyGamePreviewEnded()
    {
        m_pWorld->ResumeUpdates();
        m_isGamePreviewRunning = false;
    }

    //-------------------------------------------------------------------------

    void MapEditor::DrawEditModeWindow( UpdateContext const& context, bool isFocused )
    {
        if ( m_pActiveEditMode == nullptr )
        {
            ImGui::Text( "No Active Mode" );
        }
        else
        {
            m_pActiveEditMode->m_pickingData = GetPickingData();
            m_pActiveEditMode->UpdateAndDraw( context, isFocused );
        }
    }

    void MapEditor::SwitchEditMode( TypeSystem::TypeInfo const* pNewModeTypeInfo )
    {
        if ( m_pActiveEditMode != nullptr && pNewModeTypeInfo != nullptr && m_pActiveEditMode->GetTypeID() == pNewModeTypeInfo->m_ID )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( m_pActiveEditMode != nullptr )
        {
            m_pActiveEditMode->Shutdown();
            EE::Delete( m_pActiveEditMode );
        }

        if ( pNewModeTypeInfo != nullptr )
        {
            m_pActiveEditMode = Cast<MapEditorMode>( pNewModeTypeInfo->CreateType() );
            m_pActiveEditMode->Initialize( &m_editorContext );
        }
    }
}