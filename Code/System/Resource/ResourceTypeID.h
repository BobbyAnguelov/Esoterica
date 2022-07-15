#pragma once

#include "System/_Module/API.h"
#include "System/Types/String.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------
// Resource Type ID
//-------------------------------------------------------------------------
// Unique ID for a resource - Used for resource look up and dependencies
// Resource type IDs are lowercase FourCCs i.e. can only contain lowercase ASCII letters and digits

namespace EE
{
    class EE_SYSTEM_API ResourceTypeID
    {
        EE_SERIALIZE( m_ID );

    public:

        // Check if a given string is a valid resource type FourCC (i.e. [1:4] lowercase letters or digits)
        static bool IsValidResourceFourCC( char const* pStr );

        // Check if a given string is a valid resource type FourCC (i.e. [1:4] lowercase letters or digits)
        inline static bool IsValidResourceFourCC( String const& str ) { return IsValidResourceFourCC( str.c_str() ); }

        // Check if a given string is a valid resource type FourCC (i.e. [1:4] lowercase letters or digits)
        template<eastl_size_t S>
        inline static bool IsValidResourceFourCC( TInlineString<S> const& str ) { return IsValidResourceFourCC( str.c_str() ); }

        // Expensive verification to ensure that a resource type ID FourCC only contains uppercase or numeric chars
        static bool IsValidResourceFourCC( uint32_t fourCC );

    public:

        inline ResourceTypeID() : m_ID( 0 ) {}
        inline ResourceTypeID( uint32_t ID ) : m_ID( ID ) { EE_ASSERT( IsValidResourceFourCC( m_ID ) ); }
        explicit ResourceTypeID( char const* pStr );
        inline explicit ResourceTypeID( String const& str ) : ResourceTypeID( str.c_str() ) {}

        inline bool IsValid() const { return m_ID != 0; }
        void Clear() { m_ID = 0; }

        inline operator uint32_t() const { return m_ID; }
        inline operator uint32_t&() { return m_ID; }

        //-------------------------------------------------------------------------

        inline void GetString( char outStr[5] ) const
        {
            EE_ASSERT( IsValidResourceFourCC( m_ID ) );

            int32_t idx = 0;

            outStr[idx] = (uint8_t) ( m_ID >> 24 );
            if ( outStr[idx] != 0 )
            {
                idx++;
            }

            outStr[idx] = (uint8_t) ( ( m_ID & 0x00FF0000 ) >> 16 );
            if ( outStr[idx] != 0 )
            {
                idx++;
            }

            outStr[idx] = (uint8_t) ( ( m_ID & 0x0000FF00 ) >> 8 );
            if ( outStr[idx] != 0 )
            {
                idx++;
            }

            outStr[idx] = (uint8_t) ( ( m_ID & 0x000000FF ) );
            if ( outStr[idx] != 0 )
            {
                idx++;
            }

            outStr[idx] = 0;
        }

        inline TInlineString<5> ToString() const
        {
            TInlineString<5> str;
            str.resize( 5 );
            GetString( str.data() );
            return str;
        }

    public:

        uint32_t                 m_ID;
    };
}

//-------------------------------------------------------------------------
// Support for THashMap

namespace eastl
{
    template <>
    struct hash<EE::ResourceTypeID>
    {
        eastl_size_t operator()( EE::ResourceTypeID const& ID ) const
        {
            return ( uint32_t ) ID;
        }
    };
}