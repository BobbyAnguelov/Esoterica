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
            EE_SERIALIZE( m_inputNodeIdx, m_weightValueNodeIdx, m_boneMaskValueNodeIdx, m_isSynchronized, m_ignoreEvents, m_isStateMachineLayer, m_blendMode );

            int16_t                                         m_inputNodeIdx = InvalidIndex;
            int16_t                                         m_weightValueNodeIdx = InvalidIndex;
            int16_t                                         m_boneMaskValueNodeIdx = InvalidIndex;
            bool                                            m_isSynchronized = false;
            bool                                            m_ignoreEvents = false;
            bool                                            m_isStateMachineLayer = false;
            PoseBlendMode                                   m_blendMode;
        };

        //-------------------------------------------------------------------------

        struct EE_ENGINE_API Settings : public PoseNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_baseNodeIdx, m_onlySampleBaseRootMotion, m_layerSettings );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                                         m_baseNodeIdx = InvalidIndex;
            bool                                            m_onlySampleBaseRootMotion = true;
            TInlineVector<LayerSettings, 3>                 m_layerSettings;
        };

        struct Layer
        {
            PoseNode*                                       m_pInputNode = nullptr;
            FloatValueNode*                                 m_pWeightValueNode = nullptr;
            BoneMaskValueNode*                              m_pBoneMaskValueNode = nullptr;
        };

    public:

        virtual SyncTrack const& GetSyncTrack() const override { EE_ASSERT( IsValid() ); return m_pBaseLayerNode->GetSyncTrack(); }
        virtual bool IsValid() const override { return PoseNode::IsValid() && m_pBaseLayerNode->IsValid(); }

        #if EE_DEVELOPMENT_TOOLS
        inline float GetLayerWeight( int32_t layerIdx ) const { return m_debugLayerWeights[layerIdx]; }
        #endif

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;

        GraphPoseNodeResult UpdateLocalLayer( GraphContext& context, LayerSettings const& layerSettings, Layer const& layer );
        GraphPoseNodeResult UpdateStateMachineLayer( GraphContext& context, LayerSettings const& layerSettings, Layer const& layer );
        void UpdateLayers( GraphContext& context, GraphPoseNodeResult& NodeResult );

    private:

        PoseNode*                                           m_pBaseLayerNode = nullptr;
        TInlineVector<Layer, 3>                             m_layers;
        GraphLayerContext                                   m_previousContext;

        #if EE_DEVELOPMENT_TOOLS
        int16_t                                             m_rootMotionActionIdxBase = InvalidIndex;
        TVector<float>                                      m_debugLayerWeights;
        #endif
    };
}