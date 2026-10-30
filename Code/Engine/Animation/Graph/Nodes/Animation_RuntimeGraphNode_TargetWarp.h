#pragma once

#include "Animation_RuntimeGraphNode_AnimationClip.h"
#include "Engine/Animation/AnimationRootMotionWarp.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API TargetWarpNode final : public PoseNode
    {
        enum class InternalState : uint8_t
        {
            RequiresInitialUpdate,
            AllowUpdates,
            Completed,
            Failed,
        };

        enum class WarpedRootMotionUpdateType : uint8_t
        {
            None,
            Offset,
            Recalculate,
        };

    public:

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
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_clipReferenceNodeIdx, m_targetValueNodeIdx, m_samplingMode, m_targetUpdateRule, m_allowTargetUpdate, m_alignWithTargetAtLastWarpEvent, m_alignmentBoneID, m_samplingPositionErrorThresholdSq, m_maxTangentLength, m_lerpFallbackDistanceThreshold, m_targetUpdateDistanceThreshold, m_targetUpdateAngleThresholdRadians );

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

        void EnsureWarpContextInitialized( GraphContext &context );
        bool TryReadTarget( GraphContext& context, Transform& outTarget );
        WarpedRootMotionUpdateType ResolveWarpedRootMotionUpdateType( Transform const &targetTransform ) const;
        void UpdateWarpContext( GraphContext &context );
        bool UpdateWarp( GraphContext &context );

        // Sample the warped root motion
        void SampleWarpedRootMotion( GraphContext& context, GraphPoseNodeResult& result );

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

        Transform                               m_warpRootTargetTransform; // This is where the code wants the root of the character be. Adjusted for alignment bone if specified.
        Transform                               m_alignmentBoneRootTargetOffset = Transform::Identity; // Transform that is applied to the code requested target

        int32_t                                 m_alignmentBoneIdx = InvalidIndex;
        RootMotionWarpContext                   m_warpContext;
        bool                                    m_performWarp = false;
    };
}