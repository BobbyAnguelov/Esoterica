#pragma once

#include "Base/_Module/API.h"
#include "Base/Esoterica.h"

#if EE_DEVELOPMENT_TOOLS
#include "Base/Types/String.h"

//-------------------------------------------------------------------------
// Base ImGui integration
//-------------------------------------------------------------------------

namespace EE
{
    namespace Input{ class InputSystem; }
}

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    //-------------------------------------------------------------------------
    // Main integration point for DearImGui in Esoterica
    //-------------------------------------------------------------------------

    class EE_BASE_API ImguiSystem final
    {

    public:

        bool Initialize( Input::InputSystem* pInputSystem = nullptr, bool enableViewports = false );
        void Shutdown();

        //-------------------------------------------------------------------------


        void StartFrame( float deltaTime );
        void EndFrame();

    private:

        void InitializePlatform();
        void ShutdownPlatform();
        void PlatformNewFrame();

        void InitializeFonts();
        void ShutdownFonts();

    private:

        Input::InputSystem*             m_pInputSystem = nullptr;
        String                          m_iniFilename;
        String                          m_logFilename;
    };
}
#endif