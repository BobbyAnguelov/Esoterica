#pragma once
//-------------------------------------------------------------------------

#if EE_DLL
    #ifdef ESOTERICA_GAME_RUNTIME
        #define EE_GAME_API __declspec(dllexport)
    #else
        #define EE_GAME_API __declspec(dllimport)
    #endif
#else
    #define EE_GAME_API
#endif