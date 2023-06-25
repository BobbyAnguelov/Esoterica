#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Math/Easing.h"
#include "System/Math/FloatCurve.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API FloatRemapNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API RemapRange : public IReflectedType
        {
            EE_REFLECT_TYPE( RemapRange );
            EE_SERIALIZE( m_begin, m_end );

            EE_REFLECT() float             m_begin = 0;
            EE_REFLECT() float             m_end = 0;
        };

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_inputValueNodeIdx, m_inputRange, m_outputRange );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputValueNodeIdx = InvalidIndex;
            RemapRange                  m_inputRange;
            RemapRange                  m_outputRange;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        FloatValueNode*                 m_pInputValueNode = nullptr;
        float                           m_value = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FloatClampNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_inputValueNodeIdx, m_clampRange );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputValueNodeIdx = InvalidIndex;
            FloatRange                  m_clampRange = FloatRange( 0 );
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        FloatValueNode*                 m_pInputValueNode = nullptr;
        float                           m_value = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FloatAbsNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_inputValueNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputValueNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        FloatValueNode*                 m_pInputValueNode = nullptr;
        float                           m_value = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FloatEaseNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_easeTime, m_startValue, m_inputValueNodeIdx, m_easingType, m_useStartValue );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            float                       m_easeTime = 1.0f;
            float                       m_startValue = 0.0f;
            int16_t                     m_inputValueNodeIdx = InvalidIndex;
            Math::Easing::Type          m_easingType = Math::Easing::Type::Linear;
            bool                        m_useStartValue = false;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        FloatValueNode*                 m_pInputValueNode = nullptr;
        FloatRange                      m_easeRange = FloatRange( 0.0f );
        float                           m_currentValue = 0.0f;
        float                           m_currentEaseTime = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FloatCurveNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_inputValueNodeIdx, m_curve );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputValueNodeIdx = InvalidIndex;
            FloatCurve                  m_curve;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        FloatValueNode*                 m_pInputValueNode = nullptr;
        float                           m_currentValue = 0.0f;
        FloatCurve                      m_curve;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FloatMathNode final : public FloatValueNode
    {
    public:

        enum class Operator : uint8_t
        {
            EE_REFLECT_ENUM

            Add,
            Sub,
            Mul,
            Div,
        };

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_inputValueNodeIdxA, m_inputValueNodeIdxB, m_returnAbsoluteResult, m_operator, m_valueB );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputValueNodeIdxA = InvalidIndex;
            int16_t                     m_inputValueNodeIdxB = InvalidIndex; // Optional
            bool                        m_returnAbsoluteResult = false;
            Operator                    m_operator = Operator::Add;
            float                       m_valueB = 0.0f;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        FloatValueNode*                 m_pValueNodeA = nullptr;
        FloatValueNode*                 m_pValueNodeB = nullptr;
        float                           m_value = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FloatComparisonNode final : public BoolValueNode
    {
    public:

        enum class Comparison : uint8_t
        {
            EE_REFLECT_ENUM

            GreaterThanEqual = 0,
            LessThanEqual,
            NearEqual,
            GreaterThan,
            LessThan,
        };

        struct EE_ENGINE_API Settings final : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_inputValueNodeIdx, m_comparandValueNodeIdx, m_comparison, m_epsilon, m_comparisonValue );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputValueNodeIdx = InvalidIndex;
            int16_t                     m_comparandValueNodeIdx = InvalidIndex;
            Comparison                  m_comparison = Comparison::GreaterThanEqual;
            float                       m_epsilon = 0.0f;
            float                       m_comparisonValue = 0.0f;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        FloatValueNode*                 m_pInputValueNode = nullptr;
        FloatValueNode*                 m_pComparandValueNode = nullptr;
        bool                            m_result = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FloatRangeComparisonNode final : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_inputValueNodeIdx, m_range, m_isInclusiveCheck );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            FloatRange                  m_range;
            int16_t                     m_inputValueNodeIdx = InvalidIndex;
            bool                        m_isInclusiveCheck = true;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        FloatValueNode*                 m_pInputValueNode = nullptr;
        bool                            m_result = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FloatSwitchNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_switchValueNodeIdx, m_trueValueNodeIdx, m_falseValueNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                    m_switchValueNodeIdx = InvalidIndex;
            int16_t                    m_trueValueNodeIdx = InvalidIndex;
            int16_t                    m_falseValueNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        BoolValueNode*                  m_pSwitchValueNode = nullptr;
        FloatValueNode*                 m_pTrueValueNode = nullptr;
        FloatValueNode*                 m_pFalseValueNode = nullptr;
        float                           m_value = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FloatAngleMathNode final : public FloatValueNode
    {
    public:

        enum class Operation : uint8_t
        {
            EE_REFLECT_ENUM

            ClampTo180, // Clamps the input angle between [-180, 180]
            ClampTo360, // Clamps the input angle between [0, 360]
            FlipHemisphere, // Flips the hemisphere and returns the angle relative to 180 (e.g. 135deg will result in -45deg )
            FlipHemisphereNegate, // Flips the hemisphere and returns the negated angle relative to 180 ( e.g. 135deg will result in 45deg )
        };

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_inputValueNodeIdx, m_operation );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputValueNodeIdx = InvalidIndex;
            Operation                   m_operation;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        FloatValueNode*                 m_pInputValueNode = nullptr;
        float                           m_value = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FloatSelectorNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_conditionNodeIndices, m_values, m_defaultValue );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            TInlineVector<int16_t, 5>   m_conditionNodeIndices;
            TInlineVector<float, 5>     m_values;
            float                       m_defaultValue = 0.0f;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        TInlineVector<BoolValueNode*,5> m_conditionNodes;
        float                           m_result;
    };
}