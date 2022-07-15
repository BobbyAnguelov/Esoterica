#pragma once
#include "System/Types/UUID.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct PointerID
    {
        EE_SERIALIZE( m_ID );

        PointerID() = default;
        PointerID( void const* pV ) : m_ID( reinterpret_cast<uint64_t>( pV ) ) {}
        explicit PointerID( uint64_t v ) : m_ID( v ) {}

        inline bool IsValid() const { return m_ID != 0; }
        inline void Clear() { m_ID = 0; }
        inline bool operator==( PointerID const& rhs ) const { return m_ID == rhs.m_ID; }
        inline bool operator!=( PointerID const& rhs ) const { return m_ID != rhs.m_ID; }

        inline uint64_t ToUint64() const { return m_ID; }

        uint64_t m_ID = 0;
    };

    using ComponentID = PointerID;
    using EntityID = PointerID;
    using EntityMapID = UUID;
    using EntityWorldID = UUID;
}

//-------------------------------------------------------------------------

namespace eastl
{
    template <>
    struct hash<EE::PointerID>
    {
        EE_FORCE_INLINE eastl_size_t operator()( EE::PointerID const& t ) const { return t.m_ID; }
    };
}