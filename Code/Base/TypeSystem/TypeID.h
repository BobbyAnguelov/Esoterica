#pragma once

#include "Base/_Module/API.h"
#include "Base/Types/StringID.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class EE_BASE_API TypeID
    {
        EE_SERIALIZE( m_ID );

    public:

        TypeID() {}
        TypeID( String const& type ) : m_ID( type ) {}
        TypeID( char const* pType ) : m_ID( pType ) {}
        TypeID( StringID ID ) : m_ID( ID ) {}
        TypeID( uint32_t ID ) : m_ID( ID ) {}

        EE_FORCE_INLINE bool IsValid() const { return m_ID.IsValid(); }

        EE_FORCE_INLINE explicit operator uint32_t() const { return m_ID.ToUint(); }
        EE_FORCE_INLINE uint32_t ToUint() const { return m_ID.ToUint(); }
        EE_FORCE_INLINE StringID ToStringID() const { return m_ID; }
        EE_FORCE_INLINE char const* c_str() const { return m_ID.c_str(); }

        EE_FORCE_INLINE bool operator==( TypeID const& rhs ) const { return m_ID == rhs.m_ID; }
        EE_FORCE_INLINE bool operator!=( TypeID const& rhs ) const { return m_ID != rhs.m_ID; }

        EE_FORCE_INLINE bool operator==( StringID const& rhs ) const { return m_ID == rhs; }
        EE_FORCE_INLINE bool operator!=( StringID const& rhs ) const { return m_ID != rhs; }

    private:

        StringID m_ID;
    };
}

//-------------------------------------------------------------------------

namespace eastl
{
    template <typename T> struct hash;

    template <>
    struct hash<EE::TypeSystem::TypeID>
    {
        size_t operator()( EE::TypeSystem::TypeID const& ID ) const 
        {
            return ID.ToUint();
        }
    };
}