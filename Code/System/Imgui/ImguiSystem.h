#pragma once

#include "../_Module/API.h"
#include "System/Types/String.h"

#if EE_DEVELOPMENT_TOOLS
#include "System/ThirdParty/imgui/imgui.h"

//-------------------------------------------------------------------------
// Base ImGui integration
//-------------------------------------------------------------------------

namespace EE
{
    namespace Render{ class RenderDevice; }
    namespace Input{ class InputSystem; }
}

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    class EE_SYSTEM_API ImguiSystem
    {

    public:

        bool Initialize( Render::RenderDevice* pRenderDevice, Input::InputSystem* pInputSystem = nullptr, bool enableViewports = false);
        void Shutdown();

        void StartFrame( float deltaTime );
        void EndFrame();

    private:

        void InitializePlatform();
        void ShutdownPlatform();
        void PlatformUpdate();

        void InitializeFonts();
        void ShutdownFonts();

    private:

        Input::InputSystem*     m_pInputSystem = nullptr;
        String                  m_iniFilename;
        String                  m_logFilename;
    };
}
#endif