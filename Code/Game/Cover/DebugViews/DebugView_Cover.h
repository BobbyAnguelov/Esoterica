#pragma once

#include "Engine/DebugViews/DebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class CoverManager;

    //-------------------------------------------------------------------------

    class CoverDebugView : public DebugView
    {
        EE_REFLECT_TYPE( CoverDebugView );

    public:

        CoverDebugView() : DebugView( "Game/Covers" ) {}

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;

    private:

        CoverManager*                   m_pCoverManager = nullptr;
    };
}
#endif