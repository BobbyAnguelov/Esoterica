#include "Animation_RuntimeGraphNode_OrientationWarp.h"
#include "System/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void OrientationWarpNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<OrientationWarpNode>( context, options );
        context.SetNodePtrFromIndex( m_clipReferenceNodeIdx, pNode->m_pClipReferenceNode );
        context.SetNodePtrFromIndex( m_targetValueNodeIdx, pNode->m_pTargetValueNode );
    }

    void OrientationWarpNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pTargetValueNode != nullptr );
        PoseNode::InitializeInternal( context, initialTime );
        m_pClipReferenceNode->Initialize( context, initialTime );
        m_pTargetValueNode->Initialize( context );

        //-------------------------------------------------------------------------

        PerformWarp( context );
    }

    void OrientationWarpNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pTargetValueNode != nullptr );
        m_pTargetValueNode->Shutdown( context );
        m_pClipReferenceNode->Shutdown( context );
        PoseNode::ShutdownInternal( context );
    }

    //-------------------------------------------------------------------------

    void OrientationWarpNode::PerformWarp( GraphContext& context )
    {
        // Copy root motion
        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pAnimation != nullptr );
        auto const& originalRootMotion = pAnimation->GetRootMotion();
        m_warpedRootMotion = originalRootMotion;

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
        Seconds const warpRangeStartTime = Math::Max( pWarpEvent->GetStartTime().ToFloat(), currentTime.ToFloat() );
        Seconds const warpRangeLength = pWarpEvent->GetEndTime() - warpRangeStartTime;

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

        // Calculate the delta of the remaining root motion post warp, this is the section that we are going to be aligning to the new direction
        Vector postWarpOriginalDirCS = ( originalRootMotion.m_transforms.back().GetTranslation() - originalRootMotion.m_transforms[warpEndFrame].GetTranslation() ).GetNormalized2();
        if ( postWarpOriginalDirCS.IsZero3() )
        {
            postWarpOriginalDirCS = originalRootMotion.m_transforms.back().GetRotation().RotateVector( Vector::WorldForward );
        }

        // Calculate the target direction we need to align to
        Vector targetDirCS;

        auto pNodeSettings = GetSettings<OrientationWarpNode>();
        if ( pNodeSettings->m_isOffsetNode )
        {
            Radians const offset = m_pTargetValueNode->GetValue<float>( context ) * Math::DegreesToRadians;
            Quaternion const offsetRotation( AxisAngle( Float3::WorldUp, offset ) );

            if ( pNodeSettings->m_isOffsetRelativeToCharacter )
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
            targetDirCS = m_pTargetValueNode->GetValue<Vector>( context );
            targetDirCS.Normalize2();
        }

        // Calculate the desired modification we need to make
        Quaternion const desiredOrientationDelta = Quaternion::FromRotationBetweenNormalizedVectors( postWarpOriginalDirCS, targetDirCS );

        // Record debug info
        #if EE_DEVELOPMENT_TOOLS
        m_warpStartWorldTransform = context.m_worldTransform;
        m_debugCharacterOffsetPosWS = m_warpStartWorldTransform.GetTranslation() + context.m_worldTransform.RotateVector( originalRootMotion.m_transforms[warpEndFrame].GetTranslation() );
        m_debugTargetDirWS = context.m_worldTransform.RotateVector( targetDirCS );
        #endif

        // Perform warp
        //-------------------------------------------------------------------------

        m_warpedRootMotion.m_transforms.front() = context.m_worldTransform;

        // Set initial world space positions up to the end of the rotation warp event
        for ( auto i = 1; i <= warpEndFrame; i++ )
        {
            Transform const originalDelta = Transform::Delta( originalRootMotion.m_transforms[i - 1], originalRootMotion.m_transforms[i] );
            m_warpedRootMotion.m_transforms[i] = originalDelta * m_warpedRootMotion.m_transforms[i - 1];
        }

        // Spread out the delta rotation across the entire warp section
        for ( auto i = warpStartFrame + 1; i <= warpEndFrame; i++ )
        {
            float const percentage = float( i - warpStartFrame ) / numWarpFrames;
            Quaternion const frameDelta = Quaternion::SLerp( Quaternion::Identity, desiredOrientationDelta, percentage );
            Quaternion const adjustedRotation = frameDelta * m_warpedRootMotion.m_transforms[i].GetRotation();
            m_warpedRootMotion.m_transforms[i].SetRotation( adjustedRotation );
        }

        // Adjust the remaining root motion relative to the warped frames
        int32_t const numFrames = originalRootMotion.GetNumFrames();
        for ( auto i = warpEndFrame + 1; i < numFrames; i++ )
        {
            Transform const originalDelta = Transform::Delta( originalRootMotion.m_transforms[i - 1], originalRootMotion.m_transforms[i] );
            m_warpedRootMotion.m_transforms[i] = originalDelta * m_warpedRootMotion.m_transforms[i - 1];
        }
    }

    GraphPoseNodeResult OrientationWarpNode::Update( GraphContext& context )
    {
        auto pNodeSettings = GetSettings<OrientationWarpNode>();
        MarkNodeActive( context );

        auto result = m_pClipReferenceNode->Update( context );
        m_duration = m_pClipReferenceNode->GetDuration();
        m_previousTime = m_pClipReferenceNode->GetPreviousTime();
        m_currentTime = m_pClipReferenceNode->GetCurrentTime();

        // Sample warped root motion
        if ( pNodeSettings->m_samplingMode == RootMotionData::SamplingMode::WorldSpace )
        {
            result.m_rootMotionDelta = m_warpedRootMotion.SampleRootMotion( RootMotionData::SamplingMode::WorldSpace, context.m_worldTransform, m_previousTime, m_currentTime );
        }
        else
        {
            result.m_rootMotionDelta = m_warpedRootMotion.SampleRootMotion( RootMotionData::SamplingMode::Delta, context.m_worldTransform, m_previousTime, m_currentTime );
        }

        return result;
    }

    GraphPoseNodeResult OrientationWarpNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        auto pNodeSettings = GetSettings<OrientationWarpNode>();
        MarkNodeActive( context );

        auto result = m_pClipReferenceNode->Update( context, updateRange );
        m_duration = m_pClipReferenceNode->GetDuration();
        m_previousTime = m_pClipReferenceNode->GetPreviousTime();
        m_currentTime = m_pClipReferenceNode->GetCurrentTime();

        // Sample warped root motion
        if ( pNodeSettings->m_samplingMode == RootMotionData::SamplingMode::WorldSpace )
        {
            result.m_rootMotionDelta = m_warpedRootMotion.SampleRootMotion( RootMotionData::SamplingMode::WorldSpace, context.m_worldTransform, m_previousTime, m_currentTime );
        }
        else
        {
            result.m_rootMotionDelta = m_warpedRootMotion.SampleRootMotion( RootMotionData::SamplingMode::Delta, context.m_worldTransform, m_previousTime, m_currentTime );
        }

        return result;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void OrientationWarpNode::DrawDebug( GraphContext& graphContext, Drawing::DrawContext& drawCtx )
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