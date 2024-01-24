#pragma once

#include "Engine/DebugViews/DebugView.h"
#include "Base/Input/InputDevices/InputDevice_Controller.h"

//-------------------------------------------------------------------------

namespace EE { class PlayerManager; }

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Input
{
    class ControllerDevice;
    class InputSystem;

    //-------------------------------------------------------------------------

    class InputDebugView : public DebugView
    {
        EE_REFLECT_TYPE( InputDebugView );

    public:

        InputDebugView() : DebugView( "Engine/Input" ) {}

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;
        virtual void Update( EntityWorldUpdateContext const& context ) override;

        void DrawVirtualInputState( EntityWorldUpdateContext const& context );
        void DrawControllerState( EntityWorldUpdateContext const& context, ControllerDevice const& controllerState );

    private:

        InputSystem*                                    m_pInputSystem = nullptr;
        PlayerManager*                                  m_pPlayerManager = nullptr;
        int32_t                                         m_numControllers = 0;
    };
}
#endif