#pragma once

#include "Animation_RuntimeGraphNode_Passthrough.h"
#include "Engine/Animation/AnimationBlender.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API RootMotionOverrideNode final : public PassthroughNode
    {
    public:

        // What elements of the root motion do we wish to override
        enum class OverrideFlags : uint8_t
        {
            AllowMoveX,
            AllowMoveY,
            AllowMoveZ,
            AllowFacingPitch,
            ListenForEvents,
        };

        struct EE_ENGINE_API Definition final : public PassthroughNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PassthroughNode::Definition, m_desiredMovingVelocityNodeIdx, m_desiredFacingDirectionNodeIdx, m_linearVelocityLimitNodeIdx, m_angularVelocityLimitNodeIdx, m_maxLinearVelocity, m_maxAngularVelocity, m_overrideFlags );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                             m_desiredMovingVelocityNodeIdx = InvalidIndex;
            int16_t                             m_desiredFacingDirectionNodeIdx = InvalidIndex;
            int16_t                             m_linearVelocityLimitNodeIdx = InvalidIndex;
            int16_t                             m_angularVelocityLimitNodeIdx = InvalidIndex;
            float                               m_maxLinearVelocity = -1.0f;
            Radians                             m_maxAngularVelocity = -1.0f;
            TBitFlags<OverrideFlags>            m_overrideFlags;
        };

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        void ModifyRootMotion( GraphContext& context, GraphPoseNodeResult& nodeResult );

        // Uses events to calculate the weight of the override( 0.0f mean use the original root motion whereas 1.0f means fully override)
        float CalculateOverrideWeight( GraphContext& context );

    private:

        VectorValueNode*                        m_pDesiredMovingVelocityNode = nullptr;
        VectorValueNode*                        m_pDesiredFacingDirectionNode = nullptr;
        FloatValueNode*                         m_pLinearVelocityLimitNode = nullptr;
        FloatValueNode*                         m_pAngularVelocityLimitNode = nullptr;
        float                                   m_blendTime = 0.0f; // The time in the current blend
        float                                   m_desiredBlendDuration = 0.0f; // The total time the blend should take
        BlendState                              m_blendState = BlendState::None;
        bool                                    m_isFirstUpdate = false;
    };
}