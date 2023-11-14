#if _WIN32
#include "Base/Esoterica.h"
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>

//-------------------------------------------------------------------------

namespace EE::Log
{
    void TraceMessage( const char* format, ... )
    {
        constexpr size_t const bufferSize = 2048;
        char messageBuffer[bufferSize]; // Dont make this static as we need this to be threadsafe!!!

        va_list args;
        va_start( args, format );
        int32_t numCharsWritten = _vsnprintf_s( messageBuffer, bufferSize, bufferSize - 1, format, args );
        va_end( args );

        // Add newlines
        if ( numCharsWritten > 0 && numCharsWritten < 509 )
        {
            messageBuffer[numCharsWritten] = '\r';
            messageBuffer[numCharsWritten + 1] = '\n';
            messageBuffer[numCharsWritten + 2] = 0;
        }

        OutputDebugStringA( messageBuffer );
    }
}

#endif