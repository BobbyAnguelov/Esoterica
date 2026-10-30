#include "AnimationRootMotionWarp.h"
#include "AnimationClip.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Math/Curves.h"
#include "Base/Math/Line.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    constexpr float const g_curveBlowoutDetectionThreshold = 2.5f;
    constexpr float const g_minAllowedRootMotionScale = 0.05f;

    // Utility Functions
    //-------------------------------------------------------------------------

    static Quaternion CalculateWarpedOrientationReferenceQuat( RootMotionData const& rootMotion, int32_t frameIdx )
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

    // Section Validation
    //-------------------------------------------------------------------------

    static void ValidateWarpSections( TVector<RootMotionWarpContext::Section> const& sections, int32_t numRootMotionFrames )
    {
        #if EE_DEVELOPMENT_TOOLS
        for ( int32_t i = 0; i < (int32_t) sections.size(); i++ )
        {
            EE_ASSERT( sections[i].m_startFrame <= sections[i].m_endFrame ); // Inverted frame range
            EE_ASSERT( sections[i].m_startFrame >= 0 && sections[i].m_endFrame < numRootMotionFrames ); // Frame range outside of the root motion
            EE_ASSERT( i == 0 || sections[i].m_startFrame >= sections[i - 1].m_endFrame ); // Sections must be ordered by start frame and must not overlap
        }
        #endif
    }

    // Root Motion Warp Context
    //-------------------------------------------------------------------------

    RootMotionWarpContext::RootMotionWarpContext( AnimationClip const& clip )
    {
        InitializeFromClip( clip );
    }

    RootMotionWarpContext::~RootMotionWarpContext()
    {
        Clear();
    }

    void RootMotionWarpContext::InitializeFromClip( AnimationClip const& clip )
    {
        InitializeFromRootMotion( clip.GetRootMotion(), nullptr, 0 );

        EE_ASSERT( m_sections.empty() );

        for ( auto const pEvent : clip.GetEvents() )
        {
            // Do some simple exclusions first to avoid the cast
            if ( pEvent->IsImmediateEvent() )
            {
                continue;
            }

            auto pWarpEvent = TryCast<TargetWarpEvent>( pEvent );
            if ( pWarpEvent == nullptr )
            {
                continue;
            }

            Section section;
            section.m_startFrame = clip.GetFrameTime( pWarpEvent->GetStartTime() ).GetNearestFrameIndex();
            section.m_endFrame = Math::Min( clip.GetNumFrames() - 1, clip.GetFrameTime( pWarpEvent->GetEndTime() ).GetNearestFrameIndex() );
            section.m_warpRule = pWarpEvent->GetRule();
            section.m_translationAlgorithm = pWarpEvent->GetTranslationAlgorithm();

            m_sections.emplace_back( section );
        }

        ValidateWarpSections( m_sections, m_rootMotion.GetNumFrames() );
    }

    void RootMotionWarpContext::InitializeFromRootMotion( RootMotionData const& rootMotion, Section const* pSections, int32_t sectionCount )
    {
        EE_ASSERT( rootMotion.IsValid() );

        Clear();

        m_rootMotion = rootMotion;

        int32_t const numFrames = m_rootMotion.GetNumFrames();

        if ( m_rootMotion.IsStationary() )
        {
            m_deltaTransforms.resize( numFrames, Transform::Identity );
            m_inverseDeltaTransforms.resize( numFrames, Transform::Identity );
        }
        else
        {
            m_deltaTransforms.resize( numFrames );
            m_inverseDeltaTransforms.resize( numFrames );

            m_deltaTransforms[0] = Transform::Identity;
            m_inverseDeltaTransforms[0] = Transform::Identity;

            for ( int32_t i = 1; i < numFrames; i++ )
            {
                m_deltaTransforms[i] = Transform::DeltaNoScale( m_rootMotion.m_transforms[i - 1], m_rootMotion.m_transforms[i] );
                m_inverseDeltaTransforms[i] = m_deltaTransforms[i].GetInverse();
            }
        }

        if ( pSections != nullptr )
        {
            for ( int32_t i = 0; i < sectionCount; ++i )
            {
                m_sections.emplace_back( pSections[i] );
            }
        }

        ValidateWarpSections( m_sections, m_rootMotion.GetNumFrames() );
    }

    bool RootMotionWarpContext::IsInitialized() const
    {
        return m_rootMotion.IsValid();
    }

    void RootMotionWarpContext::Clear()
    {
        m_startTime = FrameTime();
        m_startTransform = Transform::Identity;
        m_targetTransform = Transform::Identity;
        m_deltaTransforms.clear();
        m_inverseDeltaTransforms.clear();
        m_rootMotion.Clear();
        m_warpedRootMotion.Clear();
        m_sections.clear();

        m_alignTargetAtLastSection = false;

        #if EE_DEVELOPMENT_TOOLS
        m_log.Clear();
        EE::Delete( m_pDebugData );
        #endif
    }

    RootMotionWarpContext::Capability RootMotionWarpContext::GetWarpCapabilities( FrameTime const& startTime ) const
    {
        Capability capabilities;

        RootMotionWarper::Sections warpSections;
        if ( !RootMotionWarper::InitializeWarpSections( *this, RootMotionWarper::InitializeWarpSectionsMode::ValidityCheckOnly, &warpSections, &startTime ) )
        {
            return capabilities;
        }

        capabilities.m_canWarpXY = warpSections.m_translationXYSectionIdx != InvalidIndex;
        capabilities.m_canWarpZ = warpSections.m_isTranslationAllowedZ;
        capabilities.m_canWarpRotation = capabilities.m_canWarpXY;

        return capabilities;
    }

    void RootMotionWarpContext::OffsetWarpedRootMotion( Transform const& delta )
    {
        for ( Transform& rootTransform : m_warpedRootMotion.m_transforms )
        {
            rootTransform = rootTransform * delta;
        }
    }

    void RootMotionWarpContext::PrepareForWarp()
    {
        m_warpedRootMotion.Clear();

        #if EE_DEVELOPMENT_TOOLS
        m_log.Clear();
        EE::Delete( m_pDebugData );
        #endif
    }

    #if EE_DEVELOPMENT_TOOLS
    void RootMotionWarpContext::LogError( char const* pFormat, ... )
    {
        if ( !IsDebuggingEnabled() )
        {
            return;
        }

        va_list args;
        va_start( args, pFormat );
        m_log.LogErrorVarArgs( pFormat, args );
        va_end( args );
    }

    void RootMotionWarpContext::LogWarning( char const* pFormat, ... )
    {
        if ( !IsDebuggingEnabled() )
        {
            return;
        }

        va_list args;
        va_start( args, pFormat );
        m_log.LogWarningVarArgs( pFormat, args );
        va_end( args );
    }

    void RootMotionWarpContext::DrawDebug( DebugDrawContext& drawCtx ) const
    {
        if ( m_warpedRootMotion.IsValid() )
        {
            m_warpedRootMotion.DrawDebug( drawCtx, Transform::Identity );
        }

        if ( m_pDebugData != nullptr )
        {
            m_pDebugData->DrawDebug( *this, drawCtx );
        }
    }

    void RootMotionWarpContext::SetDebugData( IRootMotionWarpDebugData* pDebugData )
    {
        EE::Delete( m_pDebugData );
        m_pDebugData = pDebugData;
    }

    // Root Motion Warp Debug Data
    //-------------------------------------------------------------------------

    class RootMotionWarpDebugData final : public IRootMotionWarpDebugData
    {
    public:

        RootMotionWarpDebugData( Transform const& targetTransform, Transform const& localToWorld, RootMotionWarper::Sections const& sections )
            : m_targetTransform( targetTransform )
            , m_localToWorld( localToWorld )
            , m_sections( sections )
        {}

        virtual void DrawDebug( RootMotionWarpContext const& context, DebugDrawContext& drawCtx ) const override
        {
            // Draw Target
            drawCtx.DrawAxis( m_targetTransform, 0.1f, 5.0f );

            // Draw Section debug
            constexpr float const debugPointSize = 15.0f;

            RootMotionData const& warpedRootMotion = context.GetWarpedRootMotion();

            for ( auto i = 0u; i < m_sections.m_sections.size(); i++ )
            {
                auto const& section = m_sections.m_sections[i];

                Vector const sectionStartPosition = warpedRootMotion.m_transforms[section.m_startFrame].GetTranslation();
                Vector const sectionEndPosition = warpedRootMotion.m_transforms[section.m_endFrame].GetTranslation();

                if ( section.IsFixedSection() )
                {
                    drawCtx.DrawPoint( sectionStartPosition, Colors::White, debugPointSize );
                    drawCtx.DrawPoint( sectionEndPosition, Colors::White, debugPointSize );
                    drawCtx.DrawLine( sectionStartPosition, sectionEndPosition, Colors::White, 3.0f );
                }
                else if ( section.m_warpRule == TargetWarpRule::RotationOnly )
                {
                    Color const ruleColor = GetDebugColorForWarpRule( section.m_warpRule );
                    drawCtx.DrawPoint( sectionStartPosition, ruleColor, debugPointSize );
                    drawCtx.DrawPoint( sectionEndPosition, ruleColor, debugPointSize );
                    drawCtx.DrawLine( sectionStartPosition, sectionEndPosition, ruleColor, 3.0f );

                    Vector const dp0 = m_localToWorld.TransformPoint( section.m_debugPoints[0] );
                    Vector const dp1 = m_localToWorld.TransformPoint( section.m_debugPoints[1] );
                    Vector const dp2 = m_localToWorld.TransformPoint( section.m_debugPoints[2] );

                    drawCtx.DrawArrow( dp0, dp1, Colors::White, 2.0f );
                    drawCtx.DrawArrow( dp0, dp2, ruleColor, 2.0f );
                }
                else // Translation
                {
                    Color const ruleColor = GetDebugColorForWarpRule( section.m_warpRule );
                    drawCtx.DrawPoint( sectionStartPosition, ruleColor, debugPointSize );
                    drawCtx.DrawPoint( sectionEndPosition, ruleColor, debugPointSize );
                    drawCtx.DrawLine( sectionStartPosition, sectionEndPosition, ruleColor, 3.0f );

                    if ( section.m_translationAlgorithm == TargetWarpAlgorithm::Bezier )
                    {
                        // Four points
                        Vector const dp0 = m_localToWorld.TransformPoint( section.m_debugPoints[0] );
                        Vector const dp1 = m_localToWorld.TransformPoint( section.m_debugPoints[1] );
                        Vector const dp2 = m_localToWorld.TransformPoint( section.m_debugPoints[2] );
                        Vector const dp3 = m_localToWorld.TransformPoint( section.m_debugPoints[3] );

                        drawCtx.DrawArrow( dp0, dp1, Colors::White, 2.0f );
                        drawCtx.DrawArrow( dp3, dp2, ruleColor, 2.0f );
                    }
                    else
                    {
                        // Two points and two tangents
                        Vector const dp0 = m_localToWorld.TransformPoint( section.m_debugPoints[0] );
                        Vector const dp1 = m_localToWorld.TransformPoint( section.m_debugPoints[0] + section.m_debugPoints[1] );
                        Vector const dp2 = m_localToWorld.TransformPoint( section.m_debugPoints[2] );
                        Vector const dp3 = m_localToWorld.TransformPoint( section.m_debugPoints[2] + section.m_debugPoints[3] );

                        drawCtx.DrawArrow( dp0, dp1, Colors::White, 2.0f );
                        drawCtx.DrawArrow( dp2, dp3, ruleColor, 2.0f );
                    }
                }
            }
        }

    private:

        Transform                               m_targetTransform;
        Transform                               m_localToWorld;
        RootMotionWarper::Sections              m_sections;
    };
    #endif

    // Root Motion Warper
    //-------------------------------------------------------------------------

    bool RootMotionWarper::Warp( RootMotionWarpContext& context, float maxTangentLength, float lerpFallbackDistanceThreshold )
    {
        context.PrepareForWarp();

        Sections sections;
        if ( !InitializeWarpSections( context, InitializeWarpSectionsMode::Full, &sections ) )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogError( "Invalid warp sections to perform a root motion warp for the given start time!" );
            #endif
            return false;
        }

        RootMotionData const& originalRM = context.m_rootMotion;
        EE_ASSERT( originalRM.IsValid() );

        int32_t const numFrames = originalRM.GetNumFrames();
        EE_ASSERT( numFrames > 0 );

        Section const& lastWarpSection = sections.m_sections.back();
        int32_t const targetAlignmentFrameIdx = context.IsAlignTargetAtLastSection() ? lastWarpSection.m_endFrame : numFrames - 1;

        // Calculate the warp start transform
        //-------------------------------------------------------------------------

        Transform warpStartWorldTransform = context.GetStartTransform();

        // Handle the case were we don't start the animation from the first frame
        if ( !context.GetStartTime().IsAtStart() )
        {
            // Offset the warp start transform by the distance we've 'already' covered due to our later start time
            Transform const rootTransformAtActualStartTime = originalRM.GetTransform( context.GetStartTime() );
            Transform const rootTransformOffset = rootTransformAtActualStartTime.GetInverse();
            warpStartWorldTransform = rootTransformOffset * warpStartWorldTransform;
        }

        // Calculate actual target
        //-------------------------------------------------------------------------

        Transform targetTransform = context.GetTargetTransform();

        if ( sections.m_translationXYSectionIdx == InvalidIndex || !sections.m_isTranslationAllowedZ )
        {
            Transform const requestedWarpTargetCS = context.GetTargetTransform() * warpStartWorldTransform.GetInverse();
            Transform warpTargetCS = requestedWarpTargetCS;

            int32_t const originalRootMotionFrame = originalRM.IsStationary() ? 0 : targetAlignmentFrameIdx;
            Transform const& unwarpedTargetCS = originalRM.m_transforms[originalRootMotionFrame];

            // If we don't allow horizontal translation, just keep the target's Z value
            if ( sections.m_translationXYSectionIdx == InvalidIndex )
            {
                Vector adjustedTranslation = unwarpedTargetCS.GetTranslation();
                adjustedTranslation.SetZ( requestedWarpTargetCS.GetTranslation().GetZ() );
                warpTargetCS.SetTranslation( adjustedTranslation );
            }

            // If we don't allow vertical translation, just keep the target's XY value
            if ( !sections.m_isTranslationAllowedZ )
            {
                Vector adjustedTranslation = requestedWarpTargetCS.GetTranslation();
                Vector const originalFinalTranslation = unwarpedTargetCS.GetTranslation();
                adjustedTranslation.SetZ( originalFinalTranslation.GetZ() );
                warpTargetCS.SetTranslation( adjustedTranslation );
            }

            targetTransform = warpTargetCS * warpStartWorldTransform;
        }

        // Prepare for warping
        //-------------------------------------------------------------------------

        // Create storage for warped root motion
        TVector<Transform>& warpedTransforms = context.m_warpedRootMotion.m_transforms;
        warpedTransforms.resize( numFrames, Transform::Identity );
        context.m_warpedRootMotion.m_numFrames = numFrames;

        // Add the unwarped start and end portions of the root motion
        //-------------------------------------------------------------------------

        // Set all unwarped start frames
        for ( auto i = 0; i <= sections.m_sections[0].m_startFrame; i++ )
        {
            warpedTransforms[i] = originalRM.m_transforms[i];
        }

        // Set the frame at which we want to be aligned with our target to that target
        warpedTransforms[targetAlignmentFrameIdx] = targetTransform * warpStartWorldTransform.GetInverse();

        // Add trailing unwarped frames
        for ( int32_t frameIdx = targetAlignmentFrameIdx + 1; frameIdx < numFrames; frameIdx++ )
        {
            warpedTransforms[frameIdx] = context.m_deltaTransforms[frameIdx] * warpedTransforms[frameIdx - 1];
        }

        for ( int32_t frameIdx = targetAlignmentFrameIdx - 1; frameIdx >= lastWarpSection.m_endFrame; frameIdx-- )
        {
            warpedTransforms[frameIdx] = context.m_inverseDeltaTransforms[frameIdx + 1] * warpedTransforms[frameIdx + 1];
        }

        // Forward Solve
        //-------------------------------------------------------------------------
        // Stops on XY or R sections

        int32_t const numSections = (int32_t) sections.m_sections.size();

        // What the delta between where we end up and where we want to be?
        Transform const warpDelta = Transform::DeltaNoScale( originalRM.m_transforms[lastWarpSection.m_endFrame], warpedTransforms[lastWarpSection.m_endFrame] );

        auto CalculateCorrectionZ = [&] ( Section const& warpSection )
        {
            float correctionZ = warpDelta.GetTranslation().GetZ();
            if ( sections.m_numSectionZ > 1 )
            {
                float const percentageCorrectionToApply = float( warpSection.GetNumWarpableFrames() ) / sections.m_totalNumWarpableZFrames;
                correctionZ *= percentageCorrectionToApply;
            }
            return correctionZ;
        };

        // Forward solve (stop on Rotation section or XY section)
        int32_t const rIdx = sections.m_rotationSectionIdx == InvalidIndex ? numSections : sections.m_rotationSectionIdx;
        int32_t const tIdx = sections.m_translationXYSectionIdx == InvalidIndex ? numSections : sections.m_translationXYSectionIdx;
        int32_t const forwardEndIdx = Math::Min( rIdx, tIdx );
        for ( auto i = 0; i < forwardEndIdx; i++ )
        {
            auto const& warpSection = sections.m_sections[i];
            if ( warpSection.IsFixedSection() )
            {
                Transform const& sectionStartTransform = warpedTransforms[warpSection.m_startFrame];
                SolveFixedSection( context, warpSection, sectionStartTransform );
            }
            else if ( warpSection.m_warpRule == TargetWarpRule::WarpZ )
            {
                Transform const& sectionStartTransform = warpedTransforms[warpSection.m_startFrame];
                SolveTranslationZSection( context, warpSection, sectionStartTransform, CalculateCorrectionZ( warpSection ) );
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
            auto const& warpSection = sections.m_sections[i];
            if ( warpSection.IsFixedSection() )
            {
                Transform const sectionStartTransform = warpSection.m_deltaTransform.GetInverse() * warpedTransforms[warpSection.m_endFrame];
                SolveFixedSection( context, warpSection, sectionStartTransform );
            }
            else if ( warpSection.m_warpRule == TargetWarpRule::WarpZ )
            {
                float const correctionZ = CalculateCorrectionZ( warpSection );
                Transform sectionStartTransform = warpSection.m_deltaTransform.GetInverse() * warpedTransforms[warpSection.m_endFrame];
                sectionStartTransform.AddTranslation( Vector( 0, 0, -correctionZ ) );
                SolveTranslationZSection( context, warpSection, sectionStartTransform, correctionZ );
            }
            else if ( warpSection.m_warpRule == TargetWarpRule::RotationOnly )
            {
                // We need to calculate the start transform for this section,
                // in doing so we will be determining the target transform of the XY (T) segment
                Transform const& sectionTargetTransform = warpedTransforms[warpSection.m_endFrame];
                Transform const& startTransformT = warpedTransforms[sections.m_sections[tIdx].m_startFrame];

                // Calculate the delta between the XY section and the rotation (including the rotation section)
                Transform unwarpedDeltaPostT = sections.m_sections[tIdx + 1].m_deltaTransform;
                for ( auto j = tIdx + 2; j <= i; j++ )
                {
                    unwarpedDeltaPostT = sections.m_sections[j].m_deltaTransform * unwarpedDeltaPostT;
                }

                Vector alignmentDir;

                // Align all the section post T to the incoming movement dir for T's start frame
                Vector const startToTarget = sectionTargetTransform.GetTranslation() - startTransformT.GetTranslation();
                if ( startToTarget.IsNearZero2() )
                {
                    alignmentDir = ( warpedTransforms.back().GetTranslation() - sectionTargetTransform.GetTranslation() );

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
                sectionStartTransform.SetTranslation( Vector::NegativeMultiplySubtract( alignmentDir, deltaDistance, sectionTargetTransform.GetTranslation() ) );

                // Put the section start location at the correct height so it doesn't get doubled
                sectionStartTransform.AddTranslation( Vector::WorldDown * unwarpedDeltaPostT.GetTranslation().GetZ() );

                // Use IncomingMovementOrientation so that it allows placing translation section on the last frame of translation
                Quaternion const unwarpedMovementOrientation = originalRM.GetIncomingMovementOrientation2DAtFrame( sections.m_sections[tIdx].m_endFrame );
                Quaternion const originalFacingOrientation = originalRM.m_transforms[sections.m_sections[tIdx].m_endFrame].GetRotation();
                Quaternion const offset = Quaternion::Delta( unwarpedMovementOrientation, originalFacingOrientation );
                sectionStartTransform.SetRotation( offset * Quaternion::FromRotationBetweenUnitVectors( Vector::WorldForward, alignmentDir ) );

                // Calculate the start transform for this section by applying the delta for each section
                for ( auto j = tIdx + 1; j < i; j++ )
                {
                    sectionStartTransform = sections.m_sections[j].m_deltaTransform * sectionStartTransform;
                }

                // Calculate the required delta and solve segment
                Transform const estimatedSectionEndTransform = warpSection.m_deltaTransform * sectionStartTransform;
                Quaternion const requiredCorrection = Quaternion::Delta( estimatedSectionEndTransform.GetRotation(), sectionTargetTransform.GetRotation() );

                #if EE_DEVELOPMENT_TOOLS
                if ( context.IsDebuggingEnabled() )
                {
                    warpSection.m_debugPoints[0] = sectionTargetTransform.GetTranslation();
                    warpSection.m_debugPoints[1] = sectionTargetTransform.GetTranslation() + unwarpedMovementOrientation.RotateVector( Vector::WorldForward );
                    warpSection.m_debugPoints[2] = sectionTargetTransform.GetTranslation() + estimatedSectionEndTransform.GetRotation().RotateVector( Vector::WorldForward );
                }
                #endif

                SolveRotationSection( context, warpSection, sectionStartTransform, requiredCorrection );
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
            auto const& warpSection = sections.m_sections[i];
            if ( warpSection.IsFixedSection() )
            {
                Transform const& sectionStartTransform = warpedTransforms[warpSection.m_startFrame];
                SolveFixedSection( context, warpSection, sectionStartTransform );
            }
            else if ( warpSection.m_warpRule == TargetWarpRule::RotationOnly )
            {
                Quaternion requiredCorrection;

                // The target that we want to align to is the XY section's end transform as that was set in the backward pass
                Transform const& sectionStartTransform = warpedTransforms[warpSection.m_startFrame];
                Transform const sectionEndTransform = warpSection.m_deltaTransform * sectionStartTransform;
                Transform const& sectionTargetTransform = warpedTransforms[sections.m_sections[backwardsEndIdx].m_endFrame];

                // If we end up at the target at the end of the segment, align to end orientation
                Vector const toTarget = ( sectionTargetTransform.GetTranslation() - sectionEndTransform.GetTranslation() );
                if ( toTarget.IsNearZero2() )
                {
                    requiredCorrection = Quaternion::Delta( sectionEndTransform.GetRotation(), sectionTargetTransform.GetRotation() );

                    #if EE_DEVELOPMENT_TOOLS
                    if ( context.IsDebuggingEnabled() )
                    {
                        warpSection.m_debugPoints[0] = sectionEndTransform.GetTranslation();
                        warpSection.m_debugPoints[1] = sectionEndTransform.GetTranslation() + sectionEndTransform.GetRotation().RotateVector( Vector::WorldForward );
                        warpSection.m_debugPoints[2] = sectionEndTransform.GetTranslation() + sectionTargetTransform.GetRotation().RotateVector( Vector::WorldForward );
                    }
                    #endif
                }
                else
                {
                    // Calculate estimated unwarped movement
                    Transform const endFramePlusOne = context.m_deltaTransforms[warpSection.m_endFrame + 1] * sectionEndTransform;
                    Vector outgoingMovement = endFramePlusOne.GetTranslation() - sectionEndTransform.GetTranslation();
                    if ( outgoingMovement.IsNearZero2() )
                    {
                        outgoingMovement = sectionEndTransform.GetRotation().RotateVector( Vector::WorldForward );
                    }

                    #if EE_DEVELOPMENT_TOOLS
                    if ( context.IsDebuggingEnabled() )
                    {
                        warpSection.m_debugPoints[0] = sectionEndTransform.GetTranslation();
                        warpSection.m_debugPoints[1] = sectionEndTransform.GetTranslation() + outgoingMovement.GetNormalized2();
                        warpSection.m_debugPoints[2] = sectionEndTransform.GetTranslation() + toTarget.GetNormalized2();
                    }
                    #endif

                    requiredCorrection = Quaternion::FromRotationBetweenUnitVectors( outgoingMovement.GetNormalized2(), toTarget.GetNormalized2() );
                }

                SolveRotationSection( context, warpSection, sectionStartTransform, requiredCorrection );
            }
            else if ( warpSection.m_warpRule == TargetWarpRule::WarpXY || warpSection.m_warpRule == TargetWarpRule::WarpXYZ )
            {
                Transform const& sectionStartTransform = warpedTransforms[warpSection.m_startFrame];
                Transform const& sectionEndTransform = warpedTransforms[warpSection.m_endFrame];
                SolveTranslationSection( context, maxTangentLength, lerpFallbackDistanceThreshold, warpSection, sectionStartTransform, sectionEndTransform );
            }
            else if ( warpSection.m_warpRule == TargetWarpRule::WarpZ )
            {
                Transform const& sectionStartTransform = warpedTransforms[warpSection.m_startFrame];
                SolveTranslationZSection( context, warpSection, sectionStartTransform, CalculateCorrectionZ( warpSection ) );
            }
            else
            {
                EE_UNREACHABLE_CODE();
            }
        }

        // Put the warped root motion to the world space for the world space sampling mode
        for ( Transform& warpedRootMotionTransform : warpedTransforms )
        {
            warpedRootMotionTransform = warpedRootMotionTransform * warpStartWorldTransform;
        }

        #if EE_DEVELOPMENT_TOOLS
        if ( context.IsDebuggingEnabled() )
        {
            context.SetDebugData( EE::New<RootMotionWarpDebugData>( targetTransform, warpStartWorldTransform, sections ) );
        }
        #endif

        return true;
    }

    bool RootMotionWarper::InitializeWarpSections( RootMotionWarpContext const& context, InitializeWarpSectionsMode mode, Sections* pOutSections, FrameTime const* pStartTimeOverride )
    {
        EE_ASSERT( pOutSections != nullptr );

        RootMotionData const& originalRM = context.m_rootMotion;
        EE_ASSERT( originalRM.IsValid() );

        // Read warp events
        //-------------------------------------------------------------------------

        FrameTime const startTime = ( pStartTimeOverride != nullptr ) ? *pStartTimeOverride : context.GetStartTime();
        int32_t const minimumStartFrame = startTime.GetFrameIndex() + 1;

        for ( auto const& contextSection : context.m_sections )
        {
            if ( contextSection.m_endFrame < minimumStartFrame )
            {
                continue;
            }

            // Create a section per warp event
            Section section;
            section.m_startFrame = contextSection.m_startFrame;
            section.m_endFrame = contextSection.m_endFrame;
            section.m_warpRule = contextSection.m_warpRule;
            section.m_translationAlgorithm = contextSection.m_translationAlgorithm;

            // Adjust start frame to animation start time
            if ( contextSection.m_startFrame <= minimumStartFrame )
            {
                section.m_startFrame = minimumStartFrame;

                // Skip 1 frame sections
                if ( !section.HasValidFrameRange() )
                {
                    continue;
                }
            }

            EE_ASSERT( section.m_startFrame < section.m_endFrame );

            // Should we insert a fixed section?
            if ( !pOutSections->m_sections.empty() )
            {
                if ( pOutSections->m_sections.back().m_endFrame != section.m_startFrame )
                {
                    Section fixedSection;
                    fixedSection.m_startFrame = pOutSections->m_sections.back().m_endFrame;
                    fixedSection.m_endFrame = section.m_startFrame;
                    fixedSection.m_warpRule = TargetWarpRule::FixedSection;
                    pOutSections->m_sections.emplace_back( fixedSection );
                }
            }

            // Add new section
            pOutSections->m_sections.emplace_back( section );

            // Track the options for this warp
            switch ( section.m_warpRule )
            {
                case TargetWarpRule::WarpXY:
                {
                    pOutSections->m_translationXYSectionIdx = (int8_t) pOutSections->m_sections.size() - 1;
                }
                break;

                case TargetWarpRule::WarpZ:
                {
                    pOutSections->m_isTranslationAllowedZ = true;
                }
                break;

                case TargetWarpRule::WarpXYZ:
                {
                    pOutSections->m_translationXYSectionIdx = (int8_t) pOutSections->m_sections.size() - 1;
                    pOutSections->m_isTranslationAllowedZ = true;
                }
                break;

                case TargetWarpRule::RotationOnly:
                {
                    pOutSections->m_rotationSectionIdx = (int8_t) pOutSections->m_sections.size() - 1;
                }
                break;

                case TargetWarpRule::FixedSection:
                {
                    // Do Nothing
                }
                break;
            }
        }

        if ( pOutSections->m_translationXYSectionIdx == InvalidIndex && !pOutSections->m_isTranslationAllowedZ )
        {
            return false;
        }

        if ( mode == InitializeWarpSectionsMode::ValidityCheckOnly )
        {
            return true;
        }

        // Calculate section info
        //-------------------------------------------------------------------------

        pOutSections->m_numSectionZ = 0;
        pOutSections->m_totalNumWarpableZFrames = 0;

        for ( auto& section : pOutSections->m_sections )
        {
            // Calculate the overall delta transform for the section
            section.m_deltaTransform = Transform::DeltaNoScale( originalRM.m_transforms[section.m_startFrame], originalRM.m_transforms[section.m_endFrame] );

            // For fixed sections, we don't care about the length
            if ( section.IsFixedSection() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            section.m_totalProgress.resize( section.m_endFrame - section.m_startFrame + 1, 0.0f );
            section.m_distanceCovered = 0.0f;

            // Rotation
            if ( section.m_warpRule == TargetWarpRule::RotationOnly )
            {
                for ( auto i = section.m_startFrame + 1; i <= section.m_endFrame; i++ )
                {
                    section.m_hasTranslation |= context.m_deltaTransforms[i].GetTranslation().GetLengthSquared3() > 0;
                    float const angularDistance = Math::Abs( context.m_deltaTransforms[i].GetRotation().GetAngle().ToFloat() );
                    section.m_totalProgress[i - section.m_startFrame] = angularDistance;
                    section.m_distanceCovered += angularDistance;
                }
            }
            else // Translation
            {
                for ( auto i = section.m_startFrame + 1; i <= section.m_endFrame; i++ )
                {
                    float const distance = context.m_deltaTransforms[i].GetTranslation().GetLength3();
                    section.m_totalProgress[i - section.m_startFrame] = distance;
                    section.m_distanceCovered += distance;
                }

                if ( section.m_warpRule == TargetWarpRule::WarpZ || section.m_warpRule == TargetWarpRule::WarpXYZ )
                {
                    pOutSections->m_numSectionZ++;
                    pOutSections->m_totalNumWarpableZFrames += section.GetNumWarpableFrames();
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

        return true;
    }

    void RootMotionWarper::SolveFixedSection( RootMotionWarpContext& context, Section const& section, Transform const& startTransform )
    {
        EE_ASSERT( section.IsFixedSection() );

        TVector<Transform>& warpedTransforms = context.m_warpedRootMotion.m_transforms;
        warpedTransforms[section.m_startFrame] = startTransform;

        //-------------------------------------------------------------------------

        for ( auto f = section.m_startFrame + 1; f <= section.m_endFrame; f++ )
        {
            warpedTransforms[f] = context.m_deltaTransforms[f] * warpedTransforms[f - 1];
        }
    }

    void RootMotionWarper::SolveTranslationZSection( RootMotionWarpContext& context, Section const& section, Transform const& startTransform, float correctionZ )
    {
        EE_ASSERT( section.m_warpRule == TargetWarpRule::WarpZ );

        TVector<Transform>& warpedTransforms = context.m_warpedRootMotion.m_transforms;
        warpedTransforms[section.m_startFrame] = startTransform;

        //-------------------------------------------------------------------------

        for ( auto f = section.m_startFrame + 1; f <= section.m_endFrame; f++ )
        {
            warpedTransforms[f] = context.m_deltaTransforms[f] * warpedTransforms[f - 1];
            int32_t const stepIdx = f - section.m_startFrame;
            float const progressThisFrame = ( section.m_totalProgress[stepIdx] - section.m_totalProgress[stepIdx - 1] );
            warpedTransforms[f].AddTranslation( Vector( 0, 0, correctionZ * progressThisFrame, 0 ) );
        }
    }

    void RootMotionWarper::SolveTranslationSection( RootMotionWarpContext& context, float maxTangentLength, float lerpFallbackDistanceThreshold, Section const& section, Transform const& startTransform, Transform const& endTransform )
    {
        EE_ASSERT( section.m_warpRule == TargetWarpRule::WarpXY || section.m_warpRule == TargetWarpRule::WarpXYZ );

        RootMotionData const& originalRM = context.m_rootMotion;
        EE_ASSERT( originalRM.IsValid() );

        //-------------------------------------------------------------------------

        Vector const& startPoint = startTransform.GetTranslation();
        Vector const& endPoint = endTransform.GetTranslation();
        float const desiredDistanceToCover = startPoint.GetDistance3( endPoint );

        TVector<Transform>& warpedTransforms = context.m_warpedRootMotion.m_transforms;
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
            context.LogWarning( "Trying to XY warp a section with no displacement, falling back to LERP!" );
            #endif
        }

        // If the distance to be covered is under the threshold, then just LERP as trying to fit a curve will look ugly
        if ( !triggerFallback && desiredDistanceToCover <= lerpFallbackDistanceThreshold )
        {
            triggerFallback = true;

            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( "Trying to XY warp to under the distance threshold, falling back to LERP!" );
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
                triggerFallback = true;

                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( "XY warp curve blowout detected! Falling back to LERP!" );
                #endif
            }

            return !triggerFallback;
        };

        // Basic Curve Generation
        //-------------------------------------------------------------------------

        if ( !triggerFallback && ( section.m_translationAlgorithm == TargetWarpAlgorithm::Hermite || section.m_translationAlgorithm == TargetWarpAlgorithm::Bezier ) )
        {
            // Calculate the tangents for the curves
            Vector startTangent = ( ( context.m_deltaTransforms[section.m_startFrame + 1] * startTransform ).GetTranslation() - startTransform.GetTranslation() ).GetNormalized3();
            Vector endTangent = ( endTransform.GetTranslation() - ( context.m_inverseDeltaTransforms[section.m_endFrame] * endTransform ).GetTranslation() ).GetNormalized3();

            // If the tangents are in the same direction but we are moving in the opposite direction of the start tangent and the original displacement isn't similar then just LERP
            bool const tangentsInSameHemisphere = Math::IsVectorInTheSameHemisphere2D( startTangent, endTangent );
            if ( tangentsInSameHemisphere )
            {
                if ( Line( Line::StartDirection, startPoint, startTangent ).GetDistanceAlongLineToProjectedPoint( endPoint ) <= 0 )
                {
                    triggerFallback = true;

                    #if EE_DEVELOPMENT_TOOLS
                    context.LogWarning( "Trying to XY warp backwards, falling back to LERP!" );
                    #endif
                }
            }

            if ( !triggerFallback )
            {
                // Scale tangents - We use a half distance value clamped to the specified max length in the settings
                float const scalingFactor = Math::Min( maxTangentLength, endPoint.GetDistance3( startPoint ) / 2 );
                startTangent *= scalingFactor;
                endTangent *= scalingFactor;

                // Hermite
                if ( section.m_translationAlgorithm == TargetWarpAlgorithm::Hermite )
                {
                    float const curveLength = Math::CubicHermite::GetSplineLength( startPoint, startTangent, endPoint, endTangent );
                    if ( EnsureValidCurve( curveLength ) )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        if ( context.IsDebuggingEnabled() )
                        {
                            section.m_debugPoints[0] = startPoint;
                            section.m_debugPoints[1] = startTangent;
                            section.m_debugPoints[2] = endPoint;
                            section.m_debugPoints[3] = endTangent;
                        }
                        #endif

                        //-------------------------------------------------------------------------

                        int32_t const numWarpFrames = section.m_endFrame - section.m_startFrame;
                        for ( auto i = 1; i < numWarpFrames; i++ )
                        {
                            int32_t const frameIdx = section.m_startFrame + i;

                            Vector const curvePoint = Math::CubicHermite::GetPoint( startPoint, startTangent, endPoint, endTangent, section.m_totalProgress[i] );
                            warpedTransforms[frameIdx].SetTranslation( curvePoint );

                            Quaternion const orientation = CalculateWarpedOrientationForFrame( originalRM, context.m_warpedRootMotion, frameIdx );
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
                        if ( context.IsDebuggingEnabled() )
                        {
                            section.m_debugPoints[0] = startPoint;
                            section.m_debugPoints[1] = cp0;
                            section.m_debugPoints[2] = cp1;
                            section.m_debugPoints[3] = endPoint;
                        }
                        #endif

                        int32_t const numWarpFrames = section.m_endFrame - section.m_startFrame;
                        for ( auto i = 1; i < numWarpFrames; i++ )
                        {
                            int32_t const frameIdx = section.m_startFrame + i;

                            Vector const curvePoint = Math::CubicBezier::GetPoint( startPoint, cp0, cp1, endPoint, section.m_totalProgress[i] );
                            warpedTransforms[frameIdx].SetTranslation( curvePoint );

                            Quaternion const orientation = CalculateWarpedOrientationForFrame( originalRM, context.m_warpedRootMotion, frameIdx );
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
            if ( size_t( section.m_endFrame + 1 ) < context.m_deltaTransforms.size() )
            {
                Transform const endPlusOne = context.m_deltaTransforms[section.m_endFrame + 1] * endTransform;
                hermiteEndTangent = ( endPlusOne.GetTranslation() - endPoint ).GetNormalized3();
            }
            else // This is the last segment so just use in the entry direction
            {
                Transform const end = context.m_inverseDeltaTransforms[section.m_endFrame] * endTransform;
                hermiteEndTangent = ( endPoint - end.GetTranslation() ).GetNormalized3();
            }

            if ( hermiteEndTangent.IsNearZero3() )
            {
                hermiteEndTangent = endTransform.GetForwardVector();
            }

            // Scale guide curve tangents
            //-------------------------------------------------------------------------

            float const scalingFactor = Math::Min( maxTangentLength, endPoint.GetDistance3( startPoint ) / 2 );
            hermiteStartTangent *= scalingFactor;
            hermiteEndTangent *= scalingFactor;

            // Calculate original RM scale factor
            //-------------------------------------------------------------------------

            float const curveLength = Math::CubicHermite::GetSplineLength( startPoint, hermiteStartTangent, endPoint, hermiteEndTangent );
            float const rootMotionScaleFactor = curveLength / sectionDeltaDistance;
            if ( rootMotionScaleFactor < g_minAllowedRootMotionScale )
            {
                triggerFallback = true;

                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( "Feature preserving XY warp required scaling is lower than the minimum allowed threshold. Falling back to LERP!" );
                #endif
            }

            //-------------------------------------------------------------------------

            if ( !triggerFallback && EnsureValidCurve( curveLength ) )
            {
                #if EE_DEVELOPMENT_TOOLS
                if ( context.IsDebuggingEnabled() )
                {
                    section.m_debugPoints[0] = startPoint;
                    section.m_debugPoints[1] = hermiteStartTangent;
                    section.m_debugPoints[2] = endPoint;
                    section.m_debugPoints[3] = hermiteEndTangent;
                }
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
                    currentTransform = context.m_deltaTransforms[frameIdx] * currentTransform;
                    Transform const scaledCurrentTransform( currentTransform.GetRotation(), currentTransform.GetTranslation() * rootMotionScaleFactor );
                    Transform const straightTransform( sectionDeltaOrientation, Vector::Lerp( startPoint, scaledUnwarpedEndPoint, percentageAlongCurve ) );
                    Transform const curveDelta = Transform::DeltaNoScale( straightTransform, scaledCurrentTransform );

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
                warpedTransforms[frameIdx] = Transform::SLerp( startTransform, endTransform, section.m_totalProgress[i] );
            }
        }
    }

    void RootMotionWarper::SolveRotationSection( RootMotionWarpContext& context, Section const& section, Transform const& startTransform, Quaternion const& correction )
    {
        EE_ASSERT( section.m_warpRule == TargetWarpRule::RotationOnly );

        TVector<Transform>& warpedTransforms = context.m_warpedRootMotion.m_transforms;
        warpedTransforms[section.m_startFrame] = startTransform;

        // Generate base root motion
        //-------------------------------------------------------------------------

        for ( auto f = section.m_startFrame + 1; f <= section.m_endFrame; f++ )
        {
            warpedTransforms[f] = context.m_deltaTransforms[f] * warpedTransforms[f - 1];
        }

        // Modify facing
        //-------------------------------------------------------------------------

        for ( auto f = section.m_startFrame + 1; f <= section.m_endFrame; f++ )
        {
            Quaternion const frameDelta = Quaternion::SLerp( Quaternion::Identity, correction, section.m_totalProgress[f - section.m_startFrame] );
            Quaternion const adjustedRotation = frameDelta * warpedTransforms[f].GetRotation();
            warpedTransforms[f].SetRotation( adjustedRotation );
        }
    }
}
