#pragma once

#include "Game/_Module/API.h"
#include "Game/Player/StateMachine/PlayerActionStateMachine.h"
#include "Engine/Entity/EntitySystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    class OrbitCameraComponent;
    namespace Render { class CharacterMeshComponent; }
    namespace Animation { class GraphComponent; }
}

//-------------------------------------------------------------------------

namespace EE::Player
{
    class EE_GAME_API PlayerController final : public EntitySystem
    {
        friend class PlayerDebugView;

        EE_ENTITY_SYSTEM( PlayerController, RequiresUpdate( UpdateStage::PrePhysics, UpdatePriority::Highest ), RequiresUpdate( UpdateStage::PostPhysics, UpdatePriority::Highest ), RequiresUpdate( UpdateStage::Paused ) );

    private:

        virtual void PostComponentRegister() override;
        virtual void Shutdown() override;

        virtual void RegisterComponent( EntityComponent* pComponent ) override;
        virtual void UnregisterComponent( EntityComponent* pComponent ) override;
        virtual void Update( EntityWorldUpdateContext const& ctx ) override;

    private:

        ActionContext                                           m_actionContext;
        ActionStateMachine                                      m_actionStateMachine = ActionStateMachine( m_actionContext );

        Animation::GraphComponent*                              m_pAnimGraphComponent = nullptr;
        Render::CharacterMeshComponent*                         m_pCharacterMeshComponent = nullptr;
        OrbitCameraComponent*                                   m_pCameraComponent = nullptr;
    };
}