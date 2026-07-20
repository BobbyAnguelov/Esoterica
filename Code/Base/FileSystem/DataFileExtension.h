#pragma once
#include "Base/Serialization/BinarySerialization.h"
#include "Base/FileSystem/FileSystemExtension.h"

//-------------------------------------------------------------------------
// Resource File Extension
//-------------------------------------------------------------------------
// This is a file extension to be used for all engine resources and data files 
// Stored as a 64bit 8CC - so can be passed around by value
// Only supports a max of 8 lowercase alphanumeric characters

//-------------------------------------------------------------------------

namespace EE
{
    class EE_BASE_API DataFileExtension
    {
        EE_SERIALIZE( m_code );

    public:

        static bool IsValidExtension( char const* pStr ) { return IsValidExtension( pStr, pStr != nullptr ? strlen( pStr ) : 0 ); }
        static bool IsValidExtension( FileSystem::Extension const& ext ) { return IsValidExtension( ext.c_str(), ext.length() ); }
        template<eastl_size_t S> static bool IsValidExtension( TInlineString<S> const& str ) { return IsValidExtension( str.c_str(), str.length() ); }

        static bool IsValidExtensionCode( uint64_t code );

    private:

        static bool IsValidExtension( char const* pStr, size_t length );

    public:

        DataFileExtension() = default;
        DataFileExtension( uint64_t code );
        explicit DataFileExtension( char const* pStr );

        template<eastl_size_t S>
        DataFileExtension( TInlineString<S> const& str ) : DataFileExtension( str.c_str() ) {}
        DataFileExtension( InlineString const& str ) : DataFileExtension( str.c_str() ) {}
        DataFileExtension( String const& str ) : DataFileExtension( str.c_str() ) {}
        DataFileExtension( FileSystem::Extension const& ext ) : DataFileExtension( ext.c_str() ) {}

        EE_FORCE_INLINE bool operator==( DataFileExtension const& rhs ) const { return m_code == rhs.m_code; }
        EE_FORCE_INLINE bool operator!=( DataFileExtension const& rhs ) const { return m_code != rhs.m_code; }
        EE_FORCE_INLINE bool operator==( uint64_t const& rhs ) const { return m_code == rhs; }
        EE_FORCE_INLINE bool operator!=( uint64_t const& rhs ) const { return m_code != rhs; }

        // Is this a valid code?
        bool IsValid() const;

        // Clear the code
        inline void Clear() { m_code = 0; }

        // Get the code
        inline uint64_t GetCode() const { return m_code; }

        // Get the string value for this code
        TInlineString<9> ToString() const;

        // Get the string value for this code
        inline FileSystem::Extension ToFileSystemExtension() const { return ToString(); }

        // Get the string value for this code
        void GetString( TInlineString<9> &outStr ) const;

        // Get the string value for this code
        void GetString( InlineString &outStr ) const;

        // Get the string value for this code
        void GetString( String &outStr ) const;

    private:

        uint64_t m_code = 0;
    };
}

//-------------------------------------------------------------------------

namespace eastl
{
    template <typename T> struct hash;

    template <>
    struct hash<EE::DataFileExtension>
    {
        size_t operator()( EE::DataFileExtension const& ID ) const { return ID.GetCode(); }
    };
}