#include "Animation_RuntimeGraphNode_Floats.h"
#include "System/Log.h"
#include "System/Math/MathHelpers.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void FloatSwitchNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatSwitchNode>( context, options );
        context.SetNodePtrFromIndex( m_switchValueNodeIdx, pNode->m_pSwitchValueNode );
        context.SetNodePtrFromIndex( m_trueValueNodeIdx, pNode->m_pTrueValueNode );
        context.SetNodePtrFromIndex( m_falseValueNodeIdx, pNode->m_pFalseValueNode );
    }

    void FloatSwitchNode::InitializeInternal( GraphContext& context )
    {
        FloatValueNode::InitializeInternal( context );
        m_pSwitchValueNode->Initialize( context );
        m_pTrueValueNode->Initialize( context );
        m_pFalseValueNode->Initialize( context );
    }

    void FloatSwitchNode::ShutdownInternal( GraphContext& context )
    {
        m_pSwitchValueNode->Shutdown( context );
        m_pTrueValueNode->Shutdown( context );
        m_pFalseValueNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void FloatSwitchNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            if ( m_pSwitchValueNode->GetValue<bool>( context ) )
            {
                m_value = m_pTrueValueNode->GetValue<float>( context );
            }
            else
            {
                m_value = m_pFalseValueNode->GetValue<float>( context );
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void FloatRemapNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
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
        auto pSettings = GetSettings<FloatRemapNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            float const inputValue = m_pInputValueNode->GetValue<float>( context );
            m_value = Math::RemapRangeClamped( inputValue, pSettings->m_inputRange.m_begin, pSettings->m_inputRange.m_end, pSettings->m_outputRange.m_begin, pSettings->m_outputRange.m_end );
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void FloatClampNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
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
        auto pSettings = GetSettings<FloatClampNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            auto const inputValue = m_pInputValueNode->GetValue<float>( context );
            m_value = pSettings->m_clampRange.GetClampedValue( inputValue );
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void FloatAbsNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatAbsNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void FloatAbsNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        FloatValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );
    }

    void FloatAbsNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void FloatAbsNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        auto pSettings = GetSettings<FloatAbsNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            auto const inputValue = m_pInputValueNode->GetValue<float>( context );
            m_value = Math::Abs( inputValue );
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void FloatEaseNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatEaseNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void FloatEaseNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );

        auto pSettings = GetSettings<FloatEaseNode>();

        FloatValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );

        m_easeRange = FloatRange( m_pInputValueNode->GetValue<float>( context ) );
        m_currentValue = pSettings->m_initalValue < 0.0f ? m_easeRange.m_end : pSettings->m_initalValue;
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
        auto pSettings = GetSettings<FloatEaseNode>();

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
                float const T = Math::Clamp( m_currentEaseTime / pSettings->m_easeTime, 0.0f, 1.0f );
                float const blendValue = Math::Easing::EvaluateEasingFunction( pSettings->m_easingType, T ) * m_easeRange.GetLength();
                m_currentValue = m_easeRange.m_begin + blendValue;
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_currentValue;
    }

    //-------------------------------------------------------------------------

    void FloatCurveNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
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
        auto pSettings = GetSettings<FloatCurveNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            float const inputTargetValue = m_pInputValueNode->GetValue<float>( context );
            m_currentValue = pSettings->m_curve.Evaluate( inputTargetValue );
        }

        *reinterpret_cast<float*>( pOutValue ) = m_currentValue;
    }

    //-------------------------------------------------------------------------

    void FloatMathNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatMathNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdxA, pNode->m_pValueNodeA );
        context.SetNodePtrFromIndex( m_inputValueNodeIdxB, pNode->m_pValueNodeB );
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
        auto FloatNodeSettings = GetSettings<FloatMathNode>();

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
                valueB = FloatNodeSettings->m_valueB;
            }

            // Calculate Result
            switch ( FloatNodeSettings->m_operator )
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
                        EE_LOG_WARNING( "Animation", "TODO", "Dividing by zero in FloatMathNode" );
                        m_value = FLT_MAX;
                    }
                    else
                    {
                        m_value = valueA / valueB;
                    }
                }
                break;
            }

            //-------------------------------------------------------------------------

            if ( FloatNodeSettings->m_returnAbsoluteResult )
            {
                m_value = Math::Abs( m_value );
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void FloatComparisonNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
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
        auto pSettings = GetSettings<FloatComparisonNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            float const a = m_pInputValueNode->GetValue<float>( context );
            float const b = ( m_pComparandValueNode != nullptr ) ? m_pComparandValueNode->GetValue<float>( context ) : pSettings->m_comparisonValue;

            switch ( pSettings->m_comparison )
            {
                case Comparison::GreaterThanEqual:
                m_result = a >= b;
                break;

                case Comparison::LessThanEqual:
                m_result = a <= b;
                break;

                case Comparison::NearEqual:
                m_result = Math::IsNearEqual( a, b, pSettings->m_epsilon );
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

    void FloatRangeComparisonNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
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
        auto pSettings = GetSettings<FloatRangeComparisonNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            float const value = m_pInputValueNode->GetValue<float>( context );
            m_result = pSettings->m_isInclusiveCheck ? pSettings->m_range.ContainsInclusive( value ) : pSettings->m_range.ContainsExclusive( value );
        }

        *( (bool*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void FloatReverseDirectionNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FloatReverseDirectionNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void FloatReverseDirectionNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        FloatValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );
    }

    void FloatReverseDirectionNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void FloatReverseDirectionNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            float const inputValue = m_pInputValueNode->GetValue<float>( context );
            
            if ( inputValue < 0.0f )
            {
                m_value = -180.0f -inputValue;
            }
            else
            {
                m_value = 180.0f - inputValue;
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }
}