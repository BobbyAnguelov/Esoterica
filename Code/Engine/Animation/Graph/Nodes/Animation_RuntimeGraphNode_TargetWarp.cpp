#include "Animation_RuntimeGraphNode_TargetWarp.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "System/Log.h"
#include "System/Drawing/DebugDrawing.h"
#include "System/Math/Curves.h"
#include "System/Math/MathHelpers.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    static Quaternion CalculateWarpedOrientationForFrame( RootMotionData const& originalRM, RootMotionData const& warpedRM, int32_t frameIdx )
    {
        EE_ASSERT( frameIdx > 0 && frameIdx < originalRM.GetNumFrames() );

        // Calculate relative orientation in the original root motion
        Quaternion originalReferenceQuat;
        Vector const originalReferenceDirection = ( originalRM.m_transforms[frameIdx].GetTranslation() - originalRM.m_transforms[frameIdx - 1].GetTranslation() ).GetNormalized3();
        if ( !originalReferenceDirection.IsNearZero3() )
        {
            originalReferenceQuat = Quaternion::FromRotationBetweenNormalizedVectors( Vector::WorldForward, originalReferenceDirection );
        }
        else // Use the previous frame's orientation as a fallback
        {
            originalReferenceQuat = originalRM.m_transforms[frameIdx - 1].GetRotation();
        }

        Quaternion const originalRotationOffset = Quaternion::Delta( originalReferenceQuat, originalRM.m_transforms[frameIdx].GetRotation() );

        // Calculate reference direction for warped root motion
        Quaternion warpedReferenceQuat;
        Vector const warpedReferenceDirection = ( warpedRM.m_transforms[frameIdx].GetTranslation() - warpedRM.m_transforms[frameIdx - 1].GetTranslation() ).GetNormalized3();
        if ( !warpedReferenceDirection.IsNearZero3() )
        {
            warpedReferenceQuat = Quaternion::FromRotationBetweenNormalizedVectors( Vector::WorldForward, warpedReferenceDirection );
        }
        else // Use the previous frame's orientation as a fallback
        {
            warpedReferenceQuat = warpedRM.m_transforms[frameIdx - 1].GetRotation();
        }

        Quaternion const warpedOrientation = originalRotationOffset * warpedReferenceQuat;
        return warpedOrientation;
    }

    //-------------------------------------------------------------------------

    bool TargetWarpNode::IsValid() const
    {
        return PoseNode::IsValid() && m_pClipReferenceNode->IsValid();
    }

    void TargetWarpNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TargetWarpNode>( context, options );
        context.SetNodePtrFromIndex( m_clipReferenceNodeIdx, pNode->m_pClipReferenceNode );
        context.SetNodePtrFromIndex( m_targetValueNodeIdx, pNode->m_pTargetValueNode );
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
            EE_LOG_ERROR( "Animation", "TODO", "Invalid target detected for warp node!" );
            return false;
        }

        if ( warpTarget.IsBoneTarget() )
        {
            EE_LOG_ERROR( "Animation", "TODO", "Invalid target detected for warp node!" );
            return false;
        }

        m_warpTarget = warpTarget.GetTransform();
        return true;
    }

    //-------------------------------------------------------------------------

    void TargetWarpNode::WarpOrientation( WarpSection const& ws, Transform const& orientationTarget )
    {
        EE_ASSERT( ws.m_type == WarpEvent::Type::RotationOnly );

        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pAnimation != nullptr );

        RootMotionData const& originalRM = pAnimation->GetRootMotion();
        EE_ASSERT( originalRM.IsValid() );

        // Crete world space warped transforms
        //-------------------------------------------------------------------------

        // Set start transform
        Transform const sectionStartTransform = m_deltaTransforms[ws.m_startFrame] * m_warpedRootMotion.m_transforms[ws.m_startFrame - 1];
        m_warpedRootMotion.m_transforms[ws.m_startFrame] = sectionStartTransform;

        // Calculate all transforms for the section (we'll modify the orientation in a second pass)
        int32_t const numWarpFrames = ws.m_endFrame - ws.m_startFrame;
        Transform currentTransform = sectionStartTransform;
        for ( auto i = 1; i <= numWarpFrames; i++ )
        {
            int32_t const frameIdx = ws.m_startFrame + i;
            currentTransform = m_deltaTransforms[frameIdx] * currentTransform;
            m_warpedRootMotion.m_transforms[frameIdx] = currentTransform;
        }

        // Calculate the segment's end direction
        //-------------------------------------------------------------------------

        Transform const& sectionEndTransform = m_warpedRootMotion.m_transforms[ws.m_endFrame];
        Vector const& endPoint = sectionEndTransform.GetTranslation();
        Vector segmentEndDirection;

        // End movement direction - ideally use the direction of the next segment of the path to try to smooth out the motion
        if ( size_t( ws.m_endFrame + 1 ) < m_deltaTransforms.size() )
        {
            Transform const endPlusOne = m_deltaTransforms[ws.m_endFrame + 1] * sectionEndTransform;
            segmentEndDirection = ( endPlusOne.GetTranslation() - endPoint ).GetNormalized3();
        }
        else // This is the last segment so just use in the entry direction
        {
            Transform const endMinusOne = m_inverseDeltaTransforms[ws.m_endFrame - 1] * sectionEndTransform;
            segmentEndDirection = ( endPoint - endMinusOne.GetTranslation() ).GetNormalized3();
        }

        // If this segment has no direction, i.e., no displacement, then just use the character orientation
        if ( segmentEndDirection.IsNearZero3() )
        {
            segmentEndDirection = sectionEndTransform.GetRotation().RotateVector( Vector::WorldForward );
        }

        // Calculate desired delta rotation
        //-------------------------------------------------------------------------

        Vector const toTarget = ( orientationTarget.GetTranslation() - endPoint ).GetNormalized3();
        Quaternion const desiredOrientationDelta = Quaternion::FromRotationBetweenNormalizedVectors( segmentEndDirection, toTarget );

        // Warp orientations
        //-------------------------------------------------------------------------

        // Spread out the delta across the entire warp section
        for ( auto i = 1; i <= numWarpFrames; i++ )
        {
            int32_t const frameIdx = ws.m_startFrame + i;
            float const percentage = float( i ) / ( numWarpFrames + 1 );
            Quaternion const frameDelta = Quaternion::SLerp( Quaternion::Identity, desiredOrientationDelta, percentage );
            Quaternion const adjustedRotation = frameDelta * m_warpedRootMotion.m_transforms[frameIdx].GetRotation();
            m_warpedRootMotion.m_transforms[frameIdx].SetRotation( adjustedRotation );
        }
    }

    //-------------------------------------------------------------------------

    void TargetWarpNode::WarpTranslationBezier( WarpSection const& ws, Transform const& sectionEndTransform )
    {
        EE_ASSERT( ws.m_type == WarpEvent::Type::Full && ws.HasValidFrameRange() );

        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pAnimation != nullptr );

        RootMotionData const& originalRM = pAnimation->GetRootMotion();
        EE_ASSERT( originalRM.IsValid() );

        //-------------------------------------------------------------------------

        // Set start and end transforms
        Transform const sectionStartTransform = m_deltaTransforms[ws.m_startFrame] * m_warpedRootMotion.m_transforms[ws.m_startFrame - 1];
        m_warpedRootMotion.m_transforms[ws.m_startFrame] = sectionStartTransform;
        m_warpedRootMotion.m_transforms[ws.m_endFrame] = sectionEndTransform;

        //-------------------------------------------------------------------------

        Vector const& startPoint = sectionStartTransform.GetTranslation();
        Vector const& endPoint = sectionEndTransform.GetTranslation();
        Vector const startTangent = ( ( m_deltaTransforms[ws.m_startFrame + 1] * sectionStartTransform ).GetTranslation() - sectionStartTransform.GetTranslation() ).GetNormalized3();
        Vector const endTangent = ( ( m_inverseDeltaTransforms[ws.m_endFrame] * sectionEndTransform ).GetTranslation() - sectionEndTransform.GetTranslation() ).GetNormalized3();

        // TODO: investigate bias lengths
        Vector const cp0 = startPoint + startTangent;
        Vector const cp1 = endPoint + endTangent;

        #if EE_DEVELOPMENT_TOOLS
        ws.m_debugPoints[0] = startPoint;
        ws.m_debugPoints[1] = cp0;
        ws.m_debugPoints[2] = cp1;
        ws.m_debugPoints[3] = endPoint;
        #endif

        int32_t const numWarpFrames = ws.m_endFrame - ws.m_startFrame;
        for ( auto i = 1; i < numWarpFrames; i++ )
        {
            int32_t const frameIdx = ws.m_startFrame + i;

            Vector const curvePoint = Math::CubicBezier::GetPoint( startPoint, cp0, cp1, endPoint, ws.m_progressPerFrameAlongSection[i] );
            m_warpedRootMotion.m_transforms[frameIdx].SetTranslation( curvePoint );

            Quaternion const orientation = CalculateWarpedOrientationForFrame( originalRM, m_warpedRootMotion, frameIdx );
            m_warpedRootMotion.m_transforms[frameIdx].SetRotation( orientation );
        }
    }

    void TargetWarpNode::WarpTranslationHermite( WarpSection const& ws, Transform const& sectionEndTransform )
    {
        EE_ASSERT( ws.m_type == WarpEvent::Type::Full && ws.HasValidFrameRange() );

        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pAnimation != nullptr );

        RootMotionData const& originalRM = pAnimation->GetRootMotion();
        EE_ASSERT( originalRM.IsValid() );

        //-------------------------------------------------------------------------

        // Set start and end transforms
        Transform const sectionStartTransform = m_deltaTransforms[ws.m_startFrame] * m_warpedRootMotion.m_transforms[ws.m_startFrame - 1];
        m_warpedRootMotion.m_transforms[ws.m_startFrame] = sectionStartTransform;
        m_warpedRootMotion.m_transforms[ws.m_endFrame] = sectionEndTransform;

        //-------------------------------------------------------------------------

        Vector const& startPoint = sectionStartTransform.GetTranslation();
        Vector const& endPoint = sectionEndTransform.GetTranslation();
        Vector const startTangent = ( ( m_deltaTransforms[ws.m_startFrame + 1] * sectionStartTransform ).GetTranslation() - sectionStartTransform.GetTranslation() ).GetNormalized3();
        Vector const endTangent = ( sectionEndTransform.GetTranslation() - ( m_inverseDeltaTransforms[ws.m_endFrame] * sectionEndTransform ).GetTranslation() ).GetNormalized3();

        #if EE_DEVELOPMENT_TOOLS
        ws.m_debugPoints[0] = startPoint;
        ws.m_debugPoints[1] = startTangent;
        ws.m_debugPoints[2] = endPoint;
        ws.m_debugPoints[3] = endTangent;
        #endif

        //-------------------------------------------------------------------------

        int32_t const numWarpFrames = ws.m_endFrame - ws.m_startFrame;
        for ( auto i = 1; i < numWarpFrames; i++ )
        {
            int32_t const frameIdx = ws.m_startFrame + i;

            Vector const curvePoint = Math::CubicHermite::GetPoint( startPoint, startTangent, endPoint, endTangent, ws.m_progressPerFrameAlongSection[i] );
            m_warpedRootMotion.m_transforms[frameIdx].SetTranslation( curvePoint );

            Quaternion const orientation = CalculateWarpedOrientationForFrame( originalRM, m_warpedRootMotion, frameIdx );
            m_warpedRootMotion.m_transforms[frameIdx].SetRotation( orientation );
        }
    }

    void TargetWarpNode::WarpTranslationFeaturePreserving( WarpSection const& ws, Transform const& sectionEndTransform )
    {
        EE_ASSERT( ws.m_type == WarpEvent::Type::Full && ws.HasValidFrameRange() );

        Transform const sectionStartTransform = m_deltaTransforms[ws.m_startFrame] * m_warpedRootMotion.m_transforms[ws.m_startFrame - 1];

        // Calculate the straight path that describes the total displacement of this section
        //-------------------------------------------------------------------------

        // Get the original endpoint of the root motion relative to the current world space position
        Vector const originalEndPoint = ( ws.m_deltaTransform * sectionStartTransform ).GetTranslation();

        // Calculate the overall displacement for this section
        Vector sectionDeltaDirection;
        float sectionDeltaDistance;
        ( originalEndPoint - sectionStartTransform.GetTranslation() ).ToDirectionAndLength3( sectionDeltaDirection, sectionDeltaDistance );

        // If we have no displacement then no matter what we do it's gonna be weird so just use the hermite interp
        if ( Math::IsNearZero( sectionDeltaDistance ) )
        {
            return WarpTranslationBezier( ws, sectionEndTransform );
        }

        // Calculate curve parameters
        //-------------------------------------------------------------------------

        Vector const startPoint = sectionStartTransform.GetTranslation();
        Vector const endPoint = sectionEndTransform.GetTranslation();
        Vector endDirection;

        // End direction - ideally use the direction of the next segment of the path to try to smooth out the motion
        if( size_t( ws.m_endFrame + 1 ) < m_deltaTransforms.size() )
        {
            Transform const endPlusOne = m_deltaTransforms[ws.m_endFrame + 1] * sectionEndTransform;
            endDirection = ( endPlusOne.GetTranslation() - endPoint ).GetNormalized3();
        }
        else // This is the last segment so just use in the entry direction
        {
            Transform const endMinusOne = m_inverseDeltaTransforms[ws.m_endFrame - 1] * sectionEndTransform;
            endDirection = ( endPoint - endMinusOne.GetTranslation() ).GetNormalized3();
        }

        if ( endDirection.IsNearZero3() )
        {
            endDirection = sectionEndTransform.GetForwardVector();
        }

        float const curveLength = Math::CubicHermite::GetSplineLength( startPoint, sectionDeltaDirection, endPoint, endDirection );
        float const scaleFactor = curveLength / sectionDeltaDistance;

        // Create curve
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        ws.m_debugPoints[0] = startPoint;
        ws.m_debugPoints[1] = sectionDeltaDirection;
        ws.m_debugPoints[2] = endPoint;
        ws.m_debugPoints[3] = endDirection;
        #endif

        Quaternion const sectionDeltaOrientation = Quaternion::FromRotationBetweenNormalizedVectors( Vector::WorldForward, sectionDeltaDirection );
        Vector const scaledOriginalEndPoint = originalEndPoint * scaleFactor;
        int32_t const numWarpFrames = ws.m_endFrame - ws.m_startFrame;

        Transform currentTransform = sectionStartTransform;
        for ( auto i = 1; i <= numWarpFrames; i++ )
        {
            int32_t const frameIdx = ws.m_startFrame + i;
            float const percentageAlongCurve = ws.m_progressPerFrameAlongSection[i];

            // Calculate original path delta from the displacement line
            currentTransform = m_deltaTransforms[frameIdx] * currentTransform;
            Transform const scaledCurrentTransform( currentTransform.GetRotation(), currentTransform.GetTranslation() * scaleFactor );
            Transform const straightTransform( sectionDeltaOrientation, Vector::Lerp( startPoint, scaledOriginalEndPoint, percentageAlongCurve ) );
            Transform const curveDelta = Transform::Delta( straightTransform, scaledCurrentTransform );

            // Calculate new curved path
            auto const pt = Math::CubicHermite::GetPointAndTangent( startPoint, sectionDeltaDirection, endPoint, endDirection, percentageAlongCurve );
            Quaternion const curveTangentOrientation = Quaternion::FromRotationBetweenNormalizedVectors( Vector::WorldForward, pt.m_tangent.GetNormalized3() );
            Transform const curveTangentTransform( curveTangentOrientation, pt.m_point );

            // Calculate final path point
            Transform const finalTransform = curveDelta * curveTangentTransform;
            m_warpedRootMotion.m_transforms[frameIdx] = finalTransform;
        }
    }

    //-------------------------------------------------------------------------

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
                section.m_startFrame = pAnimation->GetFrameTime( pWarpEvent->GetStartTime() ).GetNearestFrameIndex();
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
            EE_LOG_ERROR( "Animation", "TODO", "Warp attempted for animation with invalid warp events! %s", m_pClipReferenceNode->GetAnimation()->GetResourceID().c_str() );
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
                EE_LOG_ERROR( "Animation", "TODO", "Warp failed since we started the animation after all the warp events! %s", m_pClipReferenceNode->GetAnimation()->GetResourceID().c_str() );
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
                EE_LOG_ERROR( "Animation", "TODO", "Warp failed since we started the animation at a later point and dont have the necessary events to perform the warp! %s", m_pClipReferenceNode->GetAnimation()->GetResourceID().c_str() );
                return false;
            }

            // Adjust first warp section frame range
            if ( minimumStartFrameForFirstSection > m_warpSections[0].m_startFrame )
            {
                m_warpSections[0].m_startFrame = minimumStartFrameForFirstSection;
            }
        }

        // Calculate per-frame root motion deltas
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

        // Calculate section info
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
                    section.m_progressPerFrameAlongSection[i] = Math::Min( section.m_progressPerFrameAlongSection[i], 1.0f );
                }
            }
            else // Each frame has the exact same contribution
            {
                uint32_t const numWarpFrames = (uint32_t) section.m_progressPerFrameAlongSection.size();
                float const contributionPerFrame = 1.0f / ( numWarpFrames - 1 );
                for ( auto i = 1u; i < numWarpFrames - 1; i++ )
                {
                    section.m_progressPerFrameAlongSection[i] = section.m_progressPerFrameAlongSection[i - 1] + contributionPerFrame;
                }

                section.m_progressPerFrameAlongSection.back() = 1.0f;
            }
        }

        // Prepare for warping and handle start time
        //-------------------------------------------------------------------------

        // Create storage for warped root motion
        m_warpedRootMotion.Clear();
        auto& warpedTransforms = m_warpedRootMotion.m_transforms;
        warpedTransforms.resize( originalRM.GetNumFrames() );

        #if EE_DEVELOPMENT_TOOLS
        m_characterStartTransform = m_warpStartTransform;
        #endif

        // Handle the case were we dont start the animation from the first frame
        if ( startTime != 0.0f )
        {
            // Offset the warp start transform by the distance we've 'already' covered due to our later start time
            Transform const rootTransformAtActualStartTime = originalRM.GetTransform( startTime );
            m_warpStartTransform = rootTransformAtActualStartTime.GetInverse() * m_warpStartTransform;

            // Offset the first warp section if it overlaps the new start time
            FrameTime const startFrameTime = pAnimation->GetFrameTime( startTime );
            if ( m_warpSections[0].m_startFrame <= (int32_t) startFrameTime.GetFrameIndex() )
            {
                m_warpSections[0].m_startFrame = startFrameTime.GetFrameIndex() + 1;
            }
        }

        // Add the unwarped start and end portions of the root motion
        //-------------------------------------------------------------------------

        // Always set the first and last frames
        warpedTransforms.front() = m_warpStartTransform;
        warpedTransforms.back() = m_warpTarget;

        // Add start unwarped frames (till the anim start time)
        m_warpedRootMotion.m_transforms[0] = m_warpStartTransform;

        // Add start unwarped frames (from anim start time to first warp section)
        for ( int32_t frameIdx = 1; frameIdx < m_warpSections[0].m_startFrame; frameIdx++ )
        {
            warpedTransforms[frameIdx] = m_deltaTransforms[frameIdx] * warpedTransforms[frameIdx - 1];
        }

        // Add trailing unwarped frames
        WarpSection const& lastWarpSection = m_warpSections.back();
        for ( int32_t frameIdx = numFrames - 2; frameIdx > lastWarpSection.m_endFrame; frameIdx-- )
        {
            warpedTransforms[frameIdx] = m_inverseDeltaTransforms[frameIdx] * warpedTransforms[frameIdx + 1];
        }

        // Calculate the final warp target for the last warp section
        Transform const finalWarpTarget = m_inverseDeltaTransforms[lastWarpSection.m_endFrame] * warpedTransforms[lastWarpSection.m_endFrame + 1];

        // Orientation Warp
        //-------------------------------------------------------------------------

        int32_t warpSectionIdx = 0;
        WarpSection const& firstWarpSection = m_warpSections.front();
        if ( firstWarpSection.m_type == WarpEvent::Type::RotationOnly )
        {
            WarpOrientation( m_warpSections[0], finalWarpTarget );
            warpSectionIdx++;
        }

        // Translation Warp
        //-------------------------------------------------------------------------

        // Fill any gaps between orientation event and the first translation event
        for ( int32_t frameIdx = m_warpSections[warpSectionIdx-1].m_endFrame + 1; frameIdx < m_warpSections[warpSectionIdx].m_startFrame; frameIdx++ )
        {
            warpedTransforms[frameIdx] = m_deltaTransforms[frameIdx] * warpedTransforms[frameIdx - 1];
        }

        // Translation warp
        auto const& warpSection = m_warpSections[warpSectionIdx];
        if ( warpSection.m_translationWarpMode == WarpEvent::TranslationWarpMode::Hermite )
        {
            WarpTranslationHermite( warpSection, finalWarpTarget );
        }
        else if ( warpSection.m_translationWarpMode == WarpEvent::TranslationWarpMode::Bezier )
        {
            WarpTranslationBezier( warpSection, finalWarpTarget );
        }
        else // Feature Preserving
        {
            WarpTranslationFeaturePreserving( warpSection, finalWarpTarget );
        }

        return true;
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

        //// Draw Start Point
        //drawCtx.DrawPoint( m_actualStartTransform.GetTranslation(), Colors::LimeGreen, 10.0f );

        for ( auto i = 0u; i < m_warpSections.size(); i++ )
        {
            auto const& ws = m_warpSections[i];

            if ( ws.m_translationWarpMode == WarpEvent::TranslationWarpMode::Bezier )
            {
                // Four points
                drawCtx.DrawArrow( ws.m_debugPoints[0], ws.m_debugPoints[1], Colors::Yellow, 2.0f );
                drawCtx.DrawArrow( ws.m_debugPoints[3], ws.m_debugPoints[2], Colors::Cyan, 2.0f );
            }
            else
            {
                // Two points and two tangents
                drawCtx.DrawArrow( ws.m_debugPoints[0], ws.m_debugPoints[0] + ws.m_debugPoints[1], Colors::Yellow, 2.0f );
                drawCtx.DrawArrow( ws.m_debugPoints[2], ws.m_debugPoints[2] + ws.m_debugPoints[3], Colors::Cyan, 2.0f );
            }
        }
    }
    #endif
}