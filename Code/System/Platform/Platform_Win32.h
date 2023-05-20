#pragma once
#ifdef _WIN32

#include "System/_Module/API.h"

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
// Core Functions
//-------------------------------------------------------------------------

namespace EE::Platform::Win32
{
    // Prints a message to the output log with a newline
    EE_SYSTEM_API void OutputDebugMessage( const char* format, ... );
}

//-------------------------------------------------------------------------
// Debug defines
//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS

#define EE_DISABLE_OPTIMIZATION __pragma( optimize( "", off ) )
#define EE_ENABLE_OPTIMIZATION __pragma( optimize( "", on ) )

#define EE_TRACE_MSG( msgFormat, ... ) EE::Platform::Win32::OutputDebugMessage( msgFormat, __VA_ARGS__ )
#define EE_ASSERT( cond ) do { if( !(cond) ) { EE_TRACE_MSG( "Assert fired: " #cond " (" EE_FILE_LINE ")" ); __debugbreak(); } } while( 0 )
#define EE_BREAK() __debugbreak()
#define EE_HALT() __debugbreak()

#endif

#endif