#include "Workspace_MapEditor.h"
#include "EngineTools/Navmesh/NavmeshGeneratorDialog.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "EngineTools/Core/CommonDialogs.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "Engine/Entity/EntitySerialization.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Navmesh/Components/Component_Navmesh.h"
#include "Engine/Navmesh/DebugViews/DebugView_Navmesh.h"
#include "Engine/Navmesh/Systems/WorldSystem_Navmesh.h"
#include "System/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityMapEditor::EntityMapEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld )
        : EntityEditorWorkspace( pToolsContext, pWorld, String( "Map Editor" ) )
    {
        SetCameraSpeed( 15.0f );
    }

    EntityMapEditor::~EntityMapEditor()
    {
        EE_ASSERT( m_pNavmeshGeneratorDialog == nullptr );
    }

    //-------------------------------------------------------------------------

    EntityMap* EntityMapEditor::GetEditedMap() const
    {
        return m_pWorld->GetMap( m_editedMapID );
    }

    void EntityMapEditor::CreateNewMap()
    {
        // Should we save the current map before unloading?
        if ( IsDirty() )
        {
            pfd::message saveChangesMsg( m_loadedMap.c_str(), "You have unsaved changes! Do you want to save map?", pfd::choice::yes_no );
            if ( saveChangesMsg.result() == pfd::button::yes )
            {
                Save();
            }
        }

        // Get new map filename
        //-------------------------------------------------------------------------

        auto const mapFilename = pfd::save_file( "Create New Map", m_pToolsContext->m_pResourceDatabase->GetRawResourceDirectoryPath().c_str(), { "Map Files", "*.map" } ).result();
        if ( mapFilename.empty() )
        {
            return;
        }

        FileSystem::Path const mapFilePath( mapFilename.c_str() );
        if ( FileSystem::Exists( mapFilePath ) )
        {
            if ( pfd::message( "Confirm Overwrite", "Map file exists, should we overwrite it?", pfd::choice::yes_no, pfd::icon::error ).result() == pfd::button::no )
            {
                return;
            }
        }

        ResourceID const mapResourceID = GetResourcePath( mapFilePath );
        if ( mapResourceID == m_loadedMap )
        {
            pfd::message( "Error", "Cant override currently loaded map!", pfd::choice::ok, pfd::icon::error ).result();
            return;
        }

        if ( mapResourceID.GetResourceTypeID() != SerializedEntityMap::GetStaticResourceTypeID() )
        {
            pfd::message( "Error", "Invalid map extension provided! Maps need to have the .map extension!", pfd::choice::ok, pfd::icon::error ).result();
            return;
        }

        // Write the map out to a new path and load it
        //-------------------------------------------------------------------------

        if ( WriteMapToFile( *m_pToolsContext->m_pTypeRegistry, EntityMap(), mapFilePath ) )
        {
            LoadMap( mapResourceID );
        }
        else
        {
            pfd::message( "Error", "Failed to create new map!", pfd::choice::ok, pfd::icon::error ).result();
        }
    }

    void EntityMapEditor::SelectAndLoadMap()
    {
        auto const selectedMap = pfd::open_file( "Load Map", m_pToolsContext->m_pResourceDatabase->GetRawResourceDirectoryPath().c_str(), { "Map Files", "*.map" }, pfd::opt::none ).result();
        if ( selectedMap.empty() )
        {
            return;
        }

        FileSystem::Path const mapFilePath( selectedMap[0].c_str() );
        ResourceID const mapToLoad = GetResourcePath( mapFilePath );
        LoadMap( mapToLoad );
    }

    void EntityMapEditor::LoadMap( TResourcePtr<EntityModel::SerializedEntityMap> const& mapToLoad )
    {
        if ( mapToLoad.GetResourceID() != m_loadedMap )
        {
           // m_outliner.ClearSelection();

            // Should we save the current map before unloading?
            if ( IsDirty() )
            {
                pfd::message saveChangesMsg( m_loadedMap.c_str(), "You have unsaved changes! Do you want to save map?", pfd::choice::yes_no);
                if ( saveChangesMsg.result() == pfd::button::yes )
                {
                    Save();
                }
            }

            // Unload current map
            if ( m_loadedMap.IsValid() && m_pWorld->HasMap( m_loadedMap ) )
            {
                m_pWorld->UnloadMap( m_loadedMap );
            }

            // Load map
            m_loadedMap = mapToLoad.GetResourceID();
            m_editedMapID = m_pWorld->LoadMap( m_loadedMap );
            SetDisplayName( m_loadedMap.GetResourcePath().GetFileNameWithoutExtension() );

            // Reset widget
          //  m_entityStructureEditor.SetEntityToEdit( nullptr );
            m_undoStack.Reset();
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

        FileSystem::Path const mapFilePath = SaveDialog( "Map", GetFileSystemPath( m_loadedMap ).GetParentDirectory().c_str(), "Map File");
        if ( !mapFilePath.IsValid() )
        {
            return;
        }

        // Write the map out to a new path and load it
        //-------------------------------------------------------------------------

        if ( WriteMapToFile( *m_pToolsContext->m_pTypeRegistry, *pEditedMap, mapFilePath ) )
        {
            ResourceID const mapResourcePath = GetResourcePath( mapFilePath );
            LoadMap( mapResourcePath );
        }
        else
        {
            pfd::message( "Error", "Failed to save file!", pfd::choice::ok, pfd::icon::error ).result();
        }
    }

    bool EntityMapEditor::Save()
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
        if ( ImGui::MenuItem( EE_ICON_FILE"##NewMap" ) )
        {
            CreateNewMap();
        }
        ImGuiX::ItemTooltip( "New" );

        EntityEditorWorkspace::DrawMenu( context );

        // Tools
        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( !HasLoadedMap() );
        if ( ImGui::BeginMenu( EE_ICON_HAMMER_WRENCH" Tools" ) )
        {
            // Navmesh
            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( EE_ICON_WALK" Navmesh" );
            
            auto const& navmeshComponents = m_pWorld->GetAllRegisteredComponentsOfType<Navmesh::NavmeshComponent>();
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
            ImGuiX::TextSeparator( EE_ICON_MATH_COMPASS" Physics" );

            //auto pPhysicsWorldSystem = m_pWorld->GetWorldSystem<Physics::PhysicsWorldSystem>();
            bool isDebugEnabled = false;// pPhysicsWorldSystem->IsDebugDrawingEnabled();
            if ( ImGui::MenuItem( "Show Physics Collision", nullptr, &isDebugEnabled ) )
            {
                //pPhysicsWorldSystem->SetDebugDrawingEnabled( isDebugEnabled );
            }

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( EE_ICON_WALK" Navmesh" );

            auto pNavmeshWorldSystem = m_pWorld->GetWorldSystem<Navmesh::NavmeshWorldSystem>();
            Navmesh::NavmeshDebugView::DrawNavmeshRuntimeSettings( pNavmeshWorldSystem );

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( EE_ICON_CUBE_OUTLINE" Volumes" );

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

        // Help
        //-------------------------------------------------------------------------

        if ( ImGui::BeginMenu( EE_ICON_HELP_CIRCLE_OUTLINE" Help" ) )
        {
            auto DrawHelpRow = []( char const* pLabel, char const* pHotkey )
            {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                {
                    ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
                    ImGui::Text( pLabel );
                }

                ImGui::TableNextColumn();
                {
                    ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );
                    ImGui::Text( pHotkey );
                }
            };

            //-------------------------------------------------------------------------

            if ( ImGui::BeginTable( "HelpTable", 2 ) )
            {
                DrawHelpRow( "Switch Gizmo Mode", "Spacebar" );
                DrawHelpRow( "Multi Select", "Ctrl/Shift + Left Click" );
                DrawHelpRow( "Directly Select Component", "Alt + Left Click" );
                DrawHelpRow( "Duplicate Selected Entities", "Alt + translate" );

                ImGui::EndTable();
            }

            ImGui::EndMenu();
        }
    }

    void EntityMapEditor::DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        EntityEditorWorkspace::DrawViewportToolbar( context, pViewport );

        ImGui::SameLine();

        //-------------------------------------------------------------------------

        ImGuiX::ScopedFont const sf( ImGuiX::Font::MediumBold );
        constexpr float const buttonWidth = 120;

        if ( !m_isGamePreviewRunning )
        {
            if ( ImGuiX::IconButton( EE_ICON_PLAY, "Play Map", ImGuiX::ImColors::Lime, ImVec2( buttonWidth, 0 ) ) )
            {
                m_requestStartGamePreview.Execute( context );
            }
        }
        else
        {
            if ( ImGuiX::IconButton( EE_ICON_STOP, "Stop Playing", ImGuiX::ImColors::Red, ImVec2( buttonWidth, 0 ) ) )
            {
                m_requestStopGamePreview.Execute( context );
            }
        }
    }

    void EntityMapEditor::Update( UpdateContext const& context, bool isFocused )
    {
        EntityEditorWorkspace::Update( context, isFocused );

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
        ResourcePath navmeshResourcePath = GetEditedMap()->GetMapResourceID().GetResourcePath();
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
        auto const& navmeshComponents = m_pWorld->GetAllRegisteredComponentsOfType<Navmesh::NavmeshComponent>();
        if ( navmeshComponents.size() > 1 )
        {
            EE_LOG_WARNING( "Entity", "Map Editor", "Multiple navmesh components found in the map! This is not supported!" );
        }

        Navmesh::NavmeshComponent const* pNavmeshComponent = navmeshComponents.back();

        // Navmesh Generation
        //-------------------------------------------------------------------------

        FileSystem::Path navmeshFilePath = GetEditedMap()->GetMapResourceID().GetResourcePath().ToFileSystemPath( m_pToolsContext->m_pResourceDatabase->GetRawResourceDirectoryPath() );
        navmeshFilePath.ReplaceExtension( Navmesh::NavmeshData::GetStaticResourceTypeID().ToString() );

        SerializedEntityMap map;
        Serializer::SerializeEntityMap( *m_pToolsContext->m_pTypeRegistry, GetEditedMap(), map );
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