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
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_clipReferenceNodeIdx, m_targetValueNodeIdx, m_isOffsetNode, m_isOffsetRelativeToCharacter );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_clipReferenceNodeIdx = InvalidIndex;
            int16_t                     m_targetValueNodeIdx = InvalidIndex;
            bool                        m_isOffsetNode = false;
            bool                        m_isOffsetRelativeToCharacter = true;
        };

    private:

        virtual SyncTrack const& GetSyncTrack() const override { return m_pClipReferenceNode->GetSyncTrack(); }
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;

        void PerformWarp( GraphContext& context );

        #if EE_DEVELOPMENT_TOOLS
        virtual void DrawDebug( GraphContext& graphContext, Drawing::DrawContext& drawCtx ) override;
        #endif

    private:

        AnimationClipReferenceNode*     m_pClipReferenceNode = nullptr;
        ValueNode*                      m_pTargetValueNode = nullptr;
        RootMotionData                  m_warpedRootMotion;

        #if EE_DEVELOPMENT_TOOLS
        Transform                       m_warpStartWorldTransform;
        Vector                          m_debugCharacterOffsetPosWS;
        Vector                          m_debugTargetDirWS;
        #endif
    };
}