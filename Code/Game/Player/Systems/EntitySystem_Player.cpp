#include "EntitySystem_Player.h"
#include "Game/Player/Components/Component_PlayerCamera.h"
#include "Game/Player/Components/Component_Player.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Game/Player/PlayerInputState.h"
#include "Game/Player/PlayerGameState.h"
#include "Game/Damage/Components/Component_Damage.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Controller.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Input/Components/Component_GameInput.h"
#include "Engine/Entity/Entity.h"
#include "Base/Types/ScopedValue.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE
{
    void PlayerSystem::CreateAdditionalRequiredComponents( Entity* pEntity ) const
    {
        auto pDamageComponent = EE::New<DamageComponent>();
        pEntity->AddComponent( pDamageComponent );
    }

    void PlayerSystem::PostComponentRegister()
    {
        if ( m_pAnimGraphComponent != nullptr && m_pCharacterMeshComponent != nullptr )
        {
            if ( m_pAnimGraphComponent->HasGraphInstance() )
            {
                m_actionContext.m_pAnimationController = EE::New<PlayerAnimationController>( m_pAnimGraphComponent, m_pCharacterMeshComponent );
            }
        }

        if ( m_pCameraComponent != nullptr )
        {
            m_actionContext.m_pCamera = m_pCameraComponent;
        }
    }

    void PlayerSystem::Shutdown()
    {
        EE_ASSERT( m_actionContext.m_pCamera == nullptr );
        EE_ASSERT( m_actionContext.m_pAnimationController == nullptr );
    }

    //-------------------------------------------------------------------------

    void PlayerSystem::RegisterComponent( EntityComponent* pComponent )
    {
        if ( auto pPlayerComponent = TryCast<PlayerComponent>( pComponent ) )
        {
            EE_ASSERT( m_actionContext.m_pPlayer == nullptr );
            m_actionContext.m_pPlayer = pPlayerComponent;

            if ( pPlayerComponent->HasGameState() )
            {
                m_actionContext.m_pPlayerState = pPlayerComponent->GetGameState<PlayerGameState>();
            }
        }

        else if ( auto pCameraComponent = TryCast<PlayerCameraComponent>( pComponent ) )
        {
            EE_ASSERT( m_pCameraComponent == nullptr );
            m_pCameraComponent = pCameraComponent;
        }

        else if ( auto pCharacterMeshComponent = TryCast<Render::CharacterMeshComponent>( pComponent ) )
        {
            EE_ASSERT( m_pCharacterMeshComponent == nullptr );
            m_pCharacterMeshComponent = pCharacterMeshComponent;
        }

        else if ( auto pGraphComponent = TryCast<Animation::GraphComponent>( pComponent ) )
        {
            // We only support one component ATM - animation graph comps are not singletons
            EE_ASSERT( m_pAnimGraphComponent == nullptr );
            m_pAnimGraphComponent = pGraphComponent;
        }

        else if ( auto pGameInputComponent = TryCast<Input::GameInputComponent>( pComponent ) )
        {
            EE_ASSERT( m_pGameInputComponent == nullptr );
            m_pGameInputComponent = pGameInputComponent;
        }

        //-------------------------------------------------------------------------

        m_actionContext.m_components.emplace_back( pComponent );
    }

    void PlayerSystem::UnregisterComponent( EntityComponent* pComponent )
    {
        m_actionContext.m_components.erase_first( pComponent );

        //-------------------------------------------------------------------------

        if ( auto pPlayerComponent = TryCast<PlayerComponent>( pComponent ) )
        {
            EE_ASSERT( m_actionContext.m_pPlayer == pPlayerComponent );
            m_actionStateMachine.ForceStopAllRunningActions();
            m_actionContext.m_pPlayer = nullptr;
            m_actionContext.m_pPlayerState = nullptr;
            m_actionContext.m_pInput = nullptr;
        }

        else if ( auto pCameraComponent = TryCast<PlayerCameraComponent>( pComponent ) )
        {
            EE_ASSERT( m_pCameraComponent == pCameraComponent );
            m_actionStateMachine.ForceStopAllRunningActions();
            m_actionContext.m_pCamera = nullptr;
            m_pCameraComponent = nullptr;
        }

        else if ( auto pCharacterMeshComponent = TryCast<Render::CharacterMeshComponent>( pComponent ) )
        {
            EE_ASSERT( m_pCharacterMeshComponent != nullptr );
            m_actionStateMachine.ForceStopAllRunningActions();
            EE::Delete( m_actionContext.m_pAnimationController );
            m_pCharacterMeshComponent = nullptr;
        }

        else if ( auto pGraphComponent = TryCast<Animation::GraphComponent>( pComponent ) )
        {
            // We only support one component atm - animgraph comps are not singletons
            EE_ASSERT( m_pAnimGraphComponent != nullptr );
            m_actionStateMachine.ForceStopAllRunningActions();
            EE::Delete( m_actionContext.m_pAnimationController );
            m_pAnimGraphComponent = nullptr;
        }

        else if ( auto pGameInputComponent = TryCast<Input::GameInputComponent>( pComponent ) )
        {
            m_actionStateMachine.ForceStopAllRunningActions();
            m_pGameInputComponent = nullptr;
        }
    }

    //-------------------------------------------------------------------------

    void PlayerSystem::Update( EntityWorldUpdateContext const& ctx )
    {
        if ( !ctx.IsGameWorld() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        EE_PROFILE_FUNCTION_GAMEPLAY();

        //-------------------------------------------------------------------------

        TScopedGuardValue const contextGuardValue( m_actionContext.m_pEntityWorldUpdateContext, &ctx );
        TScopedGuardValue const physicsSystemGuard( m_actionContext.m_pPhysicsWorld, ctx.GetWorldSystem<Physics::PhysicsWorldSystem>()->GetPhysicsWorld() );
        TScopedGuardValue const inputStateGuard( m_actionContext.m_pInput, m_pGameInputComponent ? m_pGameInputComponent->GetInputState<PlayerInputState>() : nullptr );

        if ( !m_actionContext.IsValid() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        UpdateStage const updateStage = ctx.GetUpdateStage();
        if ( updateStage == UpdateStage::GamePrePhysics )
        {
            UpdateGamePrePhysics( ctx );
        }
        else if ( updateStage == UpdateStage::GamePostPhysics )
        {
            UpdateGamePostPhysics( ctx );
        }
        else
        {
            EE_UNREACHABLE_CODE();
        }
    }

    void PlayerSystem::UpdateGamePrePhysics( EntityWorldUpdateContext const& ctx )
    {
        {
            EE_PROFILE_SCOPE_GAMEPLAY( "Player SM Update" );

            // Update camera
            m_actionContext.m_pCamera->Update( ctx, m_actionContext.m_pInput->m_look.GetValue() );

            // Update player state
            m_actionContext.m_pPlayerState->Update( m_actionContext.GetDeltaTime() );

            // Update player actions
            m_actionStateMachine.Update();
        }

        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_GAMEPLAY( "Player Animation Graph Update" );

            // Update animation and get root motion delta (remember that root motion is in character space, so we need to convert the displacement to world space)
            m_actionContext.m_pAnimationController->PreGraphUpdate( ctx.GetDeltaTime() );
            m_pAnimGraphComponent->EvaluateGraph( ctx.GetDeltaTime(), m_pCharacterMeshComponent->GetWorldTransform(), m_actionContext.m_pPhysicsWorld );
            m_actionContext.m_pAnimationController->PostGraphUpdate( ctx.GetDeltaTime() );
        }

        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_GAMEPLAY( "Player Position Update" );

            Vector const& deltaTranslation = m_pCharacterMeshComponent->GetWorldTransform().RotateVector( m_pAnimGraphComponent->GetRootMotionDelta().GetTranslation() );
            Quaternion const& deltaRotation = m_pAnimGraphComponent->GetRootMotionDelta().GetRotation();
            m_actionContext.m_pPlayer->MoveCharacter( ctx.GetDeltaTime(), deltaRotation, deltaTranslation );
        }

        //-------------------------------------------------------------------------

        // Run animation pose tasks
        m_pAnimGraphComponent->ExecutePrePhysicsTasks( ctx.GetDeltaTime(), m_pCharacterMeshComponent->GetWorldTransform() );

        // Update camera position relative to new character position
        m_actionContext.m_pCamera->FinalizeCamera();
    }

    void PlayerSystem::UpdateGamePostPhysics( EntityWorldUpdateContext const& ctx )
    {
        m_pAnimGraphComponent->ExecutePostPhysicsTasks();
    }
}