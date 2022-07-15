#pragma once
#include "System/Esoterica.h"

//-------------------------------------------------------------------------

struct HWND__;

//-------------------------------------------------------------------------

namespace EE
{
    intptr_t DefaultEngineWindowProcessor( class Engine* pEngine, HWND__* hWnd, uint32_t message, uintptr_t wParam, intptr_t lParam );
}