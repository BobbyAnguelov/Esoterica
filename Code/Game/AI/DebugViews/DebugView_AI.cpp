#include "DebugView_AI.h"
#include "Engine/AI/Systems/WorldSystem_AIManager.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntitySystem.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/UpdateContext.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::AI
{
    AIDebugView::AIDebugView()
    {
        m_menus.emplace_back( DebugMenu( "Game/AI", [this] ( EntityWorldUpdateContext const& context ) { DrawMenu( context ); } ) );
    }

    void AIDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        m_pWorld = pWorld;
        m_pAIManager = pWorld->GetWorldSystem<AIManager>();
    }

    void AIDebugView::Shutdown()
    {
        m_pAIManager = nullptr;
        m_pWorld = nullptr;
    }

    void AIDebugView::DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        EE_ASSERT( m_pWorld != nullptr );

        if ( m_isOverviewWindowOpen )
        {
            if ( pWindowClass != nullptr ) ImGui::SetNextWindowClass( pWindowClass );
            DrawOverviewWindow( context );
        }
    }

    void AIDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        ImGui::Text( "Num AI: %u", m_pAIManager->m_AIs.size() );

        if ( ImGui::MenuItem( "Overview" ) )
        {
            m_isOverviewWindowOpen = true;
        }

        //-------------------------------------------------------------------------

        if ( ImGui::Button( "Hack Spawn" ) )
        {
            m_pAIManager->TrySpawnAI( context );
        }
    }

    void AIDebugView::DrawOverviewWindow( EntityWorldUpdateContext const& context )
    {
        if ( ImGui::Begin( "AI Overview", &m_isOverviewWindowOpen ) )
        {
            ImGui::Text( "Num AI: %u", m_pAIManager->m_AIs.size() );
        }
        ImGui::End();
    }
}
#endif