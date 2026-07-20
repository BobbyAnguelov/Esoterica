#include "EditorTool_GamePreviewer.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Camera/Systems/WorldSystem_Camera.h"
#include "Game/ToolsUI/GameDebugUI.h"

//-------------------------------------------------------------------------
namespace EE
{

    GamePreviewer::GamePreviewer( ToolsContext const* pToolsContext, String const& displayName, EntityWorld* pWorld )
        : EditorTool( pToolsContext, displayName, pWorld )
    {
        // Switch back to player camera
        auto pCameraSystem = m_pWorld->GetWorldSystem<CameraSystem>();
        pCameraSystem->DisableToolsCamera();
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
            String const displayName( String::CtorSprintf(), "Preview: %s", m_loadedMap.GetDataPath().GetFilename().c_str() );
            SetDisplayName( displayName );
        }
    }

    void GamePreviewer::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        m_pDebugUI = EE::New<GameDebugUI>( GetToolWindowClass() );
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

        EditorTool::Shutdown( context );
    }

    void GamePreviewer::DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused )
    {
        EE_ASSERT( context.GetUpdateStage() == UpdateStage::FrameEnd );
        m_pDebugUI->DrawOverlayUI( context, pViewport );
    }

    void GamePreviewer::DrawMenu( UpdateContext const& context )
    {
        EE_ASSERT( context.GetUpdateStage() == UpdateStage::FrameEnd );
        if ( ImGui::BeginMenu( EE_ICON_MONITOR_SCREENSHOT" View" ) )
        {
            ImGui::SeparatorText( "Size" );

            if ( ImGui::MenuItem( "720" ) )
            {
                m_desiredViewportSize = Int2( 1280, 720 );
            }

            if ( ImGui::MenuItem( "1080" ) )
            {
                m_desiredViewportSize = Int2( 1920, 1080 );
            }

            if ( ImGui::MenuItem( "1440" ) )
            {
                m_desiredViewportSize = Int2( 2560, 1440 );
            }

            ImGui::EndMenu();
        }
        
        m_pDebugUI->DrawMenu( context );
    }

    void GamePreviewer::DrawEngineDebugUI( UpdateContext const& context )
    {
        EE_ASSERT( context.GetUpdateStage() == UpdateStage::FrameEnd );
        m_pDebugUI->EndFrame( context );
    }
}