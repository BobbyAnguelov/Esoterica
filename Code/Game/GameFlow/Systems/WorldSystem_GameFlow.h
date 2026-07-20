#pragma once

#include "Game/_Module/API.h"
#include "Game/NPC/Components/Component_NPC.h"
#include "Engine/Entity/EntityWorldSystem.h"
#include "Base/Types/IDVector.h"

//-------------------------------------------------------------------------

namespace EE
{
    class CameraComponent;
    class SpawnPointComponent;
    class PlayerComponent;
    class NPCComponent;

    //-------------------------------------------------------------------------

    class EE_GAME_API GameFlowManager : public EntityWorldSystem
    {
        EE_ENTITY_WORLD_SYSTEM( GameFlowManager, RequiresUpdate( UpdateStage::GameSetup, UpdatePriority::Highest ) );

    public:

        struct NPC
        {
            NPC() = default;
            NPC( Entity* pEntity, NPCComponent* pComponent ) : m_pEntity( pEntity ), m_pComponent( pComponent ) { EE_ASSERT( m_pEntity != nullptr && m_pComponent != nullptr ); }

            inline ComponentID GetID() const { return m_pComponent->GetID(); }
            inline bool operator==( NPCComponent const* pComponent ) const { return m_pComponent == pComponent; }

        public:

            Entity*             m_pEntity = nullptr;
            NPCComponent*       m_pComponent = nullptr;
            Seconds             m_timeSinceDeath = 0.0;
        };

    public:

        // Characters
        //-------------------------------------------------------------------------

        inline int32_t GetNumCharacters() const { return (int32_t) m_characters.size(); }
        TIDVector<ComponentID, CharacterComponent*> const& GetCharacters() const { return m_characters; }

        // Player
        //-------------------------------------------------------------------------

        inline bool HasPlayer() const { return !m_players.empty(); }
        EntityID GetPlayerEntityID() const;

        // NPCs
        //-------------------------------------------------------------------------

        inline int32_t GetNumNPCs() const { return (int32_t) m_NPCs.size(); }

        // Hack
        void HackTrySpawnNPCs( EntityWorldUpdateContext const& ctx, int32_t numToSpawn );

    private:

        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

        SpawnPointComponent* GetEnabledSpawnPoint( TIDVector<ComponentID, SpawnPointComponent*> const& spawnPoints, bool pickRandom = false );

        // Player
        //-------------------------------------------------------------------------

        bool TrySpawnPlayer( EntityWorldUpdateContext const& ctx );

        // NPCs
        //-------------------------------------------------------------------------

        bool TrySpawnNPCs( EntityWorldUpdateContext const& ctx );

        // Hack
        void HackTryRespawnDeadNPCs( EntityWorldUpdateContext const& ctx );

    private:

        TIDVector<ComponentID, CharacterComponent*>     m_characters;

        //-------------------------------------------------------------------------

        TIDVector<ComponentID, PlayerComponent*>        m_players;
        TIDVector<ComponentID, SpawnPointComponent*>    m_playerSpawnPoints;
        bool                                            m_hasSpawnedPlayer = false;

        //-------------------------------------------------------------------------

        TIDVector<ComponentID, SpawnPointComponent*>    m_NPCSpawnPoints;
        TIDVector<ComponentID, NPC>                     m_NPCs;
        bool                                            m_hasSpawnedNPC = false;
    };
} 