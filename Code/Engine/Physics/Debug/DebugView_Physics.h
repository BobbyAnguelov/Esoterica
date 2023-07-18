#pragma once

#include "Engine/_Module/API.h"
#include "Engine/DebugViews/DebugView.h"

//-------------------------------------------------------------------------

namespace EE { class UpdateContext; }

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Physics
{
    class PhysicsWorldSystem;
    class PhysicsShapeComponent;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PhysicsDebugView : public DebugView
    {
        EE_REFLECT_TYPE( PhysicsDebugView );

    public:

        static void DrawMaterialDatabaseView( UpdateContext const& context );

    public:

        PhysicsDebugView() : DebugView( "Engine/Physics" ) {}

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;

    private:

        PhysicsWorldSystem*     m_pPhysicsWorldSystem = nullptr;
    };
}
#endif