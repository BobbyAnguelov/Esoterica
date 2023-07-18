#include "DebugView_Cover.h"
#include "Game/Cover/Systems/WorldSystem_CoverManager.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntitySystem.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/UpdateContext.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    void CoverDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pCoverManager = pWorld->GetWorldSystem<CoverManager>();
    }

    void CoverDebugView::Shutdown()
    {
        m_pCoverManager = nullptr;
        DebugView::Shutdown();
    }

    void CoverDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
       
    }
}
#endif