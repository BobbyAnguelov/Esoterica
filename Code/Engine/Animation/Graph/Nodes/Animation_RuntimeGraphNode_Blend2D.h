#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/AnimationBlender.h"
#include "Base/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API Blend2DNode : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings : public PoseNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_sourceNodeIndices, m_inputParameterNodeIdx0, m_inputParameterNodeIdx1, m_values, m_indices, m_hullIndices );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            TInlineVector<int16_t, 5>               m_sourceNodeIndices;
            int16_t                                 m_inputParameterNodeIdx0 = InvalidIndex;
            int16_t                                 m_inputParameterNodeIdx1 = InvalidIndex;
            TInlineVector<Float2, 10>               m_values;
            TInlineVector<uint8_t, 30>              m_indices;
            TInlineVector<uint8_t, 10>              m_hullIndices;
        };

    private:

        struct BlendSpaceResult
        {
            void Reset() { *this = BlendSpaceResult(); }

            Float2                                  m_parameter;
            PoseNode*                               m_pSource0 = nullptr;
            PoseNode*                               m_pSource1 = nullptr;
            PoseNode*                               m_pSource2 = nullptr;
            float                                   m_blendWeightBetween0And1 = 0.0f;
            float                                   m_blendWeightBetween1And2 = 0.0f;
            uint32_t                                m_updateID = 0;
        };

    public:

        virtual bool IsValid() const override;
        virtual SyncTrack const& GetSyncTrack() const override { return m_blendedSyncTrack; }

        #if EE_DEVELOPMENT_TOOLS
        inline Float2 GetParameter() const { return m_bsr.m_parameter; }
        #endif

    protected:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context ) override final;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override final;

        void EvaluateBlendSpace( GraphContext& context );

        // Debugging
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    protected:

        TInlineVector<PoseNode*, 5>                 m_sourceNodes;
        FloatValueNode*                             m_pInputParameterNode0 = nullptr;
        FloatValueNode*                             m_pInputParameterNode1 = nullptr;
        SyncTrack                                   m_blendedSyncTrack;
        BlendSpaceResult                            m_bsr;
    };
}