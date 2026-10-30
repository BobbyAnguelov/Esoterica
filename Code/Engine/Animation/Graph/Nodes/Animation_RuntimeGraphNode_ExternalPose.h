#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/TaskSystem/Animation_TaskPosePool.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API ExternalPoseNode final : public PoseNode
    {
        friend class GraphInstance;
        friend class GraphRecordingPlayer;

    public:

        struct EE_ENGINE_API Definition final : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition );

            virtual void InstantiateNode( InstantiationContext const &context, InstantiationOptions options ) const override;
            virtual bool RequiresPostInstantiationStage() const override { return true; }
            virtual void PostInstantiateNode( InstantiationContext const &context ) const override;
        };

    private:

        virtual SyncTrack const &GetSyncTrack() const override { return SyncTrack::s_defaultTrack; }
        virtual void InitializeInternal( GraphContext &context, SyncTrackTime const &initialTime ) override;
        virtual void ShutdownInternal( GraphContext &context ) override;
        virtual GraphPoseNodeResult Update( GraphContext &context, SyncTrackTimeRange const *pUpdateRange ) override;

    private:

        Transform                       m_rootMotionDelta = Transform::Identity;
        TVector<SampledEvent>           m_externalEvents;
        BoneMaskTaskList                m_boneMask;
        CachedPoseID                    m_externalPoseBufferID;
    };
}