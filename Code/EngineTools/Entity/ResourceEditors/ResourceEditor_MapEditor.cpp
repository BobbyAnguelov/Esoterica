#include "ResourceEditor_MapEditor.h"
#include "EngineTools/Navmesh/NavmeshGeneratorDialog.h"
#include "EngineTools/Core/Dialogs.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Navmesh/Components/Component_Navmesh.h"
#include "Engine/Navmesh/DebugViews/DebugView_Navmesh.h"
#include "Engine/Navmesh/Systems/WorldSystem_Navmesh.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityMapEditor::EntityMapEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld )
        : EntityEditor( pToolsContext, "Map Editor", pWorld )
    {}

    EntityMapEditor::~EntityMapEditor()
    {
        EE_ASSERT( m_pNavmeshGeneratorDialog == nullptr );
    }

    void EntityMapEditor::Initialize( UpdateContext const& context )
    {
        EntityEditor::Initialize( context );
        SetCameraSpeed( 15.0f );
    }

    //-------------------------------------------------------------------------

    EntityMap* EntityMapEditor::GetEditedMap() const
    {
        return m_pWorld->GetMap( m_editedMapID );
    }

    void EntityMapEditor::CreateNewFile() const
    {
        // Get new map filename
        //-------------------------------------------------------------------------

        FileDialog::Result const result = FileDialog::Save( m_pToolsContext, EntityMapDescriptor::GetStaticResourceTypeID(), m_pToolsContext->m_pFileRegistry->GetSourceDataDirectoryPath().c_str() );
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

        ResourceID const newMapResourceID = DataPath::FromFileSystemPath( m_pToolsContext->GetSourceDataDirectory(), mapFilePath );
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

        if ( WriteMapToFile( *m_pToolsContext->m_pTypeRegistry, EntityMap(), mapFilePath ) )
        {
            m_pToolsContext->TryOpenResource( newMapResourceID );
        }
        else
        {
            MessageDialog::Error( "Error", "Failed to create new map!" );
        }
    }

    void EntityMapEditor::SelectAndLoadMap()
    {
        FileDialog::Result result = FileDialog::Load( m_pToolsContext, EntityMapDescriptor::GetStaticResourceTypeID() );
        if ( !result )
        {
            return;
        }

        ResourceID const mapToLoad = DataPath::FromFileSystemPath( m_pToolsContext->GetSourceDataDirectory(), result );
        LoadMap( mapToLoad );
    }

    void EntityMapEditor::LoadMap( TResourcePtr<EntityModel::EntityMapDescriptor> const& mapToLoad )
    {
        if ( mapToLoad.GetResourceID() != m_loadedMap )
        {
            // Should we save the current map before unloading?
            if ( IsDirty() )
            {
                if ( MessageDialog::Confirmation( Severity::Warning, "Unsaved Changes", "You have unsaved changes! Do you want to save map?" ) )
                {
                    Save();
                }
            }

            // Clear all widget state
            m_structureEditorTreeView.ClearSelection();
            m_outlinerTreeView.ClearSelection();
            m_propertyGrid.SetTypeToEdit( nullptr );
            m_undoStack.Reset();

            // Unload current map
            if ( m_loadedMap.IsValid() && m_pWorld->HasMap( m_loadedMap ) )
            {
                m_pWorld->UnloadMap( m_loadedMap );
            }

            // Load map
            m_loadedMap = mapToLoad.GetResourceID();
            m_editedMapID = m_pWorld->LoadMap( m_loadedMap );
            SetDisplayName( m_loadedMap.GetResourcePath().GetFilenameWithoutExtension() );
        }
    }

    void EntityMapEditor::SaveMap()
    {
        Save();
    }

    void EntityMapEditor::SaveMapAs()
    {
        auto pEditedMap = GetEditedMap();
        if ( pEditedMap == nullptr || !( pEditedMap->IsLoaded() ) )
        {
            return;
        }

        // Get new map filename
        //-------------------------------------------------------------------------

        FileDialog::Result const result = FileDialog::Save( m_pToolsContext, EntityMapDescriptor::GetStaticResourceTypeID(), GetFileSystemPath( m_loadedMap ) );
        if ( !result )
        {
            return;
        }

        // Write the map out to a new path and load it
        //-------------------------------------------------------------------------

        if ( WriteMapToFile( *m_pToolsContext->m_pTypeRegistry, *pEditedMap, result.m_filePaths[0] ) )
        {
            ResourceID const mapResourcePath = DataPath::FromFileSystemPath( m_pToolsContext->GetSourceDataDirectory(), result.m_filePaths[0] );
            LoadMap( mapResourcePath );
        }
        else
        {
            MessageDialog::Error( "Error", "Failed to save file!" );
        }
    }

    bool EntityMapEditor::SaveData()
    {
        auto pEditedMap = GetEditedMap();
        if ( pEditedMap == nullptr || !( pEditedMap->IsLoaded() ) )
        {
            return false;
        }

        FileSystem::Path const mapFilePath = GetFileSystemPath( m_loadedMap );
        if ( !WriteMapToFile( *m_pToolsContext->m_pTypeRegistry, *pEditedMap, mapFilePath ) )
        {
            return false;
        }

        ClearDirty();
        return true;
    }

    //-------------------------------------------------------------------------

    void EntityMapEditor::DrawMenu( UpdateContext const& context )
    {
        EntityEditor::DrawMenu( context );

        // Tools
        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( !HasLoadedMap() );
        if ( ImGui::BeginMenu( EE_ICON_HAMMER_WRENCH" Tools" ) )
        {
            // Navmesh
            //-------------------------------------------------------------------------

            ImGui::SeparatorText( EE_ICON_WALK" Navmesh" );
            
            auto const& navmeshComponents = m_pWorld->GetAllComponentsOfType<Navmesh::NavmeshComponent>();
            bool const hasNavmeshComponent = !navmeshComponents.empty();

            if ( !hasNavmeshComponent )
            {
                // Create navmesh component entity
                if ( ImGui::MenuItem( "Create Navmesh Component" ) )
                {
                    CreateNavmeshComponent();
                }
            }

            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( !hasNavmeshComponent );
            if ( ImGui::MenuItem( "Generate Navmesh" ) )
            {
                BeginNavmeshGeneration( context );
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            ImGui::EndMenu();
        }
        ImGui::EndDisabled();

        // Visualizations
        //-------------------------------------------------------------------------

        if ( ImGui::BeginMenu( EE_ICON_MONITOR_EYE" Visualize" ) )
        {
            ImGui::SeparatorText( EE_ICON_MATH_COMPASS" Physics" );

            //auto pPhysicsWorldSystem = m_pWorld->GetWorldSystem<Physics::PhysicsWorldSystem>();
            bool isDebugEnabled = false;// pPhysicsWorldSystem->IsDebugDrawingEnabled();
            if ( ImGui::MenuItem( "Show Physics Collision", nullptr, &isDebugEnabled ) )
            {
                //pPhysicsWorldSystem->SetDebugDrawingEnabled( isDebugEnabled );
            }

            //-------------------------------------------------------------------------

            ImGui::SeparatorText( EE_ICON_WALK" Navmesh" );

            auto pNavmeshWorldSystem = m_pWorld->GetWorldSystem<Navmesh::NavmeshWorldSystem>();
            Navmesh::NavmeshDebugView::DrawNavmeshRuntimeSettings( pNavmeshWorldSystem );

            //-------------------------------------------------------------------------

            ImGui::SeparatorText( EE_ICON_CUBE_OUTLINE" Volumes" );

            for ( auto pVolumeTypeInfo : m_volumeTypes )
            {
                bool isVisualized = VectorContains( m_visualizedVolumeTypes, pVolumeTypeInfo );
                if ( ImGui::MenuItem( pVolumeTypeInfo->GetFriendlyTypeName(), nullptr, isVisualized ) )
                {
                    if ( isVisualized )
                    {
                        m_visualizedVolumeTypes.erase_first( pVolumeTypeInfo );
                    }
                    else
                    {
                        m_visualizedVolumeTypes.emplace_back( pVolumeTypeInfo );
                    }
                }
            }

            ImGui::EndMenu();
        }
    }

    void EntityMapEditor::DrawHelpMenu() const
    {
        DrawHelpTextRow( "Switch Gizmo Mode", "Spacebar" );
        DrawHelpTextRow( "Multi Select", "Ctrl/Shift + Left Click" );
        DrawHelpTextRow( "Directly Select Component", "Alt + Left Click" );
        DrawHelpTextRow( "Duplicate Selected Entities", "Alt + translate" );
    }

    void EntityMapEditor::DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        EntityEditor::DrawViewportToolbar( context, pViewport );

        ImGui::SameLine();

        //-------------------------------------------------------------------------

        ImGuiX::ScopedFont const sf( ImGuiX::Font::MediumBold );
        constexpr float const buttonWidth = 120;

        if ( !m_isGamePreviewRunning )
        {
            if ( ImGuiX::IconButton( EE_ICON_PLAY, "Play Map", Colors::Lime, ImVec2( buttonWidth, 0 ) ) )
            {
                m_requestStartGamePreview.Execute( context );
            }
        }
        else
        {
            if ( ImGuiX::IconButton( EE_ICON_STOP, "Stop Playing", Colors::Red, ImVec2( buttonWidth, 0 ) ) )
            {
                m_requestStopGamePreview.Execute( context );
            }
        }
    }

    void EntityMapEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        EntityEditor::Update( context, isVisible, isFocused );

        if ( m_pNavmeshGeneratorDialog != nullptr )
        {
            UpdateNavmeshGeneration( context );
        }
    }

    //-------------------------------------------------------------------------

    void EntityMapEditor::NotifyGamePreviewStarted()
    {
        m_pWorld->SuspendUpdates();
        m_isGamePreviewRunning = true;
    }

    void EntityMapEditor::NotifyGamePreviewEnded()
    {
        m_pWorld->ResumeUpdates();
        m_isGamePreviewRunning = false;
    }

    //-------------------------------------------------------------------------

    void EntityMapEditor::CreateNavmeshComponent()
    {
        // Create the appropriate resource ID for the navmesh data
        DataPath navmeshResourcePath = GetEditedMap()->GetMapResourceID().GetResourcePath();
        navmeshResourcePath.ReplaceExtension( Navmesh::NavmeshData::GetStaticResourceTypeID().ToString().c_str() );
        ResourceID const navmeshResourceID( navmeshResourcePath );

        // Create a new entity with a navmesh component if one isnt found
        Entity* pEntity = EE::New<Entity>( StringID( "Navmesh Entity" ) );
        pEntity->AddComponent( EE::New<Navmesh::NavmeshComponent>( navmeshResourceID ) );
        m_pWorld->GetFirstNonPersistentMap()->AddEntity( pEntity );
    }

    void EntityMapEditor::BeginNavmeshGeneration( UpdateContext const& context )
    {
        EE_ASSERT( m_pNavmeshGeneratorDialog == nullptr );

        // Try find navmesh component
        auto const& navmeshComponents = m_pWorld->GetAllComponentsOfType<Navmesh::NavmeshComponent>();
        if ( navmeshComponents.size() > 1 )
        {
            EE_LOG_WARNING( "Entity", "Map Editor", "Multiple navmesh components found in the map! This is not supported!" );
        }

        Navmesh::NavmeshComponent const* pNavmeshComponent = navmeshComponents.back();

        // Navmesh Generation
        //-------------------------------------------------------------------------

        FileSystem::Path navmeshFilePath = GetEditedMap()->GetMapResourceID().GetFileSystemPath( m_pToolsContext->m_pFileRegistry->GetSourceDataDirectoryPath() );
        navmeshFilePath.ReplaceExtension( Navmesh::NavmeshData::GetStaticResourceTypeID().ToString() );

        EntityMapDescriptor map;
        CreateEntityMapDescriptor( *m_pToolsContext->m_pTypeRegistry, GetEditedMap(), map );
        m_pNavmeshGeneratorDialog = EE::New<Navmesh::NavmeshGeneratorDialog>( m_pToolsContext, pNavmeshComponent->GetBuildSettings(), map, navmeshFilePath);
    }

    void EntityMapEditor::UpdateNavmeshGeneration( UpdateContext const& context )
    {
        EE_ASSERT( m_pNavmeshGeneratorDialog != nullptr );
        if ( !m_pNavmeshGeneratorDialog->UpdateAndDrawDialog( context ) )
        {
            EndNavmeshGeneration( context );
        }
    }

    void EntityMapEditor::EndNavmeshGeneration( UpdateContext const& context )
    {
        EE_ASSERT( m_pNavmeshGeneratorDialog != nullptr );

        // Update navmesh build settings if needed
        if ( m_pNavmeshGeneratorDialog->WereBuildSettingsUpdated() )
        {
            auto const& navmeshComponents = m_pWorld->GetAllRegisteredComponentsOfType<Navmesh::NavmeshComponent>();
            EE_ASSERT( navmeshComponents.size() == 1 );
           
            auto pNavmeshComponent = const_cast<Navmesh::NavmeshComponent*>( navmeshComponents.back() );
            m_pWorld->BeginComponentEdit( pNavmeshComponent );
            pNavmeshComponent->SetBuildSettings( m_pNavmeshGeneratorDialog->GetBuildSettings() );
            m_pWorld->EndComponentEdit( pNavmeshComponent );
        }

        // Destroy dialog
        EE::Delete( m_pNavmeshGeneratorDialog );
    }
}