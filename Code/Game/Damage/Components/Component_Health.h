#pragma once

#include "Game/_Module/API.h"
#include "Engine/Entity/EntityComponent.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_GAME_API HealthComponent : public EntityComponent
    {
        EE_TRANSIENT_ENTITY_COMPONENT( HealthComponent );

    public:

        inline bool IsDead() const { return m_hitpoints <= 0; }

        inline float GetHP() const { return m_hitpoints; }

        inline void SetHP( float health )
        {
            m_hitpoints = Math::Max( health, 0.0f );
        }

    private:

        EE_REFLECT();
        float               m_hitpoints = 100.0f;
    };
}