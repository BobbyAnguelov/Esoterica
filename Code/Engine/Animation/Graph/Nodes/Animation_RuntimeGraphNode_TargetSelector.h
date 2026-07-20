#pragma once

#include "Animation_RuntimeGraphNode_AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API TargetSelectorNode final : public AnimationClipReferenceNode
    {
    public:

        struct EE_ENGINE_API Definition final : public AnimationClipReferenceNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( AnimationClipReferenceNode::Definition, m_optionNodeIndices, m_orientationScoreWeight, m_positionScoreWeight, m_parameterNodeIdx, m_ignoreInvalidOptions );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            TInlineVector<int16_t, 5>                       m_optionNodeIndices;
            float                                           m_orientationScoreWeight = 1.0f;
            float                                           m_positionScoreWeight = 1.0f;
            int16_t                                         m_parameterNodeIdx = InvalidIndex;
            bool                                            m_ignoreInvalidOptions = false;
            bool                                            m_isWorldSpaceTarget = false;
        };

    private:

        struct Option
        {
            int32_t                                         m_optionIdx = InvalidIndex;
            Vector                                          m_endPoint;
            Quaternion                                      m_endOrientation;
            float                                           m_orientationScore = 0.0f;
            float                                           m_positionScore = 0.0f;
        };

    public:

        virtual bool IsValid() const override { return AnimationClipReferenceNode::IsValid() && m_pSelectedNode != nullptr; }
        virtual SyncTrack const& GetSyncTrack() const override { return IsValid() ? m_pSelectedNode->GetSyncTrack() : SyncTrack::s_defaultTrack; }

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        virtual bool HasAnimation() const override;
        virtual AnimationClip const* GetAnimation() const override;
        virtual void DisableRootMotionSampling() override;
        virtual bool IsLooping() const override;

        int32_t SelectOption( GraphContext& context, SyncTrackTime const& initialTime );

        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual bool RestoreGraphState( RecordedGraphState const& inState ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void DrawDebug( GraphContext& graphContext, DebugDrawContext& drawCtx ) override;
        #endif

    private:

        TInlineVector<AnimationClipReferenceNode*, 5>       m_optionNodes;
        TargetValueNode*                                    m_pParameterNode = nullptr;
        AnimationClipReferenceNode*                         m_pSelectedNode = nullptr;
        Transform                                           m_targetTransform;
        TInlineVector<Option,10>                            m_selectionOptions;
        int32_t                                             m_selectedOptionIdx = InvalidIndex;

        #if EE_DEVELOPMENT_TOOLS
        Transform                                           m_selectionWorldTransform;
        #endif
    };
}