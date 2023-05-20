#pragma once

#include "System/_Module/API.h"
#include "System/Esoterica.h"

#if EE_DEVELOPMENT_TOOLS
#include "ImguiImageCache.h"
#include "System/Types/String.h"

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
    //-------------------------------------------------------------------------
    // Main integration point for DearImGui in Esoterica
    //-------------------------------------------------------------------------

    class EE_SYSTEM_API ImguiSystem final
    {

    public:

        bool Initialize( Render::RenderDevice* pRenderDevice, Input::InputSystem* pInputSystem = nullptr, bool enableViewports = false );
        void Shutdown();

        //-------------------------------------------------------------------------

        ImageCache* GetImageCache() { return &m_imageCache; }

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
        ImageCache                      m_imageCache;
        String                          m_iniFilename;
        String                          m_logFilename;
    };
}
#endif