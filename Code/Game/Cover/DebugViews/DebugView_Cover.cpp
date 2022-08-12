#include "DebugView_Cover.h"
#include "Game/Cover/Systems/WorldSystem_CoverManager.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntitySystem.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/UpdateContext.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    CoverDebugView::CoverDebugView()
    {
        m_menus.emplace_back( DebugMenu( "Game/Covers", [this] ( EntityWorldUpdateContext const& context ) { DrawMenu( context ); } ) );
    }

    void CoverDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        m_pWorld = pWorld;
        m_pCoverManager = pWorld->GetWorldSystem<CoverManager>();
    }

    void CoverDebugView::Shutdown()
    {
        m_pCoverManager = nullptr;
        m_pWorld = nullptr;
    }

    void CoverDebugView::DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        EE_ASSERT( m_pWorld != nullptr );

        if ( m_isOverviewWindowOpen )
        {
            if ( pWindowClass != nullptr ) ImGui::SetNextWindowClass( pWindowClass );
            DrawOverviewWindow( context );
        }
    }

    void CoverDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
       
    }

    void CoverDebugView::DrawOverviewWindow( EntityWorldUpdateContext const& context )
    {
       
    }
}
#endif