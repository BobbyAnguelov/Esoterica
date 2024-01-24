#pragma once
#include "Base/Types/UUID.h"

//-------------------------------------------------------------------------

namespace EE
{
    // Serializable Entity Map ID
    //-------------------------------------------------------------------------

    using EntityMapID = UUID;

    //-------------------------------------------------------------------------

    struct EntityWorldID
    {
        static EntityWorldID Generate();

    public:

        EntityWorldID() = default;
        explicit EntityWorldID( uint64_t v ) : m_value( v ) { EE_ASSERT( v != 0 ); }

        EE_FORCE_INLINE bool IsValid() const { return m_value != 0; }
        EE_FORCE_INLINE void Clear() { m_value = 0; }
        EE_FORCE_INLINE bool operator==( EntityWorldID const& rhs ) const { return m_value == rhs.m_value; }
        EE_FORCE_INLINE bool operator!=( EntityWorldID const& rhs ) const { return m_value != rhs.m_value; }

    public:

        uint64_t m_value = 0;
    };

    //-------------------------------------------------------------------------

    struct EntityID
    {
        static EntityID Generate();

    public:

        EntityID() = default;
        explicit EntityID( uint64_t v ) : m_value( v ) { EE_ASSERT( v != 0 ); }

        EE_FORCE_INLINE bool IsValid() const { return m_value != 0; }
        EE_FORCE_INLINE void Clear() { m_value = 0; }
        EE_FORCE_INLINE bool operator==( EntityID const& rhs ) const { return m_value == rhs.m_value; }
        EE_FORCE_INLINE bool operator!=( EntityID const& rhs ) const { return m_value != rhs.m_value; }

    public:

        uint64_t m_value = 0;
    };

    //-------------------------------------------------------------------------

    struct ComponentID
    {
        static ComponentID Generate();

    public:

        ComponentID() = default;
        explicit ComponentID( uint64_t v ) : m_value( v ) { EE_ASSERT( v != 0 ); }

        EE_FORCE_INLINE bool IsValid() const { return m_value != 0; }
        EE_FORCE_INLINE void Clear() { m_value = 0; }
        EE_FORCE_INLINE bool operator==( ComponentID const& rhs ) const { return m_value == rhs.m_value; }
        EE_FORCE_INLINE bool operator!=( ComponentID const& rhs ) const { return m_value != rhs.m_value; }

    public:

        uint64_t m_value = 0;
    };
}

//-------------------------------------------------------------------------

namespace eastl
{
    template <>
    struct hash<EE::EntityWorldID>
    {
        EE_FORCE_INLINE eastl_size_t operator()( EE::EntityWorldID const& t ) const { return t.m_value; }
    };

    template <>
    struct hash<EE::EntityID>
    {
        EE_FORCE_INLINE eastl_size_t operator()( EE::EntityID const& t ) const { return t.m_value; }
    };

    template <>
    struct hash<EE::ComponentID>
    {
        EE_FORCE_INLINE eastl_size_t operator()( EE::ComponentID const& t ) const { return t.m_value; }
    };
}