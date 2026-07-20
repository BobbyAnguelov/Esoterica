#pragma once

#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Passthrough.h"
#include "Engine/Animation/IK/IK.h"
#include "Base/Math/BlendWeight.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API TwoBoneIKNode final : public PassthroughNode
    {
    public:

        struct EE_ENGINE_API Definition final : public PassthroughNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PassthroughNode::Definition, m_effectorBoneID, m_effectorTargetNodeIdx, m_enableNodeIdx, m_blendTime, m_blendMode, m_isTargetInWorldSpace, m_referencePoseTwistWeight );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            StringID        m_effectorBoneID;
            int16_t         m_effectorTargetNodeIdx = InvalidIndex;
            int16_t         m_enableNodeIdx = InvalidIndex;
            Seconds         m_blendTime = 0.0f;
            IKBlendMode     m_blendMode = IKBlendMode::Effector;
            bool            m_isTargetInWorldSpace = false;
            float           m_referencePoseTwistWeight = 0.0f;
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

        TargetValueNode*    m_pEffectorTargetNode = nullptr;
        BoolValueNode*      m_pEnableNode = nullptr;
        int32_t             m_effectorBoneIdx = InvalidIndex;
        Math::BlendWeight   m_IKWeight;
        bool                m_isValidSetup = false;
    };
}