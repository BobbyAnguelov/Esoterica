#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API TimeControlledAnimationClipNode final : public PoseNode
    {

    public:

        struct EE_ENGINE_API Definition final : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_dataSlotIdx, m_playInReverseValueNodeIdx, m_timeValueNodeIdx, m_sampleRootMotion, m_graphEvents );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                                     m_dataSlotIdx = InvalidIndex;
            int16_t                                     m_playInReverseValueNodeIdx = InvalidIndex;
            int16_t                                     m_timeValueNodeIdx = InvalidIndex;
            bool                                        m_sampleRootMotion = true;
            TInlineVector<StringID, 2>	                m_graphEvents;
        };

    public:

        virtual bool IsValid() const override;

        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        virtual SyncTrack const& GetSyncTrack() const override;

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        void SetCurrentTimeFromParameters( GraphContext& context );

        GraphPoseNodeResult CalculateResult( GraphContext& context );

        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual bool RestoreGraphState( RecordedGraphState const& inState ) override;

    private:

        AnimationClip const*                            m_pAnimation = nullptr;
        FloatValueNode*                                 m_pTimeValueNode = nullptr;
        BoolValueNode*                                  m_pPlayInReverseValueNode = nullptr;
        bool                                            m_shouldPlayInReverse = false;
        bool                                            m_isFirstUpdate = true;
        bool                                            m_hasLooped = false;
    };
}