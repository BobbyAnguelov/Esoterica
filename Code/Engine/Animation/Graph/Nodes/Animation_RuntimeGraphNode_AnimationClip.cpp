#include "Animation_RuntimeGraphNode_AnimationClip.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Sample.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_DataSet.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void AnimationClipNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<AnimationClipNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_playInReverseValueNodeIdx, pNode->m_pPlayInReverseValueNode );
        pNode->m_pAnimation = context.GetResource<AnimationClip>( m_dataSlotIdx );

        //-------------------------------------------------------------------------

        if ( pNode->m_pAnimation->GetSkeleton() != context.m_pDataSet->GetSkeleton() )
        {
            pNode->m_pAnimation = nullptr;
        }
    }

    bool AnimationClipNode::IsValid() const
    {
        return AnimationClipReferenceNode::IsValid() && m_pAnimation != nullptr;
    }

    void AnimationClipNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        AnimationClipReferenceNode::InitializeInternal( context, initialTime );
        auto pSettings = GetSettings<AnimationClipNode>();

        // Try to create play in reverse node
        if ( m_pPlayInReverseValueNode != nullptr )
        {
            m_pPlayInReverseValueNode->Initialize( context );
        }

        // Initialize state data
        if ( m_pAnimation != nullptr )
        {
            m_duration = m_pAnimation->IsSingleFrameAnimation() ? s_oneFrameDuration : m_pAnimation->GetDuration();
            m_currentTime = m_previousTime = m_pAnimation->GetSyncTrack().GetPercentageThrough( initialTime );
            EE_ASSERT( m_currentTime >= 0.0f && m_currentTime <= 1.0f );
        }
        else
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "No animation set for clip node!" );
            #endif
        }

        m_shouldSampleRootMotion = pSettings->m_sampleRootMotion;
        m_shouldPlayInReverse = false;
    }

    void AnimationClipNode::ShutdownInternal( GraphContext& context )
    {
        if ( m_pPlayInReverseValueNode != nullptr )
        {
            m_pPlayInReverseValueNode->Shutdown( context );
        }

        m_currentTime = m_previousTime = 0.0f;
        AnimationClipReferenceNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult AnimationClipNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && IsInitialized() );

        if ( !IsValid() )
        {
            return GraphPoseNodeResult();
        }

        MarkNodeActive( context );
        EE_ASSERT( m_currentTime >= 0.0f && m_currentTime <= 1.0f );

        // Should we change the playback direction?
        if ( m_pPlayInReverseValueNode != nullptr )
        {
            if ( m_shouldPlayInReverse != m_pPlayInReverseValueNode->GetValue<bool>( context ) )
            {
                m_shouldPlayInReverse = !m_shouldPlayInReverse;
                m_currentTime = ( 1.0f - m_currentTime );
                if ( m_currentTime == 1.0f )
                {
                    m_currentTime = 0.0f;
                }
            }
        }

        //-------------------------------------------------------------------------

        Percentage const deltaPercentage = Percentage( context.m_deltaTime / m_duration );

        auto pSettings = GetSettings<AnimationClipNode>();
        if ( !pSettings->m_allowLooping )
        {
            // We might have come from a sync update so ensure the previous time is set to the normalized current time
            m_previousTime = m_currentTime;
            m_currentTime = ( m_previousTime + deltaPercentage ).GetClamped( false );
        }
        else // Regular update
        {
            m_previousTime = m_currentTime;
            m_currentTime += deltaPercentage;
            if ( m_currentTime > 1 )
            {
                m_currentTime = m_currentTime.GetNormalizedTime();
                m_loopCount++;
            }
        }

        //-------------------------------------------------------------------------

        return CalculateResult( context );
    }

    GraphPoseNodeResult AnimationClipNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        EE_ASSERT( context.IsValid() && IsInitialized() );

        if ( !IsValid() )
        {
            return GraphPoseNodeResult();
        }

        MarkNodeActive( context );
        EE_ASSERT( m_currentTime >= 0.0f && m_currentTime <= 1.0f );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_pPlayInReverseValueNode != nullptr || m_shouldPlayInReverse )
        {
            context.LogWarning( GetNodeIndex(), "'Play reversed' has no effect when used with time synchronization!" );
        }
        #endif

        // Handle single frame animations
        if ( m_pAnimation->IsSingleFrameAnimation() )
        {
            m_previousTime = 0.0f;
            m_currentTime = 0.0f;
        }
        else // Regular time update
        {
            m_previousTime = GetSyncTrack().GetPercentageThrough( updateRange.m_startTime );
            m_currentTime = GetSyncTrack().GetPercentageThrough( updateRange.m_endTime );
            m_loopCount = 0;
        }

        //-------------------------------------------------------------------------

        return CalculateResult( context );
    }

    GraphPoseNodeResult AnimationClipNode::CalculateResult( GraphContext& context ) const
    {
        EE_ASSERT( m_pAnimation != nullptr );

        GraphPoseNodeResult result;
        result.m_sampledEventRange = context.GetEmptySampledEventRange();

        // Events
        //-------------------------------------------------------------------------

        bool const isFromActiveBranch = ( context.m_branchState == BranchState::Active );

        Percentage actualAnimationSampleStartTime = m_previousTime;
        Percentage actualAnimationSampleEndTime = m_currentTime;

        // Invert times and swap the start and end times to create the correct sampling range for events
        TInlineVector<Event const*, 10> sampledAnimationEvents;
        if ( m_shouldPlayInReverse )
        {
            actualAnimationSampleEndTime = 1.0f - m_currentTime;
            actualAnimationSampleStartTime = 1.0f - m_previousTime;
            m_pAnimation->GetEventsForRange( actualAnimationSampleEndTime, actualAnimationSampleStartTime, sampledAnimationEvents );
        }
        else
        {
            m_pAnimation->GetEventsForRange( actualAnimationSampleStartTime, actualAnimationSampleEndTime, sampledAnimationEvents );
        }

        // Post-process sampled events
        for ( auto pEvent : sampledAnimationEvents )
        {
            Percentage percentageThroughEvent = 1.0f;

            if ( pEvent->IsDurationEvent() )
            {
                Seconds const currentAnimTimeSeconds( m_pAnimation->GetDuration() * actualAnimationSampleEndTime.ToFloat() );
                percentageThroughEvent = pEvent->GetTimeRange().GetPercentageThroughClamped( currentAnimTimeSeconds );
                EE_ASSERT( percentageThroughEvent <= 1.0f );

                if ( m_shouldPlayInReverse )
                {
                    percentageThroughEvent = 1.0f - percentageThroughEvent;
                }
            }

            context.m_sampledEventsBuffer.EmplaceAnimationEvent( GetNodeIndex(), pEvent, percentageThroughEvent, isFromActiveBranch );
        }

        result.m_sampledEventRange.m_endIdx = context.m_sampledEventsBuffer.GetNumSampledEvents();

        // Root Motion
        //-------------------------------------------------------------------------

        if ( m_shouldSampleRootMotion )
        {
            // The root motion calculation is done manually here since we dont want to pollute the base function with a rare special case
            if ( m_shouldPlayInReverse )
            {
                if ( m_previousTime <= m_currentTime )
                {
                    result.m_rootMotionDelta = m_pAnimation->GetRootMotionDeltaNoLooping( actualAnimationSampleStartTime, actualAnimationSampleEndTime );
                }
                else
                {
                    Transform const preLoopDelta = m_pAnimation->GetRootMotionDeltaNoLooping( actualAnimationSampleStartTime, 0.0f );
                    Transform const postLoopDelta = m_pAnimation->GetRootMotionDeltaNoLooping( 1.0f, actualAnimationSampleEndTime );
                    result.m_rootMotionDelta = postLoopDelta * preLoopDelta;
                }
            }
            else //[[likely]] - enable with C++ 20
            {
                result.m_rootMotionDelta = m_pAnimation->GetRootMotionDelta( m_previousTime, m_currentTime );
            }

            #if EE_DEVELOPMENT_TOOLS
            context.GetRootMotionDebugger()->RecordSampling( GetNodeIndex(), result.m_rootMotionDelta );
            #endif
        }

        // Register pose tasks
        //-------------------------------------------------------------------------

        Percentage const sampleTime = m_shouldPlayInReverse ? Percentage( 1.0f - m_currentTime.ToFloat() ) : m_currentTime;
        result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::SampleTask>( GetNodeIndex(), m_pAnimation, sampleTime );
        return result;
    }

    #if EE_DEVELOPMENT_TOOLS
    void AnimationClipNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_shouldPlayInReverse );
        outState.WriteValue( m_shouldSampleRootMotion );
    }

    void AnimationClipNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );
        inState.ReadValue( m_shouldPlayInReverse );
        inState.ReadValue( m_shouldSampleRootMotion );
    }
    #endif
}