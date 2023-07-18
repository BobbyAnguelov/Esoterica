#include "ResourceTypeID.h"
#include <cctype>

//-------------------------------------------------------------------------

namespace EE
{
    static uint32_t CalculateResourceFourCC( char const* pStr )
    {
        uint32_t ID = 0;
        size_t const numChars = strlen( pStr );
        if ( numChars > 0 && numChars <= 4 )
        {
            for ( size_t x = 0; x < numChars; x++ )
            {
                if ( !std::islower( pStr[x] ) && !std::isdigit( pStr[x] ) )
                {
                    ID = 0;
                    break;
                }

                ID |= (uint32_t) pStr[x] << ( numChars - 1 - x ) * 8;
            }
        }

        return ID;
    }

    bool ResourceTypeID::IsValidResourceFourCC( uint32_t fourCC )
    {
        char const str[4] =
        {
            (char) ( fourCC >> 24 ),
            (char) ( ( fourCC & 0x00FF0000 ) >> 16 ),
            (char) ( ( fourCC & 0x0000FF00 ) >> 8 ),
            (char) ( ( fourCC & 0x000000FF ) ),
        };

        // All zero values is not allowed
        if ( str[0] == 0 && str[1] == 0 && str[2] == 0 && str[3] == 0 )
        {
            return false;
        }

        // Ensure that we dont have a zero value between set values
        bool nonZeroFound = false;
        for ( auto i = 0; i < 4; i++ )
        {
            if ( str[i] != 0 )
            {
                nonZeroFound = true;
            }

            if ( str[i] == 0 && nonZeroFound )
            {
                return false;
            }
        }

        // Ensure all characters are lowercase ascii or digits
        return
            ( str[0] == 0 || std::islower( str[0] ) || std::isdigit( str[0] ) ) &&
            ( str[1] == 0 || std::islower( str[1] ) || std::isdigit( str[1] ) ) &&
            ( str[2] == 0 || std::islower( str[2] ) || std::isdigit( str[2] ) ) &&
            ( str[3] == 0 || std::islower( str[3] ) || std::isdigit( str[3] ) );
    }

    //-------------------------------------------------------------------------

    bool ResourceTypeID::IsValidResourceFourCC( char const* pStr )
    {
        return CalculateResourceFourCC( pStr ) != 0;
    }

    //-------------------------------------------------------------------------

    ResourceTypeID::ResourceTypeID( char const* pStr )
    {
        m_ID = CalculateResourceFourCC( pStr );
    }
}