#pragma once
#ifdef _WIN32

//-------------------------------------------------------------------------

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define EE_FORCE_INLINE __forceinline

//-------------------------------------------------------------------------
// Enable specific warnings
//-------------------------------------------------------------------------

#pragma warning(default:4800)
#pragma warning(default:4389)

//-------------------------------------------------------------------------
// Dev Defines
//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS

    #define EE_DISABLE_OPTIMIZATION __pragma( optimize( "", off ) )
    #define EE_ENABLE_OPTIMIZATION __pragma( optimize( "", on ) )
    #define EE_DEBUG_BREAK() __debugbreak()

#endif

#endif