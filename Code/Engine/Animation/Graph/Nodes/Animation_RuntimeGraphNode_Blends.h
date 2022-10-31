#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/AnimationBlender.h"
#include "System/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API ParameterizedBlendNode : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_sourceNodeIndices, m_inputParameterValueNodeIdx, m_isSynchronized );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            TInlineVector<int16_t, 5>        m_sourceNodeIndices;
            int16_t                          m_inputParameterValueNodeIdx = InvalidIndex;
            bool                                    m_isSynchronized = false;
        };

    public:

        struct BlendRange
        {
            EE_SERIALIZE( m_inputIdx0, m_inputIdx1, m_parameterValueRange );

            inline bool operator<( BlendRange const& rhs ) const
            {
                return m_parameterValueRange.m_begin < rhs.m_parameterValueRange.m_begin;
            }

            int16_t                                   m_inputIdx0 = InvalidIndex;
            int16_t                                   m_inputIdx1 = InvalidIndex;
            FloatRange                              m_parameterValueRange = FloatRange( 0 );
        };

        struct EE_ENGINE_API Parameterization
        {
            EE_SERIALIZE( m_blendRanges, m_parameterRange );

            static Parameterization CreateParameterization( TInlineVector<float, 5> values );

            inline void Reset()
            {
                m_blendRanges.clear();
                m_parameterRange = FloatRange( 0 );
            }

            TInlineVector<BlendRange, 5>            m_blendRanges;
            FloatRange                              m_parameterRange;
        };

    public:

        virtual bool IsValid() const override;
        virtual SyncTrack const& GetSyncTrack() const override { return m_blendedSyncTrack; }

    protected:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context ) override final;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override final;

        // Parameterization
        //-------------------------------------------------------------------------

        // Optional function to create a parameterization if one isnt serialized
        virtual void InitializeParameterization( GraphContext& context ) {}
        virtual void ShutdownParameterization( GraphContext& context ) {}

        virtual Parameterization const& GetParameterization() const = 0;
        void SelectBlendRange( GraphContext& context );

    protected:

        TInlineVector<PoseNode*, 5>                 m_sourceNodes;
        FloatValueNode*                             m_pInputParameterValueNode = nullptr;
        int32_t                                       m_selectedRangeIdx = InvalidIndex;
        float                                       m_blendWeight = 0.0f;
        SyncTrack                                   m_blendedSyncTrack;
    };

    //-------------------------------------------------------------------------

    class RangedBlendNode final : public ParameterizedBlendNode
    {
    public:

        struct EE_ENGINE_API Settings final : public ParameterizedBlendNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( ParameterizedBlendNode::Settings, m_parameterization );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            Parameterization                        m_parameterization;
        };

    private:

        virtual Parameterization const& GetParameterization() const { return GetSettings<RangedBlendNode>()->m_parameterization; }
    };

    //-------------------------------------------------------------------------

    class VelocityBlendNode final : public ParameterizedBlendNode
    {
    public:

        struct EE_ENGINE_API Settings final : public ParameterizedBlendNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual void InitializeParameterization( GraphContext& context ) override;
        virtual void ShutdownParameterization( GraphContext& context ) override;
        virtual Parameterization const& GetParameterization() const override { return m_parameterization; }

    protected:

        Parameterization                            m_parameterization; // Lazily initialized parameterization
        bool                                        m_parameterizationInitialized = false;
    };
}