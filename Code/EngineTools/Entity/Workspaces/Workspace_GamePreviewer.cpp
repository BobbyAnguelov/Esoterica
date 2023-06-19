#include "Workspace_GamePreviewer.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Camera/Systems/WorldSystem_CameraManager.h"
#include "Engine/ToolsUI/EngineToolsUI.h"
#include "System/IniFile.h"

//-------------------------------------------------------------------------

namespace EE
{
    GamePreviewer::GamePreviewer( ToolsContext const* pToolsContext, EntityWorld* pWorld )
        : Workspace( pToolsContext, pWorld, String( "Game Preview" ) )
    {
        // Switch back to player camera
        auto pCameraManager = m_pWorld->GetWorldSystem<CameraManager>();
        pCameraManager->DisableDebugCamera();
    }

    void GamePreviewer::LoadMapToPreview( ResourceID mapResourceID )
    {
        if ( mapResourceID != m_loadedMap )
        {
            // Unload current map
            if ( m_loadedMap.IsValid() && m_pWorld->IsMapLoaded( m_loadedMap ) )
            {
                m_pWorld->UnloadMap( m_loadedMap );
            }

            // Load map
            m_loadedMap = mapResourceID;
            m_pWorld->LoadMap( m_loadedMap );
            String const displayName( String::CtorSprintf(), "Game Preview: %s", m_loadedMap.GetResourcePath().GetFileName().c_str() );
            SetDisplayName( displayName );
        }
    }

    void GamePreviewer::Initialize( UpdateContext const& context )
    {
        Workspace::Initialize( context );
        m_pEngineToolsUI = EE::New<EngineToolsUI>();
        m_pEngineToolsUI->Initialize( context, nullptr );
        m_pEngineToolsUI->LockToWindow( "Viewport" );
    }

    void GamePreviewer::Shutdown( UpdateContext const& context )
    {
        if ( m_loadedMap.IsValid() )
        {
            m_pWorld->UnloadMap( m_loadedMap );
        }

        m_pEngineToolsUI->Shutdown( context );
        EE::Delete( m_pEngineToolsUI );

        Workspace::Shutdown( context );
    }

    //-------------------------------------------------------------------------

    void GamePreviewer::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID topDockID = 0;
        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.5f, nullptr, &topDockID );

        // Dock viewport
        ImGuiDockNode* pTopNode = ImGui::DockBuilderGetNode( topDockID );
        pTopNode->LocalFlags |= ImGuiDockNodeFlags_NoDockingSplitMe | ImGuiDockNodeFlags_NoDockingOverMe;
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Viewport" ).c_str(), topDockID );
    }

    void GamePreviewer::DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        EE_ASSERT( context.GetUpdateStage() == UpdateStage::FrameEnd );
        m_pEngineToolsUI->DrawOverlayElements( context, pViewport );
    }

    void GamePreviewer::DrawMenu( UpdateContext const& context )
    {
        EE_ASSERT( context.GetUpdateStage() == UpdateStage::FrameEnd );
        if ( m_pEngineToolsUI->m_debugOverlayEnabled )
        {
            m_pEngineToolsUI->DrawMenu( context, m_pWorld );
        }
    }

    void GamePreviewer::Update( UpdateContext const& context, bool isFocused )
    {
        EE_ASSERT( context.GetUpdateStage() == UpdateStage::FrameEnd );
        m_pEngineToolsUI->m_debugOverlayEnabled = true;
        m_pEngineToolsUI->HandleUserInput( context, m_pWorld );
        m_pEngineToolsUI->DrawWindows( context, m_pWorld, GetToolWindowClass() );
    }

    void GamePreviewer::BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded )
    {
        Workspace::BeginHotReload( usersToBeReloaded, resourcesToBeReloaded );
        m_pEngineToolsUI->BeginHotReload( usersToBeReloaded, resourcesToBeReloaded );
    }

    void GamePreviewer::EndHotReload()
    {
        m_pEngineToolsUI->EndHotReload();
        Workspace::EndHotReload();
    }
}