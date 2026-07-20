#pragma once

#include "Game/_Module/API.h"
#include "Game/NPC/Behavior/BehaviorSelector.h"
#include "Engine/Entity/EntitySystem.h"

//-------------------------------------------------------------------------

namespace EE::Animation { class GraphComponent; }
namespace EE::Render { class CharacterMeshComponent; }

//-------------------------------------------------------------------------

namespace EE
{
    class EE_GAME_API NPCSystem final : public EntitySystem
    {
        friend class NPCDebugView;

        EE_ENTITY_SYSTEM( NPCSystem, RequiresUpdate( UpdateStage::PrePhysics ), RequiresUpdate( UpdateStage::PostPhysics ) );

    private:

        virtual void PostComponentRegister() override;
        virtual void Shutdown() override;

        virtual void CreateAdditionalRequiredComponents( Entity* pEntity ) const override;
        virtual void RegisterComponent( EntityComponent* pComponent ) override;
        virtual void UnregisterComponent( EntityComponent* pComponent ) override;
        virtual void Update( EntityWorldUpdateContext const& ctx ) override;

    private:

        BehaviorContext                                         m_behaviorContext;
        BehaviorSelector                                        m_behaviorSelector = BehaviorSelector( m_behaviorContext );

        Animation::GraphComponent*                              m_pAnimGraphComponent = nullptr;
        Render::CharacterMeshComponent*                         m_pCharacterMeshComponent = nullptr;
    };
}