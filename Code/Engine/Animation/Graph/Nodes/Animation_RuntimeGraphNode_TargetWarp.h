#pragma once

#include "Animation_RuntimeGraphNode_AnimationClip.h"
#include "Engine/Animation/Events/AnimationEvent_Warp.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API TargetWarpNode final : public PoseNode
    {
        enum class InternalState
        {
            RequiresInitialUpdate,
            AllowUpdates,
            Completed,
            Failed,
        };

    public:

        struct WarpSection
        {
            // Is this a section that should be ignored by the warp
            inline bool IsFixedSection() const { return m_warpRule == TargetWarpRule::FixedSection; }

            // Ensure that the section is at least 3 frames in length (i.e. a single warp frame)
            inline bool HasValidFrameRange() const{ return m_endFrame > m_startFrame + 1; }

            // Get number of warpable frames (including last frame)
            inline int32_t GetNumWarpableFrames() const { return m_endFrame - m_startFrame; }

        public:

            int32_t                             m_startFrame = 0;
            int32_t                             m_endFrame = 0;
            Transform                           m_deltaTransform;
            TVector<float>                      m_totalProgress; // The total progress along the section for each frame
            float                               m_distanceCovered = 0.0f;
            TargetWarpRule                      m_warpRule;
            TargetWarpAlgorithm                 m_translationAlgorithm;
            bool                                m_hasTranslation = false;

            #if EE_DEVELOPMENT_TOOLS
            mutable Vector                      m_debugPoints[4];
            #endif
        };

        enum class TargetUpdateRule : uint8_t
        {
            EE_REFLECT_ENUM

            None = 0,
            Recalculate, // Recalculate Warped Root Motion
            Offset, // Offset Warped Root Motion
            RecalculateOrOffset, // Recalculate Or Offset Warped Root Motion, Will offset the warped root motion if we are pass warp events
        };

        struct EE_ENGINE_API Definition final : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_clipReferenceNodeIdx, m_targetValueNodeIdx, m_samplingMode, m_allowTargetUpdate, m_alignWithTargetAtLastWarpEvent, m_samplingPositionErrorThresholdSq, m_maxTangentLength, m_lerpFallbackDistanceThreshold, m_targetUpdateDistanceThreshold, m_targetUpdateAngleThresholdRadians );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                             m_clipReferenceNodeIdx = InvalidIndex;
            int16_t                             m_targetValueNodeIdx = InvalidIndex;
            RootMotionData::SamplingMode        m_samplingMode = RootMotionData::SamplingMode::Delta;
            TargetUpdateRule                    m_targetUpdateRule = TargetUpdateRule::None;
            bool                                m_allowTargetUpdate = false;
            bool                                m_alignWithTargetAtLastWarpEvent = false;
            StringID                            m_alignmentBoneID;
            float                               m_samplingPositionErrorThresholdSq = 0.0f; // The threshold at which we switch from accurate to inaccurate sampling
            float                               m_maxTangentLength = 1.25f;
            float                               m_lerpFallbackDistanceThreshold = 0.1f;
            float                               m_targetUpdateDistanceThreshold = 0.1f;
            float                               m_targetUpdateAngleThresholdRadians = Math::DegreesToRadians * 5.0f;
        };

    private:

        virtual bool IsValid() const override;
        virtual SyncTrack const& GetSyncTrack() const override { return m_pClipReferenceNode->GetSyncTrack(); }
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        bool TryReadTarget( GraphContext& context );
        bool UpdateWarp( GraphContext& context );
        bool OffsetWarpedRootMotion( GraphContext &context );
        bool CanRecalculateWarpedRootMotion( int32_t startFrame ) const;

        bool GenerateWarpInfo( GraphContext& context );
        void ClearWarpInfo();

        // Generate the actual warp root motion
        void GenerateWarpedRootMotion( GraphContext& context );

        // Sample the warped root motion
        void SampleWarpedRootMotion( GraphContext& context, GraphPoseNodeResult& result );

        // Section Solvers
        //-------------------------------------------------------------------------

        void SolveFixedSection( GraphContext& context, WarpSection const& section, Transform const& startTransform );
        void SolveTranslationZSection( GraphContext& context, WarpSection const& section, Transform const& startTransform, float correctionZ );
        void SolveTranslationSection( GraphContext& context, WarpSection const& section, Transform const& startTransform, Transform const& endTransform );
        void SolveRotationSection( GraphContext& context, WarpSection const& section, Transform const& startTransform, Quaternion const& correction );

        //-------------------------------------------------------------------------

        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual bool RestoreGraphState( RecordedGraphState const& inState ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void DrawDebug( GraphContext& graphContext, DebugDrawContext& drawCtx ) override;
        #endif

    private:

        AnimationClipReferenceNode*             m_pClipReferenceNode = nullptr;
        TargetValueNode*                        m_pTargetValueNode = nullptr;
        RootMotionData::SamplingMode            m_samplingMode = RootMotionData::SamplingMode::WorldSpace;
        InternalState                           m_internalState = InternalState::RequiresInitialUpdate;

        int32_t                                 m_alignmentBoneIdx = InvalidIndex;
        int8_t                                  m_translationXYSectionIdx = InvalidIndex;
        int8_t                                  m_rotationSectionIdx = InvalidIndex;
        bool                                    m_isTranslationAllowedZ = false;
        int8_t                                  m_numSectionZ = 0;
        int32_t                                 m_totalNumWarpableZFrames = 0;

        FrameTime                               m_warpStartTime;
        Transform                               m_warpStartWorldTransform;
        Transform                               m_requestedWarpTarget; //The warp target that was requested
        Transform                               m_warpTarget; // The actual warp target we can achieve based on events

        RootMotionData                          m_warpedRootMotion;

        // Scratch Data - only used for calculations
        //-------------------------------------------------------------------------

        TVector<Transform>                      m_deltaTransforms;
        TVector<Transform>                      m_inverseDeltaTransforms;
        TInlineVector<WarpSection, 3>           m_warpSections;
    };
}