#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/AnimationPose.h"
#include "Base/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API ZeroPoseNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Definition final : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual SyncTrack const& GetSyncTrack() const override { return SyncTrack::s_defaultTrack; }
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ReferencePoseNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Definition final : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual SyncTrack const& GetSyncTrack() const override { return SyncTrack::s_defaultTrack; }
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API AnimationPoseNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Definition final : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_poseTimeValueNodeIdx, m_dataSlotIndex, m_inputTimeRemapRange, m_userSpecifiedTime, m_useFramesAsInput );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                             m_poseTimeValueNodeIdx = InvalidIndex;
            int16_t                             m_dataSlotIndex = InvalidIndex;
            FloatRange                          m_inputTimeRemapRange = FloatRange( 0, 1 ); // Time range allows for remapping a time value that is not a normalized time to the animation
            float                               m_userSpecifiedTime = 0.0f;
            bool                                m_useFramesAsInput = false;
        };

    public:

        virtual bool IsValid() const override;

    private:

        virtual SyncTrack const& GetSyncTrack() const override { return SyncTrack::s_defaultTrack; }
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

    private:

        FloatValueNode*                         m_pPoseTimeValue = nullptr;
        AnimationClip const*                    m_pAnimation = nullptr;
    };
}