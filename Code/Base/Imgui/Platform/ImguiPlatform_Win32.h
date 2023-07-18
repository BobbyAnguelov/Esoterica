#if _WIN32
#pragma once

#include "Base/_Module/API.h"
#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

struct HWND__;

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX::Platform
{
    // Returns 0 when the message isnt handled, used to embed into another wnd proc
    EE_BASE_API intptr_t WindowMessageProcessor( HWND__* hWnd, uint32_t message, uintptr_t wParam, intptr_t lParam );
}
#endif
#endif