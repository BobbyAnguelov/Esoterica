#pragma once
#include "Base/Types/String.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------
// Dumb Key Value Parser
//-------------------------------------------------------------------------
// Allows you to decompose a basic comma separated key / value string into its individual parts using
// All comma, quotations, space and equals characters will be stripped

/*
  e.g. "a=5, b=\"test\", c ='sdfsd', d, e=23.0f"

  will decompose into:

    {
        { a, "5" }, 
        { b, "test" },
        { c, "sdfsd" },
        { d, "" },
        { e, "23.0f" },
    }
*/
//-------------------------------------------------------------------------

namespace EE
{
    struct EE_BASE_API KeyValueParser
    {
        struct KV
        {
            String m_key;
            String m_value;
        };

    public:

        KeyValueParser() = default;
        KeyValueParser( String const& sourceStr ) { Parse( sourceStr ); }
        KeyValueParser( String const& sourceStr, char delimiter ) { Parse( sourceStr ); }

        void Parse( String str ); // Pass by value is intentional

        bool HasKey( char const* pKey ) const;
        bool HasKey( String const& key ) const { return HasKey( key.c_str() ); }

        String const* GetValueForKey( char const* pKey ) const;
        String const* GetValueForKey( String const& key ) const { return GetValueForKey( key.c_str() ); }

        bool TryGetValueForKey( char const* pKey, String& outValue ) const;
        bool TryGetValueForKey( String const& key, String& outValue ) const { return TryGetValueForKey( key.c_str(), outValue ); }

    public:

        TInlineVector<KV, 10> m_keyValues;
    };
}