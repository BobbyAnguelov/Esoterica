#pragma once
#include "Engine/_Module/API.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Resource/IResource.h"
#include "Base/Math/Transform.h"

//-------------------------------------------------------------------------

namespace EE
{
    enum class HitboxDamageSeverity : uint8_t
    {
        EE_REFLECT_ENUM

        Critical = 0,
        High,
        Medium,
        Low,
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API HitboxShape : public IReflectedType
    {
        EE_REFLECT_TYPE( HitboxShape );
        EE_SERIALIZE( m_ID, m_severity, m_type, m_socketID, m_offset, m_radius, m_halfHeight, m_boxExtents, m_tags );

    public:

        enum class Type : uint8_t
        {
            EE_REFLECT_ENUM

            Capsule,
            Sphere,
            Box,
        };

    public:

        constexpr static float const s_minimumWidthOrRadius = 0.01f;

        EE_FORCE_INLINE bool HasMatchingTag( StringID tag ) const { return VectorContains( m_tags, tag ); }

        #if EE_DEVELOPMENT_TOOLS
        static Color GetSeverityColor( HitboxDamageSeverity type );
        #endif

    public:

        EE_REFLECT( Hidden );
        UUID                                            m_ID = UUID::GenerateID();

        EE_REFLECT();
        HitboxDamageSeverity                            m_severity = HitboxDamageSeverity::Low;

        EE_REFLECT();
        Type                                            m_type = Type::Capsule;

        EE_REFLECT();
        StringID                                        m_socketID;

        EE_REFLECT();
        Transform                                       m_offset;

        EE_REFLECT( Min = "0.001" );
        float                                           m_radius = 0.05f;

        EE_REFLECT( Min = "0.001" );
        float                                           m_halfHeight = 0.1f;

        EE_REFLECT();
        Float3                                          m_boxExtents = Float3::One;

        EE_REFLECT();
        TVector<StringID>                               m_tags;
    };

    //-------------------------------------------------------------------------
    // Hitbox Definition
    //-------------------------------------------------------------------------
    // This defines a set of hitbox shapes for damage detection

    struct EE_ENGINE_API HitboxDefinition : public Resource::IResource
    {
        EE_RESOURCE( "hitbox", "Hitbox", Colors::CadetBlue, 6, false );
        EE_SERIALIZE( m_shapes, m_instanceShapesStartOffsets, m_instanceRequiredMemory, m_instanceRequiredAlignment );

        friend class HitboxCompiler;
        friend class HitboxLoader;
        friend class Hitbox;

    public:

        constexpr static uint32_t const s_maxNumHitboxes = 64;

    public:

        virtual bool IsValid() const override { return true; }

    public:

        TVector<HitboxShape>                            m_shapes;

    private:

        TVector<uint32_t>                               m_instanceShapesStartOffsets;
        uint32_t                                        m_instanceRequiredMemory = 0;
        uint32_t                                        m_instanceRequiredAlignment = 0;
    };
}