#pragma once

#include "Game/_Module/API.h"
#include "Game/Player/StateMachine/PlayerActionStateMachine.h"
#include "Engine/Entity/EntitySystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    class PlayerCameraComponent;
    namespace Combat { class PlayerTargetingComponent; }
    namespace Render { class CharacterMeshComponent; }
    namespace Animation { class GraphComponent; }
    namespace Input { class GameInputComponent; }
}

//-------------------------------------------------------------------------

namespace EE
{
    class EE_GAME_API PlayerSystem final : public EntitySystem
    {
        friend class PlayerDebugView;
        friend class PlayerHudDebugView;

        EE_ENTITY_SYSTEM( PlayerSystem, RequiresUpdate( UpdateStage::GamePrePhysics, UpdatePriority::Highest ), RequiresUpdate( UpdateStage::GamePostPhysics ), RequiresUpdate( UpdateStage::Paused ) );

    private:

        virtual void CreateAdditionalRequiredComponents( Entity* pEntity ) const override;
        virtual void PostComponentRegister() override;
        virtual void Shutdown() override;

        virtual void RegisterComponent( EntityComponent* pComponent ) override;
        virtual void UnregisterComponent( EntityComponent* pComponent ) override;
        virtual void Update( EntityWorldUpdateContext const& ctx ) override;

        void UpdateGamePrePhysics( EntityWorldUpdateContext const& ctx );
        void UpdateGamePostPhysics( EntityWorldUpdateContext const& ctx );

    private:

        PlayerActionContext                                     m_actionContext;
        PlayerActionStateMachine                                m_actionStateMachine = PlayerActionStateMachine( m_actionContext );

        Animation::GraphComponent*                              m_pAnimGraphComponent = nullptr;
        Render::CharacterMeshComponent*                         m_pCharacterMeshComponent = nullptr;
        Input::GameInputComponent*                              m_pGameInputComponent = nullptr;
        PlayerCameraComponent*                                  m_pCameraComponent = nullptr;
    };
}