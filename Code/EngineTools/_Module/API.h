#pragma once

//-------------------------------------------------------------------------

#if EE_DLL
    #ifdef ESOTERICA_ENGINE_TOOLS
        #define EE_ENGINETOOLS_API __declspec(dllexport)
    #else
        #define EE_ENGINETOOLS_API __declspec(dllimport)
    #endif
#else
    #define EE_ENGINETOOLS_API
#endif