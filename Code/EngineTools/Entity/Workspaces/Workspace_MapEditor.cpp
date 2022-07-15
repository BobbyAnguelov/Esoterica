#include "Workspace_MapEditor.h"
#include "EngineTools/Navmesh/NavmeshGeneratorDialog.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "EngineTools/Core/Helpers/CommonDialogs.h"
#include "Engine/Navmesh/Components/Component_Navmesh.h"
#include "Engine/Entity/EntitySerialization.h"
#include "System/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityMapEditor::EntityMapEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld )
        : EntityEditorBaseWorkspace( pToolsContext, pWorld )
    {
        m_gizmo.SetTargetTransform( &m_gizmoTransform );
        SetDisplayName( "Map Editor" );
    }

    EntityMapEditor::~EntityMapEditor()
    {
        EE_ASSERT( m_pNavmeshGeneratorDialog == nullptr );
    }

    //-------------------------------------------------------------------------

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

        if ( mapResourceID.GetResourceTypeID() != EntityMapDescriptor::GetStaticResourceTypeID() )
        {
            pfd::message( "Error", "Invalid map extension provided! Maps need to have the .map extension!", pfd::choice::ok, pfd::icon::error ).result();
            return;
        }

        // Write the map out to a new path and load it
        //-------------------------------------------------------------------------

        EntityCollectionDescriptor emptyMap;
        if ( Serializer::WriteEntityCollectionToFile( *m_pToolsContext->m_pTypeRegistry, emptyMap, mapFilePath ) )
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

    void EntityMapEditor::LoadMap( TResourcePtr<EntityModel::EntityMapDescriptor> const& mapToLoad )
    {
        if ( mapToLoad.GetResourceID() != m_loadedMap )
        {
            m_context.ClearSelection();

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
            m_pWorld->LoadMap( m_loadedMap );
            m_context.SetMapToUse( m_loadedMap );
            SetDisplayName( m_loadedMap.GetResourcePath().GetFileNameWithoutExtension() );
        }
    }

    void EntityMapEditor::SaveMap()
    {
        Save();
    }

    void EntityMapEditor::SaveMapAs()
    {
        auto pEditedMap = m_context.GetMap();
        if ( pEditedMap == nullptr || !( pEditedMap->IsLoaded() || pEditedMap->IsActivated() ) )
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

        EntityCollectionDescriptor ecd;
        if ( !pEditedMap->CreateDescriptor( *m_pToolsContext->m_pTypeRegistry, ecd ) )
        {
            return;
        }

        if ( Serializer::WriteEntityCollectionToFile( *m_pToolsContext->m_pTypeRegistry, ecd, mapFilePath ) )
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
        auto pEditedMap = m_context.GetMap();
        if ( pEditedMap == nullptr || !( pEditedMap->IsLoaded() || pEditedMap->IsActivated() ) )
        {
            return false;
        }

        EntityCollectionDescriptor ecd;
        if ( !pEditedMap->CreateDescriptor( *m_pToolsContext->m_pTypeRegistry, ecd ) )
        {
            return false;
        }

        FileSystem::Path const filePath = GetFileSystemPath( m_loadedMap );
        return Serializer::WriteEntityCollectionToFile( *m_pToolsContext->m_pTypeRegistry, ecd, filePath );
    }

    //-------------------------------------------------------------------------

    void EntityMapEditor::DrawWorkspaceToolbarItems( UpdateContext const& context )
    {
        ImGuiX::VerticalSeparator();

        //-------------------------------------------------------------------------
        // Tools Menu
        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( !HasLoadedMap() );
        if ( ImGui::BeginMenu( EE_ICON_HAMMER_WRENCH" Tools" ) )
        {
            // Navmesh
            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Navmesh" );
            
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

        //-------------------------------------------------------------------------
        // Preview Button
        //-------------------------------------------------------------------------

        ImVec2 const menuDimensions = ImGui::GetContentRegionMax();
        float buttonDimensions = 130;
        ImGui::SameLine( menuDimensions.x / 2 - buttonDimensions / 2 );

        ImGui::BeginDisabled( !HasLoadedMap() );
        if ( m_isGamePreviewRunning )
        {
            if ( ImGuiX::FlatIconButton( EE_ICON_STOP, "Stop Preview", Colors::Red, ImVec2( buttonDimensions, 0 ) ) )
            {
                m_gamePreviewStopRequested.Execute( context );
            }
        }
        else
        {
            if ( ImGuiX::FlatIconButton( EE_ICON_PLAY, "Preview Map", Colors::Lime, ImVec2( buttonDimensions, 0 ) ) )
            {
                m_gamePreviewStartRequested.Execute( context );
            }
        }
        ImGui::EndDisabled();
    }

    void EntityMapEditor::UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        EntityEditorBaseWorkspace::UpdateWorkspace( context, pWindowClass, isFocused );

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
        ResourcePath navmeshResourcePath = m_context.GetMap()->GetMapResourceID().GetResourcePath();
        navmeshResourcePath.ReplaceExtension( Navmesh::NavmeshData::GetStaticResourceTypeID().ToString().c_str() );
        ResourceID const navmeshResourceID( navmeshResourcePath );

        // Create a new entity with a navmesh component if one isnt found
        Entity* pEntity = EE::New<Entity>( StringID( "Navmesh Entity" ) );
        pEntity->AddComponent( EE::New<Navmesh::NavmeshComponent>( navmeshResourceID ) );
        m_context.AddEntity( pEntity );
    }

    void EntityMapEditor::BeginNavmeshGeneration( UpdateContext const& context )
    {
        EE_ASSERT( m_pNavmeshGeneratorDialog == nullptr );

        // Try find navmesh component
        auto const& navmeshComponents = m_pWorld->GetAllRegisteredComponentsOfType<Navmesh::NavmeshComponent>();
        if ( navmeshComponents.size() > 1 )
        {
            EE_LOG_WARNING( "Map Editor", "Multiple navmesh components found in the map! This is not supported!" );
        }

        Navmesh::NavmeshComponent const* pNavmeshComponent = navmeshComponents.back();

        // Navmesh Generation
        //-------------------------------------------------------------------------

        FileSystem::Path navmeshFilePath = m_context.GetMap()->GetMapResourceID().GetResourcePath().ToFileSystemPath( m_context.m_pToolsContext->m_pResourceDatabase->GetRawResourceDirectoryPath() );
        navmeshFilePath.ReplaceExtension( Navmesh::NavmeshData::GetStaticResourceTypeID().ToString() );

        EntityCollectionDescriptor mapDesc;
        m_context.GetMap()->CreateDescriptor( *m_pToolsContext->m_pTypeRegistry, mapDesc );
        m_pNavmeshGeneratorDialog = EE::New<Navmesh::NavmeshGeneratorDialog>( m_pToolsContext, pNavmeshComponent->GetBuildSettings(), mapDesc, navmeshFilePath);
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
            m_context.BeginEditComponent( pNavmeshComponent );
            pNavmeshComponent->SetBuildSettings( m_pNavmeshGeneratorDialog->GetBuildSettings() );
            m_context.EndEditComponent();
        }

        // Destroy dialog
        EE::Delete( m_pNavmeshGeneratorDialog );
    }
}