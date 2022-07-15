#pragma once

#include "Engine/Entity/EntityWorldDebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class PlayerManager;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CameraDebugView : public EntityWorldDebugView
    {
        EE_REGISTER_TYPE( CameraDebugView );

    public:

        static void DrawDebugCameraOptions( EntityWorld const* pWorld );

    public:

        CameraDebugView();

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass ) override;

        void DrawMenu( EntityWorldUpdateContext const& context );
        void DrawCameraWindow( EntityWorldUpdateContext const& context );

    private:

        PlayerManager const*            m_pPlayerManager = nullptr;
        bool                            m_isCameraDebugWindowOpen = false;
    };
}
#endif