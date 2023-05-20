#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class StateNode;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API StateCompletedConditionNode : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Settings : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_sourceStateNodeIdx, m_transitionDurationOverrideNodeIdx, m_transitionDuration );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                             m_sourceStateNodeIdx = InvalidIndex;
            int16_t                             m_transitionDurationOverrideNodeIdx = InvalidIndex;
            float                               m_transitionDuration = 0.0f;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        StateNode*                              m_pSourceStateNode = nullptr;
        FloatValueNode*                         m_pDurationOverrideNode = nullptr;
        bool                                    m_result = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API TimeConditionNode : public BoolValueNode
    {
    public:

        enum class ComparisonType : uint8_t
        {
            EE_REFLECT_ENUM

            PercentageThroughState,
            PercentageThroughSyncEvent,
            ElapsedTime,
        };

        enum class Operator : uint8_t
        {
            EE_REFLECT_ENUM

            LessThan = 0,
            LessThanEqual,
            GreaterThan,
            GreaterThanEqual,
        };

        struct EE_ENGINE_API Settings : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_sourceStateNodeIdx, m_comparand, m_inputValueNodeIdx, m_type, m_operator );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                             m_sourceStateNodeIdx = InvalidIndex;
            int16_t                             m_inputValueNodeIdx = InvalidIndex;
            float                               m_comparand = 0.0f;
            ComparisonType                      m_type = ComparisonType::ElapsedTime;
            Operator                            m_operator = Operator::LessThan;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        StateNode*                              m_pSourceStateNode = nullptr;
        FloatValueNode*                         m_pInputValueNode = nullptr;
        bool                                    m_result = false;
    };
}