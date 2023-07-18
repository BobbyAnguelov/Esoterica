#include "DebugView_AI.h"
#include "Engine/AI/Systems/WorldSystem_AIManager.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntitySystem.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/UpdateContext.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::AI
{
    void AIDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pAIManager = pWorld->GetWorldSystem<AIManager>();
        m_windows.emplace_back( "AI Overview", [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t ) { DrawOverviewWindow( context ); } );
    }

    void AIDebugView::Shutdown()
    {
        m_pAIManager = nullptr;
        DebugView::Shutdown();
    }

    void AIDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        ImGui::Text( "Num AI: %u", m_pAIManager->m_AIs.size() );

        if ( ImGui::MenuItem( "Overview" ) )
        {
            m_windows[0].m_isOpen = true;
        }

        //-------------------------------------------------------------------------

        if ( ImGui::Button( "Hack Spawn 5" ) )
        {
            m_pAIManager->HackTrySpawnAI( context, 5 );
        }

        if ( ImGui::Button( "Hack Spawn 25" ) )
        {
            m_pAIManager->HackTrySpawnAI( context, 25 );
        }

        if ( ImGui::Button( "Hack Spawn 100" ) )
        {
            m_pAIManager->HackTrySpawnAI( context, 100 );
        }
    }

    void AIDebugView::DrawOverviewWindow( EntityWorldUpdateContext const& context )
    {
        ImGui::Text( "Num AI: %u", m_pAIManager->m_AIs.size() );
    }
}
#endif