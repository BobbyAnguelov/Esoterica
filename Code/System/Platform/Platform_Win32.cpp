#ifdef _WIN32
#include "Platform_Win32.h"

//-------------------------------------------------------------------------

#include <windows.h>

//-------------------------------------------------------------------------

namespace EE::Platform::Win32
{
    void OutputDebugMessage( char const* msg )
    {
        OutputDebugStringA( msg );
        OutputDebugStringA( "\r\n" );
    }
}

#endif