#include "FourCC.h"
#include <cctype>
#include "Base/Memory/Memory.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace
    {
        struct ValidateAlphaNumeric
        {
            EE_FORCE_INLINE static bool Validate( char c )
            {
                return std::isalnum( c ) != 0;
            }
        };

        struct ValidateLowercaseAlphaNumeric
        {
            EE_FORCE_INLINE static bool Validate( char c )
            {
                return ( std::isalnum( c ) != 0 ) || std::islower( c );
            }
        };
    }

    template<typename V>
    EE_FORCE_INLINE bool TryConvertFourCC( uint32_t fourCC, char outStr[5] )
    {
        Memory::MemsetZero( outStr, sizeof( char ) * 5 );

        //-------------------------------------------------------------------------

        char str[5] = { 0 };
        int32_t idx = 0;

        auto ValidateCode = [&] ( int32_t& idx )
        {
            // Non-zero value
            if ( str[idx] != 0 )
            {
                // Perform requires character validation
                if ( !V::Validate( str[idx] ) )
                {
                    return false;
                }

                idx++;
            }
            else // Zero value
            {
                // We do not allow zero values between non-zero values
                if ( idx != 0 )
                {
                    return false;
                }
            }

            return true;
        };

        //-------------------------------------------------------------------------

        str[idx] = (char) ( fourCC >> 24 );
        ValidateCode( idx );

        str[idx] = (char) ( ( fourCC & 0x00FF0000 ) >> 16 );
        ValidateCode( idx );

        str[idx] = (char) ( ( fourCC & 0x0000FF00 ) >> 8 );
        ValidateCode( idx );

        str[idx] = (char) ( fourCC & 0x000000FF );
        ValidateCode( idx );

        //-------------------------------------------------------------------------

        // All zero values is not allowed
        if ( str[0] == 0 && idx == 0 )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        memcpy( outStr, str, sizeof( char ) * 5 );

        return true;
    }

    //-------------------------------------------------------------------------

    template<typename V>
    EE_FORCE_INLINE bool TryConvertFourCC( char const* pStr, uint32_t& fourCC )
    {
        fourCC = 0;

        if ( pStr == nullptr )
        {
            return false;
        }

        size_t const length = strlen( pStr );
        if ( length > 4 )
        {
            return false;
        }

        for ( size_t i = 0; i < length; i++ )
        {
            if ( !V::Validate( pStr[i] ) )
            {
                return false;
            }

            fourCC |= ( uint32_t( pStr[i] ) << ( length - 1 - i ) * 8 );
        }

        return true;
    }

    //-------------------------------------------------------------------------

    bool FourCC::IsValid( char const* pStr )
    {
        uint32_t fourCC;
        return TryConvertFourCC<ValidateAlphaNumeric>( pStr, fourCC );
    }

    bool FourCC::IsValidLowercase( char const* pStr )
    {
        uint32_t fourCC;
        return TryConvertFourCC<ValidateLowercaseAlphaNumeric>( pStr, fourCC );
    }

    bool FourCC::IsValid( uint32_t fourCC )
    {
        char str[5];
        return TryConvertFourCC<ValidateAlphaNumeric>( fourCC, str );
    }

    bool FourCC::IsValidLowercase( uint32_t fourCC )
    {
        char str[5];
        return TryConvertFourCC<ValidateLowercaseAlphaNumeric>( fourCC, str );
    }

    uint32_t FourCC::FromString( char const* pStr )
    {
        uint32_t fourCC;
        bool const result = TryConvertFourCC<ValidateAlphaNumeric>( pStr, fourCC );
        EE_ASSERT( result );
        return fourCC;
    }

    uint32_t FourCC::FromLowercaseString( char const* pStr )
    {
        uint32_t fourCC;
        bool const result = TryConvertFourCC<ValidateLowercaseAlphaNumeric>( pStr, fourCC );
        EE_ASSERT( result );
        return fourCC;
    }

    void FourCC::ToString( uint32_t fourCC, char str[5] )
    {
        bool const result = TryConvertFourCC<ValidateAlphaNumeric>( fourCC, str );
        EE_ASSERT( result );
    }

    void FourCC::ToLowercaseString( uint32_t fourCC, char str[5] )
    {
        bool const result = TryConvertFourCC<ValidateLowercaseAlphaNumeric>( fourCC, str );
        EE_ASSERT( result );
    }

    TInlineString<5> FourCC::ToString( uint32_t fourCC )
    {
        char str[5];
        bool const result = TryConvertFourCC<ValidateAlphaNumeric>( fourCC, str );
        EE_ASSERT( result );
        return str;
    }

    TInlineString<5> FourCC::ToLowercaseString( uint32_t fourCC )
    {
        char str[5];
        bool const result = TryConvertFourCC<ValidateLowercaseAlphaNumeric>( fourCC, str );
        EE_ASSERT( result );
        return str;
    }
}