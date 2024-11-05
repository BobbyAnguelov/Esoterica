#include "ReflectionSettings.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection::Utils
{
    bool PrintError( char const* pFormat, ... )
    {
        char buffer[256];

        va_list args;
        va_start( args, pFormat );
        VPrintf( buffer, 256, pFormat, args );
        va_end( args );

        std::cout << std::endl << " ! Error: " << buffer << std::endl;
        EE_TRACE_MSG( buffer );
        return false;
    }

    void PrintWarning( char const* pFormat, ... )
    {
        char buffer[256];

        va_list args;
        va_start( args, pFormat );
        VPrintf( buffer, 256, pFormat, args );
        va_end( args );

        std::cout << std::endl << " * Warning: " << buffer << std::endl;
        EE_TRACE_MSG( buffer );
    }
}