#pragma once

#include "Game/_Module/API.h"
#include "Engine/Entity/EntityWorldSystem.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class MainPlayerComponent;
    class PlayerInteractibleComponent;

    //-------------------------------------------------------------------------

    class EE_GAME_API PlayerInteractionSystem final : public EntityWorldSystem
    {
        EE_ENTITY_WORLD_SYSTEM( PlayerInteractionSystem, RequiresUpdate( UpdateStage::PrePhysics ) );

        struct RegisteredPlayer
        {
            bool operator==( RegisteredPlayer const& rhs ) const
            {
                return m_pEntity == rhs.m_pEntity && m_pPlayerComp == rhs.m_pPlayerComp;
            }

            Entity const*           m_pEntity;
            MainPlayerComponent*    m_pPlayerComp;
        };

    private:

        virtual void InitializeSystem( SystemRegistry const& systemRegistry ) override;
        virtual void ShutdownSystem() override;

        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

    private:

        TVector<RegisteredPlayer>                   m_players;
        TVector<PlayerInteractibleComponent*>       m_interactibles;
    };
}