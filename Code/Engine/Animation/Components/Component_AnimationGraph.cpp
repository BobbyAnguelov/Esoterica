#include "Component_AnimationGraph.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/UpdateContext.h"
#include "Engine/Physics/PhysicsScene.h"
#include "System/Profiling.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    AnimationGraphComponent::~AnimationGraphComponent()
    {
        #if EE_DEVELOPMENT_TOOLS
        EE_ASSERT( m_pRootMotionActionRecorder == nullptr );
        #endif
    }

    void AnimationGraphComponent::Initialize()
    {
        EntityComponent::Initialize();

        if ( m_pGraphVariation == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( m_pGraphVariation.IsLoaded() );

        #if EE_DEVELOPMENT_TOOLS
        m_pRootMotionActionRecorder = EE::New<RootMotionRecorder>();
        #endif

        m_pPose = EE::New<Pose>( m_pGraphVariation->GetSkeleton() );
        m_pPose->CalculateGlobalTransforms();

        m_pTaskSystem = EE::New<TaskSystem>( m_pGraphVariation->GetSkeleton() );

        #if EE_DEVELOPMENT_TOOLS
        m_graphContext.Initialize( GetEntityID().m_ID, m_pTaskSystem, m_pPose, m_pRootMotionActionRecorder );
        #else
        m_graphContext.Initialize( GetEntityID().m_ID, m_pTaskSystem, m_pPose, nullptr );
        #endif

        m_pGraphInstance = EE::New<GraphInstance>( m_pGraphVariation.GetPtr() );
    }

    void AnimationGraphComponent::Shutdown()
    {
        // If we actually instantiated a graph
        if ( m_pGraphInstance != nullptr )
        {
            if ( m_pGraphInstance->IsInitialized() )
            {
                m_pGraphInstance->Shutdown( m_graphContext );
            }
            m_graphContext.Shutdown();
        }

        #if EE_DEVELOPMENT_TOOLS
        EE::Delete( m_pRootMotionActionRecorder );
        #endif

        EE::Delete( m_pTaskSystem );
        EE::Delete( m_pGraphInstance );
        EE::Delete( m_pPose );

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

    void AnimationGraphComponent::ResetGraphState()
    {
        if ( m_pGraphInstance != nullptr )
        {
            EE_ASSERT( m_pGraphInstance->IsInitialized() );
            m_pGraphInstance->Reset( m_graphContext );
        }
        else
        {
            EE_LOG_ERROR( "Animation", "Trying to reset graph state on a animgraph component that has no state!" );
        }
    }

    void AnimationGraphComponent::EvaluateGraph( Seconds deltaTime, Transform const& characterWorldTransform, Physics::Scene* pPhysicsScene )
    {
        EE_PROFILE_FUNCTION_ANIMATION();
        m_graphContext.Update( deltaTime, characterWorldTransform, pPhysicsScene );

        // Notify the root motion recorder we're starting an update
        #if EE_DEVELOPMENT_TOOLS
        m_pRootMotionActionRecorder->StartCharacterUpdate( characterWorldTransform );
        #endif

        // Initialize graph on the first update
        if ( !m_pGraphInstance->IsInitialized() )
        {
            m_pGraphInstance->Initialize( m_graphContext );
        }

        // Reset last frame's tasks
        m_pTaskSystem->Reset();

        // Update the graph and record the root motion
        GraphPoseNodeResult const result = m_pGraphInstance->UpdateGraph( m_graphContext );
        m_rootMotionDelta = result.m_rootMotionDelta;
    }

    void AnimationGraphComponent::ExecutePrePhysicsTasks( Transform const& characterWorldTransform )
    {
        EE_PROFILE_FUNCTION_ANIMATION();

        if ( !HasGraph() )
        {
            return;
        }

        // Notify the root motion recorder we're done with the character position update so it can track expected vs actual position
        #if EE_DEVELOPMENT_TOOLS
        m_pRootMotionActionRecorder->EndCharacterUpdate( characterWorldTransform );
        #endif

        m_pTaskSystem->UpdatePrePhysics( m_graphContext.m_deltaTime, characterWorldTransform, characterWorldTransform.GetInverse() );
    }

    void AnimationGraphComponent::ExecutePostPhysicsTasks()
    {
        EE_PROFILE_FUNCTION_ANIMATION();

        if ( !HasGraph() )
        {
            return;
        }

        m_pTaskSystem->UpdatePostPhysics( *m_pPose );
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void AnimationGraphComponent::DrawDebug( Drawing::DrawContext& drawingContext )
    {
        if ( !HasGraph() )
        {
            return;
        }

        m_pTaskSystem->DrawDebug( drawingContext );
        m_pGraphInstance->DrawDebug( m_graphContext, drawingContext );
        m_graphContext.GetRootMotionActionRecorder()->DrawDebug( drawingContext );
    }

    void AnimationGraphComponent::SetGraphDebugMode( GraphDebugMode mode )
    {
        if ( m_pGraphInstance != nullptr )
        {
            m_pGraphInstance->SetDebugMode( mode );
        }
        else
        {
            EE_LOG_ERROR( "Animation", "Trying to set debug state on a animgraph component that has no state!" );
        }
    }

    GraphDebugMode AnimationGraphComponent::GetGraphDebugMode() const
    {
        EE_ASSERT( HasGraphInstance() );
        return m_pGraphInstance->GetDebugMode();
    }

    void AnimationGraphComponent::SetGraphNodeDebugFilterList( TVector<int16_t> const& filterList )
    {
        if ( m_pGraphInstance != nullptr )
        {
            m_pGraphInstance->SetNodeDebugFilterList( filterList );
        }
        else
        {
            EE_LOG_ERROR( "Animation", "Trying to set debug state on a animgraph component that has no state!" );
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
            EE_LOG_ERROR( "Animation", "Trying to set debug state on a animgraph component that has no state!" );
        }
    }

    TaskSystemDebugMode AnimationGraphComponent::GetTaskSystemDebugMode() const
    {
        EE_ASSERT( HasGraphInstance() );
        return m_pTaskSystem->GetDebugMode();
    }

    void AnimationGraphComponent::SetRootMotionDebugMode( RootMotionRecorderDebugMode mode )
    {
        if ( m_pGraphInstance != nullptr )
        {
            m_graphContext.GetRootMotionActionRecorder()->SetDebugMode( mode );
        }
        else
        {
            EE_LOG_ERROR( "Animation", "Trying to set debug state on a animgraph component that has no state!" );
        }
    }

    RootMotionRecorderDebugMode AnimationGraphComponent::GetRootMotionDebugMode() const
    {
        EE_ASSERT( HasGraphInstance() );
        return m_graphContext.GetRootMotionActionRecorder()->GetDebugMode();
    }
    #endif
}