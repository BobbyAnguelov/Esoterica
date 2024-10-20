#pragma once

#include "Base/_Module/API.h"
#include "Base/Types/String.h"
#include "Base/Encoding/FourCC.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/FileSystem/FileSystemExtension.h"

//-------------------------------------------------------------------------
// Resource Type ID
//-------------------------------------------------------------------------
// Unique ID for a resource - Used for resource look up and dependencies
// Resource type IDs are lowercase FourCCs i.e. can only contain lowercase ASCII letters and digits

namespace EE
{
    class EE_BASE_API ResourceTypeID
    {
        EE_SERIALIZE( m_ID );

    public:

        // Check if a given string is a valid resource type FourCC (i.e. [1:4] lowercase letters or digits)
        static bool IsValidResourceFourCC( char const* pStr ) { return FourCC::IsValidLowercase( pStr ); }

        // Check if a given string is a valid resource type FourCC (i.e. [1:4] lowercase letters or digits)
        inline static bool IsValidResourceFourCC( String const& str ) { return IsValidResourceFourCC( str.c_str() ); }

        // Check if a given string is a valid resource type FourCC (i.e. [1:4] lowercase letters or digits)
        template<eastl_size_t S>
        inline static bool IsValidResourceFourCC( TInlineString<S> const& str ) { return IsValidResourceFourCC( str.c_str() ); }

        // Expensive verification to ensure that a resource type ID FourCC only contains uppercase or numeric chars
        static bool IsValidResourceFourCC( uint32_t fourCC ) { return FourCC::IsValidLowercase( fourCC ); }

    public:

        inline ResourceTypeID() = default;
        inline ResourceTypeID( uint32_t ID ) : m_ID( ID ) { EE_ASSERT( IsValidResourceFourCC( m_ID ) ); }
        explicit ResourceTypeID( char const* pStr ) : m_ID( FourCC::FromLowercaseString( pStr ) ) {}
        inline explicit ResourceTypeID( String const& str ) : ResourceTypeID( str.c_str() ) {}

        inline bool IsValid() const { return m_ID != 0; }
        void Clear() { m_ID = 0; }

        inline operator uint32_t() const { return m_ID; }
        inline operator uint32_t&() { return m_ID; }

        //-------------------------------------------------------------------------

        inline void GetString( char outStr[5] ) const { FourCC::ToString( m_ID, outStr ); }
        inline TInlineString<5> ToString() const { return FourCC::ToString( m_ID ); }
        inline FileSystem::Extension ToExtension() const { return FourCC::ToString( m_ID ); }

    public:

        uint32_t                 m_ID = 0;
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