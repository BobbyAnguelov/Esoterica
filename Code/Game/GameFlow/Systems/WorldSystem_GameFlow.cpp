#include "WorldSystem_GameFlow.h"
#include "Game/Base/Components/Component_SpawnPoint.h"
#include "Game/Player/Components/Component_Player.h"
#include "Game/NPC/Components/Component_NPC.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityMap.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/Math/MathRandom.h"
#include "Game/Damage/Components/Component_Health.h"

//-------------------------------------------------------------------------

namespace EE
{
    void GameFlowManager::ShutdownSystem()
    {
        EE_ASSERT( m_players.empty() );
        EE_ASSERT( m_playerSpawnPoints.empty() );

        EE_ASSERT( m_NPCs.empty() );
        EE_ASSERT( m_NPCSpawnPoints.empty() );
    }

    void GameFlowManager::RegisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pSpawnComponent = TryCast<SpawnPointComponent>( pComponent ) )
        {
            if ( pSpawnComponent->IsEntityCollectionSet() )
            {
                if ( pSpawnComponent->IsPlayerSpawn() )
                {
                    m_playerSpawnPoints.Add( pSpawnComponent );
                }
                else if ( pSpawnComponent->IsNPCSpawn() )
                {
                    m_NPCSpawnPoints.Add( pSpawnComponent );
                }
            }
        }
        else if ( auto pPlayerComponent = TryCast<PlayerComponent>( pComponent ) )
        {
            m_players.Add( pPlayerComponent );
            m_characters.Add( pPlayerComponent );
        }
        else if ( auto pNPCComponent = TryCast<NPCComponent>( pComponent ) )
        {
            m_NPCs.Add( { pEntity, pNPCComponent } );
            m_characters.Add( pNPCComponent );
        }
    }

    void GameFlowManager::UnregisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pSpawnComponent = TryCast<SpawnPointComponent>( pComponent ) )
        {
            if ( pSpawnComponent->IsEntityCollectionSet() )
            {
                if ( pSpawnComponent->IsPlayerSpawn() )
                {
                    m_playerSpawnPoints.Remove( pSpawnComponent );
                }
                else if ( pSpawnComponent->IsNPCSpawn() )
                {
                    m_NPCSpawnPoints.Remove( pSpawnComponent );
                }
            }
        }
        else if ( auto pPlayerComponent = TryCast<PlayerComponent>( pComponent ) )
        {
            m_players.Remove( pPlayerComponent->GetID() );
            m_characters.Remove( pPlayerComponent );
        }
        else if ( auto pNPCComponent = TryCast<NPCComponent>( pComponent ) )
        {
            m_NPCs.Remove( pNPCComponent->GetID() );
            m_characters.Remove( pNPCComponent );
        }
    }

    //-------------------------------------------------------------------------

    void GameFlowManager::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        if ( !ctx.IsGameWorld() )
        {
            return;
        }

        if ( ctx.GetUpdateStage() != UpdateStage::GameSetup )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( !m_hasSpawnedPlayer )
        {
            m_hasSpawnedPlayer = TrySpawnPlayer( ctx );
        }

        if ( !m_hasSpawnedNPC )
        {
            m_hasSpawnedNPC = TrySpawnNPCs( ctx );
        }

        // HACK : RESPAWN DEAD NPCs
        //-------------------------------------------------------------------------

        HackTryRespawnDeadNPCs( ctx );
    }

    SpawnPointComponent* GameFlowManager::GetEnabledSpawnPoint( TIDVector<ComponentID, SpawnPointComponent*> const& spawnPoints, bool pickRandom )
    {
        if ( spawnPoints.empty() )
        {
            return nullptr;
        }

        //-------------------------------------------------------------------------

        TInlineVector< SpawnPointComponent*, 20> enabledPoints;
        for ( SpawnPointComponent* pSpawnPoint : spawnPoints )
        {
            if ( pSpawnPoint->IsEnabled() )
            {
                enabledPoints.emplace_back( pSpawnPoint );
            }
        }

        if ( enabledPoints.empty() )
        {
            return nullptr;
        }

        if ( pickRandom )
        {
            int32_t const spawnPointIdx = Math::GetRandomInt( 0, (int32_t) enabledPoints.size() - 1 );
            return enabledPoints[spawnPointIdx];
        }

        return enabledPoints.front();
    }

    //-------------------------------------------------------------------------
    // Players
    //-------------------------------------------------------------------------

    bool GameFlowManager::TrySpawnPlayer( EntityWorldUpdateContext const& ctx )
    {
        auto pSpawnPoint = GetEnabledSpawnPoint( m_playerSpawnPoints );
        if ( pSpawnPoint == nullptr )
        {
            return false;
        }

        auto pTypeRegistry = ctx.GetSystem<TypeSystem::TypeRegistry>();
        auto pTaskSystem = ctx.GetSystem<TaskSystem>();
        auto pPersistentMap = ctx.GetPersistentMap();

        //-------------------------------------------------------------------------

        // For now we only support a single spawn point
        EE_ASSERT( pSpawnPoint->IsEntityCollectionSet() );
        pPersistentMap->AddEntityCollection( pTaskSystem, *pTypeRegistry, *pSpawnPoint->GetEntityCollectionDesc(), pSpawnPoint->GetWorldTransform() );
        return true;
    }

    EntityID GameFlowManager::GetPlayerEntityID() const
    {
        return m_players[0]->GetEntityID();
    }

    //-------------------------------------------------------------------------
    // NPCs
    //-------------------------------------------------------------------------

    bool GameFlowManager::TrySpawnNPCs( EntityWorldUpdateContext const& ctx )
    {
        auto pTypeRegistry = ctx.GetSystem<TypeSystem::TypeRegistry>();
        auto pTaskSystem = ctx.GetSystem<TaskSystem>();
        auto pPersistentMap = ctx.GetPersistentMap();

        //-------------------------------------------------------------------------

        bool spawnedNPC = false;
        for ( auto pSpawnPoint : m_NPCSpawnPoints )
        {
            if ( pSpawnPoint->IsEnabled() )
            {
                pPersistentMap->AddEntityCollection( pTaskSystem, *pTypeRegistry, *pSpawnPoint->GetEntityCollectionDesc(), pSpawnPoint->GetWorldTransform() );
                spawnedNPC = true;
            }
        }

        return spawnedNPC;
    }

    void GameFlowManager::HackTrySpawnNPCs( EntityWorldUpdateContext const& ctx, int32_t numAIToSpawn )
    {
        auto pTypeRegistry = ctx.GetSystem<TypeSystem::TypeRegistry>();
        auto pTaskSystem = ctx.GetSystem<TaskSystem>();
        auto pPersistentMap = ctx.GetPersistentMap();

        for ( int32_t i = 0; i < numAIToSpawn; i++ )
        {
            for ( auto pSpawnPoint : m_NPCSpawnPoints )
            {
                if ( !pSpawnPoint->IsEnabled() )
                {
                    continue;
                }

                Vector const offset( Math::GetRandomFloat( -5, 5 ), Math::GetRandomFloat( -5, 5 ), 0 );
                Transform spawnTransform = pSpawnPoint->GetWorldTransform();
                spawnTransform.AddTranslation( offset );
                pPersistentMap->AddEntityCollection( pTaskSystem, *pTypeRegistry, *pSpawnPoint->GetEntityCollectionDesc(), spawnTransform );
            }
        }
    }

    void GameFlowManager::HackTryRespawnDeadNPCs( EntityWorldUpdateContext const& ctx )
    {
        auto pTypeRegistry = ctx.GetSystem<TypeSystem::TypeRegistry>();
        auto pTaskSystem = ctx.GetSystem<TaskSystem>();
        auto pPersistentMap = ctx.GetPersistentMap();

        //-------------------------------------------------------------------------

        for ( auto& NPC : m_NPCs )
        {
            if ( auto pHealthComponent = NPC.m_pEntity->TryGetComponent<HealthComponent>() )
            {
                if ( pHealthComponent->IsDead() )
                {
                    NPC.m_timeSinceDeath += ctx.GetDeltaTime();
                }

                // Destroy entity and spawn a new one
                if ( NPC.m_timeSinceDeath > 5.0f )
                {
                    pPersistentMap->DestroyEntity( NPC.m_pEntity->GetID() );

                    auto pSpawnPoint = GetEnabledSpawnPoint( m_NPCSpawnPoints, true );
                    if ( pSpawnPoint != nullptr )
                    {
                        Vector const offset( Math::GetRandomFloat( -1, 1 ), Math::GetRandomFloat( -1, 1 ), 0 );
                        Transform spawnTransform = pSpawnPoint->GetWorldTransform();
                        spawnTransform.AddTranslation( offset );
                        pPersistentMap->AddEntityCollection( pTaskSystem, *pTypeRegistry, *pSpawnPoint->GetEntityCollectionDesc(), spawnTransform );
                    }
                }
            }
        }
    }
}