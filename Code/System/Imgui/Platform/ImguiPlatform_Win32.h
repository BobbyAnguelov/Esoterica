#if _WIN32
#pragma once

#include "System/_Module/API.h"
#include "System/Esoterica.h"

//-------------------------------------------------------------------------

struct HWND__;

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX::Platform
{
    // Returns 0 when the message isnt handled, used to embed into another wnd proc
    EE_SYSTEM_API intptr_t WindowMessageProcessor( HWND__* hWnd, uint32_t message, uintptr_t wParam, intptr_t lParam );
}
#endif
#endif