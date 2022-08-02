#include "WorldSystem_PlayerInteractions.h"
#include "Game/Player/Components/Component_PlayerInteractible.h"
#include "Game/Player/Components/Component_MainPlayer.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/Entity.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    void PlayerInteractionSystem::InitializeSystem( SystemRegistry const& systemRegistry )
    {
        
    }

    void PlayerInteractionSystem::ShutdownSystem()
    {
        EE_ASSERT( m_interactibles.empty() );
    }

    //-------------------------------------------------------------------------

    void PlayerInteractionSystem::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pPlayerComponent = TryCast<MainPlayerComponent>( pComponent ) )
        {
            RegisteredPlayer player = { pEntity, pPlayerComponent };
            m_players.emplace_back( player );
        }

        if ( auto pInteractibleComponent = TryCast<PlayerInteractibleComponent>( pComponent ) )
        {
            m_interactibles.emplace_back( pInteractibleComponent );
        }
    }

    void PlayerInteractionSystem::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pPlayerComponent = TryCast<MainPlayerComponent>( pComponent ) )
        {
            RegisteredPlayer player = { pEntity, pPlayerComponent };
            m_players.erase_first( player );
        }

        // TODO: handle removing a component that is in use!!!
        if ( auto pInteractibleComponent = TryCast<PlayerInteractibleComponent>( pComponent ) )
        {
            m_interactibles.erase_first( pInteractibleComponent );
        }
    }

    //-------------------------------------------------------------------------

    void PlayerInteractionSystem::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        if ( !ctx.IsGameWorld() )
        {
            return;
        }

        // HACK!!! just to test the external graphs feature!!
        for ( auto const& player : m_players )
        {
            Vector const playerPosition = player.m_pEntity->GetWorldTransform().GetTranslation();
            player.m_pPlayerComp->m_pAvailableInteraction = nullptr;

            for ( auto pInteractible : m_interactibles )
            {
                Vector const interactiblePosition = pInteractible->GetPosition();

                if ( interactiblePosition.GetDistance2( playerPosition ) < 2.0f )
                {
                    player.m_pPlayerComp->m_pAvailableInteraction = pInteractible->GetGraph();
                }
            }
        }
    }
}