#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

extern "C"
{
    void mpack_assert_fail( const char* message )
    {
        EE_TRACE_HALT( message );
    }
}