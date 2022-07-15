#pragma once

#include "Animation_RuntimeGraphNode_AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API SelectorNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_optionNodeIndices, m_conditionNodeIndices );

            virtual void InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const override;

            TInlineVector<int16_t, 5>                     m_optionNodeIndices;
            TInlineVector<int16_t, 5>                     m_conditionNodeIndices;
        };

    public:

        virtual bool IsValid() const override { return PoseNode::IsValid() && m_pSelectedNode != nullptr; }
        virtual SyncTrack const& GetSyncTrack() const override { return IsValid() ? m_pSelectedNode->GetSyncTrack() : SyncTrack::s_defaultTrack; }

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;
        virtual void DeactivateBranch( GraphContext& context ) override;

        int32_t SelectOption( GraphContext& context ) const;

    private:

        TInlineVector<PoseNode*, 5>                         m_optionNodes;
        TInlineVector<BoolValueNode*, 5>                    m_conditions;
        PoseNode*                                           m_pSelectedNode = nullptr;
        int32_t                                               m_selectedOptionIdx = InvalidIndex;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API AnimationClipSelectorNode final : public AnimationClipReferenceNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_optionNodeIndices, m_conditionNodeIndices );

            virtual void InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const override;

            TInlineVector<int16_t, 5>                     m_optionNodeIndices;
            TInlineVector<int16_t, 5>                     m_conditionNodeIndices;
        };

    public:

        virtual bool IsValid() const override { return PoseNode::IsValid() && m_pSelectedNode != nullptr; }
        virtual SyncTrack const& GetSyncTrack() const override { return IsValid() ? m_pSelectedNode->GetSyncTrack() : SyncTrack::s_defaultTrack; }

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;
        virtual void DeactivateBranch( GraphContext& context ) override;

        virtual AnimationClip const* GetAnimation() const override;
        virtual void DisableRootMotionSampling() override;
        virtual bool IsLooping() const override;

        int32_t SelectOption( GraphContext& context ) const;

    private:

        TInlineVector<AnimationClipReferenceNode*, 5>       m_optionNodes;
        TInlineVector<BoolValueNode*, 5>                    m_conditions;
        AnimationClipReferenceNode*                         m_pSelectedNode = nullptr;
        int32_t                                               m_selectedOptionIdx = InvalidIndex;
    };
}