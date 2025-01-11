#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API PassthroughNode : public PoseNode
    {
    public:

        struct EE_ENGINE_API Definition : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_childNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t          m_childNodeIdx = InvalidIndex;
        };

    public:

        virtual bool IsValid() const override { return PoseNode::IsValid() && m_pChildNode->IsValid(); }
        virtual SyncTrack const& GetSyncTrack() const override;

    protected:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange = nullptr ) override;

    protected:

        PoseNode*                   m_pChildNode = nullptr;
    };
}