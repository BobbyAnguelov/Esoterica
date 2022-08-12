#include "Component_AnimationGraph.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/UpdateContext.h"
#include "Engine/Physics/PhysicsScene.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void AnimationGraphComponent::Initialize()
    {
        EntityComponent::Initialize();

        if ( m_pGraphVariation == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( m_pGraphVariation.IsLoaded() );

        m_pTaskSystem = EE::New<TaskSystem>( m_pGraphVariation->GetSkeleton() );
        m_pGraphInstance = EE::New<GraphInstance>( m_pGraphVariation.GetPtr(), GetEntityID().m_ID );
    }

    void AnimationGraphComponent::Shutdown()
    {
        // If we actually instantiated a graph
        if ( m_pGraphInstance != nullptr )
        {
            if ( m_pGraphInstance->IsInitialized() )
            {
                m_pGraphInstance->Shutdown();
            }
        }

        EE::Delete( m_pTaskSystem );
        EE::Delete( m_pGraphInstance );

        EntityComponent::Shutdown();
    }

    //-------------------------------------------------------------------------

    void AnimationGraphComponent::SetGraphVariation( ResourceID graphResourceID )
    {
        EE_ASSERT( IsUnloaded() );
        EE_ASSERT( graphResourceID.IsValid() );
        m_pGraphVariation = graphResourceID;
    }

    //-------------------------------------------------------------------------

    Skeleton const* AnimationGraphComponent::GetSkeleton() const
    {
        return ( m_pGraphVariation != nullptr ) ? m_pGraphVariation->GetSkeleton() : nullptr;
    }

    Pose const* AnimationGraphComponent::GetPose() const
    {
        return m_pTaskSystem->GetPose();
    }

    void AnimationGraphComponent::ResetGraphState()
    {
        if ( m_pGraphInstance != nullptr )
        {
            EE_ASSERT( m_pGraphInstance->IsInitialized() );
            m_pGraphInstance->ResetGraphState();
        }
        else
        {
            EE_LOG_ENTITY_ERROR( this, "Animation", "TODO", "Trying to reset graph state on a animgraph component that has no state!" );
        }
    }

    void AnimationGraphComponent::EvaluateGraph( Seconds deltaTime, Transform const& characterWorldTransform, Physics::Scene* pPhysicsScene )
    {
        // Initialize graph on the first update
        if ( !m_pGraphInstance->IsInitialized() )
        {
            m_pGraphInstance->Initialize( m_pTaskSystem );
        }

        // Reset last frame's tasks
        m_pTaskSystem->Reset();

        // Update the graph
        m_pGraphInstance->StartUpdate( deltaTime, characterWorldTransform, pPhysicsScene );
        auto const result = m_pGraphInstance->UpdateGraph();
        m_rootMotionDelta = result.m_rootMotionDelta;
    }

    void AnimationGraphComponent::ExecutePrePhysicsTasks( Seconds deltaTime, Transform const& characterWorldTransform )
    {
        if ( !HasGraph() )
        {
            return;
        }

        // End graph update and calculate the pose
        m_pGraphInstance->EndUpdate( characterWorldTransform );
        m_pTaskSystem->UpdatePrePhysics( deltaTime, characterWorldTransform, characterWorldTransform.GetInverse() );
    }

    void AnimationGraphComponent::ExecutePostPhysicsTasks()
    {
        if ( !HasGraph() )
        {
            return;
        }

        m_pTaskSystem->UpdatePostPhysics();
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    Transform AnimationGraphComponent::GetDebugWorldTransform() const
    {
        return m_pTaskSystem->GetCharacterWorldTransform();
    }

    void AnimationGraphComponent::SetGraphDebugMode( GraphDebugMode mode )
    {
        if ( m_pGraphInstance != nullptr )
        {
            m_pGraphInstance->SetGraphDebugMode( mode );
        }
        else
        {
            EE_LOG_ENTITY_ERROR( this, "Animation", "Trying to set debug state on a animgraph component that has no state!" );
        }
    }

    GraphDebugMode AnimationGraphComponent::GetGraphDebugMode() const
    {
        EE_ASSERT( HasGraphInstance() );
        return m_pGraphInstance->GetGraphDebugMode();
    }

    void AnimationGraphComponent::SetGraphNodeDebugFilterList( TVector<int16_t> const& filterList )
    {
        if ( m_pGraphInstance != nullptr )
        {
            m_pGraphInstance->SetNodeDebugFilterList( filterList );
        }
        else
        {
            EE_LOG_ENTITY_ERROR( this, "Animation", "Trying to set debug state on a animgraph component that has no state!" );
        }
    }

    void AnimationGraphComponent::SetTaskSystemDebugMode( TaskSystemDebugMode mode )
    {
        if ( m_pGraphInstance != nullptr )
        {
            m_pTaskSystem->SetDebugMode( mode );
        }
        else
        {
            EE_LOG_ENTITY_ERROR( this, "Animation", "Trying to set debug state on a animgraph component that has no state!" );
        }
    }

    TaskSystemDebugMode AnimationGraphComponent::GetTaskSystemDebugMode() const
    {
        EE_ASSERT( HasGraphInstance() );
        return m_pTaskSystem->GetDebugMode();
    }

    void AnimationGraphComponent::SetRootMotionDebugMode( RootMotionDebugMode mode )
    {
        if ( m_pGraphInstance != nullptr )
        {
            m_pGraphInstance->SetRootMotionDebugMode( mode );
        }
        else
        {
            EE_LOG_ENTITY_ERROR( this, "Animation", "Trying to set debug state on a animgraph component that has no state!" );
        }
    }

    RootMotionDebugMode AnimationGraphComponent::GetRootMotionDebugMode() const
    {
        EE_ASSERT( HasGraphInstance() );
        return m_pGraphInstance->GetRootMotionDebugMode();
    }

    void AnimationGraphComponent::DrawDebug( Drawing::DrawContext& drawingContext )
    {
        if ( !HasGraph() )
        {
            return;
        }

        m_pTaskSystem->DrawDebug( drawingContext );
        m_pGraphInstance->DrawDebug( drawingContext );
    }
    #endif
}