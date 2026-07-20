#include "DataFileExtension.h"
#include "Base/Encoding/Encoding.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool DataFileExtension::IsValidExtension( char const* pStr, size_t length )
    {
        if ( pStr == nullptr )
        {
            return false;
        }

        if ( length == 0 || length > 8 )
        {
            return false;
        }

        if ( !StringUtils::IsLowercaseAlphaNumeric( pStr ) )
        {
            return false;
        }

        return true;
    }

    bool DataFileExtension::IsValidExtensionCode( uint64_t code )
    {
        TInlineString<9> str = Encoding::EightCC::Decode( code );
        if ( str.empty() )
        {
            return false;
        }

        return StringUtils::IsLowercaseAlphaNumeric( str );
    }

    //-------------------------------------------------------------------------

    DataFileExtension::DataFileExtension( uint64_t code )
        : m_code( code )
    {
        EE_ASSERT( IsValid() );
    }

    DataFileExtension::DataFileExtension( char const* pStr )
    {
        if ( IsValidExtension( pStr ) )
        {
            m_code = Encoding::EightCC::Encode( pStr );
        }
    }

    bool DataFileExtension::IsValid() const
    {
        if ( m_code == 0 )
        {
            return false;
        }

        return IsValidExtensionCode( m_code );
    }

    TInlineString<9> DataFileExtension::ToString() const
    {
        EE_ASSERT( m_code != 0 );
        TInlineString<9> str = Encoding::EightCC::Decode( m_code );
        EE_ASSERT( !str.empty() );
        return str;
    }

    void DataFileExtension::GetString( TInlineString<9> &outStr ) const
    {
        EE_ASSERT( m_code != 0 );
        outStr = Encoding::EightCC::Decode( m_code );
        EE_ASSERT( !outStr.empty() );
    }

    void DataFileExtension::GetString( String &outStr ) const
    {
        EE_ASSERT( m_code != 0 );
        TInlineString<9> str = Encoding::EightCC::Decode( m_code );
        EE_ASSERT( !str.empty() );

        outStr.resize( str.size() );
        memcpy( outStr.data(), str.data(), str.size() );
    }

    void DataFileExtension::GetString( InlineString &outStr ) const
    {
        EE_ASSERT( m_code != 0 );
        TInlineString<9> str = Encoding::EightCC::Decode( m_code );
        EE_ASSERT( !str.empty() );

        outStr.resize( str.size() );
        memcpy( outStr.data(), str.data(), str.size() );
    }
}