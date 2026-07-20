#include "DebugView_NPC.h"
#include "Game/GameFlow/Systems/WorldSystem_GameFlow.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    void NPCDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pGameFlowManager = pWorld->GetWorldSystem<GameFlowManager>();
        m_windows.emplace_back( "AI Overview", [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t ) { DrawOverviewWindow( context ); } );
    }

    void NPCDebugView::Shutdown()
    {
        m_pGameFlowManager = nullptr;
        DebugView::Shutdown();
    }

    void NPCDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        ImGui::Text( "Num NPCs: %u", m_pGameFlowManager->GetNumNPCs() );

        if ( ImGui::MenuItem( "Overview" ) )
        {
            m_windows[0].m_isOpen = true;
        }

        //-------------------------------------------------------------------------

        if ( ImGui::Button( "Hack Spawn 5" ) )
        {
            m_pGameFlowManager->HackTrySpawnNPCs( context, 5 );
        }

        if ( ImGui::Button( "Hack Spawn 25" ) )
        {
            m_pGameFlowManager->HackTrySpawnNPCs( context, 25 );
        }

        if ( ImGui::Button( "Hack Spawn 100" ) )
        {
            m_pGameFlowManager->HackTrySpawnNPCs( context, 100 );
        }

        if ( ImGui::Button( "Hack Spawn 1000" ) )
        {
            m_pGameFlowManager->HackTrySpawnNPCs( context, 1000 );
        }
    }

    void NPCDebugView::DrawOverviewWindow( EntityWorldUpdateContext const& context )
    {
        ImGui::Text( "Num AI: %d", m_pGameFlowManager->GetNumNPCs() );
    }
}
#endif