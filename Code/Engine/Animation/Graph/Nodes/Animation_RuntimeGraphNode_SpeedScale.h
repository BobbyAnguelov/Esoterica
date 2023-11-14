#pragma once

#include "Animation_RuntimeGraphNode_Passthrough.h"
#include "Animation_RuntimeGraphNode_AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API SpeedScaleNode final : public PassthroughNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PassthroughNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PassthroughNode::Settings, m_scaleValueNodeIdx, m_blendInTime );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            float                   m_blendInTime = 0.2f; // Amount of time to blend in the speed scale modifier
            int16_t                 m_scaleValueNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        FloatValueNode*             m_pScaleValueNode = nullptr;
        float                       m_blendWeight = 1.0f; // Used to ensure the modifier is slowly blended in when coming from a sync'd transition that ends
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API DurationScaleNode final : public PassthroughNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PassthroughNode::Settings
        {
            EE_REFLECT_TYPE ( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS ( PassthroughNode::Settings, m_durationValueNodeIdx, m_desiredDuration );

            virtual void InstantiateNode ( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                 m_durationValueNodeIdx = InvalidIndex;
            float                   m_desiredDuration = 0.0f;
        };

    private:

        virtual void InitializeInternal ( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal ( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update ( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

    private:

        FloatValueNode*             m_pDurationValueNode = nullptr;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API VelocityBasedSpeedScaleNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_childNodeIdx, m_desiredVelocityValueNodeIdx, m_blendInTime );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                 m_childNodeIdx = InvalidIndex;
            int16_t                 m_desiredVelocityValueNodeIdx = InvalidIndex;
            float                   m_blendInTime = 0.2f; // Amount of time to blend in the speed scale modifier
        };

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual bool IsValid() const override { return PoseNode::IsValid() && m_pChildNode->IsValid(); }
        virtual SyncTrack const& GetSyncTrack() const override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        AnimationClipReferenceNode* m_pChildNode = nullptr;
        FloatValueNode*             m_pDesiredVelocityValueNode = nullptr;
        float                       m_blendWeight = 1.0f; // Used to ensure the modifier is slowly blended in when coming from a sync'd transition that ends
    };
}