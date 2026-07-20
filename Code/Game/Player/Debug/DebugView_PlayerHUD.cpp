#include "DebugView_PlayerHUD.h"
#include "Game/Player/Systems/EntitySystem_Player.h"
#include "Game/GameFlow/Systems/WorldSystem_GameFlow.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Imgui/ImguiX.h"

// HACK
#include "Game/Player/StateMachine/Actions/PlayerAction_Jump.h"
#include "Engine/Camera/Systems/WorldSystem_Camera.h"
#include "Game/Player/StateMachine/OverlayActions/PlayerOverlayAction_Weapon.h"
#include "Game/Player/Components/Component_PlayerCamera.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    void PlayerHudDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pGameFlowManager = pWorld->GetWorldSystem<GameFlowManager>();
    }

    void PlayerHudDebugView::Shutdown()
    {
        m_pGameFlowManager = nullptr;
        DebugView::Shutdown();
    }

    void PlayerHudDebugView::Update( EntityWorldUpdateContext const& context )
    {
        EE_ASSERT( m_pWorld != nullptr );

        m_pPlayerSystem = nullptr;
        m_pPlayerEntity = nullptr;

        if ( m_pGameFlowManager->HasPlayer() )
        {
            m_pPlayerEntity = m_pWorld->GetPersistentMap()->FindEntity( m_pGameFlowManager->GetPlayerEntityID() );
            if ( m_pPlayerEntity != nullptr )
            {
                for ( auto pSystem : m_pPlayerEntity->GetSystems() )
                {
                    m_pPlayerSystem = TryCast<PlayerSystem>( pSystem );
                    if ( m_pPlayerSystem != nullptr )
                    {
                        break;
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    // Called within the context of a large overlay window allowing you to draw helpers and widgets over a viewport
    void PlayerHudDebugView::DrawOverlayUI( EntityWorldUpdateContext const& context )
    {
        EE_ASSERT( m_pWorld != nullptr );

        PlayerSystem* pPlayerSystem = nullptr;
        if ( m_pGameFlowManager->HasPlayer() )
        {
            auto pPlayerEntity = m_pWorld->GetPersistentMap()->FindEntity( m_pGameFlowManager->GetPlayerEntityID() );
            for ( auto pSystem : pPlayerEntity->GetSystems() )
            {
                pPlayerSystem = TryCast<PlayerSystem>( pSystem );
                if ( pPlayerSystem != nullptr )
                {
                    break;
                }
            }
        }

        if ( pPlayerSystem == nullptr )
        {
            return;
        }

        // Draw fake HUD only when using the player camera
        auto pCameraSystem = m_pWorld->GetWorldSystem<CameraSystem>();
        if ( pPlayerSystem->m_pCameraComponent != pCameraSystem->GetActiveCamera() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        InlineString statusString;

        bool const isChargedJumpReady = static_cast<PlayerAction_Jump const*>( pPlayerSystem->m_actionStateMachine.m_baseActions[PlayerActionStateMachine::Jump] )->IsChargedJumpReady();
        if ( isChargedJumpReady )
        {
            statusString += "Charged Jump Ready";
        }

        auto pWeaponAction = static_cast<PlayerOverlayAction_Weapon const*>( pPlayerSystem->m_actionStateMachine.m_overlayActions[PlayerActionStateMachine::Weapon] );

        //-------------------------------------------------------------------------

        auto window = ImGui::GetCurrentWindow();
        auto drawList = window->DrawList;

        ImVec2 const windowPos = ImGui::GetWindowPos();
        ImVec2 const windowDimensions = ImGui::GetWindowSize();
        ImVec2 const windowCenter = windowDimensions / 2;

        // Crosshair
        //-------------------------------------------------------------------------

        if( pPlayerSystem->m_actionContext.m_pPlayerState->m_isInTimeDilation )
        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );

            Color const crosshairColor = Color::EvaluateRedGreenGradient( pWeaponAction->m_accuracy, false );
            Color const crosshairBorderColor = Colors::Black.GetAlphaVersion( 0.75f );
            float const crosshairHalfLength = 8;
            float const crosshairThickness = 1;

            ImVec2 viewportCenter = windowPos + windowCenter;
            drawList->AddLine( viewportCenter - ImVec2( 0, crosshairHalfLength + 1 ), viewportCenter + ImVec2( 0, crosshairHalfLength + 1 ), crosshairBorderColor, crosshairThickness + 2 );
            drawList->AddLine( viewportCenter - ImVec2( crosshairHalfLength + 1, 0 ), viewportCenter + ImVec2( crosshairHalfLength + 1, 0 ), crosshairBorderColor, crosshairThickness + 2 );
            drawList->AddLine( viewportCenter - ImVec2( 0, crosshairHalfLength ), viewportCenter + ImVec2( 0, crosshairHalfLength ), crosshairColor, crosshairThickness );
            drawList->AddLine( viewportCenter - ImVec2( crosshairHalfLength, 0 ), viewportCenter + ImVec2( crosshairHalfLength, 0 ), crosshairColor, crosshairThickness );
        }

        // Status
        //-------------------------------------------------------------------------

        if ( !statusString.empty() )
        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );

            ImVec2 const statusTextSize = ImGui::CalcTextSize( statusString.c_str() );
            ImVec2 const statusTextCursorPos( windowCenter.x - ( statusTextSize.x / 2 ), windowCenter.y + 8 );
            ImGui::SetCursorPos( statusTextCursorPos );
            ImGui::TextColored( Colors::Red.ToFloat4(), statusString.c_str() );
        }

        // Energy
        //-------------------------------------------------------------------------

        {
            float const startX = windowDimensions.x - 600;
            float const startY = windowDimensions.y - 40;
            float const endX = windowDimensions.x - 10;
            float const endY = windowDimensions.y - 10;
            float const innerMargin = 4.0f;

            ImVec2 const outerBoxStart = ImVec2( startX, startY ) + windowPos;
            ImVec2 const outerBoxEnd = ImVec2( endX, endY ) + windowPos;
            drawList->AddRect( outerBoxStart, outerBoxEnd, Colors::White );

            float const energyLevelPercentage = pPlayerSystem->m_actionContext.m_pPlayerState->GetPercentageEnergyLeft().ToFloat();
            Color const energyColor = Color::EvaluateRedGreenGradient( energyLevelPercentage );

            float const boxWidth = endX - startX - innerMargin;
            ImVec2 const innerBoxStart = ImVec2( startX + innerMargin, startY + innerMargin ) + windowPos;
            ImVec2 const innerBoxEnd = ImVec2( startX + ( energyLevelPercentage * boxWidth ), endY - innerMargin ) + windowPos;

            drawList->AddRectFilled( innerBoxStart, innerBoxEnd, energyColor );
        }
    }
}
#endif