#include "Animation_RuntimeGraphNode_TimeControlledAnimationClip.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Engine/Animation/Events/AnimationEvent_SnapToFrame.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Sample.h"
#include "Animation_RuntimeGraphNode_AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void TimeControlledAnimationClipNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TimeControlledAnimationClipNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_timeValueNodeIdx, pNode->m_pTimeValueNode );
        context.SetOptionalNodePtrFromIndex( m_playInReverseValueNodeIdx, pNode->m_pPlayInReverseValueNode );

        pNode->m_pAnimation = context.GetResourceForSlot<AnimationClip>( m_dataSlotIdx );

        //-------------------------------------------------------------------------

        if ( pNode->m_pAnimation != nullptr && pNode->m_pAnimation->GetSkeleton() != context.m_pSkeleton )
        {
            pNode->m_pAnimation = nullptr;
        }
    }

    //-------------------------------------------------------------------------

    bool TimeControlledAnimationClipNode::IsValid() const
    {
        return PoseNode::IsValid() && m_pAnimation != nullptr;
    }

    SyncTrack const& TimeControlledAnimationClipNode::GetSyncTrack() const
    {
        [[likely]]
        if ( IsValid() )
        {
            return m_pAnimation->GetSyncTrack();
        }
        else
        {
            return SyncTrack::s_defaultTrack;
        }
    }

    void TimeControlledAnimationClipNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        PoseNode::InitializeInternal( context, initialTime );

        m_pTimeValueNode->Initialize( context );

        if ( m_pPlayInReverseValueNode != nullptr )
        {
            m_pPlayInReverseValueNode->Initialize( context );
        }

        // Initialize state data
        if ( m_pAnimation != nullptr )
        {
            m_duration = m_pAnimation->GetDuration();
        }

        // Set the initial time from the parameters
        SetCurrentTimeFromParameters( context );
        m_previousTime = m_currentTime;

        m_shouldPlayInReverse = false;
        m_isFirstUpdate = true;
        m_hasLooped = false;
    }

    void TimeControlledAnimationClipNode::ShutdownInternal( GraphContext& context )
    {
        if ( m_pPlayInReverseValueNode != nullptr )
        {
            m_pPlayInReverseValueNode->Shutdown( context );
        }

        m_pTimeValueNode->Shutdown( context );

        m_currentTime = m_previousTime = 0.0f;
        PoseNode::ShutdownInternal( context );
    }

    void TimeControlledAnimationClipNode::SetCurrentTimeFromParameters( GraphContext &context )
    {
        m_currentTime = Percentage( m_pTimeValueNode->GetValue<float>( context ) ).GetNormalizedTime();

        // Handle negative input values
        if ( m_currentTime < 0.0f )
        {
            m_currentTime = 1.0f - Math::Abs( m_currentTime );
        }

        EE_ASSERT( m_currentTime >= 0 && m_currentTime <= 1.0f );

        // Invert input time if we are in-reverse
        if ( m_shouldPlayInReverse )
        {
            m_currentTime = 1.0f - m_currentTime;
        }
    }

    GraphPoseNodeResult TimeControlledAnimationClipNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        EE_ASSERT( context.IsValid() && IsInitialized() );

        GraphPoseNodeResult result;
        if ( !IsValid() )
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
            return result;
        }

        MarkNodeActive( context );
        EE_ASSERT( m_currentTime >= 0.0f && m_currentTime <= 1.0f );

        //-------------------------------------------------------------------------

        m_hasLooped = false;

        // Synchronized Update
        #if EE_DEVELOPMENT_TOOLS
        if ( pUpdateRange != nullptr )
        {
            context.LogWarning( GetNodeIndex(), "Time controlled nodes ignore synchronization!" );
        }
        #endif

        // Unsynchronized Update
        {
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

            if ( m_pAnimation->IsSingleFrameAnimation() )
            {
                if ( m_isFirstUpdate )
                {
                    m_previousTime = 0.0f;
                    m_currentTime = 1.0f;
                }
                else
                {
                    m_previousTime = 1.0f;
                    m_currentTime = 1.0f;
                }
            }
            else
            {
                m_previousTime = m_currentTime;
                SetCurrentTimeFromParameters( context );

                // Check for looping
                if ( m_currentTime < m_previousTime )
                {
                    m_hasLooped = true;
                }
            }
        }

        return CalculateResult( context );
    }

    GraphPoseNodeResult TimeControlledAnimationClipNode::CalculateResult( GraphContext& context )
    {
        EE_ASSERT( m_pAnimation != nullptr );
        EE_ASSERT( m_currentTime.ToFloat() >= 0.0f && m_currentTime.ToFloat() <= 1.0f );

        GraphPoseNodeResult result;

        // Time
        //-------------------------------------------------------------------------

        Percentage actualAnimationSampleStartTime = m_previousTime;
        Percentage actualAnimationSampleEndTime = m_currentTime;

        // Invert times and swap the start and end times to create the correct sampling range for events
        if ( m_shouldPlayInReverse )
        {
            actualAnimationSampleEndTime = 1.0f - m_currentTime;
            actualAnimationSampleStartTime = 1.0f - m_previousTime;
        }

        // Events
        //-------------------------------------------------------------------------

        SampledEventsBuffer *pSampledEventsBuffer = context.GetSampledEventsBuffer();
        result.m_sampledEventRange = context.GetEmptySampledEventRange();

        Percentage const eventSampleStartTime = m_shouldPlayInReverse ? actualAnimationSampleEndTime : actualAnimationSampleStartTime;
        Percentage const eventSampleEndTime = m_shouldPlayInReverse ? actualAnimationSampleStartTime : actualAnimationSampleEndTime;

        // Get raw events
        if ( m_hasLooped )
        {
            AnimationClipNode::SampleEvents( context, this, m_pAnimation, eventSampleStartTime, 1.0f, m_shouldPlayInReverse );
            AnimationClipNode::SampleEvents( context, this, m_pAnimation, 0.0f, eventSampleEndTime, m_shouldPlayInReverse );

            // Remove any duration events that are duplicated
            // This can occur for really long duration events that end up getting sampled in both calls to the sampled event functions
            int32_t const nStartEventIdx = result.m_sampledEventRange.m_startIdx;
            int32_t const nEndEventIdx = pSampledEventsBuffer->GetNumSampledEvents() - 1;
            for ( int32_t i = nEndEventIdx; i >= nStartEventIdx; i-- )
            {
                for ( int32_t j = nStartEventIdx; j < i; j++ )
                {
                    if ( pSampledEventsBuffer->m_sampledEvents[i] == pSampledEventsBuffer->m_sampledEvents[j] )
                    {
                        pSampledEventsBuffer->m_sampledEvents.erase( pSampledEventsBuffer->m_sampledEvents.begin() + j );
                        break;
                    }
                }
            }
        }
        else
        {
            AnimationClipNode::SampleEvents( context, this, m_pAnimation, eventSampleStartTime, eventSampleEndTime, m_shouldPlayInReverse );
        }

        // Sample graph events
        auto pDefinition = GetDefinition<TimeControlledAnimationClipNode>();
        SourcePath const sourcePath = GetNodePath( context );
        for ( StringID const& ID : pDefinition->m_graphEvents )
        {
            context.GetSampledEventsBuffer()->EmplaceGraphEvent( sourcePath, GraphEventType::Generic, ID, context.m_branchState == BranchState::Active );
        }

        result.m_sampledEventRange.m_endIdx = context.GetSampledEventsBuffer()->GetNumSampledEvents();

        // Check for the Frame snap events
        //-------------------------------------------------------------------------

        bool shouldSnapToFrame = false;
        SnapToFrameEvent::SelectionMode frameSelectionMode = SnapToFrameEvent::SelectionMode::Floor;

        for ( int16_t evtIdx = result.m_sampledEventRange.m_startIdx; evtIdx < result.m_sampledEventRange.m_endIdx; evtIdx++ )
        {
            SampledEvent const &evt = context.GetSampledEventsBuffer()->GetEvent( evtIdx );
            if ( evt.IsAnimationEvent() )
            {
                if ( SnapToFrameEvent const *pSnapFrameEvent = evt.TryGetEvent<SnapToFrameEvent>() )
                {
                    shouldSnapToFrame = true;
                    frameSelectionMode = pSnapFrameEvent->GetFrameSelectionMode();
                    break;
                }
            }
        }

        // Root Motion
        //-------------------------------------------------------------------------

        if ( pDefinition->m_sampleRootMotion )
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
            else [[likely]]
            {
                result.m_rootMotionDelta = m_pAnimation->GetRootMotionDelta( m_previousTime, m_currentTime );
            }

            #if EE_DEVELOPMENT_TOOLS
            context.GetRootMotionDebugger()->RecordSampling( GetNodePath( context ), result.m_rootMotionDelta );
            #endif
        }

        // Register pose tasks
        //-------------------------------------------------------------------------

        Percentage sampleTime = m_shouldPlayInReverse ? Percentage( 1.0f - m_currentTime.ToFloat() ) : m_currentTime;

        if ( shouldSnapToFrame )
        {
            FrameTime const frameTime = m_pAnimation->GetFrameTime( sampleTime );
            uint32_t const frameIndex = ( frameSelectionMode == SnapToFrameEvent::SelectionMode::Round ) ? frameTime.GetNearestFrameIndex() : frameTime.GetLowerBoundFrameIndex();
            sampleTime = m_pAnimation->GetPercentageThrough( frameIndex );
        }

        result.m_taskIdx = context.GetTaskSystem()->RegisterTask<SampleTask>( GetNodePath( context ), m_pAnimation, sampleTime );

        //-------------------------------------------------------------------------

        m_isFirstUpdate = false;

        return result;
    }

    void TimeControlledAnimationClipNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_shouldPlayInReverse );
        outState.WriteValue( m_isFirstUpdate );
    }

    bool TimeControlledAnimationClipNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        if ( !PoseNode::RestoreGraphState( inState ) )
        {
            return false;
        }

        inState.ReadValue( m_shouldPlayInReverse );
        inState.ReadValue( m_isFirstUpdate );

        return true;
    }
}