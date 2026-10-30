#include "Animation_RuntimeGraphNode_TargetWarp.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Animation/AnimationPose.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Types/ScopedValue.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void TargetWarpNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TargetWarpNode>( context, options );
        context.SetNodePtrFromIndex( m_clipReferenceNodeIdx, pNode->m_pClipReferenceNode );
        context.SetNodePtrFromIndex( m_targetValueNodeIdx, pNode->m_pTargetValueNode );

        if ( m_alignmentBoneID.IsValid() )
        {
            pNode->m_alignmentBoneIdx = context.m_pSkeleton->GetBoneIndex( m_alignmentBoneID );

            #if EE_DEVELOPMENT_TOOLS
            if ( pNode->m_alignmentBoneIdx == InvalidIndex )
            {
                context.LogWarning( "Cant find specified alignment bone ID ('%s') for target warp node", m_alignmentBoneID.c_str() );
            }
            #endif
        }
    }

    //-------------------------------------------------------------------------

    bool TargetWarpNode::IsValid() const
    {
        return PoseNode::IsValid() && m_pClipReferenceNode->IsValid() && m_duration > 0.0f;
    }

    void TargetWarpNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pTargetValueNode != nullptr );

        PoseNode::InitializeInternal( context, initialTime );

        m_pClipReferenceNode->Initialize( context, initialTime );
        m_pTargetValueNode->Initialize( context );

        //-------------------------------------------------------------------------

        if ( m_pClipReferenceNode->IsValid() )
        {
            m_duration = m_pClipReferenceNode->GetDuration();
            m_previousTime = m_pClipReferenceNode->GetPreviousTime();
            m_currentTime = m_pClipReferenceNode->GetCurrentTime();

            auto pDefinition = GetDefinition<TargetWarpNode>();

            AnimationClip const* pClip = m_pClipReferenceNode->GetAnimation();
            EE_ASSERT( pClip != nullptr );

            m_warpContext.InitializeFromClip( *pClip );
            m_warpContext.SetAlignTargetAtLastSection( pDefinition->m_alignWithTargetAtLastWarpEvent );

            // Calculate alignment bone based offset
            if ( m_alignmentBoneIdx != InvalidIndex )
            {
                int32_t const targetAlignmentFrame = ( m_warpContext.IsAlignTargetAtLastSection() && !m_warpContext.GetSections().empty() ) ? m_warpContext.GetSections().back().m_endFrame : pClip->GetNumFrames() - 1;
                TInlineVector<BoneChainElement, 20> const modelSpaceChain = pClip->GetModelSpaceTransform( FrameTime( targetAlignmentFrame ), m_alignmentBoneIdx );
                m_alignmentBoneRootTargetOffset = modelSpaceChain.back().m_modelSpaceTransform.GetInverse();
            }
            else
            {
                m_alignmentBoneRootTargetOffset = Transform::Identity;
            }

            #if EE_DEVELOPMENT_TOOLS
            if ( m_duration == 0.0f )
            {
                context.LogError( GetNodeIndex(), "Zero frame animations are not supported for warping!" );
            }
            #endif
        }
        else
        {
            m_previousTime = m_currentTime = 0.0f;
            m_duration = 0.0f;

            m_warpContext.Clear();
        }

        //-------------------------------------------------------------------------

        m_internalState = InternalState::RequiresInitialUpdate;
    }

    void TargetWarpNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        m_warpContext.Clear();

        m_pTargetValueNode->Shutdown( context );
        m_pClipReferenceNode->Shutdown( context );

        m_alignmentBoneRootTargetOffset = Transform::Identity;

        PoseNode::ShutdownInternal( context );
    }

    bool TargetWarpNode::TryReadTarget( GraphContext& context, Transform& outTarget )
    {
        Target const warpTarget = m_pTargetValueNode->GetValue<Target>( context );
        if ( !warpTarget.IsTargetSet() )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogError( GetNodeIndex(), "Invalid target detected for warp node!" );
            #endif

            return false;
        }

        if ( warpTarget.IsBoneTarget() )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogError( GetNodeIndex(), "Invalid target detected for warp node!" );
            #endif

            return false;
        }

        // Root motion warp is all about the root bone so adjust for the alignment bone here
        outTarget = m_alignmentBoneRootTargetOffset * warpTarget.GetTransform();

        return true;
    }

    TargetWarpNode::WarpedRootMotionUpdateType TargetWarpNode::ResolveWarpedRootMotionUpdateType( Transform const& targetTransform ) const
    {
        EE_ASSERT( IsValid() );

        auto pDefinition = GetDefinition<TargetWarpNode>();

        switch ( m_internalState )
        {
            case InternalState::RequiresInitialUpdate:
            {
                return WarpedRootMotionUpdateType::Recalculate;
            }

            case InternalState::Completed:
            {
                // We are done with the warp, but if the warp was successful we still need to offset the generated world space root motion
                if ( pDefinition->m_targetUpdateRule == TargetUpdateRule::Offset && m_warpContext.GetWarpedRootMotion().IsValid() )
                {
                    return WarpedRootMotionUpdateType::Offset;
                }

                return WarpedRootMotionUpdateType::None;
            }

            case InternalState::Failed:
            {
                return WarpedRootMotionUpdateType::None;
            }

            case InternalState::AllowUpdates:
            {
                auto pClip = m_pClipReferenceNode->GetAnimation();
                EE_ASSERT( pClip != nullptr );

                RootMotionWarpContext::Capability const warpCapability = m_warpContext.GetWarpCapabilities( pClip->GetFrameTime( m_pClipReferenceNode->GetCurrentTime() ) );

                if ( pDefinition->m_targetUpdateRule == TargetUpdateRule::RecalculateOrOffset )
                {
                    // If we are in recalculate or offset mode, don't recalculate if we can't guarantee reaching the requested target completely
                    if ( !warpCapability.m_canWarpXY || !warpCapability.m_canWarpZ )
                    {
                        return WarpedRootMotionUpdateType::Offset;
                    }
                }

                // We are allowed to recalculate, so let's see if we have reached any threshold to trigger recalculation
                bool shouldUpdate = false;

                if ( warpCapability.m_canWarpRotation )
                {
                    Radians const deltaAngle = Quaternion::Distance( targetTransform.GetRotation(), m_warpRootTargetTransform.GetRotation() );
                    shouldUpdate = deltaAngle > pDefinition->m_targetUpdateAngleThresholdRadians;
                }

                if ( !shouldUpdate && warpCapability.m_canWarpXY )
                {
                    float const deltaDistanceXY = targetTransform.GetTranslation().GetDistance2( m_warpRootTargetTransform.GetTranslation() );
                    shouldUpdate = deltaDistanceXY > pDefinition->m_targetUpdateDistanceThreshold;
                }

                if ( !shouldUpdate && warpCapability.m_canWarpZ )
                {
                    float const deltaDistanceZ = targetTransform.GetTranslation().GetZ() - m_warpRootTargetTransform.GetTranslation().GetZ();
                    shouldUpdate = deltaDistanceZ > pDefinition->m_targetUpdateDistanceThreshold;
                }

                if ( !shouldUpdate )
                {
                    return WarpedRootMotionUpdateType::None;
                }

                return WarpedRootMotionUpdateType::Recalculate;
            }
        }

        EE_UNREACHABLE_CODE();
        return WarpedRootMotionUpdateType::None;
    }

    void TargetWarpNode::UpdateWarpContext( GraphContext& context )
    {
        EE_ASSERT( IsValid() );

        auto pDefinition = GetDefinition<TargetWarpNode>();

        bool const isRecalculationAllowed = pDefinition->m_targetUpdateRule == TargetUpdateRule::Recalculate || pDefinition->m_targetUpdateRule == TargetUpdateRule::RecalculateOrOffset;

        // Read the target
        //-------------------------------------------------------------------------

        Transform currentTargetTransform;
        if ( !TryReadTarget( context, currentTargetTransform ) )
        {
            // Don't fail just yet if we are allowed to recalculate later
            if ( !isRecalculationAllowed )
            {
                m_internalState = InternalState::Failed;
            }
            return;
        }

        // Determine update type based on the internal state and the target
        //-------------------------------------------------------------------------

        WarpedRootMotionUpdateType const warpedRootMotionUpdateType = ResolveWarpedRootMotionUpdateType( currentTargetTransform );

        if ( warpedRootMotionUpdateType == WarpedRootMotionUpdateType::None )
        {
            return;
        }

        if ( warpedRootMotionUpdateType == WarpedRootMotionUpdateType::Offset )
        {
            // Just offset the root motion if the target changed slightly
            if ( !m_warpRootTargetTransform.IsNearEqual( currentTargetTransform ) )
            {
                m_warpContext.OffsetWarpedRootMotion( m_warpRootTargetTransform.GetInverse() * currentTargetTransform );
                m_warpRootTargetTransform = currentTargetTransform;
            }

            return;
        }

        EE_ASSERT( warpedRootMotionUpdateType == WarpedRootMotionUpdateType::Recalculate );

        m_warpRootTargetTransform = currentTargetTransform;

        // Set up the warp
        //-------------------------------------------------------------------------

        auto pClip = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pClip != nullptr );

        m_warpContext.SetStartTransform( context.m_worldTransform );
        m_warpContext.SetTargetTransform( m_warpRootTargetTransform );
        m_warpContext.SetStartTime( pClip->GetFrameTime( m_pClipReferenceNode->GetCurrentTime() ) );

        m_performWarp = true;
    }

    bool TargetWarpNode::UpdateWarp( GraphContext& context )
    {
        UpdateWarpContext( context );

        if ( !m_performWarp )
        {
            return false;
        }

        auto pDefinition = GetDefinition<TargetWarpNode>();

        #if EE_DEVELOPMENT_TOOLS
        m_warpContext.SetDebuggingEnabled( true );
        #else
        m_warpContext.SetDebuggingEnabled( false );
        #endif

        m_warpContext.SetAlignTargetAtLastSection( pDefinition->m_alignWithTargetAtLastWarpEvent );

        bool const warpSucceeded = RootMotionWarper::Warp( m_warpContext, pDefinition->m_maxTangentLength, pDefinition->m_lerpFallbackDistanceThreshold );

        m_performWarp = false;

        // Forward warper log messages
        #if EE_DEVELOPMENT_TOOLS
        for ( Log::Entry const& logMsg : m_warpContext.GetLog().GetLogEntries() )
        {
            if ( logMsg.m_severity == Severity::Error )
            {
                context.LogError( GetNodeIndex(), "Target Warp: %s", logMsg.m_message.c_str() );
            }
            else
            {
                EE_ASSERT( logMsg.m_severity == Severity::Warning );
                context.LogWarning( GetNodeIndex(), "Target Warp: %s", logMsg.m_message.c_str() );
            }
        }
        #endif

        bool const isRecalculationAllowed = pDefinition->m_targetUpdateRule == TargetUpdateRule::Recalculate || pDefinition->m_targetUpdateRule == TargetUpdateRule::RecalculateOrOffset;

        if ( warpSucceeded )
        {
            m_samplingMode = pDefinition->m_samplingMode;
            m_internalState = isRecalculationAllowed ? InternalState::AllowUpdates : InternalState::Completed;
            return true;
        }
        else
        {
            m_internalState = isRecalculationAllowed ? InternalState::AllowUpdates : InternalState::Failed;
            return false;
        }
    }

    void TargetWarpNode::SampleWarpedRootMotion( GraphContext& context, GraphPoseNodeResult& result )
    {
        EE_ASSERT( IsValid() );

        RootMotionData const& warpedRootMotion = m_warpContext.GetWarpedRootMotion();

        // If we failed to warp, just keep the original root motion delta
        if ( !warpedRootMotion.IsValid() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        // If we are sampling accurately then we need to match the exact world space position each update
        if ( m_samplingMode == RootMotionData::SamplingMode::WorldSpace )
        {
            auto pDefinition = GetDefinition<TargetWarpNode>();

            // Calculate error between current and expected position (only if the warp hasn't been updated this frame)
            Transform const expectedTransform = warpedRootMotion.GetTransform( m_previousTime );
            float const positionErrorSq = expectedTransform.GetTranslation().GetDistanceSquared3( context.m_worldTransform.GetTranslation() );
            if ( positionErrorSq <= pDefinition->m_samplingPositionErrorThresholdSq )
            {
                result.m_rootMotionDelta = warpedRootMotion.SampleRootMotion( RootMotionData::SamplingMode::WorldSpace, context.m_worldTransform, m_previousTime, m_currentTime );
            }
            else // Exceeded the error threshold, so fallback to inaccurate sampling
            {
                m_samplingMode = RootMotionData::SamplingMode::Delta;

                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( GetNodeIndex(), "Target warp exceed accurate sampling error threshold! Switching to inaccurate sampling!" );
                #endif
            }
        }

        // Just sample the delta and return that
        if ( m_samplingMode == RootMotionData::SamplingMode::Delta )
        {
            result.m_rootMotionDelta = warpedRootMotion.SampleRootMotion( RootMotionData::SamplingMode::Delta, context.m_worldTransform, m_previousTime, m_currentTime );
        }
    }

    //-------------------------------------------------------------------------

    GraphPoseNodeResult TargetWarpNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        MarkNodeActive( context );

        // Update source node
        //-------------------------------------------------------------------------

        GraphPoseNodeResult result;

        if ( IsValid() )
        {
            // During restore we are unable to initialize the context as we might not know the clip yet if the clip came from a selector
            // This method makes sure that the context gets initialized from the current clip if it is not
            EnsureWarpContextInitialized( context );

            // Generate the warped root motion if needed
            bool const wasWarpUpdated = UpdateWarp( context );

            // Step time on target node
            result = m_pClipReferenceNode->Update( context, pUpdateRange );
            m_duration = m_pClipReferenceNode->GetDuration();
            m_previousTime = m_pClipReferenceNode->GetPreviousTime();
            m_currentTime = m_pClipReferenceNode->GetCurrentTime();

            if ( wasWarpUpdated )
            {
                // Always update in world space whenever we update the warp to correct any sampling error due to mid-frame delta calculation
                // This is okay, since the update will be based on the current character position
                TScopedGuardValue const sgv( m_samplingMode, RootMotionData::SamplingMode::WorldSpace );
                SampleWarpedRootMotion( context, result );
            }
            else
            {
                SampleWarpedRootMotion( context, result );
            }
        }
        else
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
        }

        EE_ASSERT( m_duration != 0.0f );

        return result;
    }

    void TargetWarpNode::EnsureWarpContextInitialized( GraphContext& context )
    {
        EE_ASSERT( IsValid() );

        if ( m_warpContext.IsInitialized() )
        {
            return;
        }

        AnimationClip const* pClip = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pClip != nullptr );

        // Save the context parameters, so we can restore them after the initialization as it clears them
        FrameTime const startTime = m_warpContext.GetStartTime();
        Transform const startTransform = m_warpContext.GetStartTransform();
        Transform const targetTransform = m_warpContext.GetTargetTransform();

        m_warpContext.InitializeFromClip( *pClip );
        m_warpContext.SetStartTime( startTime );
        m_warpContext.SetStartTransform( startTransform );
        m_warpContext.SetTargetTransform( targetTransform );
    }

    //-------------------------------------------------------------------------

    void TargetWarpNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );

        // Node State
        //-------------------------------------------------------------------------

        outState.WriteValue( m_samplingMode );
        outState.WriteValue( m_internalState );
        outState.WriteValue( m_warpRootTargetTransform );
        outState.WriteValue( m_alignmentBoneIdx );
        outState.WriteValue( m_alignmentBoneRootTargetOffset );

        // Warp
        //-------------------------------------------------------------------------

        auto pClip = m_pClipReferenceNode->GetAnimation();
        bool const hasWarpedRootMotion = pClip != nullptr && m_warpContext.HasWarpedRootMotion();
        outState.WriteValue( hasWarpedRootMotion );

        if ( hasWarpedRootMotion )
        {
            // Record variables that will allow us to reconstruct the exact same warp later ( assuming the animation clip hasn't changed )
            FrameTime const& startTime = m_warpContext.GetStartTime();
            outState.WriteValue( startTime.GetFrameIndex() );
            outState.WriteValue( startTime.GetPercentageThrough() );
            outState.WriteValue( m_warpContext.GetTargetTransform() );
            outState.WriteValue( m_warpContext.GetStartTransform() );
        }
    }

    bool TargetWarpNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        if ( !PoseNode::RestoreGraphState( inState ) )
        {
            return false;
        }

        // Node State
        //-------------------------------------------------------------------------

        inState.ReadValue( m_samplingMode );
        inState.ReadValue( m_internalState );
        inState.ReadValue( m_warpRootTargetTransform );
        inState.ReadValue( m_alignmentBoneIdx );
        inState.ReadValue( m_alignmentBoneRootTargetOffset );

        // Warp
        //-------------------------------------------------------------------------

        bool hasWarpedRootMotion = false;
        inState.ReadValue( hasWarpedRootMotion );
        if ( hasWarpedRootMotion )
        {
            // Note that we don't initialize the context here as the animation might not be available if sourced through a selector node
            // Initialization happens later during the next update.
            int32_t startFrameIdx = 0;
            Percentage startFramePercentageThrough = 0.0f;
            inState.ReadValue( startFrameIdx );
            inState.ReadValue( startFramePercentageThrough );
            m_warpContext.m_startTime = FrameTime( startFrameIdx, startFramePercentageThrough );

            inState.ReadValue( m_warpContext.m_targetTransform );
            inState.ReadValue( m_warpContext.m_startTransform );

            // Signal that we want to perform a warp using the restored context
            m_performWarp = true;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void TargetWarpNode::DrawDebug( GraphContext& graphContext, DebugDrawContext& drawCtx )
    {
        if ( !IsValid() )
        {
            return;
        }

        m_warpContext.DrawDebug( drawCtx );
    }
    #endif
}
