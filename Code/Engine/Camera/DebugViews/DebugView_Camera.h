#pragma once

#include "Engine/DebugViews/DebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class CameraManager;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CameraDebugView : public DebugView
    {
        EE_REFLECT_TYPE( CameraDebugView );

    public:

        static void DrawDebugCameraOptions( EntityWorld const* pWorld );

    public:

        CameraDebugView() : DebugView( "Engine/Camera" ) {}

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;

        void DrawMenu( EntityWorldUpdateContext const& context ) override;
        
        void DrawCameraWindow( EntityWorldUpdateContext const& context );

    private:

        CameraManager const*            m_pCameraManager = nullptr;
    };
}
#endif