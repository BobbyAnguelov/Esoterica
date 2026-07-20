#pragma once
#include "Engine/_Module/API.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    // These are the types of objects in the game
    // A physics shape can only have a single category set
    enum class ObjectCategory : uint8_t
    {
        EE_REFLECT_ENUM

        Environment = 0,
        Character,
        Prop,
        Projectile,
        Destructible,
    };

    //-------------------------------------------------------------------------

    // Additional categories only used for queries!
    // e.g. if you want to query for everything that collides with "navigation" or "visibility"
    enum class QueryCategory : uint8_t
    {
        EE_REFLECT_ENUM

        Navigation = 32,
        Visibility,
        Camera,
        IK
    };

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE uint64_t SetCategoryBit( uint64_t mask, ObjectCategory category )
    {
        return ( mask |= ( 1ULL << (uint8_t) category ) );
    }

    EE_FORCE_INLINE uint64_t SetCategoryBit( uint64_t mask, QueryCategory category )
    {
        return ( mask |= ( 1ULL << (uint8_t) category ) );
    }

    EE_FORCE_INLINE uint64_t ClearCategoryBit( uint64_t mask, ObjectCategory category )
    {
        return ( mask &= ~( 1ULL << (uint64_t) category ) );
    }

    EE_FORCE_INLINE uint64_t ClearCategoryBit( uint64_t mask, QueryCategory category )
    {
        return ( mask &= ~( 1ULL << (uint64_t) category ) );
    }

    EE_FORCE_INLINE bool HasCategoryBit( uint64_t mask, ObjectCategory category )
    {
        return ( mask & ( 1ULL << (uint8_t) category ) ) != 0;
    }

    EE_FORCE_INLINE bool HasCategoryBit( uint64_t mask, QueryCategory category )
    {
        return ( mask & ( 1ULL << (uint8_t) category ) ) != 0;
    }

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API CollisionSettings : public IReflectedType
    {
        EE_REFLECT_TYPE( CollisionSettings );
        EE_SERIALIZE( m_category, m_collisionMask );

    public:

        CollisionSettings() = default;

        inline bool operator==( CollisionSettings const& rhs ) const { return m_category == rhs.m_category && m_collisionMask == rhs.m_collisionMask; }
        inline bool operator!=( CollisionSettings const& rhs ) const { return m_category != rhs.m_category || m_collisionMask != rhs.m_collisionMask; }

        inline void Clear()
        {
            m_category = ObjectCategory::Environment;
            m_collisionMask = UINT64_MAX;
        }

    public:

        // What are we?
        EE_REFLECT();
        ObjectCategory                          m_category = ObjectCategory::Environment;

        // What are the things that we collide with (both object categories and query channels)
        EE_REFLECT();
        uint64_t                                m_collisionMask = UINT64_MAX;
    };
}