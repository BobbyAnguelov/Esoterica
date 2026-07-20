#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    // An interface to directly access a selected animation, this is needed to ensure certain animation nodes only operate on animations directly
    class EE_ENGINE_API AnimationClipReferenceNode : public PoseNode
    {
    public:

        virtual bool HasAnimation() const = 0;
        virtual AnimationClip const* GetAnimation() const = 0;
        virtual void DisableRootMotionSampling() = 0;
        virtual bool IsLooping() const = 0;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API AnimationClipNode final : public AnimationClipReferenceNode
    {

    public:

        struct EE_ENGINE_API Definition final : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_dataSlotIdx, m_playInReverseValueNodeIdx, m_resetTimeValueNodeIdx, m_sampleRootMotion, m_allowLooping, m_graphEvents, m_speedMultiplier, m_startSyncEventOffset );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                                     m_dataSlotIdx = InvalidIndex;
            int16_t                                     m_playInReverseValueNodeIdx = InvalidIndex;
            int16_t                                     m_resetTimeValueNodeIdx = InvalidIndex;
            bool                                        m_sampleRootMotion = true;
            bool                                        m_allowLooping = false;
            TInlineVector<StringID, 2>	                m_graphEvents;
            float                                       m_speedMultiplier = 1.0f;
            int32_t                                     m_startSyncEventOffset = 0;
        };

        static void SampleEvents( GraphContext &context, PoseNode* pSourceNode, AnimationClip const* pAnimation, Percentage fromTime, Percentage toTime, bool shouldPlayInReverse );

    public:

        ~AnimationClipNode();

        virtual bool IsValid() const override;

        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        virtual bool HasAnimation() const final { return m_pAnimation != nullptr; }
        virtual AnimationClip const* GetAnimation() const final { EE_ASSERT( IsValid() ); return m_pAnimation; }
        virtual void DisableRootMotionSampling() final { EE_ASSERT( IsValid() ); m_shouldSampleRootMotion = false; }
        virtual bool IsLooping() const final { return GetDefinition<AnimationClipNode>()->m_allowLooping; }

        virtual SyncTrack const& GetSyncTrack() const override;

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        GraphPoseNodeResult CalculateResult( GraphContext& context );

        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual bool RestoreGraphState( RecordedGraphState const& inState ) override;

    private:

        AnimationClip const*                            m_pAnimation = nullptr;
        BoolValueNode*                                  m_pPlayInReverseValueNode = nullptr;
        BoolValueNode*                                  m_pResetTimeValueNode = nullptr;
        SyncTrack*                                      m_pSyncTrack = nullptr;
        bool                                            m_shouldPlayInReverse = false;
        bool                                            m_shouldSampleRootMotion = true;
        bool                                            m_isFirstUpdate = true;
        bool                                            m_hasLooped = false;
    };
}