#pragma once

#include <stdint.h>

//-------------------------------------------------------------------------

using nullptr_t = decltype( nullptr );

//-------------------------------------------------------------------------
// Common
//-------------------------------------------------------------------------

#define EE_STRINGIZING(x) #x
#define EE_MAKE_STRING(x) EE_STRINGIZING(x)
#define EE_FILE_LINE __FILE__ ":" EE_MAKE_STRING(__LINE__)

//-------------------------------------------------------------------------
// Configurations
//-------------------------------------------------------------------------

// EE_DEBUG = unoptimized build with debug info (debug drawing, string debug names, etc...)
// EE_RELEASE = optimized build with debug info (debug drawing, string debug names, etc...)
// EE_SHIPPING = optimized build with no debug info

//-------------------------------------------------------------------------
// Development Tools
//-------------------------------------------------------------------------
// EE_DEVELOPMENT_TOOLS_LINE_IN_MACRO exists for use in macros.

#if !EE_SHIPPING
#define EE_DEVELOPMENT_TOOLS 1
#endif

#if EE_DEVELOPMENT_TOOLS
#define EE_DEVELOPMENT_TOOLS_LINE_IN_MACRO( x ) x
#define EE_DEVELOPMENT_TOOLS_ONLY( x ) x
#else
#define EE_DEVELOPMENT_TOOLS_LINE_IN_MACRO( x )
#define EE_DEVELOPMENT_TOOLS_ONLY( x )
#endif

//-------------------------------------------------------------------------
// Platform specific defines
//-------------------------------------------------------------------------

#if _WIN32
#include "Platform/Platform_Win32.h"
#endif

//-------------------------------------------------------------------------
// Debug defines
//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS

    // Platform agnostic asserts
    //-------------------------------------------------------------------------

    #define EE_STATIC_ASSERT( cond, error ) static_assert( cond, error )
    #define EE_TRACE_ASSERT( msg ) { EE_TRACE_MSG( msg ); EE_HALT(); }
    #define EE_UNIMPLEMENTED_FUNCTION() EE_TRACE_ASSERT("Function not implemented!\n")
    #define EE_UNREACHABLE_CODE() EE_TRACE_ASSERT("Unreachable code encountered!\n")

#else

    // Platform specific, need to be defined in Platform/Defines_XXX.h

    #define EE_ASSERT( cond )
    #define EE_TRACE_MSG_WIN32( msg )
    #define EE_BREAK()
    #define EE_HALT()

    #define EE_DISABLE_OPTIMIZATION
    #define EE_ENABLE_OPTIMIZATION

    // Platform agnostic asserts
    //-------------------------------------------------------------------------

    #define EE_STATIC_ASSERT( cond, error )
    #define EE_TRACE_MSG( msg ) 
    #define EE_TRACE_ASSERT( msg )
    #define EE_UNIMPLEMENTED_FUNCTION()
    #define EE_UNREACHABLE_CODE()
#endif

//-------------------------------------------------------------------------
// Core typedefs
//-------------------------------------------------------------------------

namespace EE
{
    #define InvalidIndex -1

    //-------------------------------------------------------------------------

    namespace Platform
    {
        enum class Target
        {
            PC = 0,
        };
    }
}