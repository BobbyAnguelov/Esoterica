#include "EntitySystem_AIController.h"
#include "Game/AI/Physics/AIPhysicsController.h"
#include "Game/AI/Animation/AIAnimationController.h"
#include "Engine/AI/Components/Component_AI.h"
#include "Engine/Navmesh/NavPower.h"
#include "Engine/Navmesh/Systems/WorldSystem_Navmesh.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Types/ScopedValue.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    void AIController::PostComponentRegister()
    {
        if ( m_behaviorContext.m_pCharacter != nullptr )
        {
            m_behaviorContext.m_pCharacterController = EE::New<CharacterPhysicsController>( m_behaviorContext.m_pCharacter );
        }

        if ( m_pAnimGraphComponent != nullptr && m_pCharacterMeshComponent != nullptr )
        {
            m_behaviorContext.m_pAnimationController = EE::New<AnimationController>( m_pAnimGraphComponent, m_pCharacterMeshComponent );
        }
    }

    void AIController::Shutdown()
    {
        EE_ASSERT( m_behaviorContext.m_pAnimationController == nullptr );
        EE_ASSERT( m_behaviorContext.m_pCharacterController == nullptr );
    }

    //-------------------------------------------------------------------------

    void AIController::RegisterComponent( EntityComponent* pComponent )
    {
        if ( auto pCharacterComponent = TryCast<Physics::CharacterComponent>( pComponent ) )
        {
            EE_ASSERT( m_behaviorContext.m_pCharacter == nullptr );
            m_behaviorContext.m_pCharacter = pCharacterComponent;
        }

        else if ( auto pAIComponent = TryCast<AIComponent>( pComponent ) )
        {
            EE_ASSERT( m_behaviorContext.m_pAIComponent == nullptr );
            m_behaviorContext.m_pAIComponent = pAIComponent;
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

        //-------------------------------------------------------------------------

        m_behaviorContext.m_components.emplace_back( pComponent );
    }

    void AIController::UnregisterComponent( EntityComponent* pComponent )
    {
        m_behaviorContext.m_components.erase_first( pComponent );

        //-------------------------------------------------------------------------

        if ( auto pCharacterComponent = TryCast<Physics::CharacterComponent>( pComponent ) )
        {
            EE_ASSERT( m_behaviorContext.m_pCharacter == pCharacterComponent );
            m_behaviorContext.m_pCharacter = nullptr;

            EE::Delete( m_behaviorContext.m_pCharacterController );
        }

        else if ( auto pAIComponent = TryCast<AIComponent>( pComponent ) )
        {
            EE_ASSERT( m_behaviorContext.m_pAIComponent == pAIComponent );
            m_behaviorContext.m_pAIComponent = nullptr;
        }

        else if ( auto pCharacterMeshComponent = TryCast<Render::CharacterMeshComponent>( pComponent ) )
        {
            EE_ASSERT( m_pCharacterMeshComponent != nullptr );
            m_pCharacterMeshComponent = nullptr;

            EE::Delete( m_behaviorContext.m_pAnimationController );
        }

        else if ( auto pGraphComponent = TryCast<Animation::GraphComponent>( pComponent ) )
        {
            EE_ASSERT( m_pAnimGraphComponent != nullptr );
            m_pAnimGraphComponent = nullptr;

            EE::Delete( m_behaviorContext.m_pAnimationController );
        }
    }

    //-------------------------------------------------------------------------

    void AIController::Update( EntityWorldUpdateContext const& ctx )
    {
        TScopedGuardValue const contextGuardValue( m_behaviorContext.m_pEntityWorldUpdateContext, &ctx );
        TScopedGuardValue const navmeshSystemGuardValue( m_behaviorContext.m_pNavmeshSystem, ctx.GetWorldSystem<Navmesh::NavmeshWorldSystem>() );
        TScopedGuardValue const physicsSystemGuard( m_behaviorContext.m_pPhysicsWorld, ctx.GetWorldSystem<Physics::PhysicsWorldSystem>()->GetWorld() );

        #ifndef EE_ENABLE_NAVPOWER
        if ( true )
        {
            return;
        }
        #endif

        if ( !m_behaviorContext.IsValid() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        UpdateStage const updateStage = ctx.GetUpdateStage();
        if ( updateStage == UpdateStage::PrePhysics )
        {
            m_behaviorSelector.Update();

            // Update animation and get root motion delta (remember that root motion is in character space, so we need to convert the displacement to world space)
            m_pAnimGraphComponent->EvaluateGraph( ctx.GetDeltaTime(), m_pCharacterMeshComponent->GetWorldTransform(), m_behaviorContext.m_pPhysicsWorld );
            Vector const& deltaTranslation = m_pCharacterMeshComponent->GetWorldTransform().RotateVector( m_pAnimGraphComponent->GetRootMotionDelta().GetTranslation() );
            Quaternion const& deltaRotation = m_pAnimGraphComponent->GetRootMotionDelta().GetRotation();

            // Move character
            m_behaviorContext.m_pCharacterController->TryMoveCapsule( ctx, m_behaviorContext.m_pPhysicsWorld, deltaTranslation, deltaRotation );

            // Run animation pose tasks
            m_pAnimGraphComponent->ExecutePrePhysicsTasks( ctx.GetDeltaTime(), m_pCharacterMeshComponent->GetWorldTransform() );
        }
        else if ( updateStage == UpdateStage::PostPhysics )
        {
            m_pAnimGraphComponent->ExecutePostPhysicsTasks();
        }
        else
        {
            EE_UNREACHABLE_CODE();
        }
    }
}