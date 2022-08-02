#pragma once

#include "Engine/Entity/EntityWorldSystem.h"
#include "System/Types/IDVector.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    class AISpawnComponent;
    class AIComponent;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API AIManager : public IEntityWorldSystem
    {
        friend class AIDebugView;

    public:

        EE_REGISTER_ENTITY_WORLD_SYSTEM( AIManager, RequiresUpdate( UpdateStage::PrePhysics ) );

    private:

        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

        bool TrySpawnAI( EntityWorldUpdateContext const& ctx );

    private:

        TVector<AISpawnComponent*>          m_spawnPoints;
        TVector<AIComponent*>               m_AIs;
        bool                                m_hasSpawnedAI = false;
    };
} 