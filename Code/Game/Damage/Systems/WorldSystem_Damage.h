#pragma once

#include "Game/_Module/API.h"
#include "Engine/Entity/EntityWorldSystem.h"
#include "Base/Types/IDVector.h"

//-------------------------------------------------------------------------

namespace EE
{
    class DamageComponent;
    class HitboxComponent;

    //-------------------------------------------------------------------------

    class EE_GAME_API DamageSystem final : public EntityWorldSystem
    {
        EE_ENTITY_WORLD_SYSTEM( DamageSystem, RequiresUpdate( UpdateStage::GamePostPhysics ) );

    private:

        virtual void ShutdownSystem() override;

        virtual void RegisterComponent( Entity* pEntity, EntityComponent* pComponent ) override;
        virtual void UnregisterComponent( Entity* pEntity, EntityComponent* pComponent ) override;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

    private:

        TIDVector<ComponentID, TEntityComponentPair<DamageComponent>>               m_dealers;
        TIDVector<ComponentID, TEntityComponentPair<HitboxComponent>>               m_receivers;
    };
}