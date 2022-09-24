#include "EntitySystem_Animation.h"
#include "Engine/Animation/Components/Component_AnimationClipPlayer.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Animation/AnimationPose.h"
#include "System/Profiling.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    AnimationSystem::~AnimationSystem()
    {
        EE_ASSERT( m_animGraphs.empty() && m_animPlayers.empty() && m_meshComponents.empty() );
        EE_ASSERT( m_pRootComponent == nullptr );
    }

    //-------------------------------------------------------------------------

    void AnimationSystem::RegisterComponent( EntityComponent* pComponent )
    {
        if ( auto pMeshComponent = TryCast<Render::SkeletalMeshComponent>( pComponent ) )
        {
            EE_ASSERT( !VectorContains( m_meshComponents, pMeshComponent ) );
            m_meshComponents.push_back( pMeshComponent );
        }
        else if ( auto pAnimPlayerComponent = TryCast<AnimationClipPlayerComponent>( pComponent ) )
        {
            m_animPlayers.emplace_back( pAnimPlayerComponent );
        }
        else if ( auto pGraphComponent = TryCast<AnimationGraphComponent>( pComponent ) )
        {
            m_animGraphs.emplace_back( pGraphComponent );
            pGraphComponent->ResetGraphState();
        }

        //-------------------------------------------------------------------------

        auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent );
        if ( pSpatialComponent != nullptr && pSpatialComponent->IsRootComponent() )
        {
            m_pRootComponent = pSpatialComponent;
        }
    }

    void AnimationSystem::UnregisterComponent( EntityComponent* pComponent )
    {
        if ( auto pMeshComponent = TryCast<Render::SkeletalMeshComponent>( pComponent ) )
        {
            EE_ASSERT( VectorContains( m_meshComponents, pMeshComponent ) );
            m_meshComponents.erase_first( pMeshComponent );
        }
        else if ( auto pAnimPlayerComponent = TryCast<AnimationClipPlayerComponent>( pComponent ) )
        {
            m_animPlayers.erase_first_unsorted( pAnimPlayerComponent );
        }
        else if ( auto pGraphComponent = TryCast<AnimationGraphComponent>( pComponent ) )
        {
            m_animGraphs.erase_first_unsorted( pGraphComponent );
        }

        //-------------------------------------------------------------------------

        auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent );
        if ( pSpatialComponent != nullptr && pSpatialComponent->IsRootComponent() )
        {
            m_pRootComponent = nullptr;
        }
    }

    //-------------------------------------------------------------------------

    void AnimationSystem::Update( EntityWorldUpdateContext const& ctx )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Animation System");

        if ( m_animPlayers.empty() && m_animGraphs.empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        Transform characterWorldTransform( NoInit );
        if ( m_meshComponents.empty() )
        {
            characterWorldTransform = Transform::Identity;
        }
        else
        {
            characterWorldTransform = m_meshComponents[0]->GetWorldTransform();
        }

        //-------------------------------------------------------------------------

        UpdateAnimPlayers( ctx, characterWorldTransform );
        UpdateAnimGraphs( ctx, characterWorldTransform );

        //-------------------------------------------------------------------------

        if ( ctx.GetUpdateStage() == UpdateStage::PostPhysics )
        {
            for ( auto pMeshComponent : m_meshComponents )
            {
                if ( !pMeshComponent->HasMeshResourceSet() )
                {
                    continue;
                }

                pMeshComponent->FinalizePose();
            }
        }
    }

    void AnimationSystem::UpdateAnimPlayers( EntityWorldUpdateContext const& ctx, Transform const& characterWorldTransform )
    {
        UpdateStage const updateStage = ctx.GetUpdateStage();
        if ( updateStage == UpdateStage::PrePhysics )
        {
            for ( auto pAnimComponent : m_animPlayers )
            {
                if ( !pAnimComponent->HasAnimationSet() )
                {
                    continue;
                }

                //-------------------------------------------------------------------------

                if ( !pAnimComponent->RequiresManualUpdate() )
                {
                    pAnimComponent->Update( ctx.GetDeltaTime(), characterWorldTransform );

                    // Apply the root motion if desired
                    if ( m_pRootComponent != nullptr && pAnimComponent->ShouldApplyRootMotionToEntity() )
                    {
                        Transform rootMotionDelta = pAnimComponent->GetRootMotionDelta();
                        Transform worldTransform = m_pRootComponent->GetWorldTransform();
                        worldTransform = rootMotionDelta * worldTransform;
                        m_pRootComponent->SetWorldTransform( worldTransform );
                    }
                }

                //-------------------------------------------------------------------------

                auto const* pPose = pAnimComponent->GetPose();
                EE_ASSERT( pPose->HasGlobalTransforms() );

                for ( auto pMeshComponent : m_meshComponents )
                {
                    if ( !pMeshComponent->HasMeshResourceSet() )
                    {
                        continue;
                    }

                    if ( pPose->GetSkeleton() != pMeshComponent->GetSkeleton() )
                    {
                        continue;
                    }

                    pMeshComponent->SetPose( pPose );
                }
            }
        }
    }

    void AnimationSystem::UpdateAnimGraphs( EntityWorldUpdateContext const& ctx, Transform const& characterWorldTransform )
    {
        UpdateStage const updateStage = ctx.GetUpdateStage();
        if ( updateStage == UpdateStage::PrePhysics )
        {
            auto pPhysicsWorldSystem = ctx.GetWorldSystem<Physics::PhysicsWorldSystem>();

            //-------------------------------------------------------------------------

            for ( auto pAnimComponent : m_animGraphs )
            {
                if ( !pAnimComponent->HasGraph() )
                {
                    continue;
                }

                if ( !pAnimComponent->RequiresManualUpdate() )
                {
                    // Evaluate the graph nodes and calculate the root motion delta
                    pAnimComponent->EvaluateGraph( ctx.GetDeltaTime(), characterWorldTransform, pPhysicsWorldSystem->GetScene() );

                    // Apply the root motion if desired
                    Transform adjustedCharacterTransform = characterWorldTransform;
                    if ( m_pRootComponent != nullptr && pAnimComponent->ShouldApplyRootMotionToEntity() )
                    {
                        Transform rootMotionDelta = pAnimComponent->GetRootMotionDelta();
                        Transform worldTransform = m_pRootComponent->GetWorldTransform();
                        worldTransform = rootMotionDelta * worldTransform;
                        m_pRootComponent->SetWorldTransform( worldTransform );

                        // Shift character world transform
                        adjustedCharacterTransform = rootMotionDelta * characterWorldTransform;
                    }

                    // Calculate pose tasks
                    pAnimComponent->ExecutePrePhysicsTasks( ctx.GetDeltaTime(), adjustedCharacterTransform );
                }
            }
        }
        else if ( updateStage == UpdateStage::PostPhysics )
        {
            for ( auto pAnimComponent : m_animGraphs )
            {
                if ( !pAnimComponent->HasGraph() )
                {
                    continue;
                }

                // Calculate the final pose tasks
                if ( !pAnimComponent->RequiresManualUpdate() )
                {
                    pAnimComponent->ExecutePostPhysicsTasks();
                }

                // Set poses
                //-------------------------------------------------------------------------
                // Note:    for components requiring manual update, the users need to ensure the manual update occurs before this update
                //          This update is already set to the lowest priority so in general users wont need to do anything

                auto const* pPose = pAnimComponent->GetPose();
                EE_ASSERT( pPose->HasGlobalTransforms() );

                for ( auto pMeshComponent : m_meshComponents )
                {
                    if ( !pMeshComponent->HasMeshResourceSet() )
                    {
                        continue;
                    }

                    if ( pPose->GetSkeleton() != pMeshComponent->GetSkeleton() )
                    {
                        continue;
                    }

                    pMeshComponent->SetPose( pPose );
                }
            }
        }
    }
}