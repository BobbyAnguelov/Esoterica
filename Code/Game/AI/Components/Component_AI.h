#pragma once

#include "Game/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"

//-------------------------------------------------------------------------
// AI Component
//-------------------------------------------------------------------------
// This component identifies all AI entities

namespace EE::AI
{
    class EE_GAME_API AIComponent : public EntityComponent
    {
        EE_REGISTER_SINGLETON_ENTITY_COMPONENT( AIComponent );

    public:

        inline AIComponent() = default;
        inline AIComponent( StringID name ) : EntityComponent( name ) {}
    };
}