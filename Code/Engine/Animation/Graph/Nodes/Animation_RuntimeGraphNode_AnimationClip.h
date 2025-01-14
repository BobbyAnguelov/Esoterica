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
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_playInReverseValueNodeIdx, m_resetTimeValueNodeIdx, m_speedMultiplier, m_startSyncEventOffset, m_sampleRootMotion, m_allowLooping, m_dataSlotIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                                     m_dataSlotIdx = InvalidIndex;
            int16_t                                     m_playInReverseValueNodeIdx = InvalidIndex;
            int16_t                                     m_resetTimeValueNodeIdx = InvalidIndex;
            float                                       m_speedMultiplier = 1.0f;
            int32_t                                     m_startSyncEventOffset = 0;
            bool                                        m_sampleRootMotion = true;
            bool                                        m_allowLooping = false;
        };

    public:

        ~AnimationClipNode();

        virtual bool IsValid() const override;

        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        virtual AnimationClip const* GetAnimation() const final { EE_ASSERT( IsValid() ); return m_pAnimation; }
        virtual void DisableRootMotionSampling() final { EE_ASSERT( IsValid() ); m_shouldSampleRootMotion = false; }
        virtual bool IsLooping() const final { return GetDefinition<AnimationClipNode>()->m_allowLooping; }

        virtual SyncTrack const& GetSyncTrack() const override;

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        GraphPoseNodeResult CalculateResult( GraphContext& context ) const;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        AnimationClip const*                            m_pAnimation = nullptr;
        BoolValueNode*                                  m_pPlayInReverseValueNode = nullptr;
        BoolValueNode*                                  m_pResetTimeValueNode = nullptr;
        SyncTrack*                                      m_pSyncTrack = nullptr;
        bool                                            m_shouldPlayInReverse = false;
        bool                                            m_shouldSampleRootMotion = true;
    };
}