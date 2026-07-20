#include "Animation_RuntimeGraphNode_OrientationWarp.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void OrientationWarpNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<OrientationWarpNode>( context, options );
        context.SetNodePtrFromIndex( m_clipReferenceNodeIdx, pNode->m_pClipReferenceNode );
        context.SetNodePtrFromIndex( m_targetValueNodeIdx, pNode->m_pTargetValueNode );
    }

    bool OrientationWarpNode::IsValid() const
    {
        return PoseNode::IsValid() && m_pClipReferenceNode->IsValid();
    }

    void OrientationWarpNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pTargetValueNode != nullptr );

        PoseNode::InitializeInternal( context, initialTime );
        m_pClipReferenceNode->Initialize( context, initialTime );
        m_pTargetValueNode->Initialize( context );

        m_duration = m_pClipReferenceNode->GetDuration();
        m_previousTime = m_pClipReferenceNode->GetPreviousTime();
        m_currentTime = m_pClipReferenceNode->GetCurrentTime();

        //-------------------------------------------------------------------------

        if ( m_pClipReferenceNode->IsValid() )
        {
            m_duration = m_pClipReferenceNode->GetDuration();
            m_previousTime = m_pClipReferenceNode->GetPreviousTime();
            m_currentTime = m_pClipReferenceNode->GetCurrentTime();
            m_shouldUpdateWarp = true;
        }
        else
        {
            m_previousTime = m_currentTime = 0.0f;
            m_duration = 0;
            m_shouldUpdateWarp = false;
        }
    }

    void OrientationWarpNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pTargetValueNode != nullptr );

        m_pTargetValueNode->Shutdown( context );
        m_pClipReferenceNode->Shutdown( context );
        m_warpedRootMotion.Clear();
        m_shouldUpdateWarp = false;

        PoseNode::ShutdownInternal( context );
    }

    //-------------------------------------------------------------------------

    void OrientationWarpNode::PerformWarp( GraphContext& context )
    {
        m_warpedRootMotion.Clear();

        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pAnimation != nullptr );

        // Calculate Warp Range
        //-------------------------------------------------------------------------

        // Try to find warp event
        OrientationWarpEvent* pWarpEvent = nullptr;
        for ( auto const pEvent : pAnimation->GetEvents() )
        {
            pWarpEvent = TryCast<OrientationWarpEvent>( pEvent );
            if ( pWarpEvent != nullptr )
            {
                break;
            }
        }

        // Create warp range and validate it given the current start time
        if ( pWarpEvent == nullptr )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "No Orientation Warp Event Found!" );
            #endif
            return;
        }

        Seconds const currentTime = pAnimation->GetTime( m_pClipReferenceNode->GetCurrentTime() );
        Seconds const warpRangeStartTime = Math::Max( pAnimation->GetTime( pWarpEvent->GetStartTime() ).ToFloat(), currentTime.ToFloat() );
        Seconds const warpRangeLength = pAnimation->GetTime( pWarpEvent->GetEndTime() ) - warpRangeStartTime;

        constexpr float const minimumWarpPeriodAllowed = 1 / 30.0f;
        if ( warpRangeLength < minimumWarpPeriodAllowed )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Orientation Warp failed since there is not enough time left to warp or we're past the warp event!" );
            #endif
            return;
        }

        FrameTime const warpStartTime = pAnimation->GetFrameTime( warpRangeStartTime );
        int32_t const warpStartFrame = warpStartTime.GetNearestFrameIndex();

        FrameTime const warpEndTime = pAnimation->GetFrameTime( pWarpEvent->GetEndTime() );
        int32_t const warpEndFrame = warpEndTime.GetUpperBoundFrameIndex();
        int32_t const numWarpFrames = warpEndFrame - warpStartFrame;

        EE_ASSERT( numWarpFrames > 0 );

        // Calculate Targets
        //-------------------------------------------------------------------------

        RootMotionData const& originalRootMotion = pAnimation->GetRootMotion();
        EE_ASSERT( originalRootMotion.IsValid() );

        Vector postWarpOriginalDirCS = originalRootMotion.m_transforms.back().GetTranslation() - originalRootMotion.GetTransform( FrameTime( warpEndFrame ) ).GetTranslation();

        // If we dont have a valid direction just use the end orientation
        if ( postWarpOriginalDirCS.IsNearZero3() )
        {
            postWarpOriginalDirCS = originalRootMotion.back().GetRotation().RotateVector( Vector::WorldForward );
        }
        else
        {
            postWarpOriginalDirCS.Normalize2();
        }

        // Calculate the target direction we need to align to
        Vector targetDirCS;

        auto pNodeDefinition = GetDefinition<OrientationWarpNode>();
        if ( pNodeDefinition->m_isOffsetNode )
        {
            Radians const offset = m_pTargetValueNode->GetValue<float>( context ) * Math::DegreesToRadians;
            Quaternion const offsetRotation( AxisAngle( Float3::WorldUp, offset ) );

            if ( pNodeDefinition->m_isOffsetRelativeToCharacter )
            {
                targetDirCS = offsetRotation.RotateVector( Vector::WorldForward );
            }
            else
            {
                targetDirCS = offsetRotation.RotateVector( postWarpOriginalDirCS );
            }
        }
        else
        {
            targetDirCS = m_pTargetValueNode->GetValue<Float3>( context );

            if ( targetDirCS.IsNearZero3() )
            {
                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( GetNodeIndex(), "Orientation warp failed since there wasnt a valid target direction set!" );
                #endif
                return;
            }

            targetDirCS.Normalize2();
        }

        // Calculate the desired modification we need to make
        Quaternion const desiredOrientationDelta = Quaternion::FromRotationBetweenUnitVectors( postWarpOriginalDirCS, targetDirCS );

        // Perform warp
        //-------------------------------------------------------------------------

        // Record debug info
        #if EE_DEVELOPMENT_TOOLS
        m_warpStartWorldTransform = context.m_worldTransform;
        m_debugCharacterOffsetPosWS = m_warpStartWorldTransform.GetTranslation() + m_warpStartWorldTransform.RotateVector( originalRootMotion.GetTransform( FrameTime( warpEndFrame ) ).GetTranslation() );
        m_debugTargetDirWS = m_warpStartWorldTransform.RotateVector( targetDirCS );
        m_warpStartTime = currentTime;
        #endif

        if ( originalRootMotion.IsStationary() )
        {
            m_warpedRootMotion.m_numFrames = pAnimation->GetNumFrames();
            m_warpedRootMotion.m_transforms.resize( m_warpedRootMotion.m_numFrames );

            // Set initial world space positions up to the end of the rotation warp event
            for ( auto i = 0; i <= warpEndFrame; i++ )
            {
                m_warpedRootMotion.m_transforms[i] = context.m_worldTransform;
            }

            // Distribute the desired orientation delta among the warped frames
            for ( auto i = warpStartFrame + 1; i <= warpEndFrame; i++ )
            {
                float const percentage = float( i - warpStartFrame ) / numWarpFrames;
                Quaternion const frameDelta = Quaternion::SLerp( Quaternion::Identity, desiredOrientationDelta, percentage );
                Quaternion const adjustedRotation = m_warpedRootMotion.m_transforms[i].GetRotation() * frameDelta;
                m_warpedRootMotion.m_transforms[i].SetRotation( adjustedRotation );
            }

            // Set the remaining root motion to the result of the last frame
            for ( auto i = warpEndFrame + 1; i < m_warpedRootMotion.m_numFrames; i++ )
            {
                m_warpedRootMotion.m_transforms[i] = m_warpedRootMotion.m_transforms[warpEndFrame];
            }
        }
        else
        {
            m_warpedRootMotion = originalRootMotion;
            if ( originalRootMotion.IsStationary() )
            {
                m_warpedRootMotion.m_transforms.resize( originalRootMotion.m_numFrames, originalRootMotion.front() );
            }

            // Set start transform
            m_warpedRootMotion.front() = context.m_worldTransform;

            // Record debug info
            #if EE_DEVELOPMENT_TOOLS
            m_warpStartWorldTransform = m_warpedRootMotion.front();
            m_debugCharacterOffsetPosWS = m_warpStartWorldTransform.GetTranslation() + m_warpStartWorldTransform.RotateVector( originalRootMotion[warpEndFrame].GetTranslation() );
            m_debugTargetDirWS = m_warpStartWorldTransform.RotateVector( targetDirCS );
            m_warpStartTime = currentTime;
            #endif

            // Set initial world space positions up to the end of the rotation warp event
            for ( auto i = 1; i <= warpEndFrame; i++ )
            {
                Transform const originalDelta = Transform::Delta( originalRootMotion[i - 1], originalRootMotion[i] );
                m_warpedRootMotion[i] = originalDelta * m_warpedRootMotion[i - 1];
            }

            // Spread out the delta rotation across the entire warp section
            for ( auto i = warpStartFrame + 1; i <= warpEndFrame; i++ )
            {
                float const percentage = float( i - warpStartFrame ) / numWarpFrames;
                Quaternion const frameDelta = Quaternion::SLerp( Quaternion::Identity, desiredOrientationDelta, percentage );
                Quaternion const adjustedRotation = frameDelta * m_warpedRootMotion[i].GetRotation();
                m_warpedRootMotion[i].SetRotation( adjustedRotation );
            }

            // Adjust the remaining root motion relative to the warped frames
            int32_t const numFrames = originalRootMotion.GetNumFrames();
            for ( auto i = warpEndFrame + 1; i < numFrames; i++ )
            {
                Transform const originalDelta = Transform::Delta( originalRootMotion[i - 1], originalRootMotion[i] );
                m_warpedRootMotion[i] = originalDelta * m_warpedRootMotion[i - 1];
            }
        }
    }

    GraphPoseNodeResult OrientationWarpNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        MarkNodeActive( context );

        //-------------------------------------------------------------------------

        GraphPoseNodeResult result;

        if ( IsValid() )
        {
            // Calculate the warped root motion
            //-------------------------------------------------------------------------

            if ( m_shouldUpdateWarp )
            {
                PerformWarp( context );
                m_shouldUpdateWarp = false;
            }

            // Update source node
            //-------------------------------------------------------------------------

            result = m_pClipReferenceNode->Update( context, pUpdateRange );
            m_duration = m_pClipReferenceNode->GetDuration();
            m_previousTime = m_pClipReferenceNode->GetPreviousTime();
            m_currentTime = m_pClipReferenceNode->GetCurrentTime();

            // Sample warped root motion
            //-------------------------------------------------------------------------

            if ( m_warpedRootMotion.IsValid() )
            {
                auto pNodeDefinition = GetDefinition<OrientationWarpNode>();
                if ( pNodeDefinition->m_samplingMode == RootMotionData::SamplingMode::WorldSpace )
                {
                    result.m_rootMotionDelta = m_warpedRootMotion.SampleRootMotion( RootMotionData::SamplingMode::WorldSpace, context.m_worldTransform, m_previousTime, m_currentTime );
                }
                else
                {
                    result.m_rootMotionDelta = m_warpedRootMotion.SampleRootMotion( RootMotionData::SamplingMode::Delta, context.m_worldTransform, m_previousTime, m_currentTime );
                }
            }
        }
        else
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
        }

        return result;
    }

    //-------------------------------------------------------------------------

    void OrientationWarpNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_shouldUpdateWarp );
        outState.WriteValue( m_warpStartWorldTransform );
        outState.WriteValue( m_warpStartTime );

        #if EE_DEVELOPMENT_TOOLS
        outState.WriteValue( m_debugCharacterOffsetPosWS );
        outState.WriteValue( m_debugTargetDirWS );
        #endif

        //-------------------------------------------------------------------------

        outState.WriteValue( m_warpedRootMotion.m_numFrames );
        outState.WriteValue( m_warpedRootMotion.m_averageLinearVelocity );
        outState.WriteValue( m_warpedRootMotion.m_averageAngularVelocity );
        outState.WriteValue( m_warpedRootMotion.m_totalDelta );

        int32_t const numTransforms = (int32_t) m_warpedRootMotion.m_transforms.size();
        outState.WriteValue( numTransforms );
        for ( auto const &dt : m_warpedRootMotion.m_transforms )
        {
            outState.WriteValue( dt );
        }
    }

    bool OrientationWarpNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        if ( !PoseNode::RestoreGraphState( inState ) )
        {
            return false;
        }

        inState.ReadValue( m_shouldUpdateWarp );
        inState.ReadValue( m_warpStartWorldTransform );
        inState.ReadValue( m_warpStartTime );

        #if EE_DEVELOPMENT_TOOLS
        inState.ReadValue( m_debugCharacterOffsetPosWS );
        inState.ReadValue( m_debugTargetDirWS );
        #endif

        //-------------------------------------------------------------------------

        inState.ReadValue( m_warpedRootMotion.m_numFrames );
        inState.ReadValue( m_warpedRootMotion.m_averageLinearVelocity );
        inState.ReadValue( m_warpedRootMotion.m_averageAngularVelocity );
        inState.ReadValue( m_warpedRootMotion.m_totalDelta );

        int32_t numTransforms = 0;
        inState.ReadValue( numTransforms );

        m_warpedRootMotion.m_transforms.resize( numTransforms );
        for ( int32_t i = 0; i < numTransforms; i++ )
        {
            inState.ReadValue( m_warpedRootMotion.m_transforms[i] );
        }

        return true;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void OrientationWarpNode::DrawDebug( GraphContext& graphContext, DebugDrawContext& drawCtx )
    {
        if ( !IsValid() )
        {
            return;
        }

        // Draw start position and target direction
        drawCtx.DrawPoint( m_debugCharacterOffsetPosWS, Colors::LimeGreen, 10.0f );
        drawCtx.DrawArrow( m_debugCharacterOffsetPosWS, m_debugCharacterOffsetPosWS + ( m_debugTargetDirWS * 5.0f ), Colors::LimeGreen, 2.0f );

        // Draw original root motion
        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pAnimation != nullptr );
        pAnimation->GetRootMotion().DrawDebug( drawCtx, m_warpStartWorldTransform );

        // Draw warped root motion
        if ( m_warpedRootMotion.IsValid() )
        {
            m_warpedRootMotion.DrawDebug( drawCtx, m_warpStartWorldTransform, Colors::Red, Colors::Green );
        }
    }
    #endif
}