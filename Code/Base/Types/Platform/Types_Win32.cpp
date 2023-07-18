#ifdef _WIN32
#include "../UUID.h"
#include <Objbase.h>

//-------------------------------------------------------------------------

namespace EE
{
    UUID UUID::GenerateID()
    {
        UUID newID;
        static_assert( sizeof( GUID ) == sizeof( UUID ), "Size mismatch for EE GUID vs Win32 GUID" );
        CoCreateGuid( (GUID*) &newID );
        return newID;
    }

    //-------------------------------------------------------------------------

    namespace StringUtils
    {
        int32_t CompareInsensitive( char const* pStr0, char const* pStr1 )
        {
            return _stricmp( pStr0, pStr1 );
        }

        int32_t CompareInsensitive( char const* pStr0, char const* pStr1, size_t n )
        {
            return _strnicmp( pStr0, pStr1, n );
        }
    }
}
#endif