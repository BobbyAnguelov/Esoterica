#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityWorldDebugView.h"

//-------------------------------------------------------------------------

namespace EE { class UpdateContext; }

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Physics
{
    class PhysicsWorldSystem;
    class PhysicsShapeComponent;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PhysicsDebugView : public EntityWorldDebugView
    {
        EE_REFLECT_TYPE( PhysicsDebugView );

    public:

        static bool DrawMaterialDatabaseView( UpdateContext const& context );

    public:

        PhysicsDebugView();

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass ) override;

        void DrawMenu( EntityWorldUpdateContext const& context );

    private:

        PhysicsWorldSystem*     m_pPhysicsWorldSystem = nullptr;
        bool                    m_isMaterialDatabaseWindowOpen = false;
    };
}
#endif