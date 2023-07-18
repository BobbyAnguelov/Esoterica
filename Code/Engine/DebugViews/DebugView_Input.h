#pragma once

#include "DebugView.h"
#include "Base/Input/InputDevices/InputDevice_Controller.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Input
{
    class ControllerInputDevice;
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

        void DrawControllerState( ControllerInputDevice const& controllerState );

    private:

        InputSystem*                                    m_pInputSystem = nullptr;
        int32_t                                         m_numControllers = 0;
    };
}
#endif