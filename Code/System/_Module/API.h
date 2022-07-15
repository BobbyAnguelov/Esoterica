#pragma once

//-------------------------------------------------------------------------

#if EE_DLL
    #if ESOTERICA_SYSTEM
        #define EE_SYSTEM_API __declspec(dllexport)
    #else
        #define EE_SYSTEM_API __declspec(dllimport)
    #endif
#else
    #define EE_SYSTEM_API
#endif