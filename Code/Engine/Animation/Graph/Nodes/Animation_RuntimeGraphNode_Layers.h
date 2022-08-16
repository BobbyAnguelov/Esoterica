#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/AnimationBlender.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class StateMachineNode;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API LayerBlendNode final : public PoseNode
    {

    public:

        struct LayerSettings
        {
            EE_SERIALIZE( m_layerNodeIdx, m_isSynchronized, m_ignoreEvents, m_blendOptions );

            int16_t                                         m_layerNodeIdx = InvalidIndex;
            bool                                            m_isSynchronized = false;
            bool                                            m_ignoreEvents = false;
            TBitFlags<PoseBlendOptions>                     m_blendOptions;
        };

        //-------------------------------------------------------------------------

        struct EE_ENGINE_API Settings : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_baseNodeIdx, m_onlySampleBaseRootMotion, m_layerSettings );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                                         m_baseNodeIdx = InvalidIndex;
            bool                                            m_onlySampleBaseRootMotion = true;
            TInlineVector<LayerSettings, 3>                 m_layerSettings;
        };

    public:

        virtual SyncTrack const& GetSyncTrack() const override { EE_ASSERT( IsValid() ); return m_pBaseLayerNode->GetSyncTrack(); }
        virtual bool IsValid() const override { return PoseNode::IsValid() && m_pBaseLayerNode->IsValid(); }

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void DeactivateBranch( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;
        void UpdateLayers( GraphContext& context, GraphPoseNodeResult& NodeResult );

    private:

        PoseNode*                                           m_pBaseLayerNode = nullptr;
        TInlineVector<StateMachineNode*, 3>                 m_layers;
        GraphLayerContext                                   m_previousContext;

        #if EE_DEVELOPMENT_TOOLS
        int16_t                                             m_rootMotionActionIdxBase = InvalidIndex;
        #endif
    };
}