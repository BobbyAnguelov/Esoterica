#include "Animation_ToolsGraphNode_Floats.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    FloatRemapToolsNode::FloatRemapToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Float", GraphValueType::Float );
    }

    int16_t FloatRemapToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FloatRemapNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<FloatRemapNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pDefinition->m_inputRange = m_inputRange;
            pDefinition->m_outputRange = m_outputRange;
        }
        return pDefinition->m_nodeIdx;
    }

    void FloatRemapToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx )
    {
        DrawInternalSeparator( ctx );
        ImGui::Text( "[%.2f, %.2f] to [%.2f, %.2f]", m_inputRange.m_begin, m_inputRange.m_end, m_outputRange.m_begin, m_outputRange.m_end );
    }

    //-------------------------------------------------------------------------

    FloatClampToolsNode::FloatClampToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Value", GraphValueType::Float );
    }

    int16_t FloatClampToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FloatClampNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<FloatClampNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pDefinition->m_clampRange = m_clampRange;
        }
        return pDefinition->m_nodeIdx;
    }

    void FloatClampToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx )
    {
        if ( m_clampRange.IsSet() && m_clampRange.IsValid() )
        {
            ImGui::Text( "[%.3f, %.3f]", m_clampRange.m_begin, m_clampRange.m_end );
        }
        else
        {
            ImGui::TextColored( Colors::Red.ToFloat4(), "Invalid" );
        }
    }

    //-------------------------------------------------------------------------

    FloatAbsToolsNode::FloatAbsToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Float", GraphValueType::Float );
    }

    int16_t FloatAbsToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FloatAbsNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<FloatAbsNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }
        }
        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    FloatEaseToolsNode::FloatEaseToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Value", GraphValueType::Float );
    }

    int16_t FloatEaseToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FloatEaseNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<FloatEaseNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( m_easing == Math::Easing::Operation::None )
            {
                context.LogError( this, "No easing type selected!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pDefinition->m_easingOp = m_easing;
            pDefinition->m_easeTime = m_easeTime;
            pDefinition->m_useStartValue = m_useStartValue;
            pDefinition->m_startValue = m_startValue;
        }
        return pDefinition->m_nodeIdx;
    }

    void FloatEaseToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx )
    {
        ImGui::Text( Math::Easing::GetName( m_easing ) );
        ImGui::Text( "Ease Time: %.2fs", m_easeTime );

        if ( m_useStartValue )
        {
            ImGui::Text( "Start Value: %.2fs", m_startValue );
        }
    }

    //-------------------------------------------------------------------------

    FloatCurveToolsNode::FloatCurveToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Float", GraphValueType::Float );
    }

    int16_t FloatCurveToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FloatCurveNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<FloatCurveNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pDefinition->m_curve = m_curve;
        }
        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    FloatMathToolsNode::FloatMathToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "A", GraphValueType::Float );
        CreateInputPin( "B (Optional)", GraphValueType::Float );
    }

    int16_t FloatMathToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FloatMathNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<FloatMathNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNodeA = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNodeA != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNodeA->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdxA = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            auto pInputNodeB = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pInputNodeB != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNodeB->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdxB = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pDefinition->m_returnAbsoluteResult = m_returnAbsoluteResult;
            pDefinition->m_operator = m_operator;
            pDefinition->m_valueB = m_valueB;
        }
        return pDefinition->m_nodeIdx;
    }

    void FloatMathToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx )
    {
        bool const useValueB = GetConnectedInputNode( 1 ) == nullptr;

        switch ( m_operator )
        {
            case FloatMathNode::Operator::Add:
            ImGui::Text( "A + " );
            break;

            case FloatMathNode::Operator::Sub:
            ImGui::Text( "A - " );
            break;

            case FloatMathNode::Operator::Mul:
            ImGui::Text( "A * " );
            break;

            case FloatMathNode::Operator::Div:
            ImGui::Text( "A / ");
            break;
        }

        ImGui::SameLine();

        if ( useValueB )
        {
            ImGui::Text( "%.3f", m_valueB );
        }
        else
        {
            ImGui::Text( "B" );
        }

        if ( m_returnAbsoluteResult )
        {
            ImGui::Text( "Absolute Result" );
        }
    }

    //-------------------------------------------------------------------------

    FloatComparisonToolsNode::FloatComparisonToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "Float", GraphValueType::Float );
        CreateInputPin( "Comparand (Optional)", GraphValueType::Float );
    }

    int16_t FloatComparisonToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FloatComparisonNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<FloatComparisonNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            auto pValueNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pValueNode != nullptr )
            {
                int16_t const compiledNodeIdx = pValueNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_comparandValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pDefinition->m_comparison = m_comparison;
            pDefinition->m_epsilon = m_epsilon;
            pDefinition->m_comparisonValue = m_comparisonValue;
        }
        return pDefinition->m_nodeIdx;
    }

    void FloatComparisonToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx )
    {
        DrawInternalSeparator( ctx );

        static char const* comparisionStr[] =
        {
            ">=",
            "<=",
            "~=",
            ">",
            "<",
        };

        if ( GetConnectedInputNode<FlowToolsNode>( 1 ) != nullptr )
        {
            ImGui::Text( "%s Comparand", comparisionStr[(int32_t) m_comparison] );
        }
        else
        {
            ImGui::Text( "%s %.2f", comparisionStr[(int32_t)m_comparison], m_comparisonValue );
        }
    }

    //-------------------------------------------------------------------------

    FloatRangeComparisonToolsNode::FloatRangeComparisonToolsNode()
        : FlowToolsNode()
    {
        
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "Float", GraphValueType::Float );
    }

    int16_t FloatRangeComparisonToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FloatRangeComparisonNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<FloatRangeComparisonNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pDefinition->m_range = m_range;
            pDefinition->m_isInclusiveCheck = m_isInclusiveCheck;
        }
        return pDefinition->m_nodeIdx;
    }

    void FloatRangeComparisonToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx )
    {
        if ( m_isInclusiveCheck )
        {
            ImGui::Text( "%.2f <= X <= %.2f", m_range.m_begin, m_range.m_end );
        }
        else
        {
            ImGui::Text( "%.2f < X < %.2f", m_range.m_begin, m_range.m_end );
        }
    }

    //-------------------------------------------------------------------------

    FloatSwitchToolsNode::FloatSwitchToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Bool", GraphValueType::Bool );
        CreateInputPin( "If True", GraphValueType::Float );
        CreateInputPin( "If False", GraphValueType::Float );
    }

    int16_t FloatSwitchToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FloatSwitchNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<FloatSwitchNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_switchValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_trueValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 2 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_falseValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }
        }
        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    FloatAngleMathToolsNode::FloatAngleMathToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Angle (deg)", GraphValueType::Float );
    }

    int16_t FloatAngleMathToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FloatAngleMathNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<FloatAngleMathNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }
        }

        pDefinition->m_operation = m_operation;

        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    FloatSelectorToolsNode::FloatSelectorToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Bool ) );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Bool ) );
    }

    void FloatSelectorToolsNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        FlowToolsNode::PostPropertyEdit( pPropertyEdited );

        if ( pPropertyEdited->m_ID == StringID( "m_options" ) )
        {
            RefreshDynamicPins();
        }
    }

    int16_t FloatSelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FloatSelectorNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<FloatSelectorNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numOptions = GetNumInputPins();

            if ( numOptions == 0 )
            {
                context.LogError( this, "No options on dynamic float selector!" );
                return InvalidIndex;
            }

            for ( auto i = 0; i < numOptions; i++ )
            {
                auto pInputNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pInputNode != nullptr )
                {
                    int16_t const compiledNodeIdx = pInputNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_conditionNodeIndices.emplace_back( compiledNodeIdx );
                        pDefinition->m_values.emplace_back( m_options[i].m_value );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( this, "Disconnected input pin!" );
                    return InvalidIndex;
                }
            }
        }

        //-------------------------------------------------------------------------

        pDefinition->m_defaultValue = m_defaultValue;
        pDefinition->m_easeTime = m_easeTime;
        pDefinition->m_easingOp = m_easing;

        return pDefinition->m_nodeIdx;
    }

    void FloatSelectorToolsNode::OnDynamicPinCreation( UUID const& pinID )
    {
        m_options.emplace_back( GetNewDynamicInputPinName().c_str(), 0.0f );
        RefreshDynamicPins();
    }

    void FloatSelectorToolsNode::PreDynamicPinDestruction( UUID const& pinID )
    {
        int32_t const numOptions = GetNumInputPins();
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );
        m_options.erase( m_options.begin() + pinToBeRemovedIdx );
    }

    void FloatSelectorToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx )
    {
        DrawInternalSeparator( ctx );
        ImGui::Text( "Default: %.2f", m_defaultValue );
        ImGui::Text( "Ease: %s", Math::Easing::GetName( m_easing ) );
        if ( m_easing != Math::Easing::Operation::None )
        {
            ImGui::Text( "Ease Time: %.2f", m_easeTime );
        }
    }

    void FloatSelectorToolsNode::RefreshDynamicPins()
    {
        for ( int32_t i = 0; i < GetNumInputPins(); i++ )
        {
            NodeGraph::Pin* pInputPin = GetInputPin( i );
            Option const& option = m_options[i];
            pInputPin->m_name.sprintf( "%s (%.2f)", option.m_name.empty() ? "Input" : option.m_name.c_str(), option.m_value );
        }
    }
}