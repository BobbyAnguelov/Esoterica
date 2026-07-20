#pragma once

#include "Engine/Debug/DebugView.h"
#include "Base/Input/InputDevices/InputDevice_Controller.h"

//-------------------------------------------------------------------------

namespace EE { class ActorManager; }

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

        static StaticStringID const s_controllerWindowTypeID;

    public:

        virtual Category GetCategory() const override { return Category::Engine; }
        virtual char const* GetMenuPath() const override { return "Input"; }

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;
        virtual void Update( EntityWorldUpdateContext const& context ) override;

        void DrawControllerState( EntityWorldUpdateContext const& context, ControllerDevice const& controllerState );

    private:

        InputSystem*                                    m_pInputSystem = nullptr;
        int32_t                                         m_numControllers = 0;
    };
}
#endif