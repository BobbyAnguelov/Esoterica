#include "EntitySystem_NPC.h"
#include "Game/NPC/Animation/NPCAnimationController.h"
#include "Game/Damage/Components/Component_Health.h"
#include "Engine/Navmesh/Systems/WorldSystem_Navmesh.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/Entity.h"
#include "Base/Types/ScopedValue.h"

//-------------------------------------------------------------------------

namespace EE
{
    void NPCSystem::PostComponentRegister()
    {
        if ( m_pAnimGraphComponent != nullptr && m_pCharacterMeshComponent != nullptr )
        {
            if ( m_pAnimGraphComponent->HasGraphInstance() )
            {
                m_behaviorContext.m_pAnimationController = EE::New<NPCAnimationController>( m_pAnimGraphComponent, m_pCharacterMeshComponent );
            }
        }
    }

    void NPCSystem::Shutdown()
    {
        EE_ASSERT( m_behaviorContext.m_pAnimationController == nullptr );
    }

    void NPCSystem::CreateAdditionalRequiredComponents( Entity* pEntity ) const
    {
        auto pHeathComponent = EE::New<HealthComponent>();
        pEntity->AddComponent( pHeathComponent );
    }

    //-------------------------------------------------------------------------

    void NPCSystem::RegisterComponent( EntityComponent* pComponent )
    {
        if ( auto pNPCComponent = TryCast<NPCComponent>( pComponent ) )
        {
            EE_ASSERT( m_behaviorContext.m_pNPC == nullptr );
            m_behaviorContext.m_pNPC = pNPCComponent;

            if ( pNPCComponent->HasGameState() )
            {
                m_behaviorContext.m_pNPCState = pNPCComponent->GetGameState<NPCGameState>();
            }
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

    void NPCSystem::UnregisterComponent( EntityComponent* pComponent )
    {
        m_behaviorContext.m_components.erase_first( pComponent );

        //-------------------------------------------------------------------------

        if ( auto pNPCComponent = TryCast<NPCComponent>( pComponent ) )
        {
            EE_ASSERT( m_behaviorContext.m_pNPC == pNPCComponent );
            m_behaviorContext.m_pNPC = nullptr;
            m_behaviorContext.m_pNPCState = nullptr;
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

    void NPCSystem::Update( EntityWorldUpdateContext const& ctx )
    {
        if ( !ctx.IsGameWorld() )
        {
            return;
        }

        TScopedGuardValue const contextGuardValue( m_behaviorContext.m_pEntityWorldUpdateContext, &ctx );
        TScopedGuardValue const navmeshSystemGuardValue( m_behaviorContext.m_pNavmeshSystem, ctx.GetWorldSystem<Navmesh::NavmeshWorldSystem>() );
        TScopedGuardValue const physicsSystemGuard( m_behaviorContext.m_pPhysicsWorld, ctx.GetWorldSystem<Physics::PhysicsWorldSystem>()->GetPhysicsWorld() );

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
            //m_behaviorContext.m_pCharacter->MoveCharacter( ctx.GetDeltaTime(), deltaRotation, deltaTranslation );

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