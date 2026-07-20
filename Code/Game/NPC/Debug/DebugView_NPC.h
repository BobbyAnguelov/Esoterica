#pragma once

#include "Engine/Debug/DebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class GameFlowManager;

    //-------------------------------------------------------------------------

    class NPCDebugView : public DebugView
    {
        EE_REFLECT_TYPE( NPCDebugView );

    public:

        virtual Category GetCategory() const override { return Category::Game; }
        virtual char const* GetMenuPath() const override { return "NPCs"; }

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;

        void DrawOverviewWindow( EntityWorldUpdateContext const& context );

    private:

        GameFlowManager* m_pGameFlowManager = nullptr;
    };
}
#endif