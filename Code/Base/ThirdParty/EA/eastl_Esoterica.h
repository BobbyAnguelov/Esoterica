#pragma once

#include "Base/_Module/API.h"
#include "Base/Esoterica.h"
#include <EABase/eabase.h>
#include <stdio.h>
#include <wchar.h>

//-------------------------------------------------------------------------

#if EE_DLL
    #if ESOTERICA_BASE 
        #if EASTL_DLL
            #if defined(_MSC_VER)
                #define EASTL_API      __declspec(dllexport)
            #elif defined(__CYGWIN__)
                #define EASTL_API      __attribute__((dllexport))
                #define EASTL_LOCAL
            #elif (defined(__GNUC__) && (__GNUC__ >= 4))
                #define EASTL_API      __attribute__ ((visibility("default")))
                #define EASTL_LOCAL    __attribute__ ((visibility("hidden")))
            #endif
        #else
            #define EASTL_API
            #define EASTL_LOCAL
        #endif
    #endif
#else
    #define EASTL_DLL 0
#endif

//-------------------------------------------------------------------------

#define EASTL_USER_DEFINED_ALLOCATOR 1
#define EASTL_EASTDC_VSNPRINTF 0
#define EASTL_EXCEPTIONS_ENABLED 0

#if EE_DEVELOPMENT_TOOLS
#define EASTL_DEBUG 1
#endif

#define EASTL_ASSERT EE_ASSERT

//-------------------------------------------------------------------------

inline int Vsnprintf8( char8_t* pDestination, size_t n, const char8_t* pFormat, va_list arguments )
{
    return vsnprintf( (char*) pDestination, n, (char*) pFormat, arguments );
}

inline int Vsnprintf16( char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments )
{
    EE_UNIMPLEMENTED_FUNCTION();
    return -1;
}

inline int Vsnprintf32( char32_t* pDestination, size_t n, const char32_t* pFormat, va_list arguments )
{
    EE_UNIMPLEMENTED_FUNCTION();
    return -1;
}

#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
inline int VsnprintfW( wchar_t* pDestination, size_t n, const wchar_t* pFormat, va_list arguments )
{
    return vswprintf( pDestination, n, pFormat, arguments );
}
#endif

//-------------------------------------------------------------------------

namespace eastl
{
    class allocator;
    inline bool operator==( allocator const&, allocator const& ) { return true; }
    inline bool operator!=( allocator const&, allocator const& ) { return false; }
}