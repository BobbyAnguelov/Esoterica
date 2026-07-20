#pragma once

#include "Game/_Module/API.h"
#include "Engine/Entity/EntityWorldSystem.h"
#include "Base/Types/IDVector.h"

//-------------------------------------------------------------------------

namespace EE
{
    class HitboxComponent;
    class PlayerTargetingComponent;

    //-------------------------------------------------------------------------

    class EE_GAME_API PlayerTargetingSystem : public EntityWorldSystem
    {
    public:

        EE_ENTITY_WORLD_SYSTEM( PlayerTargetingSystem, RequiresUpdate( UpdateStage::GameSetup ) );

    private:

        virtual void ShutdownSystem() override;
        virtual void RegisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

    private:

        TIDVector<ComponentID, TEntityComponentPair<HitboxComponent>>               m_targets;
        TIDVector<ComponentID, TEntityComponentPair<PlayerTargetingComponent>>      m_trackers;
    };
} 