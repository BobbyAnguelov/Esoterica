#pragma once

#include "Animation_RuntimeGraphNode_Passthrough.h"
#include "Animation_RuntimeGraphNode_AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API SpeedScaleBaseNode : public PassthroughNode
    {

    public:

        struct EE_ENGINE_API Definition : public PassthroughNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PassthroughNode::Definition, m_inputValueNodeIdx, m_defaultInputValue );

            int16_t                 m_inputValueNodeIdx = InvalidIndex;
            float                   m_defaultInputValue = 0.0f;
        };

    protected:

        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        virtual float CalculateSpeedScaleMultiplier( GraphContext& context ) const = 0;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SpeedScaleNode final : public SpeedScaleBaseNode
    {
    public:

        struct EE_ENGINE_API Definition final : public SpeedScaleBaseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( SpeedScaleBaseNode::Definition );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual float CalculateSpeedScaleMultiplier( GraphContext& context ) const override;

    private:

        FloatValueNode*             m_pScaleValueNode = nullptr;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API DurationScaleNode final : public SpeedScaleBaseNode
    {
    public:

        struct EE_ENGINE_API Definition final : public SpeedScaleBaseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( SpeedScaleBaseNode::Definition );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual void InitializeInternal ( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal ( GraphContext& context ) override;
        virtual float CalculateSpeedScaleMultiplier( GraphContext& context ) const override;

    private:

        FloatValueNode*             m_pDurationValueNode = nullptr;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API VelocityBasedSpeedScaleNode final : public SpeedScaleBaseNode
    {
    public:

        struct EE_ENGINE_API Definition final : public SpeedScaleBaseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( SpeedScaleBaseNode::Definition );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual float CalculateSpeedScaleMultiplier( GraphContext& context ) const override;

    private:

        FloatValueNode*             m_pDesiredVelocityValueNode = nullptr;
    };
}