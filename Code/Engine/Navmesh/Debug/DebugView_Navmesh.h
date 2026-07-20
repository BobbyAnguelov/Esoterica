#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Debug/DebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Navmesh
{
    class NavmeshWorldSystem;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API NavmeshDebugView : public DebugView
    {
        EE_REFLECT_TYPE( NavmeshDebugView );

    public:

        static void DrawNavmeshRuntimeSettings( NavmeshWorldSystem* pNavmeshWorldSystem );

    public:

        virtual Category GetCategory() const override { return Category::Engine; }
        virtual char const* GetMenuPath() const override { return "Navmesh"; }

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;

    private:

        NavmeshWorldSystem*     m_pNavmeshWorldSystem = nullptr;
    };
}
#endif