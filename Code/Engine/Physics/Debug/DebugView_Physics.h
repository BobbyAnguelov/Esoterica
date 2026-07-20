#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Debug/DebugView.h"

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

        virtual Category GetCategory() const override { return Category::Engine; }
        virtual char const* GetMenuPath() const override { return "Physics"; }

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;

        void DrawMaterialDatabaseView( UpdateContext const& context );

    private:

        PhysicsWorldSystem*     m_pPhysicsWorldSystem = nullptr;
    };
}
#endif