#include "WorldSystem_AIManager.h"
#include "Engine/AI/Components/Component_AI.h"
#include "Engine/AI/Components/Component_AISpawn.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityMap.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Threading/TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    void AIManager::ShutdownSystem()
    {
        EE_ASSERT( m_spawnPoints.empty() );
    }

    void AIManager::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pSpawnComponent = TryCast<AISpawnComponent>( pComponent ) )
        {
            m_spawnPoints.emplace_back( pSpawnComponent );
        }

        if ( auto pAIComponent = TryCast<AIComponent>( pComponent ) )
        {
            m_AIs.emplace_back( pAIComponent );
        }
    }

    void AIManager::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pSpawnComponent = TryCast<AISpawnComponent>( pComponent ) )
        {
            m_spawnPoints.erase_first_unsorted( pSpawnComponent );
        }

        if ( auto pAIComponent = TryCast<AIComponent>( pComponent ) )
        {
            m_AIs.erase_first( pAIComponent );
        }
    }

    //-------------------------------------------------------------------------

    void AIManager::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        if ( ctx.IsGameWorld() && !m_hasSpawnedAI )
        {
            m_hasSpawnedAI = TrySpawnAI( ctx );
        }
    }

    bool AIManager::TrySpawnAI( EntityWorldUpdateContext const& ctx )
    {
        if ( m_spawnPoints.empty() )
        {
            return false;
        }

        auto pTypeRegistry = ctx.GetSystem<TypeSystem::TypeRegistry>();
        auto pTaskSystem = ctx.GetSystem<TaskSystem>();
        auto pPersistentMap = ctx.GetPersistentMap();

        //-------------------------------------------------------------------------

        for ( auto pSpawnPoint : m_spawnPoints )
        {
            pPersistentMap->AddEntityCollection( pTaskSystem, *pTypeRegistry, *pSpawnPoint->GetEntityCollectionDesc(), pSpawnPoint->GetWorldTransform() );
        }

        return true;
    }

    void AIManager::HackTrySpawnAI( EntityWorldUpdateContext const& ctx, int32_t numAIToSpawn )
    {
        for ( int32_t i = 0; i < numAIToSpawn; i++ )
        {
            TrySpawnAI( ctx );
        }
    }
}