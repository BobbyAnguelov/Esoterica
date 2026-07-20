#pragma once

#include "Base/_Module/API.h"
#include "Base/Esoterica.h"
#include "Base/ThirdParty/EA/eastl_Esoterica_TrackedAllocator.h"
#include <EASTL/string.h>
#include <EASTL/fixed_string.h>
#include <EASTL/string_view.h>
#include <cstdio>

//-------------------------------------------------------------------------

namespace EE
{
    using String = eastl::basic_string<char, eastl::TrackedAllocator>;
    using StringView = eastl::basic_string_view<char>;
    template<eastl_size_t S> using TInlineString = eastl::fixed_string<char, S, true, eastl::TrackedAllocator>;
    using InlineString = eastl::fixed_string<char, 255, true, eastl::TrackedAllocator>;

    using WString = eastl::basic_string<wchar_t, eastl::TrackedAllocator>;

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
        // Queries
        //-------------------------------------------------------------------------

        // Compile time alphanumeric check
        template <int32_t N>
        inline consteval int32_t GetStringLiteralLength_ConstEval( const char( &stringLiteral )[N] )
        {
            return N - 1;
        }

        // Compile time alphanumeric check
        template <int32_t N>
        inline consteval bool IsAlphaNumeric_ConstEval( const char( &stringLiteral )[N] )
        {
            for ( int32_t i = 0; i < ( N - 1 ); i++ )
            {
                char const c = stringLiteral[i];
                bool const isValid = ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
                if ( !isValid )
                {
                    return false;
                }
            }

            return true;
        }

        // Compile time alphanumeric and lower case check
        template <int32_t N>
        inline consteval bool IsLowercaseAlphaNumeric_ConstEval( const char( &stringLiteral )[N] )
        {
            for ( int32_t i = 0; i < ( N - 1 ); i++ )
            {
                char const c = stringLiteral[i];
                bool const isValid = ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'z' );
                if ( !isValid )
                {
                    return false;
                }
            }

            return true;
        }

        // Run time alphanumeric check
        template<typename StringType>
        inline bool IsAlphaNumeric( StringType const& inStr )
        {
            int32_t const length = (int32_t) inStr.size();
            for ( int32_t i = 0; i < length; i++ )
            {
                char const c = inStr[i];
                bool const isValid = ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
                if ( !isValid )
                {
                    return false;
                }
            }

            return true;
        }

        // Run time alphanumeric and lower case check
        template<typename StringType>
        inline bool IsLowercaseAlphaNumeric( StringType const& inStr )
        {
            int32_t const length = (int32_t) inStr.size();
            for ( int32_t i = 0; i < length; i++ )
            {
                char const c = inStr[i];
                bool const isValid = ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'z' );
                if ( !isValid )
                {
                    return false;
                }
            }

            return true;
        }

        // Run time alphanumeric check
        inline bool IsAlphaNumeric( char const* pStr )
        {
            EE_ASSERT( pStr != nullptr );

            int32_t const length = (int32_t) strlen( pStr );
            for ( int32_t i = 0; i < length; i++ )
            {
                char const c = pStr[i];
                bool const isValid = ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
                if ( !isValid )
                {
                    return false;
                }
            }

            return true;
        }

        // Run time alphanumeric and lower case check
        inline bool IsLowercaseAlphaNumeric( char const* pStr )
        {
            EE_ASSERT( pStr != nullptr );

            int32_t const length = (int32_t) strlen( pStr );
            for ( int32_t i = 0; i < length; i++ )
            {
                char const c = pStr[i];
                bool const isValid = ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'z' );
                if ( !isValid )
                {
                    return false;
                }
            }

            return true;
        }

        template<typename StringType>
        inline bool StartsWith( StringType const& inStr, char const* pStringToMatch )
        {
            EE_ASSERT( pStringToMatch != nullptr );
            size_t const matchStrLen = strlen( pStringToMatch );
            EE_ASSERT( matchStrLen > 0 );

            if ( inStr.length() < matchStrLen )
            {
                return false;
            }

            // Compare substr
            char const* const pSubString = &inStr[0];
            return strncmp( pSubString, pStringToMatch, matchStrLen ) == 0;
        }

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

        inline bool StartsWith( char const *pStr, char const* pStringToMatch )
        {
            StringView const view( pStr );
            return view.starts_with( pStringToMatch );
        }

        inline bool EndsWith( char const *pStr, char const* pStringToMatch )
        {
            StringView const view( pStr );
            return view.ends_with( pStringToMatch );
        }

        // Modifiers
        //-------------------------------------------------------------------------

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

        EE_BASE_API void InsertSpacesAccordingToCapitalization( String& str );

        inline void StripExtensionInPlace( String& string )
        {
            size_t extensionStartIdx = string.find_first_of( "." );
            EE_ASSERT( extensionStartIdx != String::npos );
            string.erase( extensionStartIdx );
        }

        inline String StripExtension( String const& string )
        {
            String str = string;
            StripExtensionInPlace( str );
            return str;
        }

        constexpr eastl::string_view GetUnqualifiedTypeName( eastl::string_view qualifiedTypeName )
        {
            size_t const pos = qualifiedTypeName.rfind( "::" );
            return ( pos == eastl::string_view::npos ) ? qualifiedTypeName : qualifiedTypeName.substr( pos + 2 );
        }

        // Comparison
        //-------------------------------------------------------------------------

        EE_BASE_API int32_t CompareInsensitive( char const* pStr0, char const* pStr1 );
        EE_BASE_API int32_t CompareInsensitive( char const* pStr0, char const* pStr1, size_t n );

        EE_FORCE_INLINE int32_t Stricmp( char const* pStr0, char const* pStr1 ) { return CompareInsensitive( pStr0, pStr1 ); }
        EE_FORCE_INLINE int32_t Stricmp( char const* pStr0, char const* pStr1, size_t n ) { return CompareInsensitive( pStr0, pStr1, n ); }

        // Hex Helpers
        //-------------------------------------------------------------------------

        // Is this a valid hex character (0-9 & A-F)
        inline bool IsValidHexChar( char ch )
        {
            return (bool) isxdigit( ch );
        }

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

        // Conversion
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE WString ToWideString( String const& str ) { return WString( WString::CtorConvert(), str ); }
        EE_FORCE_INLINE WString ToWideString( char const* pStr ) { return WString( WString::CtorConvert(), pStr ); }
    }
}

namespace eastl
{
    template <>
    struct hash<EE::String>
    {
        size_t operator()( EE::String const& s ) const { return hash<const char*>{}( s.c_str() ); }
    };
}
