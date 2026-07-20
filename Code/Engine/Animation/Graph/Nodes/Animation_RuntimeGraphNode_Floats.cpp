#include "Animation_RuntimeGraphNode_Floats.h"

#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void FloatSwitchNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatSwitchNode>( context, options );
        context.SetNodePtrFromIndex( m_switchValueNodeIdx, pNode->m_pSwitchValueNode );
        context.SetOptionalNodePtrFromIndex( m_trueValueNodeIdx, pNode->m_pTrueValueNode );
        context.SetOptionalNodePtrFromIndex( m_falseValueNodeIdx, pNode->m_pFalseValueNode );
    }

    void FloatSwitchNode::InitializeInternal( GraphContext& context )
    {
        FloatValueNode::InitializeInternal( context );
        m_pSwitchValueNode->Initialize( context );

        if ( m_pTrueValueNode != nullptr )
        {
            m_pTrueValueNode->Initialize( context );
        }

        if ( m_pFalseValueNode != nullptr )
        {
            m_pFalseValueNode->Initialize( context );
        }
    }

    void FloatSwitchNode::ShutdownInternal( GraphContext& context )
    {
        m_pSwitchValueNode->Shutdown( context );

        if ( m_pTrueValueNode != nullptr )
        {
            m_pTrueValueNode->Shutdown( context );
        }

        if ( m_pFalseValueNode != nullptr )
        {
            m_pFalseValueNode->Shutdown( context );
        }

        FloatValueNode::ShutdownInternal( context );
    }

    void FloatSwitchNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            auto pDefinition = GetDefinition<FloatSwitchNode>();

            if ( m_pSwitchValueNode->GetValue<bool>( context ) )
            {
                m_value = m_pTrueValueNode ? m_pTrueValueNode->GetValue<float>( context ) : pDefinition->m_trueValue;
            }
            else
            {
                m_value = m_pFalseValueNode ? m_pFalseValueNode->GetValue<float>( context ) : pDefinition->m_falseValue;
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void FloatRemapNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatRemapNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void FloatRemapNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        FloatValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );
    }

    void FloatRemapNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void FloatRemapNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        auto pDefinition = GetDefinition<FloatRemapNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            float const inputValue = m_pInputValueNode->GetValue<float>( context );
            m_value = Math::RemapRange( inputValue, pDefinition->m_inputRange.m_begin, pDefinition->m_inputRange.m_end, pDefinition->m_outputRange.m_begin, pDefinition->m_outputRange.m_end );
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void FloatClampNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatClampNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void FloatClampNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        FloatValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );
    }

    void FloatClampNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void FloatClampNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        auto pDefinition = GetDefinition<FloatClampNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            auto const inputValue = m_pInputValueNode->GetValue<float>( context );
            m_value = pDefinition->m_clampRange.GetClampedValue( inputValue );
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void FloatEaseNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatEaseNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void FloatEaseNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );

        auto pDefinition = GetDefinition<FloatEaseNode>();

        FloatValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );

        if ( pDefinition->m_useStartValue )
        {
            m_easeRange = FloatRange( pDefinition->m_startValue );
        }
        else
        {
            m_easeRange = FloatRange( m_pInputValueNode->GetValue<float>( context ) );
        }

        m_currentValue = m_easeRange.m_begin;
        m_currentEaseTime = 0;
    }

    void FloatEaseNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void FloatEaseNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        auto pDefinition = GetDefinition<FloatEaseNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            float const inputTargetValue = m_pInputValueNode->GetValue<float>( context );
            if ( Math::IsNearEqual( m_currentValue, inputTargetValue, 0.01f ) )
            {
                m_easeRange = FloatRange( inputTargetValue );
                m_currentValue = inputTargetValue;
                m_currentEaseTime = 0;
            }
            else
            {
                // If the target has changed
                if ( inputTargetValue != m_easeRange.m_end )
                {
                    m_easeRange.m_end = inputTargetValue;
                    m_easeRange.m_begin = m_currentValue;
                    m_currentEaseTime = 0;
                }

                // Increment the time through the blend
                m_currentEaseTime += context.m_deltaTime;

                // Calculate the new value, based on the percentage through the blend calculated by the easing function
                float const T = Math::Clamp( m_currentEaseTime / pDefinition->m_easeTime, 0.0f, 1.0f );
                float const blendValue = Math::Easing::Evaluate( pDefinition->m_easingOp, T ) * m_easeRange.GetLength();
                m_currentValue = m_easeRange.m_begin + blendValue;
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_currentValue;
    }

    void FloatEaseNode::RecordGraphState( RecordedGraphState& outState )
    {
        FloatValueNode::RecordGraphState( outState );
        outState.WriteValue( m_easeRange );
        outState.WriteValue( m_currentValue );
        outState.WriteValue( m_currentEaseTime );
    }

    bool FloatEaseNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        if( !FloatValueNode::RestoreGraphState( inState ) )
        {
            return false;
        }

        inState.ReadValue( m_easeRange );
        inState.ReadValue( m_currentValue );
        inState.ReadValue( m_currentEaseTime );

        return true;
    }

    //-------------------------------------------------------------------------

    void FloatSpringNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatSpringNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void FloatSpringNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );

        auto pDefinition = GetDefinition<FloatSpringNode>();

        FloatValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );

        if ( pDefinition->m_useStartValue )
        {
            m_position = pDefinition->m_startValue;
        }
        else
        {
            m_position = m_pInputValueNode->GetValue<float>( context );
        }

        m_velocity = 0.0f;
    }

    void FloatSpringNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void FloatSpringNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        auto pDefinition = GetDefinition<FloatSpringNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            float const target = m_pInputValueNode->GetValue<float>( context );
            float const timeStepSeconds = context.m_deltaTime.ToFloat();
            float const omega = Math::TwoPi * pDefinition->m_hertz;
            float const omegaDt = omega * timeStepSeconds;
            float const scale = 1.0f / ( 1.0f + omegaDt * ( 2.0f * pDefinition->m_dampingRatio + omegaDt ) );

            // Implicit Euler integration of the harmonic oscillator
            m_velocity = scale * ( m_velocity - omegaDt * omega * ( m_position - target ) );
            m_position += timeStepSeconds * m_velocity;
        }

        *reinterpret_cast<float*>( pOutValue ) = m_position;
    }

    void FloatSpringNode::RecordGraphState( RecordedGraphState& outState )
    {
        FloatValueNode::RecordGraphState( outState );
        outState.WriteValue( m_position );
        outState.WriteValue( m_velocity );
    }

    bool FloatSpringNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        if ( !FloatValueNode::RestoreGraphState( inState ) )
        {
            return false;
        }

        inState.ReadValue( m_position );
        inState.ReadValue( m_velocity );

        return true;
    }

    //-------------------------------------------------------------------------

    void FloatCurveNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatCurveNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void FloatCurveNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        FloatValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );
    }

    void FloatCurveNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void FloatCurveNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        auto pDefinition = GetDefinition<FloatCurveNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            float const inputTargetValue = m_pInputValueNode->GetValue<float>( context );
            m_currentValue = pDefinition->m_curve.Evaluate( inputTargetValue );
        }

        *reinterpret_cast<float*>( pOutValue ) = m_currentValue;
    }

    //-------------------------------------------------------------------------

    void FloatMathNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatMathNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdxA, pNode->m_pValueNodeA );
        context.SetOptionalNodePtrFromIndex( m_inputValueNodeIdxB, pNode->m_pValueNodeB );
    }

    void FloatMathNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pValueNodeA != nullptr );
        FloatValueNode::InitializeInternal( context );
        m_pValueNodeA->Initialize( context );

        if ( m_pValueNodeB != nullptr )
        {
            m_pValueNodeB->Initialize( context );
        }
    }

    void FloatMathNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pValueNodeA != nullptr );
        if ( m_pValueNodeB != nullptr )
        {
            m_pValueNodeB->Shutdown( context );
        }

        m_pValueNodeA->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void FloatMathNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pValueNodeA != nullptr );
        auto pDefinition = GetDefinition<FloatMathNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            // Get A
            float const valueA = m_pValueNodeA->GetValue<float>( context );

            // Get B
            float valueB;
            if ( m_pValueNodeB != nullptr )
            {
                valueB = m_pValueNodeB->GetValue<float>( context );
            }
            else
            {
                valueB = pDefinition->m_valueB;
            }

            // Calculate Result
            switch ( pDefinition->m_operator )
            {
                case Operator::Add:
                {
                    m_value = valueA + valueB;
                }
                break;

                case Operator::Sub:
                {
                    m_value = valueA - valueB;
                }
                break;

                case Operator::Mul:
                {
                    m_value = valueA * valueB;
                }
                break;

                case Operator::Div:
                {
                    if ( Math::IsNearZero( valueB ) )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        context.LogWarning( GetNodeIndex(), "Dividing by zero in FloatMathNode" );
                        #endif
                        m_value = 0;
                    }
                    else
                    {
                        m_value = valueA / valueB;
                    }
                }
                break;

                case Operator::Mod:
                {
                    if ( Math::IsNearZero( valueB ) )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        context.LogWarning( GetNodeIndex(), "Modulus by zero in FloatMathNode" );
                        #endif
                        m_value = 0;
                    }
                    else
                    {
                        m_value = Math::FModF( valueA, valueB );
                    }
                }
                break;

                case FloatMathNode::Operator::Abs:
                {
                    m_value = Math::Abs( valueA );
                }
                break;

                case FloatMathNode::Operator::Negate:
                {
                    m_value = -valueA;
                }
                break;

                case FloatMathNode::Operator::Floor:
                {
                    m_value = Math::Floor( valueA );
                }
                break;

                case FloatMathNode::Operator::Ceiling:
                {
                    m_value = Math::Ceiling( valueA );
                }
                break;

                case FloatMathNode::Operator::IntegerPart:
                {
                    m_value = Math::GetIntegerPart( valueA );
                }
                break;

                case FloatMathNode::Operator::FractionalPart:
                {
                    m_value = Math::GetFractionalPart( valueA );
                }
                break;

                case FloatMathNode::Operator::InverseFractionalPart:
                {
                    m_value = 1.0f - Math::GetFractionalPart( valueA );
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }

            //-------------------------------------------------------------------------

            if ( pDefinition->m_returnAbsoluteResult )
            {
                m_value = Math::Abs( m_value );
            }

            if ( pDefinition->m_returnNegatedResult )
            {
                m_value = -m_value;
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void FloatComparisonNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatComparisonNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
        context.SetOptionalNodePtrFromIndex( m_comparandValueNodeIdx, pNode->m_pComparandValueNode );
    }

    void FloatComparisonNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );

        BoolValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );

        if ( m_pComparandValueNode != nullptr )
        {
            m_pComparandValueNode->Initialize( context );
        }

        m_result = false;
    }

    void FloatComparisonNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );

        if ( m_pComparandValueNode != nullptr )
        {
            m_pComparandValueNode->Shutdown( context );
        }

        m_pInputValueNode->Shutdown( context );
        BoolValueNode::ShutdownInternal( context );
    }

    void FloatComparisonNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        auto pDefinition = GetDefinition<FloatComparisonNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            float const a = m_pInputValueNode->GetValue<float>( context );
            float const b = ( m_pComparandValueNode != nullptr ) ? m_pComparandValueNode->GetValue<float>( context ) : pDefinition->m_comparisonValue;

            switch ( pDefinition->m_comparison )
            {
                case Comparison::GreaterThanEqual:
                m_result = a >= b;
                break;

                case Comparison::LessThanEqual:
                m_result = a <= b;
                break;

                case Comparison::NearEqual:
                m_result = Math::IsNearEqual( a, b, pDefinition->m_epsilon );
                break;

                case Comparison::GreaterThan:
                m_result = a > b;
                break;

                case Comparison::LessThan:
                m_result = a < b;
                break;
            }
        }

        *( (bool*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void FloatRangeComparisonNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatRangeComparisonNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void FloatRangeComparisonNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        BoolValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );
        m_result = false;
    }

    void FloatRangeComparisonNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        BoolValueNode::ShutdownInternal( context );
    }

    void FloatRangeComparisonNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        auto pDefinition = GetDefinition<FloatRangeComparisonNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            float const value = m_pInputValueNode->GetValue<float>( context );
            m_result = pDefinition->m_isInclusiveCheck ? pDefinition->m_range.ContainsInclusive( value ) : pDefinition->m_range.ContainsExclusive( value );
        }

        *( (bool*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void FloatAngleMathNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatAngleMathNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void FloatAngleMathNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        FloatValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );
    }

    void FloatAngleMathNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void FloatAngleMathNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        auto pDefinition = GetDefinition<FloatAngleMathNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            
            Degrees const inputValue = m_pInputValueNode->GetValue<float>( context );

            switch ( pDefinition->m_operation )
            {
                case Operation::ClampTo180:
                {
                    m_value = inputValue.GetClamped180().ToFloat();
                }
                break;

                case Operation::ClampTo360:
                {
                    m_value = inputValue.GetClampedPositive360().ToFloat();
                }
                break;

                case Operation::FlipHemisphere:
                {
                    // Treats 180.0f as the forward direction, so negative values are to the left and positive are to the right
                    m_value = ( inputValue - 180.0f ).GetClamped180().ToFloat();
                }
                break;

                case Operation::FlipHemisphereNegate:
                {
                    // Treats 180.0f as the forward direction but flips the result so negative values are to the right and positive are to the left
                    m_value = ( inputValue - 180.0f ).GetClamped180().ToFloat();
                    m_value = -m_value;
                }
                break;
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void FloatSelectorNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatSelectorNode>( context, options );
        for ( auto nodeIdx : m_conditionNodeIndices )
        {
            BoolValueNode*& pConditionNode = pNode->m_conditionNodes.emplace_back( nullptr );
            context.SetNodePtrFromIndex( nodeIdx, pConditionNode );
        }
    }

    void FloatSelectorNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        FloatValueNode::InitializeInternal( context );
        for ( auto pConditionNode : m_conditionNodes )
        {
            pConditionNode->Initialize( context );
        }
    }

    void FloatSelectorNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        for ( auto pConditionNode : m_conditionNodes )
        {
            pConditionNode->Shutdown( context );
        }
        FloatValueNode::ShutdownInternal( context );
    }

    void FloatSelectorNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() );
        auto pDefinition = GetDefinition<FloatSelectorNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            // Select value
            //-------------------------------------------------------------------------

            float inputTargetValue = pDefinition->m_defaultValue;
            int32_t const numConditions = (int32_t) m_conditionNodes.size();
            for ( int32_t i = 0; i < numConditions; i++ )
            {
                if ( m_conditionNodes[i]->GetValue<bool>(context) )
                {
                    inputTargetValue = pDefinition->m_values[i];
                    break;
                }
            }

            // Perform easing
            //-------------------------------------------------------------------------

            if ( pDefinition->m_easingOp != Math::Easing::Operation::None )
            {
                if ( Math::IsNearEqual( m_currentValue, inputTargetValue, 0.01f ) )
                {
                    m_easeRange = FloatRange( inputTargetValue );
                    m_currentValue = inputTargetValue;
                    m_currentEaseTime = 0;
                }
                else
                {
                    // If the target has changed
                    if ( inputTargetValue != m_easeRange.m_end )
                    {
                        m_easeRange.m_end = inputTargetValue;
                        m_easeRange.m_begin = m_currentValue;
                        m_currentEaseTime = 0;
                    }

                    // Increment the time through the blend
                    m_currentEaseTime += context.m_deltaTime;

                    // Calculate the new value, based on the percentage through the blend calculated by the easing function
                    float const T = Math::Clamp( m_currentEaseTime / pDefinition->m_easeTime, 0.0f, 1.0f );
                    float const blendValue = Math::Easing::Evaluate( pDefinition->m_easingOp, T ) * m_easeRange.GetLength();
                    m_currentValue = m_easeRange.m_begin + blendValue;
                }
            }
            else // Just use the input value
            {
                m_currentValue = inputTargetValue;
            }
        }

        *( (float*) pOutValue ) = m_currentValue;
    }

    void FloatSelectorNode::RecordGraphState( RecordedGraphState& outState )
    {
        FloatValueNode::RecordGraphState( outState );
        outState.WriteValue( m_easeRange );
        outState.WriteValue( m_currentValue );
        outState.WriteValue( m_currentEaseTime );
    }

    bool FloatSelectorNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        if ( !FloatValueNode::RestoreGraphState( inState ) )
        {
            return false;
        }

        inState.ReadValue( m_easeRange );
        inState.ReadValue( m_currentValue );
        inState.ReadValue( m_currentEaseTime );

        return true;
    }
}