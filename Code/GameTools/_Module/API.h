#pragma once

//-------------------------------------------------------------------------

#if EE_DLL
    #ifdef ESOTERICA_GAME_TOOLS
        #define EE_GAMETOOLS_API __declspec(dllexport)
    #else
        #define EE_GAMETOOLS_API __declspec(dllimport)
    #endif
#else
    #define EE_GAMETOOLS_API
#endif