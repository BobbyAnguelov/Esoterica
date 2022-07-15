#pragma once

#include "System/_Module/API.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/Memory/Memory.h"
#include "String.h"

//-------------------------------------------------------------------------

namespace EE
{
    // Universally Unique ID - 128bit Variant 1 ID
    //-------------------------------------------------------------------------
    // Guaranteed to be unique

    using UUIDString = TInlineString<37>;

    class EE_SYSTEM_API UUID
    {
        EE_SERIALIZE( m_data.m_U64 );

        union Data
        {
            uint64_t         m_U64[2];
            uint32_t         m_U32[4];
            uint8_t          m_U8[16];
        };

    public:

        static UUID GenerateID();
        static bool IsValidUUIDString( char const* pString );

    public:

        inline UUID() { Memory::MemsetZero( &m_data ); }
        inline UUID( uint64_t v0, uint64_t v1 ) { m_data.m_U64[0] = v0; m_data.m_U64[1] = v1; }
        inline UUID( uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3 ) { m_data.m_U32[0] = v0; m_data.m_U32[1] = v1; m_data.m_U32[2] = v2; m_data.m_U32[3] = v3; }
        inline UUID( String const& str ) : UUID( str.c_str() ) {}
        UUID( char const* pString );

        inline UUIDString ToString() const
        {
            return UUIDString( UUIDString::CtorSprintf(), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", m_data.m_U8[0], m_data.m_U8[1], m_data.m_U8[2], m_data.m_U8[3], m_data.m_U8[4], m_data.m_U8[5], m_data.m_U8[6], m_data.m_U8[7], m_data.m_U8[8], m_data.m_U8[9], m_data.m_U8[10], m_data.m_U8[11], m_data.m_U8[12], m_data.m_U8[13], m_data.m_U8[14], m_data.m_U8[15] );
        }

        inline bool IsValid() const { return m_data.m_U64[0] != 0 && m_data.m_U64[1] != 0; }
        inline void Clear() { Memory::MemsetZero( &m_data ); }

        inline uint8_t GetValueU8( uint32_t idx ) const { EE_ASSERT( idx < 16 ); return m_data.m_U8[idx]; }
        inline uint32_t GetValueU32( uint32_t idx ) const { EE_ASSERT( idx < 4 ); return m_data.m_U32[idx]; }
        inline uint64_t GetValueU64( uint32_t idx ) const { EE_ASSERT( idx < 2 ); return m_data.m_U64[idx]; }

        EE_FORCE_INLINE bool operator==( UUID const& RHS ) const { return m_data.m_U64[0] == RHS.m_data.m_U64[0] && m_data.m_U64[1] == RHS.m_data.m_U64[1]; }
        EE_FORCE_INLINE bool operator!=( UUID const& RHS ) const { return m_data.m_U64[0] != RHS.m_data.m_U64[0] || m_data.m_U64[1] != RHS.m_data.m_U64[1]; }

    private:

        Data m_data;
    };
}

//-------------------------------------------------------------------------

namespace eastl
{
    // Specialization for eastl::hash<UUID>
    // This uses the C# algorithm for generating a hash from a ID
    template <>
    struct hash<EE::UUID>
    {
        eastl_size_t operator()( EE::UUID const& ID ) const
        {
            struct GUIDData
            {
                uint32_t     m_a;
                uint16_t     m_b;
                uint16_t     m_c;
                uint8_t      m_d[8];
            };

            GUIDData const& tmp = reinterpret_cast<GUIDData const&>( ID );
            eastl_size_t hash = tmp.m_a ^ ( tmp.m_b << 16 | ( uint8_t ) tmp.m_c ) ^ ( tmp.m_d[2] << 24 | tmp.m_d[7] );
            return hash;
        }
    };
}