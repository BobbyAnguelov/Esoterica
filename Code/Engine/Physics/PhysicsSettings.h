#pragma once

#include "Engine/_Module/API.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------
// Simulation Settings
//-------------------------------------------------------------------------

namespace EE::Physics
{
    enum class Mobility : uint8_t
    {
        EE_REFLECT_ENUM

        Static = 0,
        Dynamic,
        Kinematic
    };

    struct EE_ENGINE_API SimulationSettings : public IReflectedType
    {
        EE_REFLECT_TYPE( SimulationSettings );
        EE_SERIALIZE( m_mobility, m_mass );

    public:

        SimulationSettings() = default;

        inline bool IsStatic() const { return m_mobility == Mobility::Static; }
        inline bool IsDynamic() const { return m_mobility == Mobility::Dynamic; }
        inline bool IsKinematic() const { return m_mobility == Mobility::Kinematic; }

    public:

        // The mobility of the object
        EE_REFLECT();
        Mobility                                m_mobility = Mobility::Static;

        // Dynamic Only: The mass of the object
        EE_REFLECT();
        float                                   m_mass = 1.0f;

        // Dynamic Only: Is this object affected by gravity?
        EE_REFLECT();
        bool                                    m_enableGravity = true;
    };
}

//-------------------------------------------------------------------------
// Collision Settings
//-------------------------------------------------------------------------

namespace EE::Physics
{
    // These are the types of objects in the game
    enum class CollisionCategory : uint8_t
    {
        EE_REFLECT_ENUM

        Environment = 0,
        Character,
        Prop,
        Projectile,
        Destructible,
    };

    //-------------------------------------------------------------------------

    // Additional types only used for queries, not used as object types
    enum class QueryChannel : uint8_t
    {
        EE_REFLECT_ENUM

        Navigation = (uint8_t) CollisionCategory::Destructible + 1,
        Visibility,
        Camera,
        IK
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API CollisionSettings : public IReflectedType
    {
        EE_REFLECT_TYPE( CollisionSettings );
        EE_SERIALIZE( m_category, m_collidesWithMask );

    public:

        CollisionSettings() = default;

        inline bool operator==( CollisionSettings const& rhs ) const { return m_category == rhs.m_category && m_collidesWithMask == rhs.m_collidesWithMask; }
        inline bool operator!=( CollisionSettings const& rhs ) const { return m_category != rhs.m_category || m_collidesWithMask != rhs.m_collidesWithMask; }

        inline void Clear()
        {
            m_category = CollisionCategory::Environment;
            m_collidesWithMask = 0xFFFFFFFF;
        }

    public:

        // What type of physics object are we
        EE_REFLECT();
        CollisionCategory                       m_category = CollisionCategory::Environment;

        // What are the things that we collide with (both object categories and query channels)
        EE_REFLECT();
        uint32_t                                m_collidesWithMask = 0xFFFFFFFF;
    };
}