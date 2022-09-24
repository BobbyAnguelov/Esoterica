#pragma once

#include "Animation_RuntimeGraphNode_AnimationClip.h"
#include "Engine/Animation/Events/AnimationEvent_Warp.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class AnimationClipReferenceNode;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API OrientationWarpNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_clipReferenceNodeIdx, m_angleOffsetValueNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_clipReferenceNodeIdx = InvalidIndex;
            int16_t                     m_angleOffsetValueNodeIdx = InvalidIndex;
        };

    private:

        virtual SyncTrack const& GetSyncTrack() const override { return m_pClipReferenceNode->GetSyncTrack(); }
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;

        void PerformWarp( GraphContext& context );
        Transform SampleWarpedRootMotion( GraphContext& context ) const;

    private:

        AnimationClipReferenceNode*     m_pClipReferenceNode = nullptr;
        FloatValueNode*                 m_pAngleOffsetValueNode = nullptr;
    };
}