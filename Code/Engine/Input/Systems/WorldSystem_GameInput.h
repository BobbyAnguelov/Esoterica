#pragma once

#include "Game/_Module/API.h"
#include "Engine/Entity/EntityWorldSystem.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    class GameInputComponent;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API GameInputSystem final : public EntityWorldSystem
    {
        EE_ENTITY_WORLD_SYSTEM( GameInputSystem, RequiresUpdate( UpdateStage::GameSetup ) );

    public:

        inline bool IsGameInputEnabled() const { return m_isGameInputEnabled; }
        inline void SetGameInputEnabled( bool isEnabled ) { m_isGameInputEnabled = isEnabled; }

    private:

        virtual void ShutdownSystem() override;

        virtual void RegisterComponent( Entity* pEntity, EntityComponent* pComponent ) override;
        virtual void UnregisterComponent( Entity* pEntity, EntityComponent* pComponent ) override;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

    private:

        TVector<GameInputComponent*>    m_inputComponents;
        bool                            m_isGameInputEnabled = true;
    };
}