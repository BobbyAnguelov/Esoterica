#ifdef _WIN32
#include "Platform_Win32.h"

//-------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>

//-------------------------------------------------------------------------

namespace EE::Platform::Win32
{
    void OutputDebugMessage( const char* format, ... )
    {
        constexpr size_t const bufferSize = 512;
        char messageBuffer[bufferSize]; // Dont make this static as we need this to be threadsafe!!!

        va_list args;
        va_start( args, format );
        int numCharsWritten = _vsnprintf_s( messageBuffer, bufferSize, bufferSize - 1, format, args );
        va_end( args );

        // Add newlines
        if ( numCharsWritten < 509 )
        {
            messageBuffer[numCharsWritten] = '\r';
            messageBuffer[numCharsWritten + 1] = '\n';
            messageBuffer[numCharsWritten + 2] = 0;
        }

        OutputDebugStringA( messageBuffer );
    }
}

#endif