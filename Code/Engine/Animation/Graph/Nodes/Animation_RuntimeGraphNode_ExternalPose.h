#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/TaskSystem/Animation_TaskPosePool.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AnimationClip;

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API ExternalPoseData
    {
        void Reset();

        Transform                                       m_rootMotion;
        bool                                            m_hasRootMotion = false;
        bool                                            m_isRootMotionDelta = true;

        bool                                            m_wasHasPose = false;
    };

    //-------------------------------------------------------------------------

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
        };

    public:

        ~ExternalPoseNode() { EE_ASSERT( !m_isPoseSet ); }

        virtual bool IsValid() const override;

        inline bool IsPoseSet() const { return m_isPoseSet; }

    private:

        virtual SyncTrack const &GetSyncTrack() const override { return SyncTrack::s_defaultTrack; }
        virtual void InitializeInternal( GraphContext &context, SyncTrackTime const &initialTime ) override;
        virtual void ShutdownInternal( GraphContext &context ) override;
        virtual GraphPoseNodeResult Update( GraphContext &context, SyncTrackTimeRange const *pUpdateRange ) override;

    private:

        ExternalPoseData                                m_poseData;
        CachedPoseID                                    m_cachedPoseID;
        bool                                            m_isPoseSet = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API IsExternalPoseSetNode : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Definition : public BoolValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( BoolValueNode::Definition, m_externalPoseNodeIdx );

            virtual void InstantiateNode( InstantiationContext const &context, InstantiationOptions options ) const override;

            int16_t                                     m_externalPoseNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext &context ) override;
        virtual void ShutdownInternal( GraphContext &context ) override;
        virtual void GetValueInternal( GraphContext &context, void *pOutValue ) override;

    private:

        ExternalPoseNode*                               m_pExternalPoseNode = nullptr;
        bool                                            m_result = false;
    };
}