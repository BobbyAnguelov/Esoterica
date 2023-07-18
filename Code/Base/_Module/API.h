#pragma once

//-------------------------------------------------------------------------

#if EE_DLL
    #if ESOTERICA_BASE
        #define EE_BASE_API __declspec(dllexport)
    #else
        #define EE_BASE_API __declspec(dllimport)
    #endif
#else
    #define EE_BASE_API
#endif