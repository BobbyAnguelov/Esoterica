#if _WIN32
#pragma once

#include "System/_Module/API.h"
#include "System/Esoterica.h"

//-------------------------------------------------------------------------

struct HWND__;

//-------------------------------------------------------------------------

namespace EE::ImGuiX::Platform
{
    void InitializePlatform();
    void ShutdownPlatform();

    // Called each frame
    void UpdateDisplayInformation();
    void UpdateInputInformation();

    // Returns 0 when the message isnt handled, used to embed into another wnd proc
    EE_SYSTEM_API intptr_t WindowsMessageHandler( HWND__* hWnd, uint32_t message, uintptr_t wParam, intptr_t lParam );
}
#endif