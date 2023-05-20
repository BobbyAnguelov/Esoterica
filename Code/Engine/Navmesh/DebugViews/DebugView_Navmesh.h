#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityWorldDebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Navmesh
{
    class NavmeshWorldSystem;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API NavmeshDebugView : public EntityWorldDebugView
    {
        EE_REFLECT_TYPE( NavmeshDebugView );

    public:

        static void DrawNavmeshRuntimeSettings( NavmeshWorldSystem* pNavmeshWorldSystem );

    public:

        NavmeshDebugView();

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass ) override;

        void DrawMenu( EntityWorldUpdateContext const& context );

    private:

        NavmeshWorldSystem*     m_pNavmeshWorldSystem = nullptr;
    };
}
#endif