#pragma once

#include "Animation_RuntimeGraphNode_AnimationClip.h"
#include "Engine/Animation/Events/AnimationEvent_Warp.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AnimationClipReferenceNode;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API OrientationWarpNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Definition final : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_clipReferenceNodeIdx, m_targetValueNodeIdx, m_isOffsetNode, m_isOffsetRelativeToCharacter, m_samplingMode );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                         m_clipReferenceNodeIdx = InvalidIndex;
            int16_t                         m_targetValueNodeIdx = InvalidIndex;
            bool                            m_isOffsetNode = false;
            bool                            m_isOffsetRelativeToCharacter = true;
            RootMotionData::SamplingMode    m_samplingMode = RootMotionData::SamplingMode::WorldSpace;
        };

    private:

        virtual bool IsValid() const override;
        virtual SyncTrack const& GetSyncTrack() const override { return m_pClipReferenceNode->GetSyncTrack(); }
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        void PerformWarp( GraphContext& context );

        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual bool RestoreGraphState( RecordedGraphState const& inState ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void DrawDebug( GraphContext& graphContext, DebugDrawContext& drawCtx ) override;
        #endif

    private:

        AnimationClipReferenceNode*         m_pClipReferenceNode = nullptr;
        ValueNode*                          m_pTargetValueNode = nullptr;
        RootMotionData                      m_warpedRootMotion;
        bool                                m_shouldUpdateWarp = false;

        //-------------------------------------------------------------------------

        Transform                           m_warpStartWorldTransform = Transform::Identity;
        Seconds                             m_warpStartTime = 0.0f;

        #if EE_DEVELOPMENT_TOOLS
        Vector                              m_debugCharacterOffsetPosWS = Vector::Zero;
        Vector                              m_debugTargetDirWS = Vector::Zero;
        #endif
    };
}