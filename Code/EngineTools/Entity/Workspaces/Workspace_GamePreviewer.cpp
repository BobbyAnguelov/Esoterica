#include "Workspace_GamePreviewer.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Camera/Systems/WorldSystem_CameraManager.h"
#include "Engine/ToolsUI/EngineDebugUI.h"
#include "Base/IniFile.h"

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
            String const displayName( String::CtorSprintf(), "Preview: %s", m_loadedMap.GetResourcePath().GetFileName().c_str() );
            SetDisplayName( displayName );
        }
    }

    void GamePreviewer::Initialize( UpdateContext const& context )
    {
        Workspace::Initialize( context );
        m_pDebugUI = EE::New<EngineDebugUI>();
        m_pDebugUI->Initialize( context, nullptr );
    }

    void GamePreviewer::Shutdown( UpdateContext const& context )
    {
        if ( m_loadedMap.IsValid() )
        {
            m_pWorld->UnloadMap( m_loadedMap );
        }

        m_pDebugUI->Shutdown( context );
        EE::Delete( m_pDebugUI );

        Workspace::Shutdown( context );
    }

    void GamePreviewer::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Viewport" ).c_str(), dockspaceID );
    }

    void GamePreviewer::DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        EE_ASSERT( context.GetUpdateStage() == UpdateStage::FrameEnd );
        m_pDebugUI->DrawOverlayElements( context, pViewport );
    }

    void GamePreviewer::DrawMenu( UpdateContext const& context )
    {
        EE_ASSERT( context.GetUpdateStage() == UpdateStage::FrameEnd );
        m_pDebugUI->DrawMenu( context );
    }

    void GamePreviewer::DrawEngineDebugUI( UpdateContext const& context )
    {
        EE_ASSERT( context.GetUpdateStage() == UpdateStage::FrameEnd );
        m_pDebugUI->EditorPreviewUpdate( context, GetToolWindowClass() );
    }
}