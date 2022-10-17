#pragma once

#include "Animation_RuntimeGraphNode_Passthrough.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API RootMotionOverrideNode final : public PassthroughNode
    {
    public:

        // What elements of the root motion do we wish to override
        enum class OverrideFlags : uint8_t
        {
            AllowHeadingX,
            AllowHeadingY,
            AllowHeadingZ,
            AllowFacingPitch,
        };

        struct EE_ENGINE_API Settings final : public PassthroughNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PassthroughNode::Settings, m_desiredHeadingVelocityNodeIdx, m_desiredFacingDirectionNodeIdx, m_linearVelocityLimitNodeIdx, m_angularVelocityLimitNodeIdx, m_maxLinearVelocity, m_maxAngularVelocity, m_overrideFlags );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                             m_desiredHeadingVelocityNodeIdx = InvalidIndex;
            int16_t                             m_desiredFacingDirectionNodeIdx = InvalidIndex;
            int16_t                             m_linearVelocityLimitNodeIdx = InvalidIndex;
            int16_t                             m_angularVelocityLimitNodeIdx = InvalidIndex;
            float                               m_maxLinearVelocity = -1.0f;
            float                               m_maxAngularVelocity = -1.0f;
            TBitFlags<OverrideFlags>            m_overrideFlags;
        };

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;

        void ModifyRootMotion( GraphContext& context, GraphPoseNodeResult& nodeResult ) const;

    private:

        VectorValueNode*                        m_pDesiredHeadingVelocityNode = nullptr;
        VectorValueNode*                        m_pDesiredFacingDirectionNode = nullptr;
        FloatValueNode*                         m_pLinearVelocityLimitNode = nullptr;
        FloatValueNode*                         m_pAngularVelocityLimitNode = nullptr;
    };
}