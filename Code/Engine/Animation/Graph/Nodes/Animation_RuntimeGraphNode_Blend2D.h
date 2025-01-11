#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/AnimationBlender.h"
#include "Base/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API Blend2DNode : public PoseNode
    {
    public:

        struct EE_ENGINE_API Definition : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_sourceNodeIndices, m_inputParameterNodeIdx0, m_inputParameterNodeIdx1, m_values, m_indices, m_hullIndices, m_allowLooping );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            TInlineVector<int16_t, 5>               m_sourceNodeIndices;
            int16_t                                 m_inputParameterNodeIdx0 = InvalidIndex;
            int16_t                                 m_inputParameterNodeIdx1 = InvalidIndex;
            TInlineVector<Float2, 10>               m_values;
            TInlineVector<uint8_t, 30>              m_indices;
            TInlineVector<uint8_t, 10>              m_hullIndices;
            bool                                    m_allowLooping = true;
        };

    private:

        struct BlendSpaceResult
        {
            void Reset() { *this = BlendSpaceResult(); }

            int32_t                                 m_sourceIndices[3] = { InvalidIndex, InvalidIndex, InvalidIndex };
            float                                   m_blendWeightBetween0And1 = 0.0f;
            float                                   m_blendWeightBetween1And2 = 0.0f;
            Float2                                  m_finalParameter;
        };

        static void CalculateBlendSpaceWeights( TInlineVector<Float2, 10> const &points, TInlineVector<uint8_t, 30> const &indices, TInlineVector<uint8_t, 10> const &hullIndices, Float2 const &point, BlendSpaceResult &result );

    public:

        virtual bool IsValid() const override;
        virtual SyncTrack const& GetSyncTrack() const override { return m_blendedSyncTrack; }

        #if EE_DEVELOPMENT_TOOLS
        inline Float2 GetParameter() const { return m_bsr.m_finalParameter; }
        #endif

    protected:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override final;

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
        uint32_t                                    m_blendSpaceUpdateID = 0;
    };
}