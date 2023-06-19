#pragma once

#include "Animation_RuntimeGraphNode_AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    //-------------------------------------------------------------------------
    // Basic Selector Node
    //-------------------------------------------------------------------------
    // User authored condition logic for selection

    class EE_ENGINE_API SelectorNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_optionNodeIndices, m_conditionNodeIndices );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

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

        int32_t SelectOption( GraphContext& context ) const;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        TInlineVector<PoseNode*, 5>                         m_optionNodes;
        TInlineVector<BoolValueNode*, 5>                    m_conditions;
        PoseNode*                                           m_pSelectedNode = nullptr;
        int32_t                                             m_selectedOptionIdx = InvalidIndex;
    };

    //-------------------------------------------------------------------------
    // Animation Clip Selector Node
    //-------------------------------------------------------------------------
    // User authored condition logic for selection - options are restricted to clip references

    class EE_ENGINE_API AnimationClipSelectorNode final : public AnimationClipReferenceNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_optionNodeIndices, m_conditionNodeIndices );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            TInlineVector<int16_t, 5>                       m_optionNodeIndices;
            TInlineVector<int16_t, 5>                       m_conditionNodeIndices;
        };

    public:

        virtual bool IsValid() const override { return PoseNode::IsValid() && m_pSelectedNode != nullptr; }
        virtual SyncTrack const& GetSyncTrack() const override { return IsValid() ? m_pSelectedNode->GetSyncTrack() : SyncTrack::s_defaultTrack; }

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;

        virtual AnimationClip const* GetAnimation() const override;
        virtual void DisableRootMotionSampling() override;
        virtual bool IsLooping() const override;

        int32_t SelectOption( GraphContext& context ) const;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        TInlineVector<AnimationClipReferenceNode*, 5>       m_optionNodes;
        TInlineVector<BoolValueNode*, 5>                    m_conditions;
        AnimationClipReferenceNode*                         m_pSelectedNode = nullptr;
        int32_t                                             m_selectedOptionIdx = InvalidIndex;
    };

    //-------------------------------------------------------------------------
    // Parameterized Selector
    //-------------------------------------------------------------------------
    // Selection is determined via an input parameter

    class EE_ENGINE_API ParameterizedSelectorNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_optionNodeIndices, m_parameterNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            TInlineVector<int16_t, 5>                       m_optionNodeIndices;
            int16_t                                         m_parameterNodeIdx;
        };

    public:

        virtual bool IsValid() const override { return PoseNode::IsValid() && m_pSelectedNode != nullptr; }
        virtual SyncTrack const& GetSyncTrack() const override { return IsValid() ? m_pSelectedNode->GetSyncTrack() : SyncTrack::s_defaultTrack; }

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;

        int32_t SelectOption( GraphContext& context ) const;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        TInlineVector<PoseNode*, 5>                         m_optionNodes;
        FloatValueNode*                                     m_pParameterNode = nullptr;
        PoseNode*                                           m_pSelectedNode = nullptr;
        int32_t                                             m_selectedOptionIdx = InvalidIndex;
    };

    //-------------------------------------------------------------------------
    // Animation Clip Parameterized Selector Node
    //-------------------------------------------------------------------------
    // Selection is determined via an input parameter - options are restricted to clip references

    class EE_ENGINE_API ParameterizedAnimationClipSelectorNode final : public AnimationClipReferenceNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_optionNodeIndices, m_parameterNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            TInlineVector<int16_t, 5>                       m_optionNodeIndices;
            int16_t                                         m_parameterNodeIdx;
        };

    public:

        virtual bool IsValid() const override { return PoseNode::IsValid() && m_pSelectedNode != nullptr; }
        virtual SyncTrack const& GetSyncTrack() const override { return IsValid() ? m_pSelectedNode->GetSyncTrack() : SyncTrack::s_defaultTrack; }

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;

        virtual AnimationClip const* GetAnimation() const override;
        virtual void DisableRootMotionSampling() override;
        virtual bool IsLooping() const override;

        int32_t SelectOption( GraphContext& context ) const;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        TInlineVector<AnimationClipReferenceNode*, 5>       m_optionNodes;
        FloatValueNode*                                     m_pParameterNode = nullptr;
        AnimationClipReferenceNode*                         m_pSelectedNode = nullptr;
        int32_t                                             m_selectedOptionIdx = InvalidIndex;
    };
}