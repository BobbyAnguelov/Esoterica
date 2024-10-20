#include "StringKeyValueParser.h"
#include "Base/Math/Math.h"

//-------------------------------------------------------------------------

namespace EE
{
    static inline void SanitizeKeyValueString( String& str )
    {
        if ( str.empty() )
        {
            return;
        }

        str.ltrim();
        str.rtrim();

        // Strip pairs of quotes
        //-------------------------------------------------------------------------

        auto StripQuotes = [&str] ( char quote )
        {
            if ( str.front() == quote && str.back() == quote )
            {
                str = str.substr( 1, str.length() - 2 );
            }
        };

        StripQuotes( '"' );
        StripQuotes( '\'' );
        StripQuotes( '`' );

        //-------------------------------------------------------------------------

        str.ltrim();
        str.rtrim();
    };

    void KeyValueParser::Parse( String str )
    {
        m_keyValues.clear();

        // Fancier split since we might have delimiter characters inside a string block
        //-------------------------------------------------------------------------

        TInlineVector<String, 10> results;

        String currentToken;
        int32_t quoteCount = 0;
        int32_t length = (int32_t) str.length();
        for ( int32_t i = 0; i < length; i++ )
        {
            if ( str[i] == ',' && Math::IsEven( quoteCount ) )
            {
                currentToken.rtrim();
                results.emplace_back( currentToken );
                currentToken.clear();
                quoteCount = 0;
                continue;
            }

            //-------------------------------------------------------------------------

            if ( str[i] == '"' )
            {
                quoteCount++;
            }

            // Strip leading whitespace
            if ( !currentToken.empty() || str[i] != ' ' )
            {
                currentToken.append( 1, str[i] );
            }
        }

        if ( !currentToken.empty() )
        {
            currentToken.rtrim();
            results.emplace_back( currentToken );
            currentToken.clear();
            quoteCount = 0;
        }

        // Split results using the '=' char
        //-------------------------------------------------------------------------

        for ( String& part : results )
        {
            int32_t separatorIdx = InvalidIndex;
            length = (int32_t) part.length();
            for ( int32_t i = 0; i < length; i++ )
            {
                if ( part[i] == '=' && Math::IsEven( quoteCount ) )
                {
                    separatorIdx = i;
                    break;
                }

                //-------------------------------------------------------------------------

                if ( part[i] == '"' )
                {
                    quoteCount++;
                }
            }

            KV& kv = m_keyValues.emplace_back();
            if ( separatorIdx != InvalidIndex )
            {
                kv.m_key = part.substr( 0, separatorIdx );
                kv.m_value = part.substr( separatorIdx + 1, part.length() - separatorIdx - 1 );
            }
            else
            {
                kv.m_key = part;
            }

            SanitizeKeyValueString( kv.m_key );
            SanitizeKeyValueString( kv.m_value );
        }
    }

    //-------------------------------------------------------------------------

    bool KeyValueParser::HasKey( char const* pKey ) const
    {
        EE_ASSERT( pKey != nullptr );

        for ( KV const& kv : m_keyValues )
        {
            if ( _stricmp( kv.m_key.c_str(), pKey ) == 0 )
            {
                return true;
            }
        }

        return false;
    }

    String const* KeyValueParser::GetValueForKey( char const* pKey ) const
    {
        EE_ASSERT( pKey != nullptr );

        for ( KV const& kv : m_keyValues )
        {
            if ( _stricmp( kv.m_key.c_str(), pKey ) == 0 )
            {
                if ( kv.m_value.empty() )
                {
                    return nullptr;
                }

                return &kv.m_value;
            }
        }

        return nullptr;
    }

    bool KeyValueParser::TryGetValueForKey( char const* pKey, String& outValue ) const
    {
        EE_ASSERT( pKey != nullptr );

        for ( KV const& kv : m_keyValues )
        {
            if ( _stricmp( kv.m_key.c_str(), pKey ) == 0 )
            {
                if ( !kv.m_value.empty() )
                {
                    outValue = kv.m_value;
                    return true;
                }
            }
        }

        //-------------------------------------------------------------------------

        outValue.clear();
        return false;
    }
}