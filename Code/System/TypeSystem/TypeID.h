#pragma once

#include "System/_Module/API.h"
#include "System/Types/StringID.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class EE_SYSTEM_API TypeID
    {
        EE_SERIALIZE( m_ID );

    public:

        TypeID() {}
        TypeID( String const& type ) : m_ID( type ) {}
        TypeID( char const* pType ) : m_ID( pType ) {}
        TypeID( StringID ID ) : m_ID( ID ) {}
        TypeID( uint32_t ID ) : m_ID( ID ) {}

        EE_FORCE_INLINE bool IsValid() const { return m_ID.IsValid(); }

        EE_FORCE_INLINE operator uint32_t() const { return m_ID.GetID(); }
        EE_FORCE_INLINE uint32_t GetID() const { return m_ID.GetID(); }
        EE_FORCE_INLINE StringID ToStringID() const { return m_ID; }
        EE_FORCE_INLINE char const* c_str() const { return m_ID.c_str(); }

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
            return ID.GetID();
        }
    };
}