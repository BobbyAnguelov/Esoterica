#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API IDComparisonNode final : public BoolValueNode
    {
    public:

        enum class Comparison : uint8_t
        {
            EE_REGISTER_ENUM
            Matches = 0,
            DoesntMatch,
        };

        struct EE_ENGINE_API Settings final : public BoolValueNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_inputValueNodeIdx, m_comparison, m_comparisionIDs );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                               m_inputValueNodeIdx = InvalidIndex;
            Comparison                              m_comparison = Comparison::Matches;
            TInlineVector<StringID, 4>              m_comparisionIDs;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        IDValueNode*                                m_pInputValueNode = nullptr;
        bool                                        m_result = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API IDToFloatNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_inputValueNodeIdx, m_defaultValue, m_IDs, m_values );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                   m_inputValueNodeIdx = InvalidIndex;
            float                       m_defaultValue = 0.0f;
            TInlineVector<StringID, 5>  m_IDs;
            TInlineVector<float, 5>     m_values;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        IDValueNode*                    m_pInputValueNode = nullptr;
        float                           m_value = 0.0f;
    };

}