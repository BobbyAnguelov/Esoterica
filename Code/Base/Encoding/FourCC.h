#pragma once

#include "Base/_Module/API.h"
#include "Base/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------
// Four Character Code (4CC) Helpers
//-------------------------------------------------------------------------
// We have restrictions in Esoterica regarding FourCCs:
// * Only alphanumeric characters are allowed
// * Only allow leading null characters e.g. this is valid { '0', 'a', 'b', 'c' } but this is not { 'a', 0, 'b', 'c' }

namespace EE::FourCC
{
    // Is this is a valid alphanumeric FourCC i.e. only [a-z], [A-Z] and [0-9] allowed
    EE_BASE_API bool IsValid( char const* pStr );

    // Is this is a valid alphanumeric FourCC i.e. only [a-z] and [0-9] allowed
    EE_BASE_API bool IsValidLowercase( char const* pStr );

    // Is this a a valid alphanumeric FourCC i.e. only [a-z], [A-Z] and [0-9] allowed
    EE_BASE_API bool IsValid( uint32_t fourCC );

    // Is this is a valid lowercase alphanumeric FourCC i.e. only [a-z] and [0-9] allowed
    EE_BASE_API bool IsValidLowercase( uint32_t fourCC );

    // Get the fourCC from a string
    EE_BASE_API uint32_t FromString( char const* pStr );

    // Get the fourCC from a lowercase only string
    EE_BASE_API uint32_t FromLowercaseString( char const* pStr );

    // Get the fourCC from a lowercase only string
    EE_BASE_API bool TryCreateFromLowercaseString( char const* pStr, uint32_t& outFourCC );

    // Get a string from a fourCC
    EE_BASE_API void ToString( uint32_t fourCC, char str[5] );

    // Get a string from a lowercase only fourCC
    EE_BASE_API void ToLowercaseString( uint32_t fourCC, char str[5] );

    // Get a string from a fourCC
    EE_BASE_API TInlineString<5> ToString( uint32_t fourCC );

    // Get a string from a lowercase only fourCC
    EE_BASE_API TInlineString<5> ToLowercaseString( uint32_t fourCC );
}