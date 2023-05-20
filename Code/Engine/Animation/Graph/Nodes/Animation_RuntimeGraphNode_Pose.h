#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/AnimationPose.h"
#include "System/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API ZeroPoseNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual SyncTrack const& GetSyncTrack() const override { return SyncTrack::s_defaultTrack; }
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override { return Update( context ); }
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ReferencePoseNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual SyncTrack const& GetSyncTrack() const override { return SyncTrack::s_defaultTrack; }
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override { return Update( context ); }
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API AnimationPoseNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_poseTimeValueNodeIdx, m_dataSlotIndex, m_inputTimeRemapRange );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                             m_poseTimeValueNodeIdx = InvalidIndex;
            int16_t                             m_dataSlotIndex = InvalidIndex;
            FloatRange                          m_inputTimeRemapRange = FloatRange( 0, 1 ); // Time range allows for remapping a time value that is not a normalized time to the animation
        };

    public:

        virtual bool IsValid() const override;

    private:

        virtual SyncTrack const& GetSyncTrack() const override { return SyncTrack::s_defaultTrack; }
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override { return Update( context ); }

    private:

        FloatValueNode*                         m_pPoseTimeValue = nullptr;
        AnimationClip const*                    m_pAnimation = nullptr;
    };
}