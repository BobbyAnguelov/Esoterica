#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API PassthroughNode : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_childNodeIdx );

            virtual void InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const override;

            int16_t          m_childNodeIdx = InvalidIndex;
        };

    public:

        virtual bool IsValid() const override { return PoseNode::IsValid() && m_pChildNode->IsValid(); }
        virtual SyncTrack const& GetSyncTrack() const override;

    protected:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;
        virtual void DeactivateBranch( GraphContext& context ) override;

    protected:

        PoseNode*                   m_pChildNode = nullptr;
    };
}