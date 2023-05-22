#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityWorldSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    class CameraComponent;

    //-------------------------------------------------------------------------

    namespace Player
    {
        class PlayerComponent;
        class PlayerSpawnComponent;
    }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PlayerManager : public IEntityWorldSystem
    {
        struct RegisteredPlayer
        {
            inline bool IsValid() const { return m_pPlayerComponent != nullptr && m_pCameraComponent != nullptr; }

        public:

            EntityID                                m_entityID;
            Player::PlayerComponent*                m_pPlayerComponent = nullptr;
            CameraComponent*                        m_pCameraComponent = nullptr;
        };

    public:

        EE_ENTITY_WORLD_SYSTEM( PlayerManager, RequiresUpdate( UpdateStage::FrameStart, UpdatePriority::Highest ), RequiresUpdate( UpdateStage::FrameEnd, UpdatePriority::Highest ) );

        // Player
        //-------------------------------------------------------------------------

        inline bool HasPlayer() const { return m_player.IsValid(); }
        inline EntityID GetPlayerEntityID() const { return m_player.m_entityID; }
        inline CameraComponent* GetPlayerCamera() const { return m_player.m_pCameraComponent; }
        bool IsPlayerEnabled() const;
        void SetPlayerControllerState( bool isEnabled );

    private:

        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

        bool TrySpawnPlayer( EntityWorldUpdateContext const& ctx );

    private:

        RegisteredPlayer                            m_player;
        TVector<CameraComponent*>                   m_cameras;
        TVector<Player::PlayerSpawnComponent*>      m_spawnPoints;
        bool                                        m_hasSpawnedPlayer = false;
        bool                                        m_registeredPlayerStateChanged = false;
        bool                                        m_isControllerEnabled = true;
    };
} 