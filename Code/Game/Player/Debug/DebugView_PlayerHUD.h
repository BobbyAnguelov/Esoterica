#pragma once

#include "Engine/Debug/DebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class Entity;
    class GameFlowManager;
    class PlayerSystem;

    //-------------------------------------------------------------------------

    class PlayerHudDebugView final : public DebugView
    {
        EE_REFLECT_TYPE( PlayerHudDebugView );

    public:

        virtual Category GetCategory() const override { return Category::Game; }

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawOverlayUI( EntityWorldUpdateContext const& context ) override;
        virtual void Update( EntityWorldUpdateContext const& context ) override;

    private:

        GameFlowManager*                            m_pGameFlowManager = nullptr;
        PlayerSystem*                               m_pPlayerSystem = nullptr;
        Entity*                                     m_pPlayerEntity = nullptr;
    };
}
#endif