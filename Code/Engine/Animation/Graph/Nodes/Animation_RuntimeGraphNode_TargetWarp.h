#pragma once

#include "Animation_RuntimeGraphNode_AnimationClip.h"
#include "Engine/Animation/Events/AnimationEvent_Warp.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API TargetWarpNode final : public PoseNode
    {
    public:

        struct WarpSection
        {
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
            bool                                m_isFixedSection = false; // Is this a fixed (unwarpable) section between other warp sections?
            bool                                m_hasTranslation = false;

            #if EE_DEVELOPMENT_TOOLS
            mutable Vector                      m_debugPoints[4];
            #endif
        };

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_clipReferenceNodeIdx, m_targetValueNodeIdx, m_samplingPositionErrorThresholdSq, m_maxTangentLength, m_lerpFallbackDistanceThreshold, m_targetUpdateDistanceThreshold, m_targetUpdateAngleThresholdRadians, m_samplingMode, m_allowTargetUpdate );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                             m_clipReferenceNodeIdx = InvalidIndex;
            int16_t                             m_targetValueNodeIdx = InvalidIndex;
            float                               m_samplingPositionErrorThresholdSq = 0.0f; // The threshold at which we switch from accurate to inaccurate sampling
            float                               m_maxTangentLength = 1.25f;
            float                               m_lerpFallbackDistanceThreshold = 0.1f;
            float                               m_targetUpdateDistanceThreshold = 0.1f;
            float                               m_targetUpdateAngleThresholdRadians = Math::DegreesToRadians * 5.0f;
            RootMotionData::SamplingMode        m_samplingMode = RootMotionData::SamplingMode::Delta;
            bool                                m_allowTargetUpdate = false;
        };

    private:

        virtual bool IsValid() const override;
        virtual SyncTrack const& GetSyncTrack() const override { return m_pClipReferenceNode->GetSyncTrack(); }
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;
        void UpdateShared( GraphContext& context, GraphPoseNodeResult& result );

        bool TryReadTarget( GraphContext& context );
        bool UpdateWarp( GraphContext& context );

        bool GenerateWarpInfo( GraphContext& context, Percentage startTime );
        void ClearWarpInfo();

        // Generate the actual warp root motion
        bool GenerateWarpedRootMotion( GraphContext& context, Percentage startTime );

        // Section Solvers
        //-------------------------------------------------------------------------

        void SolveFixedSection( GraphContext& context, WarpSection const& section, Transform const& startTransform );
        void SolveTranslationZSection( GraphContext& context, WarpSection const& section, Transform const& startTransform, float correctionZ );
        void SolveTranslationSection( GraphContext& context, WarpSection const& section, Transform const& startTransform, Transform const& endTransform );
        void SolveRotationSection( GraphContext& context, WarpSection const& section, Transform const& startTransform, Quaternion const& correction );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        virtual void DrawDebug( GraphContext& graphContext, Drawing::DrawContext& drawCtx ) override;
        #endif

    private:

        AnimationClipReferenceNode*             m_pClipReferenceNode = nullptr;
        TargetValueNode*                        m_pTargetValueNode = nullptr;
        RootMotionData::SamplingMode            m_samplingMode;

        int8_t                                  m_translationXYSectionIdx = InvalidIndex;
        int8_t                                  m_rotationSectionIdx = InvalidIndex;
        bool                                    m_isTranslationAllowedZ = false;
        int8_t                                  m_numSectionZ = 0;
        int32_t                                 m_totalNumWarpableZFrames = 0;

        Transform                               m_requestedWarpTarget; //The warp target that was requested
        Transform                               m_warpTarget; // The actual warp target we can achieve based on events

        Transform                               m_warpStartTransform;
        TVector<Transform>                      m_deltaTransforms;
        TVector<Transform>                      m_inverseDeltaTransforms;
        RootMotionData                          m_warpedRootMotion;

        TInlineVector<WarpSection, 3>           m_warpSections;

        #if EE_DEVELOPMENT_TOOLS
        Transform                               m_characterStartTransform;
        #endif
    };
}