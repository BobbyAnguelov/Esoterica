#include "ReflectorSettings.h"
#include "Base/Types/String.h"
#include <iostream>

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    bool PrintError( char const* pFormat, ... )
    {
        TInlineString<1024> str;

        va_list args;
        va_start( args, pFormat );
        str.append_sprintf_va_list( pFormat, args );
        va_end( args );

        std::cout << "  ! Error: " << str.c_str() << std::endl;
        EE_TRACE_MSG( str.c_str() );
        return false;
    }

    void PrintWarning( char const* pFormat, ... )
    {
        TInlineString<1024> str;

        va_list args;
        va_start( args, pFormat );
        str.append_sprintf_va_list( pFormat, args );
        va_end( args );

        std::cout << "  * Warning: " << str.c_str() << std::endl;
        EE_TRACE_MSG( str.c_str() );
    }
}