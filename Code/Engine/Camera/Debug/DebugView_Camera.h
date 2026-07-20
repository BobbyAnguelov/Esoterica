#pragma once

#include "Engine/Debug/DebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class CameraSystem;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CameraDebugView : public DebugView
    {
        EE_REFLECT_TYPE( CameraDebugView );

    public:

        virtual Category GetCategory() const override { return Category::Engine; }
        virtual char const* GetMenuPath() const override { return "Camera"; }

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;

        void DrawMenu( EntityWorldUpdateContext const& context ) override;
        
        void DrawCameraWindow( EntityWorldUpdateContext const& context );

    private:

        CameraSystem const*            m_pCameraManager = nullptr;
    };
}
#endif