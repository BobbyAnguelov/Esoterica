#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityComponent.h"

//-------------------------------------------------------------------------
// Player Component
//-------------------------------------------------------------------------
// This component identifies all player entities and provides common data needed for player controller systems

namespace EE::Player
{
    class EE_ENGINE_API PlayerComponent : public EntityComponent
    {
        EE_SINGLETON_ENTITY_COMPONENT( PlayerComponent );

    public:

        inline PlayerComponent() = default;
        inline PlayerComponent( StringID name ) : EntityComponent( name ) {}

        inline bool IsPlayerEnabled() const { return m_isEnabled; }
        inline void SetPlayerEnabled( bool isEnabled ) { m_isEnabled = isEnabled; }

    private:

        bool m_isEnabled = true;
    };
}