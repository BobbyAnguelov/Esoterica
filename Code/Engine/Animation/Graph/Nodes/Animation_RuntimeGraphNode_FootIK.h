#pragma once

#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Passthrough.h"
#include "Engine/Animation/IK/IK.h"
#include "Base/Math/BlendWeight.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API FootIKNode final : public PassthroughNode
    {
    public:

        struct EE_ENGINE_API Definition final : public PassthroughNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PassthroughNode::Definition, m_leftFootBoneID, m_rightFootBoneID, m_leftTargetNodeIdx, m_rightTargetNodeIdx, m_enableNodeIdx, m_blendTime, m_blendMode, m_isTargetInWorldSpace );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            StringID        m_leftFootBoneID;
            StringID        m_rightFootBoneID;
            int16_t         m_leftTargetNodeIdx = InvalidIndex;
            int16_t         m_rightTargetNodeIdx = InvalidIndex;
            int16_t         m_enableNodeIdx = InvalidIndex;
            Seconds         m_blendTime = 0.0f;
            IKBlendMode     m_blendMode = IKBlendMode::Effector;
            bool            m_isTargetInWorldSpace = false;
        };

    public:

        inline float GetIKWeight() const { return m_IKWeight.GetWeight(); }

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual bool IsValid() const override { return m_isValidSetup && PassthroughNode::IsValid(); }
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual bool RestoreGraphState( RecordedGraphState const& inState ) override;

    private:

        TargetValueNode*    m_pLeftTargetNode = nullptr;
        TargetValueNode*    m_pRightTargetNode = nullptr;
        BoolValueNode*      m_pEnableNode = nullptr;
        int32_t             m_leftFootBoneIdx = InvalidIndex;
        int32_t             m_rightFootBoneIdx = InvalidIndex;
        Math::BlendWeight   m_IKWeight;
        bool                m_isValidSetup = false;
    };
}