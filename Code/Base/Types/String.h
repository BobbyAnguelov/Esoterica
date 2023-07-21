#pragma once

#include "Base/_Module/API.h"
#include "Base/Esoterica.h"
#include <EASTL/string.h>
#include <EASTL/fixed_string.h>
#include <cstdio>

//-------------------------------------------------------------------------

namespace EE
{
    using String = eastl::basic_string<char>;
    template<eastl_size_t S> using TInlineString = eastl::fixed_string<char, S, true>;
    using InlineString = eastl::fixed_string<char, 255, true>;

    //-------------------------------------------------------------------------
    // Additional utility functions for string class
    //-------------------------------------------------------------------------

    inline int32_t VPrintf( char* pBuffer, uint32_t bufferSize, char const* pMessageFormat, va_list args )
    {
        return vsnprintf( pBuffer, size_t( bufferSize ), pMessageFormat, args );
    }

    inline int32_t Printf( char* pBuffer, uint32_t bufferSize, char const* pMessageFormat, ... )
    {
        va_list args;
        va_start( args, pMessageFormat );
        int32_t const numChars = VPrintf( pBuffer, size_t( bufferSize ), pMessageFormat, args );
        va_end( args );
        return numChars;
    }

    //-------------------------------------------------------------------------

    namespace StringUtils
    {
        template<typename StringType>
        inline bool EndsWith( StringType const& inStr, char const* pStringToMatch )
        {
            EE_ASSERT( pStringToMatch != nullptr );
            size_t const matchStrLen = strlen( pStringToMatch );
            EE_ASSERT( matchStrLen > 0 );

            if ( inStr.length() < matchStrLen )
            {
                return false;
            }

            // Compare substr
            char const* const pSubString = &inStr[inStr.length() - matchStrLen];
            return strcmp( pSubString, pStringToMatch ) == 0;
        }

        template<typename StringType>
        inline StringType ReplaceAllOccurrences( StringType const& originalString, char const* pSearchString, char const* pReplacement )
        {
            EE_ASSERT( pSearchString != nullptr );
            int32_t const searchLength = (int32_t) strlen( pSearchString );
            if ( originalString.empty() || searchLength == 0 )
            {
                return originalString;
            }

            int32_t const replacementLength = ( pReplacement == nullptr ) ? 1 : (int32_t) strlen( pReplacement ) + 1;

            StringType copiedString = originalString;
            auto idx = originalString.find( pSearchString );
            while ( idx != StringType::npos )
            {
                copiedString.replace( idx, searchLength, pReplacement == nullptr ? "" : pReplacement );
                idx = copiedString.find( pSearchString, idx + replacementLength );
            }

            return copiedString;
        }

        template<typename StringType>
        inline StringType& ReplaceAllOccurrencesInPlace( StringType& originalString, char const* pSearchString, char const* pReplacement )
        {
            EE_ASSERT( pSearchString != nullptr );
            int32_t const searchLength = (int32_t) strlen( pSearchString );
            if ( originalString.empty() || searchLength == 0 )
            {
                return originalString;
            }

            int32_t const replacementLength = ( pReplacement == nullptr ) ? 1 : (int32_t) strlen( pReplacement ) + 1;

            auto idx = originalString.find( pSearchString );
            while ( idx != StringType::npos )
            {
                originalString.replace( idx, searchLength, pReplacement == nullptr ? "" : pReplacement );
                idx = originalString.find( pSearchString, idx + replacementLength );
            }

            return originalString;
        }

        template<typename StringType>
        inline StringType RemoveAllOccurrences( StringType const& originalString, char const* searchString )
        {
            return ReplaceAllOccurrences( originalString, searchString, "" );
        }

        template<typename StringType>
        inline StringType& RemoveAllOccurrencesInPlace( StringType& originalString, char const* searchString )
        {
            return ReplaceAllOccurrencesInPlace( originalString, searchString, "" );
        }

        template<typename StringType>
        inline StringType StripAllWhitespace( StringType const& originalString )
        {
            StringType strippedString = originalString;
            strippedString.erase( eastl::remove( strippedString.begin(), strippedString.end(), ' ' ), strippedString.end() );
            return strippedString;
        }

        inline void StripTrailingWhitespace( char* string )
        {
            size_t const origStringLength = strlen( string );
            InlineString tmp = string;
            tmp.rtrim();
            strncpy_s( string, origStringLength + 1, tmp.c_str(), tmp.length() );
        }

        template<typename StringType, typename StringTypeVector>
        inline void Split( StringType const& str, StringTypeVector& results, char const* pDelimiter = " ", bool ignoreEmptyStrings = true )
        {
            size_t idx, lastIdx = 0;
            results.clear();

            while ( true )
            {
                idx = str.find_first_of( pDelimiter, lastIdx );
                if ( idx == StringType::npos )
                {
                    idx = str.length();

                    if ( idx != lastIdx || !ignoreEmptyStrings )
                    {
                        results.push_back( StringType( str.data() + lastIdx, idx - lastIdx ) );
                    }
                    break;
                }
                else
                {
                    if ( idx != lastIdx || !ignoreEmptyStrings )
                    {
                        results.push_back( StringType( str.data() + lastIdx, idx - lastIdx ) );
                    }
                }

                lastIdx = idx + 1;
            }
        }

        //-------------------------------------------------------------------------

        // Is this a valid hex character (0-9 & A-F)
        inline bool IsValidHexChar( char ch )
        {
            return (bool) isxdigit( ch );
        }

        EE_BASE_API int32_t CompareInsensitive( char const* pStr0, char const* pStr1 );
        EE_BASE_API int32_t CompareInsensitive( char const* pStr0, char const* pStr1, size_t n );

        //-------------------------------------------------------------------------

        // Convert hex character (0-9 & A-F) to byte value
        inline uint8_t HexCharToByteValue( char ch )
        {
            // 0-9
            if ( ch > 47 && ch < 58 ) return (uint8_t) ( ch - 48 );

            // a-f
            if ( ch > 96 && ch < 103 ) return (uint8_t) ( ch - 87 );

            // A-F
            if ( ch > 64 && ch < 71 ) return (uint8_t) ( ch - 55 );

            return 0;
        }

        //-------------------------------------------------------------------------

        // Convert hex character pair (0-9 & A-F) to byte value
        inline uint8_t HexCharToByteValue( char a, char b )
        {
            return (uint8_t) ( HexCharToByteValue( a ) * 16 + HexCharToByteValue( b ) );
        }
    }
}