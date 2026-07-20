#include "Animation_RuntimeGraphNode_TargetWarp.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Animation/AnimationPose.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Math/Curves.h"
#include "Base/Math/MathUtils.h"
#include "Base/Types/ScopedValue.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    constexpr float const g_curveBlowoutDetectionThreshold = 2.5f;
    constexpr float const g_minAllowedRootMotionScale = 0.05f;

    //-------------------------------------------------------------------------

    static Quaternion CalculateWarpedOrientationReferenceQuat( RootMotionData const &rootMotion, int32_t frameIdx )
    {
        EE_ASSERT( frameIdx > 0 && frameIdx < rootMotion.GetNumFrames() );

        Vector const referenceDirection = ( rootMotion.m_transforms[frameIdx].GetTranslation() - rootMotion.m_transforms[frameIdx - 1].GetTranslation() ).GetNormalized2();
        if ( referenceDirection.IsZero2() )
        {
            // Use the previous frame's orientation as a fallback
            return rootMotion.m_transforms[frameIdx - 1].GetRotation();
        }

        if ( Math::IsNearEqual( referenceDirection.GetDot3( Vector::WorldForward ), -1.0f, 1e-3f ) )
        {
            return Quaternion::LookAt( referenceDirection, rootMotion.m_transforms[frameIdx].GetUpVector() );
        }

        return Quaternion::FromRotationBetweenUnitVectors( Vector::WorldForward, referenceDirection );
    }

    static Quaternion CalculateWarpedOrientationForFrame( RootMotionData const& originalRM, RootMotionData const& warpedRM, int32_t frameIdx )
    {
        EE_ASSERT( frameIdx > 0 && frameIdx < originalRM.GetNumFrames() );

        // Calculate relative orientation in the original root motion
        Quaternion const originalReferenceQuat = CalculateWarpedOrientationReferenceQuat( originalRM, frameIdx );
        Quaternion const originalRotationOffset = Quaternion::Delta( originalReferenceQuat, originalRM.m_transforms[frameIdx].GetRotation() );

        // Calculate reference direction for warped root motion
        Quaternion const warpedReferenceQuat = CalculateWarpedOrientationReferenceQuat( warpedRM, frameIdx );

        Quaternion const warpedOrientation = originalRotationOffset * warpedReferenceQuat;
        return warpedOrientation;
    }

    //-------------------------------------------------------------------------

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

        auto pDefinition = GetDefinition<TargetWarpNode>();
        m_pClipReferenceNode->Initialize( context, initialTime );
        m_pTargetValueNode->Initialize( context );

        //-------------------------------------------------------------------------

        if ( m_pClipReferenceNode->IsValid() )
        {
            m_duration = m_pClipReferenceNode->GetDuration();
            m_previousTime = m_pClipReferenceNode->GetPreviousTime();
            m_currentTime = m_pClipReferenceNode->GetCurrentTime();

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
        }

        //-------------------------------------------------------------------------

        ClearWarpInfo();
        m_internalState = InternalState::RequiresInitialUpdate;
    }

    void TargetWarpNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        ClearWarpInfo();
        m_pTargetValueNode->Shutdown( context );
        m_pClipReferenceNode->Shutdown( context );

        PoseNode::ShutdownInternal( context );
    }

    bool TargetWarpNode::TryReadTarget( GraphContext& context )
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

        m_requestedWarpTarget = warpTarget.GetTransform();
        return true;
    }

    //-------------------------------------------------------------------------

    void TargetWarpNode::SolveFixedSection( GraphContext& context, WarpSection const& section, Transform const& startTransform )
    {
        EE_ASSERT( section.IsFixedSection() );

        auto& warpedTransforms = m_warpedRootMotion.m_transforms;
        warpedTransforms[section.m_startFrame] = startTransform;

        //-------------------------------------------------------------------------

        for ( auto f = section.m_startFrame + 1; f <= section.m_endFrame; f++ )
        {
            warpedTransforms[f] = m_deltaTransforms[f] * warpedTransforms[f - 1];
        }
    }

    void TargetWarpNode::SolveTranslationZSection( GraphContext& context, WarpSection const& section, Transform const& startTransform, float correctionZ )
    {
        EE_ASSERT( section.m_warpRule == TargetWarpRule::WarpZ );

        auto& warpedTransforms = m_warpedRootMotion.m_transforms;
        warpedTransforms[section.m_startFrame] = startTransform;

        //-------------------------------------------------------------------------

        for ( auto f = section.m_startFrame + 1; f <= section.m_endFrame; f++ )
        {
            warpedTransforms[f] = m_deltaTransforms[f] * warpedTransforms[f - 1];
            int32_t const stepIdx = f - section.m_startFrame;
            float const progressThisFrame = ( section.m_totalProgress[stepIdx] - section.m_totalProgress[stepIdx - 1] );
            warpedTransforms[f].AddTranslation( Vector( 0, 0, correctionZ * progressThisFrame, 0 ) );
        }
    }

    void TargetWarpNode::SolveRotationSection( GraphContext& context, WarpSection const& section, Transform const& startTransform, Quaternion const& correction )
    {
        EE_ASSERT( section.m_warpRule == TargetWarpRule::RotationOnly );

        auto& warpedTransforms = m_warpedRootMotion.m_transforms;
        warpedTransforms[section.m_startFrame] = startTransform;

        // Generate base root motion
        //-------------------------------------------------------------------------
        
        for ( auto f = section.m_startFrame + 1; f <= section.m_endFrame; f++ )
        {
            warpedTransforms[f] = m_deltaTransforms[f] * warpedTransforms[f - 1];
        }

        // Modify facing
        //-------------------------------------------------------------------------

        for ( auto f = section.m_startFrame + 1; f <= section.m_endFrame; f++ )
        {
            Quaternion const frameDelta = Quaternion::SLerp( Quaternion::Identity, correction, section.m_totalProgress[f - section.m_startFrame] );
            Quaternion const adjustedRotation = frameDelta * m_warpedRootMotion.m_transforms[f].GetRotation();
            m_warpedRootMotion.m_transforms[f].SetRotation( adjustedRotation );
        }
    }

    void TargetWarpNode::SolveTranslationSection( GraphContext& context, WarpSection const& section, Transform const& startTransform, Transform const& endTransform )
    {
        auto pDefinition = GetDefinition<TargetWarpNode>();

        EE_ASSERT( section.m_warpRule == TargetWarpRule::WarpXY || section.m_warpRule == TargetWarpRule::WarpXYZ );

        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pAnimation != nullptr );

        RootMotionData const& originalRM = pAnimation->GetRootMotion();
        EE_ASSERT( originalRM.IsValid() );

        //-------------------------------------------------------------------------

        Vector const& startPoint = startTransform.GetTranslation();
        Vector const& endPoint = endTransform.GetTranslation();
        float const desiredDistanceToCover = startPoint.GetDistance3( endPoint );

        auto& warpedTransforms = m_warpedRootMotion.m_transforms;
        warpedTransforms[section.m_startFrame] = startTransform;
        warpedTransforms[section.m_endFrame] = endTransform;

        // Fallback Checks
        //-------------------------------------------------------------------------

        // If this section has no displacement, no matter what we do it will be weird, so just LERP
        bool triggerFallback = false;

        if ( Math::IsNearZero( section.m_distanceCovered ) )
        {
            triggerFallback = true;

            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Target Warp: trying to XY warp a section with no displacement, falling back to LERP!" );
            #endif
        }

        // If the distance to be covered is under the threshold, then just LERP as trying to fit a curve will look ugly
        if ( !triggerFallback && desiredDistanceToCover <= pDefinition->m_lerpFallbackDistanceThreshold )
        {
            triggerFallback = true;

            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Target Warp: trying to XY warp to under the distance threshold, falling back to LERP!" );
            #endif
        }

        auto EnsureValidCurve = [&] ( float curveLength )
        {
            EE_ASSERT( !triggerFallback );

            // If the curve end ups significantly longer than the straight distance then we likely have some loops or blowouts and it's going to create an ugly result so just LERP
            EE_ASSERT( desiredDistanceToCover > 0 );
            float const guideCurveRatio = curveLength / desiredDistanceToCover;
            if ( guideCurveRatio >= g_curveBlowoutDetectionThreshold )
            {
                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( GetNodeIndex(), "Target Warp: XY warp curve blowout detected! Falling back to LERP!" );
                #endif

                triggerFallback = true;
            }

            return !triggerFallback;
        };

        // Basic Curve Generation
        //-------------------------------------------------------------------------

        if ( !triggerFallback && ( section.m_translationAlgorithm == TargetWarpAlgorithm::Hermite || section.m_translationAlgorithm == TargetWarpAlgorithm::Bezier ) )
        {
            // Calculate the tangents for the curves
            Vector startTangent = ( ( m_deltaTransforms[section.m_startFrame + 1] * startTransform ).GetTranslation() - startTransform.GetTranslation() ).GetNormalized3();
            Vector endTangent = ( endTransform.GetTranslation() - ( m_inverseDeltaTransforms[section.m_endFrame] * endTransform ).GetTranslation() ).GetNormalized3();

            // If the tangents are in the same direction but we are moving in the opposite direction of the start tangent and the original displacement isnt similar then just LERP
            bool const tangentsInSameHemisphere = Math::IsVectorInTheSameHemisphere2D( startTangent, endTangent );
            if ( tangentsInSameHemisphere )
            {
                if ( Line( Line::StartDirection, startPoint, startTangent ).GetDistanceAlongLineToProjectedPoint( endPoint ) <= 0 )
                {
                    triggerFallback = true;

                    #if EE_DEVELOPMENT_TOOLS
                    context.LogWarning( GetNodeIndex(), "Target Warp: Trying to XY warp backwards, falling back to LERP!" );
                    #endif
                }
            }

            if ( !triggerFallback )
            {
                // Scale tangents - We use a half distance value clamped to the specified max length in the settings
                float const scalingFactor = Math::Min( pDefinition->m_maxTangentLength, endPoint.GetDistance3( startPoint ) / 2 );
                startTangent *= scalingFactor;
                endTangent *= scalingFactor;

                // Hermite
                if ( section.m_translationAlgorithm == TargetWarpAlgorithm::Hermite )
                {
                    float const curveLength = Math::CubicHermite::GetSplineLength( startPoint, startTangent, endPoint, endTangent );
                    if ( EnsureValidCurve( curveLength ) )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        section.m_debugPoints[0] = m_warpStartWorldTransform.TransformPoint( startPoint );
                        section.m_debugPoints[1] = m_warpStartWorldTransform.TransformVector( startTangent );
                        section.m_debugPoints[2] = m_warpStartWorldTransform.TransformPoint( endPoint );
                        section.m_debugPoints[3] = m_warpStartWorldTransform.TransformVector( endTangent );
                        #endif

                        //-------------------------------------------------------------------------

                        int32_t const numWarpFrames = section.m_endFrame - section.m_startFrame;
                        for ( auto i = 1; i < numWarpFrames; i++ )
                        {
                            int32_t const frameIdx = section.m_startFrame + i;

                            Vector const curvePoint = Math::CubicHermite::GetPoint( startPoint, startTangent, endPoint, endTangent, section.m_totalProgress[i] );
                            warpedTransforms[frameIdx].SetTranslation( curvePoint );

                            Quaternion const orientation = CalculateWarpedOrientationForFrame( originalRM, m_warpedRootMotion, frameIdx );
                            warpedTransforms[frameIdx].SetRotation( orientation );
                        }
                    }
                }
                // Bezier
                else if ( section.m_translationAlgorithm == TargetWarpAlgorithm::Bezier )
                {
                    Vector const cp0 = startPoint + startTangent;
                    Vector const cp1 = endPoint - endTangent;

                    float const curveLength = Math::CubicBezier::GetEstimatedLength( startPoint, cp0, cp1, endPoint );
                    if ( EnsureValidCurve( curveLength ) )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        section.m_debugPoints[0] = m_warpStartWorldTransform.TransformPoint( startPoint );
                        section.m_debugPoints[1] = m_warpStartWorldTransform.TransformPoint( cp0 );
                        section.m_debugPoints[2] = m_warpStartWorldTransform.TransformPoint( cp1 );
                        section.m_debugPoints[3] = m_warpStartWorldTransform.TransformPoint( endPoint );
                        #endif

                        int32_t const numWarpFrames = section.m_endFrame - section.m_startFrame;
                        for ( auto i = 1; i < numWarpFrames; i++ )
                        {
                            int32_t const frameIdx = section.m_startFrame + i;

                            Vector const curvePoint = Math::CubicBezier::GetPoint( startPoint, cp0, cp1, endPoint, section.m_totalProgress[i] );
                            warpedTransforms[frameIdx].SetTranslation( curvePoint );

                            Quaternion const orientation = CalculateWarpedOrientationForFrame( originalRM, m_warpedRootMotion, frameIdx );
                            warpedTransforms[frameIdx].SetRotation( orientation );
                        }
                    }
                }
            }
        }

        // Feature Preserving Curve Generation
        //-------------------------------------------------------------------------

        else if ( section.m_translationAlgorithm == TargetWarpAlgorithm::HermiteFeaturePreserving )
        {
            // Calculate guide curve start tangent
            //-------------------------------------------------------------------------

            Vector hermiteStartTangent;
            float sectionDeltaDistance;
            Vector const unwarpedEndPoint = ( section.m_deltaTransform * startTransform ).GetTranslation();
            ( unwarpedEndPoint - startTransform.GetTranslation() ).ToDirectionAndLength3( hermiteStartTangent, sectionDeltaDistance );

            // Calculate guide curve end tangent
            //-------------------------------------------------------------------------

            Vector hermiteEndTangent;

            // End direction - ideally use the direction of the next segment of the path to try to smooth out the motion
            if ( size_t( section.m_endFrame + 1 ) < m_deltaTransforms.size() )
            {
                Transform const endPlusOne = m_deltaTransforms[section.m_endFrame + 1] * endTransform;
                hermiteEndTangent = ( endPlusOne.GetTranslation() - endPoint ).GetNormalized3();
            }
            else // This is the last segment so just use in the entry direction
            {
                Transform const endMinusOne = m_inverseDeltaTransforms[section.m_endFrame - 1] * endTransform;
                hermiteEndTangent = ( endPoint - endMinusOne.GetTranslation() ).GetNormalized3();
            }

            if ( hermiteEndTangent.IsNearZero3() )
            {
                hermiteEndTangent = endTransform.GetForwardVector();
            }

            // Scale guide curve tangents
            //-------------------------------------------------------------------------
 
            float const scalingFactor = Math::Min( pDefinition->m_maxTangentLength, endPoint.GetDistance3( startPoint ) / 2 );
            hermiteStartTangent *= scalingFactor;
            hermiteEndTangent *= scalingFactor;

            // Calculate original RM scale factor
            //-------------------------------------------------------------------------

            float const curveLength = Math::CubicHermite::GetSplineLength( startPoint, hermiteStartTangent, endPoint, hermiteEndTangent );
            float const rootMotionScaleFactor = curveLength / sectionDeltaDistance;
            if ( rootMotionScaleFactor < g_minAllowedRootMotionScale )
            {
                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( GetNodeIndex(), "Target Warp: Feature preserving XY warp required scaling is lower than the minimum allowed threshold. Falling back to LERP!" );
                #endif

                triggerFallback = true;
            }

            //-------------------------------------------------------------------------
            
            if ( !triggerFallback && EnsureValidCurve( curveLength ) )
            {
                #if EE_DEVELOPMENT_TOOLS
                section.m_debugPoints[0] = m_warpStartWorldTransform.TransformPoint( startPoint );
                section.m_debugPoints[1] = m_warpStartWorldTransform.TransformVector( hermiteStartTangent );
                section.m_debugPoints[2] = m_warpStartWorldTransform.TransformPoint( endPoint );
                section.m_debugPoints[3] = m_warpStartWorldTransform.TransformVector( hermiteEndTangent );
                #endif

                Quaternion const sectionDeltaOrientation = Quaternion::FromRotationBetweenUnitVectors( Vector::WorldForward, hermiteStartTangent.GetNormalized3() );
                Vector const scaledUnwarpedEndPoint = unwarpedEndPoint * rootMotionScaleFactor;
                int32_t const numWarpFrames = section.m_endFrame - section.m_startFrame;

                Transform currentTransform = startTransform;
                for ( auto i = 1; i <= numWarpFrames; i++ )
                {
                    int32_t const frameIdx = section.m_startFrame + i;
                    float const percentageAlongCurve = section.m_totalProgress[i];

                    // Calculate original path delta from the displacement line
                    currentTransform = m_deltaTransforms[frameIdx] * currentTransform;
                    Transform const scaledCurrentTransform( currentTransform.GetRotation(), currentTransform.GetTranslation() * rootMotionScaleFactor );
                    Transform const straightTransform( sectionDeltaOrientation, Vector::Lerp( startPoint, scaledUnwarpedEndPoint, percentageAlongCurve ) );
                    Transform const curveDelta = Transform::Delta( straightTransform, scaledCurrentTransform );

                    // Calculate new curved path
                    auto const pt = Math::CubicHermite::GetPointAndTangent( startPoint, hermiteStartTangent, endPoint, hermiteEndTangent, percentageAlongCurve );
                    Quaternion const curveTangentOrientation = Quaternion::FromRotationBetweenUnitVectors( Vector::WorldForward, pt.m_tangent.GetNormalized2() );
                    Transform const curveTangentTransform( curveTangentOrientation, pt.m_point );

                    // Calculate final path point
                    Transform const finalTransform = curveDelta * curveTangentTransform;
                    warpedTransforms[frameIdx] = finalTransform;
                }
            }
        }

        // Lerp / Fallback
        //-------------------------------------------------------------------------

        if ( triggerFallback || section.m_translationAlgorithm == TargetWarpAlgorithm::Lerp )
        {
            int32_t const numWarpFrames = section.m_endFrame - section.m_startFrame;
            for ( auto i = 1; i < numWarpFrames; i++ )
            {
                int32_t const frameIdx = section.m_startFrame + i;
                warpedTransforms[frameIdx] = Transform::Lerp( startTransform, endTransform, section.m_totalProgress[i] );
            }
        }
    }

    //-------------------------------------------------------------------------

    void TargetWarpNode::ClearWarpInfo()
    {
        m_warpedRootMotion.Clear();
        m_warpSections.clear();
        m_deltaTransforms.clear();
        m_inverseDeltaTransforms.clear();
    }

    bool TargetWarpNode::GenerateWarpInfo( GraphContext& context )
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( m_warpSections.empty() && m_warpedRootMotion.GetNumFrames() == 0 );

        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pAnimation != nullptr );

        int32_t const numFrames = pAnimation->GetNumFrames();
        EE_ASSERT( numFrames > 0 );

        RootMotionData const& originalRM = pAnimation->GetRootMotion();
        EE_ASSERT( originalRM.IsValid() );

        // Read warp events
        //-------------------------------------------------------------------------

        m_warpStartTime = pAnimation->GetFrameTime( m_pClipReferenceNode->GetCurrentTime() );

        FrameTime const clipStartTime = m_warpStartTime;
        int32_t const clipStartFrame = clipStartTime.GetFrameIndex();
        int32_t const minimumStartFrameForFirstSection = clipStartFrame + 1;
        Seconds const duration = pAnimation->GetDuration();

        for ( auto const pEvent : pAnimation->GetEvents() )
        {
            // Do some simple exclusions first to avoid the dynamic cast
            if ( pEvent->IsImmediateEvent() )
            {
                continue;
            }

            // Skip any events that are before our start time
            FrameTime const eventEndTime = pAnimation->GetFrameTime( pEvent->GetEndTime() );
            if ( eventEndTime < m_warpStartTime )
            {
                continue;
            }

            // Create a section per warp event
            auto pWarpEvent = TryCast<TargetWarpEvent>( pEvent );
            if ( pWarpEvent != nullptr )
            {
                WarpSection section;
                section.m_startFrame = pAnimation->GetFrameTime( pWarpEvent->GetStartTime() ).GetNearestFrameIndex();
                section.m_endFrame = Math::Min( numFrames, pAnimation->GetFrameTime( pWarpEvent->GetEndTime() ).GetNearestFrameIndex() );
                section.m_warpRule = pWarpEvent->GetRule();
                section.m_translationAlgorithm = pWarpEvent->GetTranslationAlgorithm();

                // Adjust start frame to animation start time
                if ( minimumStartFrameForFirstSection >= section.m_startFrame )
                {
                    section.m_startFrame = minimumStartFrameForFirstSection;

                    // Skip 1 frame events
                    if ( !section.HasValidFrameRange() )
                    {
                        continue;
                    }
                }

                EE_ASSERT( section.m_startFrame < section.m_endFrame );

                // Should we insert a fixed section?
                if ( !m_warpSections.empty() )
                {
                    if ( m_warpSections.back().m_endFrame != section.m_startFrame )
                    {
                        WarpSection fixedSection;
                        fixedSection.m_startFrame = m_warpSections.back().m_endFrame;
                        fixedSection.m_endFrame = section.m_startFrame;
                        fixedSection.m_warpRule = TargetWarpRule::FixedSection;
                        m_warpSections.emplace_back( fixedSection );
                    }
                }

                // Add new section
                m_warpSections.emplace_back( section );

                // Track the options for this warp
                switch ( section.m_warpRule )
                {
                    case TargetWarpRule::WarpXY:
                    {
                        m_translationXYSectionIdx = (int8_t) m_warpSections.size() - 1;
                    }
                    break;

                    case TargetWarpRule::WarpZ:
                    {
                        m_isTranslationAllowedZ = true;
                    }
                    break;

                    case TargetWarpRule::WarpXYZ:
                    {
                        m_translationXYSectionIdx = (int8_t) m_warpSections.size() - 1;
                        m_isTranslationAllowedZ = true;
                    }
                    break;

                    case TargetWarpRule::RotationOnly:
                    {
                        m_rotationSectionIdx = (int8_t) m_warpSections.size() - 1;
                    }
                    break;

                    case TargetWarpRule::FixedSection:
                    {
                        // Do Nothing
                    }
                    break;
                }
            }
        }

        if ( m_warpSections.empty() )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogError( GetNodeIndex(), "No valid warp events detected (%s)! Either no events present or start time is after warp events.", m_pClipReferenceNode->GetAnimation()->GetResourceID().c_str() );
            #endif
            return false;
        }

        if ( m_translationXYSectionIdx == InvalidIndex && !m_isTranslationAllowedZ )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogError( GetNodeIndex(), "Warp attempted for animation with invalid warp events! %s", m_pClipReferenceNode->GetAnimation()->GetResourceID().c_str() );
            #endif

            m_warpSections.clear();
            return false;
        }

        // Calculate per-frame root motion deltas
        //-------------------------------------------------------------------------

        m_deltaTransforms.reserve( numFrames );
        m_inverseDeltaTransforms.reserve( numFrames );

        // We will never use these values so no need to calculate them
        for ( int32_t i = 0; i < minimumStartFrameForFirstSection; i++ )
        {
            m_deltaTransforms.emplace_back( Transform::Identity );
            m_inverseDeltaTransforms.emplace_back( Transform::Identity );
        }

        // Calculate the relevant deltas
        for ( int32_t i = minimumStartFrameForFirstSection; i < numFrames; i++ )
        {
            m_deltaTransforms.emplace_back( Transform::DeltaNoScale( originalRM.m_transforms[i - 1], originalRM.m_transforms[i] ) );
            m_inverseDeltaTransforms.emplace_back( m_deltaTransforms.back().GetInverse() );
        }

        // Calculate section info
        //-------------------------------------------------------------------------

        m_numSectionZ = 0;
        m_totalNumWarpableZFrames = 0;

        for ( auto& section : m_warpSections )
        {
            // Calculate the overall delta transform for the section
            section.m_deltaTransform = Transform::Delta( originalRM.m_transforms[section.m_startFrame], originalRM.m_transforms[section.m_endFrame] );

            // For fixed or rotation sections, we dont care about the length
            if ( section.IsFixedSection() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            section.m_totalProgress.resize( section.m_endFrame - section.m_startFrame + 1 );
            section.m_distanceCovered = 0.0f;

            // Rotation
            if ( section.m_warpRule == TargetWarpRule::RotationOnly )
            {
                for ( auto i = section.m_startFrame + 1; i <= section.m_endFrame; i++ )
                {
                    section.m_hasTranslation |= m_deltaTransforms[i].GetTranslation().GetLengthSquared3() > 0;
                    float const angularDistance = Math::Abs( m_deltaTransforms[i].GetRotation().GetAngle().ToFloat() );
                    section.m_totalProgress[i - section.m_startFrame] = angularDistance;
                    section.m_distanceCovered += angularDistance;
                }
            }
            else // Translation
            {
                for ( auto i = section.m_startFrame + 1; i <= section.m_endFrame; i++ )
                {
                    float const distance = m_deltaTransforms[i].GetTranslation().GetLength3();
                    section.m_totalProgress[i - section.m_startFrame] = distance;
                    section.m_distanceCovered += distance;
                }

                if ( section.m_warpRule == TargetWarpRule::WarpZ || section.m_warpRule == TargetWarpRule::WarpXYZ )
                {
                    m_numSectionZ++;
                    m_totalNumWarpableZFrames += section.GetNumWarpableFrames();
                }

                section.m_hasTranslation = section.m_distanceCovered > 0;
            }

            // Convert progress from distance to percentage progress
            if ( section.m_distanceCovered > 0 )
            {
                // Normalize the distance contributions per frame and calculate cumulative progress
                for ( auto i = 1; i < section.m_totalProgress.size(); i++ )
                {
                    section.m_totalProgress[i] /= section.m_distanceCovered;
                    section.m_totalProgress[i] += section.m_totalProgress[i - 1];
                    section.m_totalProgress[i] = Math::Min( section.m_totalProgress[i], 1.0f );
                }
            }
            else // Each frame has the exact same contribution
            {
                uint32_t const numWarpFrames = (uint32_t) section.m_totalProgress.size();
                float const contributionPerFrame = 1.0f / ( numWarpFrames - 1 );
                for ( uint32_t i = 1; i < numWarpFrames - 1; i++ )
                {
                    section.m_totalProgress[i] = section.m_totalProgress[i - 1] + contributionPerFrame;
                }

                section.m_totalProgress.back() = 1.0f;
            }
        }

        // Calculate actual target
        //-------------------------------------------------------------------------

        m_warpTarget = m_requestedWarpTarget;

        if ( m_alignmentBoneIdx != InvalidIndex || !m_isTranslationAllowedZ )
        {
            auto pDefinition = GetDefinition<TargetWarpNode>();

            Transform warpStartWorldTransform = context.m_worldTransform;

            // Handle the case were we dont start the animation from the first frame
            if ( !m_warpStartTime.IsAtStart() )
            {
                // Offset the warp start transform by the distance we've 'already' covered due to our later start time
                Transform const rootTransformAtActualStartTime = originalRM.GetTransform( m_warpStartTime );
                Transform const rootTransformOffset = rootTransformAtActualStartTime.GetInverse();
                warpStartWorldTransform = rootTransformOffset * warpStartWorldTransform;
            }

            Transform requestedWarpTargetCS = m_requestedWarpTarget * warpStartWorldTransform.GetInverse();
            Transform warpTargetCS = requestedWarpTargetCS;

            WarpSection const& lastWarpSection = m_warpSections.back();

            int32_t const targetAlignmentFrame = pDefinition->m_alignWithTargetAtLastWarpEvent ? lastWarpSection.m_endFrame : numFrames - 1;
            int32_t const originalRootMotionFrame = originalRM.IsStationary() ? 0 : targetAlignmentFrame;

            Transform alignmentBoneOffset = Transform::Identity;

            if ( m_alignmentBoneIdx != InvalidIndex )
            {
                // We want to align the alignment bone with the target, so calculate where the root needs to be in order for that to happen at the time we want things to align
                TInlineVector<BoneChainElement, 20> const modelSpaceChain = pAnimation->GetModelSpaceTransform( FrameTime( targetAlignmentFrame ), m_alignmentBoneIdx );
                alignmentBoneOffset = modelSpaceChain.back().m_modelSpaceTransform;
            }

            Transform unwarpedTargetCS = alignmentBoneOffset * originalRM.m_transforms[originalRootMotionFrame];

            // If we dont allow horizontal translation, just keep the target's Z value
            if ( m_translationXYSectionIdx == InvalidIndex )
            {
                Vector adjustedTranslation = unwarpedTargetCS.GetTranslation();
                adjustedTranslation.SetZ( requestedWarpTargetCS.GetTranslation().GetZ() );
                warpTargetCS.SetTranslation( adjustedTranslation );
            }

            // If we dont allow vertical translation, just keep the target's XY value
            if ( !m_isTranslationAllowedZ )
            {
                Vector adjustedTranslation = requestedWarpTargetCS.GetTranslation();
                Vector originalFinalTranslation = unwarpedTargetCS.GetTranslation();
                adjustedTranslation.SetZ( originalFinalTranslation.GetZ() );
                warpTargetCS.SetTranslation( adjustedTranslation );
            }

            m_warpTarget = warpTargetCS * warpStartWorldTransform;
        }

        return true;
    }

    void TargetWarpNode::GenerateWarpedRootMotion( GraphContext& context )
    {
        auto pDefinition = GetDefinition<TargetWarpNode>();

        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pAnimation != nullptr );

        int32_t const numFrames = (int32_t) pAnimation->GetNumFrames();
        EE_ASSERT( numFrames > 0 );

        RootMotionData const& originalRM = pAnimation->GetRootMotion();
        EE_ASSERT( originalRM.IsValid() );

        // Prepare for warping and handle start time
        //-------------------------------------------------------------------------

        m_samplingMode = pDefinition->m_samplingMode;
        m_warpStartWorldTransform = context.m_worldTransform;

        // Create storage for warped root motion
        m_warpedRootMotion.Clear();
        auto& warpedTransforms = m_warpedRootMotion.m_transforms;
        warpedTransforms.resize( originalRM.GetNumFrames() );
        m_warpedRootMotion.m_numFrames = originalRM.GetNumFrames();

        // Handle the case were we dont start the animation from the first frame
        if ( !m_warpStartTime.IsAtStart() )
        {
            // Offset the warp start transform by the distance we've 'already' covered due to our later start time
            Transform const rootTransformAtActualStartTime = originalRM.GetTransform( m_warpStartTime );
            Transform const rootTransformOffset = rootTransformAtActualStartTime.GetInverse();
            m_warpStartWorldTransform = rootTransformOffset * m_warpStartWorldTransform;
        }

        // Add the unwarped start and end portions of the root motion
        //-------------------------------------------------------------------------

        // Set all unwarped start frames
        for ( auto i = 0; i <= m_warpSections[0].m_startFrame; i++ )
        {
            warpedTransforms[i] = originalRM.m_transforms[i];
        }

        WarpSection const &lastWarpSection = m_warpSections.back();
        int32_t const targetAlignmentFrameIdx = pDefinition->m_alignWithTargetAtLastWarpEvent ? lastWarpSection.m_endFrame : numFrames - 1;

        Transform rootTarget = m_warpTarget;

        // We want to align the alignment bone with the target, so calculate where the root needs to be in order for that to happen at the time we want things to align
        if ( m_alignmentBoneIdx != InvalidIndex )
        {
            // TODO: This is bad! Provide a direct accessor for bone transforms
            Pose alignmentPose( context.m_pSkeleton );
            pAnimation->GetPose( FrameTime( targetAlignmentFrameIdx ), &alignmentPose );

            Transform const alignmentBoneTransform = alignmentPose.GetModelSpaceTransform( m_alignmentBoneIdx );
            rootTarget = alignmentBoneTransform.GetInverse() * rootTarget;
        }

        // Set the frame at which we want to be aligned with out target to that target
        warpedTransforms[targetAlignmentFrameIdx] = m_warpStartWorldTransform.GetInverse() * m_warpTarget;

        // Add trailing unwarped frames
        for ( int32_t frameIdx = targetAlignmentFrameIdx + 1; frameIdx < numFrames; frameIdx++ )
        {
            warpedTransforms[frameIdx] = warpedTransforms[frameIdx - 1] * m_deltaTransforms[frameIdx];
        }

        for ( int32_t frameIdx = targetAlignmentFrameIdx - 1; frameIdx >= lastWarpSection.m_endFrame; frameIdx-- )
        {
            warpedTransforms[frameIdx] = warpedTransforms[frameIdx + 1] * m_inverseDeltaTransforms[frameIdx + 1];
        }

        // Forward Solve
        //-------------------------------------------------------------------------
        // Stops on XY or R sections

        int32_t const numSections = (int32_t) m_warpSections.size();

        // What the delta between where we end up and where we want to be?
        Transform const warpDelta = Transform::Delta( originalRM.m_transforms[lastWarpSection.m_endFrame], warpedTransforms[lastWarpSection.m_endFrame] );

        // Forward solve (stop on Rotation section or XY section)
        int32_t const rIdx = m_rotationSectionIdx == InvalidIndex ? numSections : m_rotationSectionIdx;
        int32_t const tIdx = m_translationXYSectionIdx == InvalidIndex ? numSections : m_translationXYSectionIdx;
        int32_t const forwardEndIdx = Math::Min( rIdx, tIdx );
        for ( auto i = 0; i < forwardEndIdx; i++ )
        {
            auto const& warpSection = m_warpSections[i];
            if ( warpSection.m_warpRule == TargetWarpRule::WarpZ )
            {
                // Calculate how much Z correction to apply
                float correctionZ = warpDelta.GetTranslation().GetZ();
                if ( m_numSectionZ > 1 )
                {
                    float const percentageCorrectionToApply = float( warpSection.GetNumWarpableFrames() ) / m_totalNumWarpableZFrames;
                    correctionZ *= percentageCorrectionToApply;
                }

                Transform const& sectionStartTransform = warpedTransforms[warpSection.m_startFrame];
                SolveTranslationZSection( context, warpSection, sectionStartTransform, correctionZ );
            }
            else if( warpSection.IsFixedSection() )
            {
                Transform const& sectionStartTransform = warpedTransforms[warpSection.m_startFrame];
                SolveFixedSection( context, warpSection, sectionStartTransform );
            }
            else
            {
                EE_UNREACHABLE_CODE();
            }
        }

        // Backward solve
        //-------------------------------------------------------------------------
        // Stops on XY section

        int32_t const backwardsEndIdx = Math::Min( numSections - 1, tIdx );
        for ( auto i = numSections - 1; i > backwardsEndIdx; i-- )
        {
            auto const& warpSection = m_warpSections[i];
            if ( warpSection.m_warpRule == TargetWarpRule::WarpZ )
            {
                // Calculate how much Z correction to apply
                float correctionZ = warpDelta.GetTranslation().GetZ();
                if ( m_numSectionZ > 1 )
                {
                    float const percentageCorrectionToApply = float( warpSection.GetNumWarpableFrames() ) / m_totalNumWarpableZFrames;
                    correctionZ *= percentageCorrectionToApply;
                }

                Transform sectionStartTransform = warpSection.m_deltaTransform.GetInverse() * warpedTransforms[warpSection.m_endFrame];
                sectionStartTransform.AddTranslation( Vector( 0, 0, -correctionZ ) );
                SolveTranslationZSection( context, warpSection, sectionStartTransform, correctionZ );
            }
            else if ( warpSection.m_warpRule == TargetWarpRule::RotationOnly )
            {
                // We need to calculate the start transform for this section,
                // in doing so we will be determining the target transform of the XY (T) segment
                Transform const& targetTransform = warpedTransforms[warpSection.m_endFrame];
                Transform const& startTransformT = warpedTransforms[m_warpSections[tIdx].m_startFrame];
                
                // Calculate the delta between the XY section and the rotation (including the rotation section)
                Transform unwarpedDeltaPostT = m_warpSections[tIdx + 1].m_deltaTransform;
                for ( auto j = tIdx + 2; j <= i; j++ )
                {
                    unwarpedDeltaPostT = m_warpSections[j].m_deltaTransform * unwarpedDeltaPostT;
                }

                Vector alignmentDir;

                // Align all the section post T to the incoming movement dir for T's start frame
                Vector const startToTarget = targetTransform.GetTranslation() - startTransformT.GetTranslation();
                if ( startToTarget.IsNearZero2() )
                {
                    alignmentDir = ( warpedTransforms.back().GetTranslation() - targetTransform.GetTranslation() );

                    if ( alignmentDir.IsNearZero2() )
                    {
                        alignmentDir = warpedTransforms.back().RotateVector( Vector::WorldForward );
                    }
                    else // Align to direction to end point
                    {
                        alignmentDir.Normalize2();
                    }
                }
                else // Align all the sections post T to the shortest path between target and start of T
                {
                    alignmentDir = startToTarget.GetNormalized2();
                }

                // Calculate the new start point for all the section postT
                Transform sectionStartTransform;
                Vector const deltaDistance = unwarpedDeltaPostT.GetTranslation().Length2();
                sectionStartTransform.SetTranslation( Vector::NegativeMultiplySubtract( alignmentDir, deltaDistance, targetTransform.GetTranslation() ) );

                // Put the section start location at the correct height so it doesn't get doubled
                sectionStartTransform.AddTranslation( Vector::WorldDown * unwarpedDeltaPostT.GetTranslation().GetZ() );

                // Use IncomingMovementOrientation so that it allows placing translation section on the last frame of translation
                Quaternion const unwarpedMovementOrientation = originalRM.GetIncomingMovementOrientation2DAtFrame( m_warpSections[tIdx].m_endFrame );
                Quaternion const originalFacingOrientation = originalRM.m_transforms[m_warpSections[tIdx].m_endFrame].GetRotation();
                Quaternion const offset = Quaternion::Delta( unwarpedMovementOrientation, originalFacingOrientation );
                sectionStartTransform.SetRotation( offset * Quaternion::FromRotationBetweenUnitVectors( Vector::WorldForward, alignmentDir ) );

                // Calculate the start transform for this section by applying the delta for each section
                for ( auto j = tIdx + 1; j < i; j++ )
                {
                    sectionStartTransform = m_warpSections[j].m_deltaTransform * sectionStartTransform;
                }

                // Calculate the required delta and solve segment
                Transform const estimatedSectionEndTransform = warpSection.m_deltaTransform * sectionStartTransform;
                Quaternion const requiredCorrection = Quaternion::Delta( estimatedSectionEndTransform.GetRotation(), targetTransform.GetRotation() );

                #if EE_DEVELOPMENT_TOOLS
                warpSection.m_debugPoints[0] = m_warpStartWorldTransform.TransformPoint( targetTransform.GetTranslation() );
                warpSection.m_debugPoints[1] = m_warpStartWorldTransform.TransformPoint( targetTransform.GetTranslation() + unwarpedMovementOrientation.RotateVector( Vector::WorldForward ) );
                warpSection.m_debugPoints[2] = m_warpStartWorldTransform.TransformPoint( targetTransform.GetTranslation() + estimatedSectionEndTransform.GetRotation().RotateVector( Vector::WorldForward ) );
                #endif

                SolveRotationSection( context, warpSection, sectionStartTransform, requiredCorrection );
            }
            else if ( warpSection.IsFixedSection() )
            {
                Transform sectionStartTransform = warpSection.m_deltaTransform.GetInverse() * warpedTransforms[warpSection.m_endFrame];
                SolveFixedSection( context, warpSection, sectionStartTransform );
            }
            else
            {
                EE_UNREACHABLE_CODE();
            }
        }

        // Final Forward Solve
        //-------------------------------------------------------------------------
        // Solve remaining sections (should only be XY and R sections)

        for ( auto i = forwardEndIdx; i <= backwardsEndIdx; i++ )
        {
            auto const& warpSection = m_warpSections[i];
            if ( warpSection.m_warpRule == TargetWarpRule::RotationOnly )
            {
                Quaternion requiredCorrection;

                // The target that we want to align to is the XY section's end transform as that was set in the backward pass
                Transform const& sectionStartTransform = warpedTransforms[warpSection.m_startFrame];
                Transform const sectionEndTransform = warpSection.m_deltaTransform * sectionStartTransform;
                Transform const& targetTransform = warpedTransforms[m_warpSections[backwardsEndIdx].m_endFrame];

                // If we end up at the target at the end of the segment, align to end orientation
                Vector const toTarget = ( targetTransform.GetTranslation() - sectionEndTransform.GetTranslation() );
                if ( toTarget.IsNearZero2() )
                {
                    requiredCorrection = Quaternion::Delta( sectionEndTransform.GetRotation(), targetTransform.GetRotation() );

                    #if EE_DEVELOPMENT_TOOLS
                    warpSection.m_debugPoints[0] = m_warpStartWorldTransform.TransformPoint( sectionEndTransform.GetTranslation() );
                    warpSection.m_debugPoints[1] = m_warpStartWorldTransform.TransformPoint( sectionEndTransform.GetTranslation() + sectionEndTransform.GetRotation().RotateVector( Vector::WorldForward ) );
                    warpSection.m_debugPoints[2] = m_warpStartWorldTransform.TransformPoint( sectionEndTransform.GetTranslation() + targetTransform.GetRotation().RotateVector( Vector::WorldForward ) );
                    #endif
                }
                else
                {
                    // Calculate estimated unwarped movement
                    Transform const endFramePlusOne = m_deltaTransforms[warpSection.m_endFrame + 1] * sectionEndTransform;
                    Vector outgoingMovement = endFramePlusOne.GetTranslation() - sectionEndTransform.GetTranslation();
                    if ( outgoingMovement.IsNearZero2() )
                    {
                        outgoingMovement = sectionEndTransform.GetRotation().RotateVector( Vector::WorldForward );
                    }

                    #if EE_DEVELOPMENT_TOOLS
                    warpSection.m_debugPoints[0] = m_warpStartWorldTransform.TransformPoint( sectionEndTransform.GetTranslation() );
                    warpSection.m_debugPoints[1] = m_warpStartWorldTransform.TransformPoint( sectionEndTransform.GetTranslation() + outgoingMovement.GetNormalized2() );
                    warpSection.m_debugPoints[2] = m_warpStartWorldTransform.TransformPoint( sectionEndTransform.GetTranslation() + toTarget.GetNormalized2() );
                    #endif

                    requiredCorrection = Quaternion::FromRotationBetweenUnitVectors( outgoingMovement.GetNormalized2(), toTarget.GetNormalized2() );
                }

                SolveRotationSection( context, warpSection, sectionStartTransform, requiredCorrection );
            }
            else if ( warpSection.m_warpRule == TargetWarpRule::WarpXY || warpSection.m_warpRule == TargetWarpRule::WarpXYZ )
            {
                Transform const& sectionStartTransform = warpedTransforms[warpSection.m_startFrame];
                Transform const& sectionEndTransform = warpedTransforms[warpSection.m_endFrame];
                SolveTranslationSection( context, warpSection, sectionStartTransform, sectionEndTransform );
            }
            else if ( warpSection.IsFixedSection() )
            {
                Transform const& sectionStartTransform = warpedTransforms[warpSection.m_startFrame];
                SolveFixedSection( context, warpSection, sectionStartTransform );
            }
            else if ( warpSection.m_warpRule == TargetWarpRule::WarpZ )
            {
                // Calculate how much Z correction to apply
                float correctionZ = warpDelta.GetTranslation().GetZ();
                if ( m_numSectionZ > 1 )
                {
                    float const percentageCorrectionToApply = float( warpSection.GetNumWarpableFrames() ) / m_totalNumWarpableZFrames;
                    correctionZ *= percentageCorrectionToApply;
                }

                Transform const& sectionStartTransform = warpedTransforms[warpSection.m_startFrame];
                SolveTranslationZSection( context, warpSection, sectionStartTransform, correctionZ );
            }
            else
            {
                EE_UNREACHABLE_CODE();
            }
        }

        // Put the warped root motion to the world space for the world space sampling mode
        for ( Transform &warpedRootMotion : m_warpedRootMotion.m_transforms )
        {
            warpedRootMotion = warpedRootMotion * m_warpStartWorldTransform;
        }
    }

    //-------------------------------------------------------------------------

    bool TargetWarpNode::UpdateWarp( GraphContext& context )
    {
        EE_ASSERT( IsValid() );

        auto pDefinition = GetDefinition<TargetWarpNode>();

        bool const isRecalculationAllowed = pDefinition->m_targetUpdateRule == TargetUpdateRule::Recalculate || pDefinition->m_targetUpdateRule == TargetUpdateRule::RecalculateOrOffset;

        //-------------------------------------------------------------------------

        if ( m_internalState == InternalState::Completed || m_internalState == InternalState::Failed )
        {
            if ( pDefinition->m_targetUpdateRule == TargetUpdateRule::Offset && m_warpedRootMotion.IsValid() )
            {
                OffsetWarpedRootMotion( context );
            }

            return false;
        }

        //-------------------------------------------------------------------------

        // Initial update
        if ( m_internalState == InternalState::RequiresInitialUpdate )
        {
            if ( !TryReadTarget( context ) )
            {
                m_internalState = isRecalculationAllowed ? InternalState::AllowUpdates : InternalState::Failed;
                return false;
            }
        }
        else // Update check
        {
            EE_ASSERT( m_internalState == InternalState::AllowUpdates );

            // Cannot update an invalid warp
            if ( !m_warpedRootMotion.IsValid() )
            {
                return false;
            }

            // Cannot update a warp that is past all the warp events
            auto pAnimation = m_pClipReferenceNode->GetAnimation();
            EE_ASSERT( pAnimation != nullptr );
            int32_t currentFrameIdx = (int32_t) pAnimation->GetFrameTime( m_currentTime ).GetUpperBoundFrameIndex();
            if ( !CanRecalculateWarpedRootMotion( currentFrameIdx ) )
            {
                if ( pDefinition->m_targetUpdateRule == TargetUpdateRule::RecalculateOrOffset )
                {
                    OffsetWarpedRootMotion( context );
                }

                return false;
            }

            // Check if the target has changed
            //-------------------------------------------------------------------------

            Transform const previousRequestedTarget = m_requestedWarpTarget;
            if ( !TryReadTarget( context ) )
            {
                return false;
            }

            Radians const deltaAngle = Quaternion::Distance( m_requestedWarpTarget.GetRotation(), previousRequestedTarget.GetRotation() );
            bool shouldUpdate = deltaAngle > pDefinition->m_targetUpdateAngleThresholdRadians;

            if ( !shouldUpdate && m_translationXYSectionIdx != InvalidIndex )
            {
                float const deltaDistanceXY = m_requestedWarpTarget.GetTranslation().GetDistance2( previousRequestedTarget.GetTranslation() );
                shouldUpdate = deltaDistanceXY > pDefinition->m_targetUpdateDistanceThreshold;
            }

            if ( !shouldUpdate && m_isTranslationAllowedZ )
            {
                float const deltaDistanceZ = ( m_requestedWarpTarget.GetTranslation().GetSplatZ() - previousRequestedTarget.GetTranslation().GetSplatZ() ).ToFloat();
                shouldUpdate = deltaDistanceZ > pDefinition->m_targetUpdateDistanceThreshold;
            }

            // If no change, then reset the requested target back to the original value and return
            if ( !shouldUpdate )
            {
                m_requestedWarpTarget = previousRequestedTarget;
                return false;
            }
        }

        // Calculate the warp
        //-------------------------------------------------------------------------

        ClearWarpInfo();

        if ( GenerateWarpInfo( context ) )
        {
            GenerateWarpedRootMotion( context );
            m_internalState = isRecalculationAllowed ? InternalState::AllowUpdates : InternalState::Completed;
            return true;
        }
        else
        {
            m_internalState = isRecalculationAllowed ? InternalState::AllowUpdates : InternalState::Failed;
            return false;
        }
    }

    bool TargetWarpNode::OffsetWarpedRootMotion( GraphContext &context )
    {
        Transform const previousRequestedTarget = m_requestedWarpTarget;
        if ( !TryReadTarget( context ) )
        {
            return false;
        }

        if ( m_requestedWarpTarget.IsNearEqual( previousRequestedTarget ) )
        {
            m_requestedWarpTarget = previousRequestedTarget;
            return false;
        }

        Transform const delta = previousRequestedTarget.GetInverse() * m_requestedWarpTarget;
        for ( Transform &rootTransform : m_warpedRootMotion.m_transforms )
        {
            rootTransform = rootTransform * delta;
        }

        m_warpStartWorldTransform = m_warpStartWorldTransform * delta;
        m_warpTarget = m_warpTarget * delta;

        return true;
    }

    bool TargetWarpNode::CanRecalculateWarpedRootMotion( int32_t startFrameIdx ) const
    {
        bool hasXYSection = false;
        bool hasZSection = false;

        for ( WarpSection const &warpSection : m_warpSections )
        {
            if ( warpSection.m_endFrame <= startFrameIdx )
            {
                continue;
            }

            switch ( warpSection.m_warpRule )
            {
                case TargetWarpRule::WarpXY:
                hasXYSection = true;
                break;

                case TargetWarpRule::WarpZ:
                hasZSection = true;
                break;

                case TargetWarpRule::WarpXYZ:
                hasZSection = true;
                hasXYSection = true;
                break;

                default:
                break;
            }
        }

        return hasXYSection && hasZSection;
    }

    void TargetWarpNode::SampleWarpedRootMotion( GraphContext& context, GraphPoseNodeResult& result )
    {
        EE_ASSERT( IsValid() );

        // If we failed to warp, just keep the original root motion delta
        if ( !m_warpedRootMotion.IsValid() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        // If we are sampling accurately then we need to match the exact world space position each update
        if ( m_samplingMode == RootMotionData::SamplingMode::WorldSpace )
        {
            auto pDefinition = GetDefinition<TargetWarpNode>();

            // Calculate error between current and expected position (only if the warp hasnt been updated this frame)
            Transform const expectedTransform = m_warpedRootMotion.GetTransform( m_previousTime );
            float const positionErrorSq = expectedTransform.GetTranslation().GetDistanceSquared3( context.m_worldTransform.GetTranslation() );
            if ( positionErrorSq <= pDefinition->m_samplingPositionErrorThresholdSq )
            {
                result.m_rootMotionDelta = m_warpedRootMotion.SampleRootMotion( RootMotionData::SamplingMode::WorldSpace, context.m_worldTransform, m_previousTime, m_currentTime );
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
            result.m_rootMotionDelta = m_warpedRootMotion.SampleRootMotion( RootMotionData::SamplingMode::Delta, context.m_worldTransform, m_previousTime, m_currentTime );
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

    void TargetWarpNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_samplingMode );
        outState.WriteValue( m_internalState );

        outState.WriteValue( m_translationXYSectionIdx );
        outState.WriteValue( m_rotationSectionIdx );
        outState.WriteValue( m_isTranslationAllowedZ );
        outState.WriteValue( m_numSectionZ );
        outState.WriteValue( m_totalNumWarpableZFrames );

        //-------------------------------------------------------------------------

        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        if ( pAnimation != nullptr )
        {
            Percentage startTime = pAnimation->GetPercentageThrough( m_warpStartTime );
            outState.WriteValue( startTime );
        }
        else
        {
            outState.WriteValue( 0.0f );
        }

        outState.WriteValue( m_warpStartWorldTransform );
        outState.WriteValue( m_requestedWarpTarget );
        outState.WriteValue( m_warpTarget );

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

        outState.WriteValue( m_alignmentBoneIdx );
    }

    bool TargetWarpNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        if ( !PoseNode::RestoreGraphState( inState ) )
        {
            return false;
        }

        inState.ReadValue( m_samplingMode );
        inState.ReadValue( m_internalState );

        inState.ReadValue( m_translationXYSectionIdx );
        inState.ReadValue( m_rotationSectionIdx );
        inState.ReadValue( m_isTranslationAllowedZ );
        inState.ReadValue( m_numSectionZ );
        inState.ReadValue( m_totalNumWarpableZFrames );

        //-------------------------------------------------------------------------

        Percentage startTime = 0.0f;
        inState.ReadValue( startTime );

        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        if ( pAnimation != nullptr )
        {
            m_warpStartTime = pAnimation->GetFrameTime( startTime );
        }
        else
        {
            m_warpStartTime = FrameTime();
        }

        inState.ReadValue( m_warpStartWorldTransform );
        inState.ReadValue( m_requestedWarpTarget );
        inState.ReadValue( m_warpTarget );

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

        inState.ReadValue( m_alignmentBoneIdx );

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

        // Draw Target
        drawCtx.DrawAxis( m_warpTarget, 0.1f, 5.0f );

        // Draw Warped Root Motion
        if ( m_warpedRootMotion.IsValid() )
        {
            m_warpedRootMotion.DrawDebug( drawCtx, Transform::Identity );
        }

        // Draw Section debug
        constexpr float const debugPointSize = 15.0f;
        for ( auto i = 0u; i < m_warpSections.size(); i++ )
        {
            auto const& section = m_warpSections[i];

            Color const ruleColor = GetDebugColorForWarpRule( section.m_warpRule );

            if ( section.IsFixedSection() )
            {
                Vector const debugSectionStart = m_warpedRootMotion.m_transforms[section.m_startFrame].GetTranslation();
                Vector const debugSectionEnd = m_warpedRootMotion.m_transforms[section.m_endFrame].GetTranslation();
                drawCtx.DrawPoint( debugSectionStart, Colors::White, debugPointSize );
                drawCtx.DrawPoint( debugSectionEnd, Colors::White, debugPointSize );
                drawCtx.DrawLine( debugSectionStart, debugSectionEnd, Colors::White, 3.0f );
            }
            else if ( section.m_warpRule == TargetWarpRule::RotationOnly )
            {
                Vector const debugSectionStart = m_warpedRootMotion.m_transforms[section.m_startFrame].GetTranslation();
                Vector const debugSectionEnd = m_warpedRootMotion.m_transforms[section.m_endFrame].GetTranslation();
                drawCtx.DrawPoint( debugSectionStart, ruleColor, debugPointSize );
                drawCtx.DrawPoint( debugSectionEnd, ruleColor, debugPointSize );
                drawCtx.DrawLine( debugSectionStart, debugSectionEnd, ruleColor, 3.0f );
                drawCtx.DrawArrow( section.m_debugPoints[0], section.m_debugPoints[1], Colors::White, 2.0f );
                drawCtx.DrawArrow( section.m_debugPoints[0], section.m_debugPoints[2], ruleColor, 2.0f );
            }
            else // Translation
            {
                Vector const debugSectionStart = m_warpedRootMotion.m_transforms[section.m_startFrame].GetTranslation();
                Vector const debugSectionEnd = m_warpedRootMotion.m_transforms[section.m_endFrame].GetTranslation();
                drawCtx.DrawPoint( debugSectionStart, ruleColor, debugPointSize );
                drawCtx.DrawPoint( debugSectionEnd, ruleColor, debugPointSize );
                drawCtx.DrawLine( debugSectionStart, debugSectionEnd, ruleColor, 3.0f );

                if ( section.m_translationAlgorithm == TargetWarpAlgorithm::Bezier )
                {
                    // Four points
                    drawCtx.DrawArrow( section.m_debugPoints[0], section.m_debugPoints[1], Colors::White, 2.0f );
                    drawCtx.DrawArrow( section.m_debugPoints[3], section.m_debugPoints[2], ruleColor, 2.0f );
                }
                else
                {
                    // Two points and two tangents
                    drawCtx.DrawArrow( section.m_debugPoints[0], section.m_debugPoints[0] + section.m_debugPoints[1], Colors::White, 2.0f );
                    drawCtx.DrawArrow( section.m_debugPoints[2], section.m_debugPoints[2] + section.m_debugPoints[3], ruleColor, 2.0f );
                }
            }
        }
    }
    #endif
}