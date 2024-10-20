#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/AnimationBlender.h"
#include "Base/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API ParameterizedBlendNode : public PoseNode
    {
    public:

        struct EE_ENGINE_API Definition : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_sourceNodeIndices, m_inputParameterValueNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            TInlineVector<int16_t, 5>               m_sourceNodeIndices;
            int16_t                                 m_inputParameterValueNodeIdx = InvalidIndex;
            bool                                    m_allowLooping = true;
        };

    public:

        struct BlendRange
        {
            EE_SERIALIZE( m_inputIdx0, m_inputIdx1, m_parameterValueRange );

            inline bool operator<( BlendRange const& rhs ) const
            {
                return m_parameterValueRange.m_begin < rhs.m_parameterValueRange.m_begin;
            }

            int16_t                                 m_inputIdx0 = InvalidIndex;
            int16_t                                 m_inputIdx1 = InvalidIndex;
            FloatRange                              m_parameterValueRange = FloatRange( 0 );
        };

        struct EE_ENGINE_API Parameterization
        {
            EE_SERIALIZE( m_blendRanges, m_parameterRange );

            static Parameterization CreateParameterization( TInlineVector<float, 5> const& values );

            inline void Reset()
            {
                m_blendRanges.clear();
                m_parameterRange = FloatRange( 0 );
            }

            TInlineVector<BlendRange, 5>            m_blendRanges;
            FloatRange                              m_parameterRange;
        };

    private:

        struct BlendSpaceResult
        {
            void Reset() { *this = BlendSpaceResult(); }

            PoseNode*                               m_pSource0 = nullptr;
            PoseNode*                               m_pSource1 = nullptr;
            float                                   m_blendWeight = 0.0f;
            uint32_t                                m_updateID = 0;
        };

    public:

        virtual bool IsValid() const override;
        virtual SyncTrack const& GetSyncTrack() const override { return m_blendedSyncTrack; }

        #if EE_DEVELOPMENT_TOOLS
        inline void GetDebugInfo( int16_t& outSourceIdx0, int16_t& outSourceIdx1, float& outBlendweight ) const;
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
        FloatValueNode*                             m_pInputParameterValueNode = nullptr;
        BlendSpaceResult                            m_bsr;
        SyncTrack                                   m_blendedSyncTrack;
        Parameterization                            m_parameterization;
    };

    //-------------------------------------------------------------------------

    class Blend1DNode final : public ParameterizedBlendNode
    {
    public:

        struct EE_ENGINE_API Definition final : public ParameterizedBlendNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( ParameterizedBlendNode::Definition, m_parameterization, m_allowLooping );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            Parameterization                        m_parameterization;
        };
    };

    //-------------------------------------------------------------------------

    class VelocityBlendNode final : public ParameterizedBlendNode
    {
    public:

        struct EE_ENGINE_API Definition final : public ParameterizedBlendNode::Definition
        {
            EE_REFLECT_TYPE( Definition );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;

        void CreateParameterizationFromSpeeds();

        #if EE_DEVELOPMENT_TOOLS
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        bool m_lazyInitializationPerformed = false;
    };
}