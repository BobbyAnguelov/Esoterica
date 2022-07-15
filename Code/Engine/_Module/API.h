#pragma once

//-------------------------------------------------------------------------

#if EE_DLL
    #ifdef ESOTERICA_ENGINE_RUNTIME
        #define EE_ENGINE_API __declspec(dllexport)
    #else
        #define EE_ENGINE_API __declspec(dllimport)
    #endif
#else
    #define EE_ENGINE_API
#endif