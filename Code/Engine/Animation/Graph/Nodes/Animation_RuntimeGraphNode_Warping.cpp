#include "Animation_RuntimeGraphNode_Warping.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Contexts.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "System/Log.h"
#include "System/Drawing/DebugDrawing.h"
#include "System/Math/Curves.h"
#include "System/Math/MathHelpers.h"

//-------------------------------------------------------------------------
// Target Warp
//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void OrientationWarpNode::Settings::InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<OrientationWarpNode>( nodePtrs, options );
        SetNodePtrFromIndex( nodePtrs, m_clipReferenceNodeIdx, pNode->m_pClipReferenceNode );
        SetNodePtrFromIndex( nodePtrs, m_angleOffsetValueNodeIdx, pNode->m_pAngleOffsetValueNode );
    }

    void OrientationWarpNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pAngleOffsetValueNode != nullptr );
        PoseNode::InitializeInternal( context, initialTime );
        m_pClipReferenceNode->Initialize( context, initialTime );
        m_pAngleOffsetValueNode->Initialize( context );

        //-------------------------------------------------------------------------

        PerformWarp( context );
    }

    void OrientationWarpNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pAngleOffsetValueNode != nullptr );
        m_pAngleOffsetValueNode->Shutdown( context );
        m_pClipReferenceNode->Shutdown( context );
        PoseNode::ShutdownInternal( context );
    }

    void OrientationWarpNode::PerformWarp( GraphContext& context )
    {
        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pAnimation != nullptr );
    }

    Transform OrientationWarpNode::SampleWarpedRootMotion( GraphContext& context ) const
    {
        return Transform();
    }

    GraphPoseNodeResult OrientationWarpNode::Update( GraphContext& context )
    {
        auto result = m_pClipReferenceNode->Update( context );
        m_duration = m_pClipReferenceNode->GetDuration();
        m_previousTime = m_pClipReferenceNode->GetPreviousTime();
        m_currentTime = m_pClipReferenceNode->GetCurrentTime();

        result.m_rootMotionDelta = SampleWarpedRootMotion( context );
        return result;
    }

    GraphPoseNodeResult OrientationWarpNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        auto result = m_pClipReferenceNode->Update( context, updateRange );
        m_duration = m_pClipReferenceNode->GetDuration();
        m_previousTime = m_pClipReferenceNode->GetPreviousTime();
        m_currentTime = m_pClipReferenceNode->GetCurrentTime();

        result.m_rootMotionDelta = SampleWarpedRootMotion( context );
        return result;
    }
}

//-------------------------------------------------------------------------
// Target Warp
//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void TargetWarpNode::Settings::InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TargetWarpNode>( nodePtrs, options );
        SetNodePtrFromIndex( nodePtrs, m_clipReferenceNodeIdx, pNode->m_pClipReferenceNode );
        SetNodePtrFromIndex( nodePtrs, m_targetValueNodeIdx, pNode->m_pTargetValueNode );
    }

    void TargetWarpNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pTargetValueNode != nullptr );

        PoseNode::InitializeInternal( context, initialTime );
        m_pClipReferenceNode->Initialize( context, initialTime );
        m_pTargetValueNode->Initialize( context );

        //-------------------------------------------------------------------------

        auto pSettings = GetSettings<TargetWarpNode>();
        m_samplingMode = pSettings->m_samplingMode;
        m_warpStartTransform = context.m_worldTransform;

        //-------------------------------------------------------------------------

        if ( m_pClipReferenceNode->IsValid() )
        {
            auto pAnimation = m_pClipReferenceNode->GetAnimation();
            EE_ASSERT( pAnimation != nullptr );
            Percentage const& initialAnimationTime = m_pClipReferenceNode->GetSyncTrack().GetPercentageThrough( initialTime );
            if ( !CalculateWarpedRootMotion( context, initialAnimationTime ) )
            {
                m_warpSections.clear();
                m_warpedRootMotion.Clear();
            }
        }
    }

    void TargetWarpNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pTargetValueNode != nullptr );
        m_pTargetValueNode->Shutdown( context );
        m_pClipReferenceNode->Shutdown( context );

        m_warpedRootMotion.Clear();
        m_warpSections.clear();
        m_deltaTransforms.clear();
        m_inverseDeltaTransforms.clear();

        PoseNode::ShutdownInternal( context );
    }

    bool TargetWarpNode::TryReadTarget( GraphContext& context )
    {
        Target const warpTarget = m_pTargetValueNode->GetValue<Target>( context );
        if ( !warpTarget.IsTargetSet() )
        {
            EE_LOG_ERROR( "Animation", "Invalid target detected for warp node!" );
            return false;
        }

        if ( warpTarget.IsBoneTarget() )
        {
            EE_LOG_ERROR( "Animation", "Invalid target detected for warp node!" );
            return false;
        }

        m_warpTarget = warpTarget.GetTransform();
        return true;
    }

    bool TargetWarpNode::CalculateWarpedRootMotion( GraphContext& context, Percentage startTime )
    {
        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pAnimation != nullptr );
        int32_t const numFrames = (int32_t) pAnimation->GetNumFrames();
        EE_ASSERT( numFrames > 0 );
        RootMotionData const& originalRM = pAnimation->GetRootMotion();
        EE_ASSERT( originalRM.IsValid() );

        // Try Read Target
        //-------------------------------------------------------------------------

        if ( !TryReadTarget( context ) )
        {
            m_warpedRootMotion.Clear();
            return false;
        }

        // Read warp events
        //-------------------------------------------------------------------------

        bool motionAdjustmentEventFound = false;
        bool rotationEventFound = false;
        for ( auto const pEvent : pAnimation->GetEvents() )
        {
            auto pWarpEvent = Cast<WarpEvent>( pEvent );
            if ( pWarpEvent != nullptr )
            {
                WarpSection section;
                section.m_startFrame = pAnimation->GetFrameTime( pWarpEvent->GetStartTime() ).GetLowerBoundFrameIndex();
                section.m_endFrame = Math::Min( pAnimation->GetNumFrames(), pAnimation->GetFrameTime( pWarpEvent->GetEndTime() ).GetUpperBoundFrameIndex() );
                section.m_type = pWarpEvent->GetWarpAdjustmentType();
                section.m_translationWarpMode = pWarpEvent->GetTranslationWarpMode();

                motionAdjustmentEventFound = motionAdjustmentEventFound || ( section.m_type == WarpEvent::Type::Full );

                EE_ASSERT( section.m_startFrame < section.m_endFrame );
                m_warpSections.emplace_back( section );
            }
        }

        if ( !motionAdjustmentEventFound )
        {
            m_warpSections.clear();
            EE_LOG_ERROR( "Animation", "Warp attempted for animation with invalid warp events! %s", m_pClipReferenceNode->GetAnimation()->GetResourceID().c_str() );
            return false;
        }

        // Adjust sections to take into account the start time
        FrameTime const clipStartTime = pAnimation->GetFrameTime( startTime );
        int32_t const clipStartFrame = clipStartTime.GetFrameIndex();
        int32_t const minimumStartFrameForFirstSection = clipStartFrame + 1;
        if ( minimumStartFrameForFirstSection >= m_warpSections[0].m_startFrame )
        {
            int32_t startWarpSectionIdx = InvalidIndex;
            int32_t const numWarpSections = (int32_t) m_warpSections.size();
            for ( auto i = 0; i < numWarpSections; i++ )
            {
                // Ensure there's at least one frame of warping available for the section (i.e. start and end frame are different)
                if ( minimumStartFrameForFirstSection < m_warpSections[i].m_endFrame )
                {
                    startWarpSectionIdx = i;
                    break;
                }
            }

            if ( startWarpSectionIdx == InvalidIndex )
            {
                EE_LOG_ERROR( "Animation", "Warp failed since we started the animation after all the warp events! %s", m_pClipReferenceNode->GetAnimation()->GetResourceID().c_str() );
                return false;
            }

            // Remove irrelevant warp sections
            while ( startWarpSectionIdx > 0 )
            {
                m_warpSections.erase( m_warpSections.begin() );
                startWarpSectionIdx--;
            }

            for ( auto const& ws : m_warpSections )
            {
                motionAdjustmentEventFound = motionAdjustmentEventFound || ( ws.m_type == WarpEvent::Type::Full );
            }

            if ( !motionAdjustmentEventFound )
            {
                EE_LOG_ERROR( "Animation", "Warp failed since we started the animation at a later point and dont have the necessary events to perform the warp! %s", m_pClipReferenceNode->GetAnimation()->GetResourceID().c_str() );
                return false;
            }

            // Adjust first warp section frame range
            if ( minimumStartFrameForFirstSection > m_warpSections[0].m_startFrame )
            {
                m_warpSections[0].m_startFrame = minimumStartFrameForFirstSection;
            }
        }

        // Calculate deltas
        //-------------------------------------------------------------------------

        m_deltaTransforms.reserve( numFrames );
        m_inverseDeltaTransforms.reserve( numFrames );

        m_deltaTransforms.emplace_back( Transform::Identity );
        m_inverseDeltaTransforms.emplace_back( Transform::Identity );

        for ( auto i = 1; i < numFrames; i++ )
        {
            m_deltaTransforms.emplace_back( Transform::DeltaNoScale( originalRM.m_transforms[i - 1], originalRM.m_transforms[i] ) );
            m_inverseDeltaTransforms.emplace_back( m_deltaTransforms.back().GetInverse() );
        }

        // Calculate section data
        //-------------------------------------------------------------------------

        for ( auto& section : m_warpSections )
        {
            // Calculate the overall delta transform for the section
            section.m_deltaTransform = Transform::Delta( originalRM.m_transforms[section.m_startFrame], originalRM.m_transforms[section.m_endFrame] );

            // Add empty contribution for the first frame
            section.m_progressPerFrameAlongSection.emplace_back( 0 );

            // Calculate the distance deltas per frame
            section.m_length = 0;
            for ( auto i = section.m_startFrame + 1; i <= section.m_endFrame; i++ )
            {
                float const length = m_deltaTransforms[i].GetTranslation().GetLength3();
                section.m_progressPerFrameAlongSection.emplace_back( length );
                section.m_length += length;
            }

            if ( section.m_length > 0 )
            {
                // Normalize the distance contributions per frame and calculate cumulative progress
                for ( auto i = 1; i < section.m_progressPerFrameAlongSection.size(); i++ )
                {
                    section.m_progressPerFrameAlongSection[i] /= section.m_length;
                    section.m_progressPerFrameAlongSection[i] += section.m_progressPerFrameAlongSection[i - 1];
                }
            }
            else
            {
                for ( auto i = 1; i < section.m_progressPerFrameAlongSection.size(); i++ )
                {
                    section.m_progressPerFrameAlongSection[i] = 0;
                }
            }
        }

        // Prepare root motion for warping
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_actualStartTransform = m_warpStartTransform;
        #endif

        // Copy the original root motion into the warped one
        m_warpedRootMotion = originalRM;
        auto& warpedTransforms = m_warpedRootMotion.m_transforms;
        warpedTransforms.back() = m_warpTarget;

        if ( startTime != 0.0f )
        {
            // Offset the warp start transform by the distance we've 'already' covered due to our later start time
            Transform const expectedStartTransform = m_warpedRootMotion.GetTransform( startTime );
            m_warpStartTransform = expectedStartTransform.GetInverse() * m_warpStartTransform;
        }

        // Add start unwarped frames (till the anim start time)
        m_warpedRootMotion.m_transforms[0] = m_warpStartTransform;

        // Add start unwarped frames (from anim start time to first warp section)
        for ( int32_t frameIdx = 1; frameIdx <= m_warpSections[0].m_startFrame; frameIdx++ )
        {
            warpedTransforms[frameIdx] = m_deltaTransforms[frameIdx] * warpedTransforms[frameIdx - 1];
        }

        // Add trailing unwarped frames
        WarpSection const& lastWarpSection = m_warpSections.back();
        for ( int32_t frameIdx = numFrames - 2; frameIdx > lastWarpSection.m_endFrame; frameIdx-- )
        {
            warpedTransforms[frameIdx] = m_inverseDeltaTransforms[frameIdx] * warpedTransforms[frameIdx + 1];
        }

        // Set last warp section end transform
        Transform const lastWarpSectionEndTransform = m_inverseDeltaTransforms[lastWarpSection.m_endFrame] * warpedTransforms[lastWarpSection.m_endFrame + 1];

        // Calculate first section warp start transform
        //-------------------------------------------------------------------------

        Transform const animStartTransform = originalRM.GetTransform( startTime );
        Transform const& warpSectionStartTranform = originalRM.m_transforms[m_warpSections[0].m_startFrame];
        Transform const deltaStartTransform = Transform::Delta( animStartTransform, warpSectionStartTranform );
        Transform const firstSectionStartTransform = deltaStartTransform * m_warpStartTransform;

        // Calculate warp section sub-targets
        //-------------------------------------------------------------------------

        auto const& warpSection = m_warpSections[0];
        auto startTransform = m_deltaTransforms[warpSection.m_endFrame] * m_warpedRootMotion.m_transforms[warpSection.m_startFrame - 1]; // TODO
        auto endTransform = m_inverseDeltaTransforms[warpSection.m_endFrame] * m_warpedRootMotion.m_transforms[warpSection.m_endFrame + 1];
        WarpMotion( 0, startTransform, endTransform );

        return true;
    }

    void TargetWarpNode::WarpRotation( int32_t sectionIdx, Transform const& sectionStartTransform, Transform const& target )
    {
        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pAnimation != nullptr );

        RootMotionData const& originalRM = pAnimation->GetRootMotion();
        EE_ASSERT( originalRM.IsValid() );

        auto const& ws = m_warpSections[sectionIdx];
        EE_ASSERT( ws.m_type == WarpEvent::Type::RotationOnly );
        int32_t const numWarpFrames = ws.m_endFrame - ws.m_startFrame;

        // Get the original end transform relative to the new start transform
        Transform const originalEndTransform = ws.m_deltaTransform * sectionStartTransform;

        //-------------------------------------------------------------------------

        Vector targetPoint2D;
        Vector circleOrigin2D;
        Vector circleToTarget;
        Vector circleToOriginalEndPoint;
        Vector originalEndPoint2D;
        float radius = 0.0f;
        float distanceToTarget = 0.0f;

        // If this section has no motion or we have an invalid target, just rotate in place
        bool rotateInPlace = ( ws.m_length == 0 );
        if ( !rotateInPlace )
        {
            targetPoint2D = target.GetTranslation().Get2D();
            circleOrigin2D = sectionStartTransform.GetTranslation().Get2D();

            circleToTarget = targetPoint2D - circleOrigin2D;
            distanceToTarget = circleToTarget.GetLength2();

            // This defines the circle radius
            originalEndPoint2D = originalEndTransform.GetTranslation().Get2D();
            circleToOriginalEndPoint = originalEndPoint2D - circleOrigin2D;
            radius = circleToOriginalEndPoint.GetLength2();

            // If the target is inside the circle then we should rotate in place
            if ( distanceToTarget < radius )
            {
                rotateInPlace = true;
            }
        }

        // Perform rotation
        if ( rotateInPlace )
        {
            Quaternion const desiredDelta = Quaternion::Delta( originalEndTransform.GetRotation(), target.GetRotation() );
            
            // Set start transform
            m_warpedRootMotion.m_transforms[ws.m_startFrame] = sectionStartTransform;

            // Spread out the delta across the entire warp section
            for ( auto i = 1; i <= numWarpFrames; i++ )
            {
                int32_t const frameIdx = ws.m_startFrame + i;
                m_warpedRootMotion.m_transforms[frameIdx] = m_warpedRootMotion.m_transforms[ws.m_startFrame];

                float const percentage = float( i ) / numWarpFrames;
                Quaternion const frameDelta = Quaternion::SLerp( Quaternion::Identity, desiredDelta, percentage );
                m_warpedRootMotion.m_transforms[frameIdx].SetRotation( frameDelta * m_warpedRootMotion.m_transforms[frameIdx].GetRotation() );
            }
        }
        else
        {
            // We basically create a circle from the original start position of the section with radius (distance to the end position)
            // then we rotate the whole section so that the end orientation is in the direction of the target

            Vector const originalEndDirection = ( originalEndTransform.GetRotation().RotateVector( Vector::WorldForward ).Get2D() ).GetNormalized2();
            Vector const originalEndPointToCircleOriginDirection = ( -circleToOriginalEndPoint ).GetNormalized2();
            Radians const angleBetweenDirAndRadius = Math::GetAngleBetweenNormalizedVectors( originalEndPointToCircleOriginDirection, originalEndDirection );

            // Sine Rule
            float const sine = distanceToTarget / Math::Sin( angleBetweenDirAndRadius.ToFloat() );
            float const sineAngleBetweenNewRadiusLineAndTarget = radius / sine;
            Radians const angleBetweenNewRadiusLineAndTarget( Math::ASin( sineAngleBetweenNewRadiusLineAndTarget ) );

            Radians angleBetweenTargetAndRadius = Math::GetYawAngleBetweenVectors( -originalEndPointToCircleOriginDirection, circleToTarget.GetNormalized2() );
            Radians desiredDeltaAngle;
            if ( angleBetweenTargetAndRadius > 0 )
            {
                desiredDeltaAngle = angleBetweenTargetAndRadius - angleBetweenNewRadiusLineAndTarget;
            }
            else
            {
                desiredDeltaAngle = angleBetweenTargetAndRadius + angleBetweenNewRadiusLineAndTarget;
            }

            //-------------------------------------------------------------------------

            Quaternion const deltaRotation( AxisAngle( Vector::WorldUp, desiredDeltaAngle ) );

            Transform adjustedDelta = m_deltaTransforms[ws.m_startFrame];
            adjustedDelta.SetRotation( deltaRotation * adjustedDelta.GetRotation() );
            m_warpedRootMotion.m_transforms[ws.m_startFrame] = adjustedDelta * m_warpedRootMotion.m_transforms[ws.m_startFrame -1];

            for ( auto i = 1; i <= numWarpFrames; i++ )
            {
                int32_t const frameIdx = ws.m_startFrame + i;
                m_warpedRootMotion.m_transforms[frameIdx] = m_deltaTransforms[ws.m_startFrame] * m_warpedRootMotion.m_transforms[frameIdx - 1];
            }
        }
    }

    //-------------------------------------------------------------------------

    void TargetWarpNode::WarpTranslationBezier( WarpSection const& ws, Transform const& sectionStartTransform, Transform const& sectionEndTransform )
    {
        // Set start and end transforms
        m_warpedRootMotion.m_transforms[ws.m_startFrame] = sectionStartTransform;
        m_warpedRootMotion.m_transforms[ws.m_endFrame] = sectionEndTransform;

        //-------------------------------------------------------------------------

        int32_t const numWarpFrames = ws.m_endFrame - ws.m_startFrame;
        Vector const& startPoint = sectionStartTransform.GetTranslation();
        Vector const& endPoint = sectionEndTransform.GetTranslation();

        //-------------------------------------------------------------------------

        Vector const cp0 = m_warpedRootMotion.m_transforms[ws.m_startFrame].GetRotation().RotateVector( Vector::WorldForward ) + startPoint;
        Vector const cp1 = endPoint - m_warpedRootMotion.m_transforms[ws.m_endFrame].RotateVector( Vector::WorldForward );

        if ( ws.m_length > 0 )
        {
            for ( auto i = 1; i < numWarpFrames; i++ )
            {
                int32_t const frameIdx = ws.m_startFrame + i;
                m_warpedRootMotion.m_transforms[frameIdx].SetTranslation( Math::CubicBezier::Evaluate( startPoint, cp0, cp1, endPoint, ws.m_progressPerFrameAlongSection[i] ) );
            }
        }
        else
        {
            for ( auto i = 1; i < numWarpFrames; i++ )
            {
                int32_t const frameIdx = ws.m_startFrame + i;
                m_warpedRootMotion.m_transforms[frameIdx].SetTranslation( Math::CubicBezier::Evaluate( startPoint, cp0, cp1, endPoint, float( i ) / numWarpFrames ) );
            }
        }
    }

    void TargetWarpNode::WarpTranslationHermite( WarpSection const& ws, Transform const& sectionStartTransform, Transform const& sectionEndTransform )
    {
        // Set start and end transforms
        m_warpedRootMotion.m_transforms[ws.m_startFrame] = sectionStartTransform;
        m_warpedRootMotion.m_transforms[ws.m_endFrame] = sectionEndTransform;

        //-------------------------------------------------------------------------

        int32_t const numWarpFrames = ws.m_endFrame - ws.m_startFrame;
        Vector const& startPoint = sectionStartTransform.GetTranslation();
        Vector const& endPoint = sectionEndTransform.GetTranslation();

        //-------------------------------------------------------------------------

        auto t0 = m_warpedRootMotion.m_transforms[ws.m_startFrame].GetRotation().RotateVector( Vector::WorldForward );
        auto t1 = m_warpedRootMotion.m_transforms[ws.m_endFrame].RotateVector( Vector::WorldForward );

        for ( auto i = 1; i < numWarpFrames; i++ )
        {
            float const percentageAlongCurve = ws.m_progressPerFrameAlongSection[i];

            int32_t const frameIdx = ws.m_startFrame + i;
            m_warpedRootMotion.m_transforms[frameIdx].SetTranslation( Math::CubicHermite::Evaluate( startPoint, t0, endPoint, t1, percentageAlongCurve ) );
        }
    }

    void TargetWarpNode::WarpTranslationFeaturePreserving( WarpSection const& ws, Transform const& sectionStartTransform, Transform const& sectionEndTransform )
    {
        int32_t const numWarpFrames = ws.m_endFrame - ws.m_startFrame;

        // Split translation into separate curves
        //-------------------------------------------------------------------------

        struct CurvePoint
        {
            CurvePoint( Vector V ) : m_originalValue( V ), m_offsetValue( V ), m_lerpedValue( V ) {}

            Vector m_originalValue;
            Vector m_offsetValue;
            Vector m_lerpedValue;
        };

        TInlineVector<CurvePoint, 200> curve;
        curve.reserve( numWarpFrames + 1 );

        // Set start value
        Transform currentTransform = sectionStartTransform;
        curve.emplace_back( CurvePoint( currentTransform.GetTranslation() ) );

        // Create curve points
        for ( auto i = 1; i <= numWarpFrames; i++ )
        {
            currentTransform = m_deltaTransforms[i] * currentTransform;
            curve.emplace_back( CurvePoint( currentTransform.GetTranslation() ) );
        }

        Vector const& target = sectionEndTransform.GetTranslation();
        Vector const offset = target - curve.back().m_originalValue;

        for ( auto i = 0; i <= numWarpFrames; i++ )
        {
            curve[i].m_offsetValue += offset;
        }

        EE_ASSERT( curve.back().m_offsetValue.IsNearEqual3( target ) );
        curve.back().m_lerpedValue = target;

        // Warp curves
        //-------------------------------------------------------------------------

        for ( auto i = 1; i < numWarpFrames; i++ )
        {
            float const weight = float( i ) / ( numWarpFrames + 1 );
            curve[i].m_lerpedValue = Vector::Lerp( curve[i].m_originalValue, curve[i].m_offsetValue, weight );
        }

        // Recombine
        //-------------------------------------------------------------------------

        for ( auto i = 0; i <= numWarpFrames; i++ )
        {
            int32_t const frameIdx = ws.m_startFrame + i;
            m_warpedRootMotion.m_transforms[frameIdx].SetTranslation( curve[i].m_lerpedValue );
        }
    }

    void TargetWarpNode::WarpMotion( int32_t sectionIdx, Transform const& sectionStartTransform, Transform const& sectionEndTransform )
    {
        auto const& ws = m_warpSections[sectionIdx];
        EE_ASSERT( ws.m_type == WarpEvent::Type::Full );

        //-------------------------------------------------------------------------

        if ( ws.m_translationWarpMode == WarpEvent::TranslationWarpMode::Hermite )
        {
            WarpTranslationHermite( ws, sectionStartTransform, sectionEndTransform );
        }
        else if ( ws.m_translationWarpMode == WarpEvent::TranslationWarpMode::Bezier )
        {
            WarpTranslationBezier( ws, sectionStartTransform, sectionEndTransform );
        }
        else // Feature Preserving
        {
            WarpTranslationFeaturePreserving( ws, sectionStartTransform, sectionEndTransform );
        }

        // Try to maintain the original orientation relative to the motion for the new warped path
        //-------------------------------------------------------------------------

        for ( auto i = ws.m_startFrame + 1; i < ws.m_endFrame; i++ )
        {
            auto pAnimation = m_pClipReferenceNode->GetAnimation();
            EE_ASSERT( pAnimation != nullptr );

            RootMotionData const& originalRM = pAnimation->GetRootMotion();
            EE_ASSERT( originalRM.IsValid() );

            bool orientationSet = false;

            Vector const motionDirection = ( m_warpedRootMotion.m_transforms[i].GetTranslation() - m_warpedRootMotion.m_transforms[i - 1].GetTranslation() ).GetNormalized3();
            if ( !motionDirection.IsNearZero3() )
            {
                Vector const origMotionDirection = ( originalRM.m_transforms[i].GetTranslation() - originalRM.m_transforms[i - 1].GetTranslation() ).GetNormalized3();
                if ( !origMotionDirection.IsNearZero3() )
                {
                    auto origMotionOrientation = Quaternion::FromRotationBetweenNormalizedVectors( Vector::WorldForward, origMotionDirection );
                    auto d = Quaternion::Delta( origMotionOrientation, originalRM.m_transforms[i].GetRotation() );

                    auto currentMotionOrientation = Quaternion::FromRotationBetweenNormalizedVectors( Vector::WorldForward, motionDirection );

                    Quaternion newR = d * currentMotionOrientation;
                    m_warpedRootMotion.m_transforms[i].SetRotation( newR );
                    orientationSet = true;
                }
            }

            //-------------------------------------------------------------------------

            if ( !orientationSet )
            {
                int32_t const numWarpFrames = ws.m_endFrame - ws.m_startFrame;
                float const percentageThroughSection = float( i - ws.m_startFrame ) / numWarpFrames;
                m_warpedRootMotion.m_transforms[i].SetRotation( Quaternion::SLerp( sectionStartTransform.GetRotation(), sectionEndTransform.GetRotation(), percentageThroughSection));
            }
        }
    }

    void TargetWarpNode::UpdateWarp( GraphContext& context )
    {
        if ( !GetSettings<TargetWarpNode>()->m_allowTargetUpdate )
        {
            return;
        }

        // Cannot update an invalid warp
        if ( !m_warpedRootMotion.IsValid() )
        {
            return;
        }

        // Check if the target has changed
        //-------------------------------------------------------------------------

        Transform const previousTarget = m_warpTarget;

        if ( !TryReadTarget( context ) )
        {
            return;
        }

        // TODO: check for significant difference
        if ( m_warpTarget == previousTarget )
        {
            return;
        }

        // Recalculate the warp
        //-------------------------------------------------------------------------

        // TODO
    }

    //-------------------------------------------------------------------------

    GraphPoseNodeResult TargetWarpNode::Update( GraphContext& context )
    {
        MarkNodeActive( context );
        GraphPoseNodeResult result = m_pClipReferenceNode->Update( context );
        UpdateShared( context, result );
        return result;
    }

    GraphPoseNodeResult TargetWarpNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        MarkNodeActive( context );
        GraphPoseNodeResult result = m_pClipReferenceNode->Update( context, updateRange );
        UpdateShared( context, result );
        return result;
    }

    void TargetWarpNode::UpdateShared( GraphContext& context, GraphPoseNodeResult& result )
    {
        auto pSettings = GetSettings<TargetWarpNode>();

        m_duration = m_pClipReferenceNode->GetDuration();
        m_previousTime = m_pClipReferenceNode->GetPreviousTime();
        m_currentTime = m_pClipReferenceNode->GetCurrentTime();

        if ( !m_warpedRootMotion.IsValid() )
        {
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
            result.m_rootMotionDelta = Transform::Identity;
            return;
        }

        //-------------------------------------------------------------------------

        UpdateWarp( context );

        // If we are sampling accurately then we need to match the exact world space position each update
        if( m_samplingMode == SamplingMode::Accurate )
        {
            // Calculate error between current and expected position
            Transform const expectedTransform = m_warpedRootMotion.GetTransform( m_previousTime );
            float const positionErrorSq = expectedTransform.GetTranslation().GetDistanceSquared3( context.m_worldTransform.GetTranslation() );
            if( positionErrorSq <= pSettings->m_samplingPositionErrorThresholdSq )
            {
                Transform const desiredFinalTransform = m_warpedRootMotion.GetTransform( m_currentTime );
                result.m_rootMotionDelta = Transform::Delta( context.m_worldTransform, desiredFinalTransform );
            }
            else // Exceeded the error threshold, so fallback to inaccurate sampling
            {
                m_samplingMode = SamplingMode::Inaccurate;
            }
        }

        // Just sample the delta and return that
        if ( m_samplingMode == SamplingMode::Inaccurate )
        {
            result.m_rootMotionDelta = m_warpedRootMotion.GetDelta( m_pClipReferenceNode->GetPreviousTime(), m_pClipReferenceNode->GetCurrentTime() );
        }
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void TargetWarpNode::DrawDebug( GraphContext& graphContext, Drawing::DrawContext& drawCtx )
    {
        if ( !IsValid() )
        {
            return;
        }

        // Draw Target
        drawCtx.DrawAxis( m_warpTarget, 0.5f, 5.0f );
       
        // Draw Warped Root Motion
        if ( m_warpedRootMotion.IsValid() )
        {
            m_warpedRootMotion.DrawDebug( drawCtx, Transform::Identity );
        }

        for ( auto& p : m_test )
        {
            drawCtx.DrawPoint( p, Colors::HotPink );
        }

        // Draw Start Point
        drawCtx.DrawPoint( m_actualStartTransform.GetTranslation(), Colors::LimeGreen, 10.0f );
    }
    #endif
}