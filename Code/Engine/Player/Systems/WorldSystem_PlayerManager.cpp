#include "WorldSystem_PlayerManager.h"
#include "Engine/Player/Components/Component_PlayerSpawn.h"
#include "Engine/Player/Components/Component_Player.h"
#include "Engine/Camera/Components/Component_Camera.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityMap.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Threading/TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    void PlayerManager::ShutdownSystem()
    {
        EE_ASSERT( !m_player.m_entityID.IsValid() );
        EE_ASSERT( m_spawnPoints.empty() );
        EE_ASSERT( m_cameras.empty() );
    }

    void PlayerManager::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pSpawnComponent = TryCast<Player::PlayerSpawnComponent>( pComponent ) )
        {
            m_spawnPoints.emplace_back( pSpawnComponent );
        }
        else if ( auto pPlayerComponent = TryCast<Player::PlayerComponent>( pComponent ) )
        {
            if ( m_player.m_entityID.IsValid() )
            {
                EE_LOG_ERROR( "Player", nullptr, "Multiple players spawned! this is not supported" );
            }
            else
            {
                m_player.m_entityID = pEntity->GetID();
                EE_ASSERT( m_player.m_pPlayerComponent == nullptr && m_player.m_pCameraComponent == nullptr );
                m_player.m_pPlayerComponent = pPlayerComponent;
                m_registeredPlayerStateChanged = true;

                // Try to find the camera for this player
                for ( auto pCamera : m_cameras )
                {
                    if ( pCamera->GetEntityID() == m_player.m_entityID )
                    {
                        m_player.m_pCameraComponent = pCamera;
                    }
                }
            }
        }
        else if ( auto pCameraComponent = TryCast<CameraComponent>( pComponent ) )
        {
            m_cameras.emplace_back( pCameraComponent );

            // If this is the camera for the player set it in the player struct
            if ( m_player.m_entityID == pEntity->GetID() )
            {
                EE_ASSERT( m_player.m_pCameraComponent == nullptr );
                m_player.m_pCameraComponent = pCameraComponent;
                m_registeredPlayerStateChanged = true;
            }
        }
    }

    void PlayerManager::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pSpawnComponent = TryCast<Player::PlayerSpawnComponent>( pComponent ) )
        {
            m_spawnPoints.erase_first_unsorted( pSpawnComponent );
        }
        else if ( auto pPlayerComponent = TryCast<Player::PlayerComponent>( pComponent ) )
        {
            // Remove the player
            if ( m_player.m_entityID == pEntity->GetID() )
            {
                EE_ASSERT( m_player.m_pPlayerComponent == pPlayerComponent );
                m_player.m_entityID.Clear();
                m_player.m_pPlayerComponent = nullptr;
                m_player.m_pCameraComponent = nullptr;
                m_registeredPlayerStateChanged = true;
            }
        }
        else if ( auto pCameraComponent = TryCast<CameraComponent>( pComponent ) )
        {
            // Remove camera from the player state
            if ( m_player.IsValid() && m_player.m_entityID == pEntity->GetID() )
            {
                EE_ASSERT( m_player.m_pCameraComponent == pCameraComponent );
                m_player.m_pCameraComponent = nullptr;
                m_registeredPlayerStateChanged = true;
            }

            m_cameras.erase_first_unsorted( pCameraComponent );
        }
    }

    //-------------------------------------------------------------------------

    bool PlayerManager::TrySpawnPlayer( EntityWorldUpdateContext const& ctx )
    {
        if ( m_spawnPoints.empty() )
        {
            return false;
        }

        auto pTypeRegistry = ctx.GetSystem<TypeSystem::TypeRegistry>();
        auto pTaskSystem = ctx.GetSystem<TaskSystem>();
        auto pPersistentMap = ctx.GetPersistentMap();

        //-------------------------------------------------------------------------

        // For now we only support a single spawn point
        pPersistentMap->AddEntityCollection( pTaskSystem, *pTypeRegistry, *m_spawnPoints[0]->GetEntityCollectionDesc(), m_spawnPoints[0]->GetWorldTransform() );
        return true;
    }

    bool PlayerManager::IsPlayerEnabled() const
    {
        if ( m_player.m_pPlayerComponent == nullptr )
        {
            return false;
        }

        return m_player.m_pPlayerComponent->IsPlayerEnabled();
    }

    void PlayerManager::SetPlayerControllerState( bool isEnabled )
    {
        EE_ASSERT( HasPlayer() );
        m_isControllerEnabled = isEnabled;

        if ( m_player.m_pPlayerComponent != nullptr )
        {
            m_player.m_pPlayerComponent->SetPlayerEnabled( m_isControllerEnabled );
        }
    }

    //-------------------------------------------------------------------------

    void PlayerManager::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        if ( ctx.GetUpdateStage() == UpdateStage::FrameStart )
        {
            // Spawn players in game worlds
            if ( ctx.IsGameWorld() && !m_hasSpawnedPlayer )
            {
                m_hasSpawnedPlayer = TrySpawnPlayer( ctx );
            }

            // Handle players state changes
            if ( m_registeredPlayerStateChanged )
            {
                // Only automatically switch to player in game worlds
                if ( ctx.IsGameWorld() && m_player.IsValid() )
                {
                    m_player.m_pPlayerComponent->SetPlayerEnabled( m_isControllerEnabled );
                }

                m_registeredPlayerStateChanged = false;
            }
        }
        // HACK
        else if ( ctx.GetUpdateStage() == UpdateStage::FrameEnd )
        {

        }
        // HACK
    }
}