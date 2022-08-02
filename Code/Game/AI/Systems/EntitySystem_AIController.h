#pragma once

#include "Game/_Module/API.h"
#include "Game/AI/Behaviors/AIBehaviorSelector.h"
#include "Engine/Entity/EntitySystem.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AnimationGraphComponent;
}

namespace EE::Render
{
    class CharacterMeshComponent;
}

//-------------------------------------------------------------------------

namespace EE::AI
{
    class EE_GAME_API AIController final : public EntitySystem
    {
        friend class AIDebugView;

        EE_REGISTER_ENTITY_SYSTEM( AIController, RequiresUpdate( UpdateStage::PrePhysics ), RequiresUpdate( UpdateStage::PostPhysics ) );

    private:

        virtual void Activate() override;
        virtual void Shutdown() override;

        virtual void RegisterComponent( EntityComponent* pComponent ) override;
        virtual void UnregisterComponent( EntityComponent* pComponent ) override;
        virtual void Update( EntityWorldUpdateContext const& ctx ) override;

    private:

        BehaviorContext                                         m_behaviorContext;
        BehaviorSelector                                        m_behaviorSelector = BehaviorSelector( m_behaviorContext );

        Animation::AnimationGraphComponent*                     m_pAnimGraphComponent = nullptr;
        Render::CharacterMeshComponent*                         m_pCharacterMeshComponent = nullptr;
    };
}