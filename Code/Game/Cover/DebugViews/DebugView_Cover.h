#pragma once

#include "Engine/Entity/EntityWorldDebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class CoverManager;

    //-------------------------------------------------------------------------

    class CoverDebugView : public EntityWorldDebugView
    {
        EE_REGISTER_TYPE( CoverDebugView );

    public:

        CoverDebugView();

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass ) override;

        void DrawMenu( EntityWorldUpdateContext const& context );
        void DrawOverviewWindow( EntityWorldUpdateContext const& context );

    private:

        EntityWorld const*              m_pWorld = nullptr;
        CoverManager*                   m_pCoverManager = nullptr;
        bool                            m_isOverviewWindowOpen = false;
    };
}
#endif