#include "DebugView_Player.h"
#include "Game/Player/Systems/EntitySystem_Player.h"
#include "Game/GameFlow/Systems/WorldSystem_GameFlow.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntitySystem.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/UpdateContext.h"
#include "Base/Imgui/ImguiX.h"

// Prototype
#include "Game/Player/Components/Component_PlayerTargeting.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    void PlayerDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pGameFlowManager = pWorld->GetWorldSystem<GameFlowManager>();

        m_windows.emplace_back( "Player Actions", [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t ) { DrawActionDebugger( context, isFocused ); } );
        m_windows.emplace_back( "Prototype", [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t ) { DrawPrototypeInfo( context, isFocused ); } );

        //m_windows[1].m_isOpen = true;
    }

    void PlayerDebugView::Shutdown()
    {
        m_pGameFlowManager = nullptr;
        DebugView::Shutdown();
    }

    void PlayerDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        UpdatePlayerEntityAndSystemPtrs();
        if ( m_pPlayerSystem == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( ImGui::MenuItem( "Action Debugger" ) )
        {
            m_windows[0].m_isOpen = true;
        }

        if ( ImGui::MenuItem( "Physics State Debugger" ) )
        {
            m_windows[1].m_isOpen = true;
        }

        if ( ImGui::MenuItem( "Prototype" ) )
        {
            m_windows[2].m_isOpen = true;
        }
    }

    void PlayerDebugView::Update( EntityWorldUpdateContext const& context )
    {
        EE_ASSERT( m_pWorld != nullptr );

        if ( IsAnyWindowOpen() )
        {
            UpdatePlayerEntityAndSystemPtrs();
        }
    }

    void PlayerDebugView::UpdatePlayerEntityAndSystemPtrs()
    {
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

    void PlayerDebugView::DrawActionDebugger( EntityWorldUpdateContext const& context, bool isFocused )
    {
        if ( m_pPlayerSystem == nullptr )
        {
            ImGui::Text( "No Player Controller Found" );
        }
        else
        {
            PlayerActionStateMachine const* pStateMachine = &m_pPlayerSystem->m_actionStateMachine;
            pStateMachine->DrawDebugUI();
        }
    }

    //-------------------------------------------------------------------------

    void PlayerDebugView::DrawPrototypeInfo( EntityWorldUpdateContext const& context, bool isFocused )
    {
        if ( m_pPlayerEntity == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        PlayerTargetingComponent* pTargetTrackerComponent = nullptr;
        for ( auto pComponent : m_pPlayerEntity->GetComponents() )
        {
            pTargetTrackerComponent = TryCast<PlayerTargetingComponent>( pComponent );
            if ( pTargetTrackerComponent != nullptr )
            {
                break;
            }
        }

        if ( pTargetTrackerComponent == nullptr )
        {
            return;
        }

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

        //-------------------------------------------------------------------------

        if ( ImGui::BeginTabBar( "Prototype" ) )
        {
            if ( ImGui::BeginTabItem( "Aim/Accuracy" ) )
            {
                /*auto pWeaponAction = static_cast<PlayerOverlayAction_Weapon*>( pPlayerSystem->m_actionStateMachine.m_overlayActions[PlayerActionStateMachine::Weapon] );
                auto pCharacterController = m_pPlayerEntity->TryGetComponent<Player()
                Vector const characterVelocity = ctx.m_pCharacterController->GetCharacterVelocity();
                float const speed = ;

                ImGui::Text( "Speed: %.2f", characterVelocity.GetLength3() );
                ImGui::TextColored( Color::EvaluateRedGreenGradient( pWeaponAction->m_movingAccuracy, false ), EE_ICON_RUN" Moving Accuracy: %.3f", pWeaponAction->m_movingAccuracy );
                ImGui::TextColored( Color::EvaluateRedGreenGradient( pWeaponAction->m_shootingAccuracy, false ), EE_ICON_PISTOL" Shooting Accuracy: %.3f", pWeaponAction->m_shootingAccuracy );

                pWeaponAction->m_curveEditor.UpdateAndDraw( false );*/

                ImGui::EndTabItem();
            }

            if ( ImGui::BeginTabItem( "Targeting" ) )
            {
                if ( pTargetTrackerComponent->HasTrackedTarget() )
                {
                    for ( int32_t i = 0; i < pTargetTrackerComponent->m_reflectedTargets.size(); i++ )
                    {
                        if ( pTargetTrackerComponent->m_reflectedTargets[i].m_entityID == pTargetTrackerComponent->m_trackedTargetEntityID )
                        {
                            ImGui::Text( "Target: %d", i );
                        }
                    }

                    if ( pTargetTrackerComponent->IsLockedOn() )
                    {
                        ImGui::SameLine();
                        ImGui::TextColored( Colors::Red, "(Locked On)" );
                    }
                }

                if ( ImGui::BeginTable( "Targets", 8 ) )
                {
                    ImGui::TableSetupColumn( "#", ImGuiTableColumnFlags_WidthFixed, 45 );
                    ImGui::TableSetupColumn( "Angle", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupColumn( "Dist", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupColumn( "Angle Score", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupColumn( "Dist Score", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupColumn( "Score", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupColumn( "Time Tracked", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupColumn( "Time Offscreen", ImGuiTableColumnFlags_WidthStretch );

                    ImGui::TableHeadersRow();

                    for ( int32_t i = 0; i < pTargetTrackerComponent->m_reflectedTargets.size(); i++ )
                    {
                        auto const& target = pTargetTrackerComponent->m_reflectedTargets[i];

                        Color rowColor = ImGuiX::Style::s_colorTextDisabled;
                        if ( target.m_entityID == pTargetTrackerComponent->m_trackedTargetEntityID )
                        {
                            rowColor = Colors::Lime;
                        }

                        ImGuiX::ScopedFont sf( rowColor );

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text( "%d", i );

                        ImGui::TableNextColumn();
                        ImGui::Text( "%.2f", target.m_angularDistance.ToFloat() );

                        ImGui::TableNextColumn();
                        ImGui::Text( "%.2f", target.m_distance );

                        ImGui::TableNextColumn();
                        ImGui::Text( "%.2f", target.m_angleScore );

                        ImGui::TableNextColumn();
                        ImGui::Text( "%.2f", target.m_distanceScore );

                        ImGui::TableNextColumn();
                        ImGui::Text( "%.2f", target.m_score );

                        ImGui::TableNextColumn();
                        ImGui::Text( "%.2f", target.m_timeTracked.ToFloat() );

                        ImGui::TableNextColumn();
                        ImGui::Text( "%.2f", target.m_timeOffScreen.ToFloat() );
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}
#endif