#pragma once

#include "Engine/DebugViews/DebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::AI
{
    class AIManager;

    //-------------------------------------------------------------------------

    class AIDebugView : public DebugView
    {
        EE_REFLECT_TYPE( AIDebugView );

    public:

        AIDebugView() : DebugView( "Game/AI" ) {}

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;

        void DrawOverviewWindow( EntityWorldUpdateContext const& context );

    private:

        AIManager*                      m_pAIManager = nullptr;
    };
}
#endif