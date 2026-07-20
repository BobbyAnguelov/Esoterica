#pragma once

#include "Game/Base/Components/Component_Character.h"

//-------------------------------------------------------------------------
// Non-Player Character Component
//-------------------------------------------------------------------------
// This component identifies all NPC entities and provides common data needed for their controller systems

namespace EE
{
    class EE_GAME_API NPCComponent final : public CharacterComponent
    {
        EE_ENTITY_COMPONENT( NPCComponent );

    public:

        inline NPCComponent() = default;
        inline NPCComponent( StringID name ) : CharacterComponent( name ) {}
    };
}