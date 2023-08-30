#include "Component_AnimationGraph.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/UpdateContext.h"
#include "Engine/Physics/PhysicsWorld.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void GraphComponent::Initialize()
    {
        EntityComponent::Initialize();

        if ( m_pGraphVariation == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( m_pGraphVariation.IsLoaded() );
        m_pGraphInstance = EE::New<GraphInstance>( m_pGraphVariation.GetPtr(), GetEntityID().m_value );
    }

    void GraphComponent::Shutdown()
    {
        EE::Delete( m_pGraphInstance );
        EntityComponent::Shutdown();
    }

    //-------------------------------------------------------------------------

    void GraphComponent::SetGraphVariation( ResourceID graphResourceID )
    {
        EE_ASSERT( IsUnloaded() );
        EE_ASSERT( graphResourceID.IsValid() );
        m_pGraphVariation = graphResourceID;
    }

    //-------------------------------------------------------------------------

    Skeleton const* GraphComponent::GetSkeleton() const
    {
        return ( m_pGraphVariation != nullptr ) ? m_pGraphVariation->GetSkeleton() : nullptr;
    }

    Pose const* GraphComponent::GetPose() const
    {
        return m_pGraphInstance->GetPose();
    }

    void GraphComponent::EvaluateGraph( Seconds deltaTime, Transform const& characterWorldTransform, Physics::PhysicsWorld* pPhysicsWorld )
    {
        EE_ASSERT( HasGraph() );

        m_pGraphInstance->SetSkeletonLOD( m_skeletonLOD );
        GraphPoseNodeResult const result = m_pGraphInstance->EvaluateGraph( deltaTime, characterWorldTransform, pPhysicsWorld, nullptr, m_graphStateResetRequested );
        m_graphStateResetRequested = false;
        m_rootMotionDelta = result.m_rootMotionDelta;

        #if EE_DEVELOPMENT_TOOLS
        m_pGraphInstance->OutputLog();
        #endif
    }

    void GraphComponent::ExecutePrePhysicsTasks( Seconds deltaTime, Transform const& characterWorldTransform )
    {
        EE_ASSERT( HasGraph() );
        m_pGraphInstance->ExecutePrePhysicsPoseTasks( characterWorldTransform );
    }

    void GraphComponent::ExecutePostPhysicsTasks()
    {
        EE_ASSERT( HasGraph() );
        m_pGraphInstance->ExecutePostPhysicsPoseTasks();
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    Transform GraphComponent::GetDebugWorldTransform() const
    {
        return m_pGraphInstance->GetTaskSystemDebugWorldTransform();
    }

    void GraphComponent::SetGraphDebugMode( GraphDebugMode mode )
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

    GraphDebugMode GraphComponent::GetGraphDebugMode() const
    {
        EE_ASSERT( HasGraphInstance() );
        return m_pGraphInstance->GetGraphDebugMode();
    }

    void GraphComponent::SetGraphNodeDebugFilterList( TVector<int16_t> const& filterList )
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

    void GraphComponent::SetTaskSystemDebugMode( TaskSystemDebugMode mode )
    {
        if ( m_pGraphInstance != nullptr )
        {
            m_pGraphInstance->SetTaskSystemDebugMode( mode );
        }
        else
        {
            EE_LOG_ENTITY_ERROR( this, "Animation", "Trying to set debug state on a animgraph component that has no state!" );
        }
    }

    TaskSystemDebugMode GraphComponent::GetTaskSystemDebugMode() const
    {
        EE_ASSERT( HasGraphInstance() );
        return m_pGraphInstance->GetTaskSystemDebugMode();
    }

    void GraphComponent::SetRootMotionDebugMode( RootMotionDebugMode mode )
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

    RootMotionDebugMode GraphComponent::GetRootMotionDebugMode() const
    {
        EE_ASSERT( HasGraphInstance() );
        return m_pGraphInstance->GetRootMotionDebugMode();
    }

    void GraphComponent::DrawDebug( Drawing::DrawContext& drawingContext )
    {
        if ( !HasGraph() || !IsInitialized() || m_pGraphInstance == nullptr )
        {
            return;
        }

        m_pGraphInstance->DrawDebug( drawingContext );
    }
    #endif
}