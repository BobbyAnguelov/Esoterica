#pragma once

#include "Game/Base/Components/Component_Character.h"

//-------------------------------------------------------------------------
// Player Character Component
//-------------------------------------------------------------------------
// This component identifies all Player entities and provides common data needed for their controller systems

namespace EE
{
    class EE_GAME_API PlayerComponent final : public CharacterComponent
    {
        EE_ENTITY_COMPONENT( PlayerComponent );

    public:

        inline PlayerComponent() = default;
        inline PlayerComponent( StringID name ) : CharacterComponent( name ) {}
    };
}