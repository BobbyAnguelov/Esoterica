#pragma once
#include "Base/_Module/API.h"

//-------------------------------------------------------------------------

namespace EE::Platform
{
    // Lifetime
    //-------------------------------------------------------------------------
    // NOTE: Do not export these functions

    void Initialize();
    void Shutdown();

    // Windowing
    //-------------------------------------------------------------------------

    EE_BASE_API void* GetMainWindowHandle();
    EE_BASE_API void SetMainWindowHandle( void* pWindowHandle );
    EE_BASE_API void ClearMainWindowHandle();
}