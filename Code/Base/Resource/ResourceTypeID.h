#pragma once

#include "Base/_Module/API.h"
#include "Base/Types/String.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/FileSystem/DataFileExtension.h"

//-------------------------------------------------------------------------
// Resource Type ID
//-------------------------------------------------------------------------
// Unique ID for a resource - Used for resource look up and dependencies
// Resource type IDs are lowercase and alphanumeric EightCCs i.e. can only contain lowercase ASCII english letters and digits

namespace EE
{
    class EE_BASE_API ResourceTypeID
    {
        EE_SERIALIZE( m_ID );

    public:

        // Check if a given string is a valid resource type EightCC (i.e. [1:8] lowercase letters or digits)
        inline static bool IsValidResourceTypeIdentifierString( String const& str ) { return DataFileExtension::IsValidExtension( str.c_str() ); }

        // Check if a given string is a valid resource type EightCC (i.e. [1:4] lowercase letters or digits)
        template<eastl_size_t S>
        inline static bool IsValidResourceTypeIdentifierString( TInlineString<S> const& str ) { return DataFileExtension::IsValidExtension( str.c_str() ); }

        // Expensive verification to ensure that a resource type ID EightCCs only contains uppercase or numeric chars
        static bool IsValidResourceTypeIdentifier( uint64_t code ) { return DataFileExtension::IsValidExtensionCode( code ); }

    public:

        inline ResourceTypeID() = default;
        inline ResourceTypeID( uint64_t ID ) : m_ID( ID ) { EE_ASSERT( IsValidResourceTypeIdentifier( ID ) ); }
        inline explicit ResourceTypeID( DataFileExtension ID ) : m_ID( ID ) { EE_ASSERT( ID.IsValid() ); }
        inline explicit ResourceTypeID( char const* pStr ) : m_ID( pStr ) { EE_ASSERT( IsValidResourceTypeIdentifierString( pStr ) ); }
        inline explicit ResourceTypeID( String const& str ) : ResourceTypeID( str.c_str() ) {}

        inline bool IsValid() const { return m_ID.IsValid() && IsValidResourceTypeIdentifierString( m_ID.ToString() ); }
        void Clear() { m_ID.Clear(); }

        inline operator uint64_t() const { return m_ID.GetCode(); }
        inline uint64_t ToUInt64() const { return m_ID.GetCode(); }

        //-------------------------------------------------------------------------

        // Convert the type ID to a string
        inline TInlineString<9> ToString() const { return m_ID.ToString(); }

        // Get the string value for this code
        inline void GetString( TInlineString<9> &outStr ) const { m_ID.GetString( outStr ); }

        // Get the string value for this code
        inline void GetString( InlineString &outStr ) const { m_ID.GetString( outStr ); }

        // Get the string value for this code
        inline void GetString( String &outStr ) const { m_ID.GetString( outStr ); }

        // Get the extension for this resource type ID
        inline FileSystem::Extension ToFileSystemExtension() const { return m_ID.ToFileSystemExtension(); }

    public:

        DataFileExtension     m_ID;
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