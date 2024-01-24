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

    void FloatRemapToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
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

    void FloatClampToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
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

    void FloatEaseToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
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

    void FloatMathToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
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

    void FloatComparisonToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
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

    void FloatRangeComparisonToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
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
        CreateInputPin( "Option 0", GraphValueType::Bool );
        CreateInputPin( "Option 1", GraphValueType::Bool );

        m_pinValues.resize( 2, 0.0f );
    }

    int16_t FloatSelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FloatSelectorNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<FloatSelectorNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numOptions = GetNumInputPins();
            for ( auto i = 0; i < numOptions; i++ )
            {
                auto pInputNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pInputNode != nullptr )
                {
                    int16_t const compiledNodeIdx = pInputNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_conditionNodeIndices.emplace_back( compiledNodeIdx );
                        pDefinition->m_values.emplace_back( m_pinValues[i] );
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

    TInlineString<100> FloatSelectorToolsNode::GetNewDynamicInputPinName() const
    {
        int32_t const numOptions = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numOptions - 1 );
        return pinName;
    }

    void FloatSelectorToolsNode::OnDynamicPinCreation( UUID pinID )
    {
        m_pinValues.emplace_back( 0.0f );
    }

    void FloatSelectorToolsNode::OnDynamicPinDestruction( UUID pinID )
    {
        int32_t const numOptions = GetNumInputPins();
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );

        m_pinValues.erase( m_pinValues.begin() + pinToBeRemovedIdx );

        // Rename all pins
        //-------------------------------------------------------------------------

        int32_t newPinIdx = 2;
        for ( auto i = 2; i < numOptions; i++ )
        {
            if ( i == pinToBeRemovedIdx )
            {
                continue;
            }

            TInlineString<100> newPinName;
            newPinName.sprintf( "Option %d", newPinIdx );
            GetInputPin( i )->m_name = newPinName;
            newPinIdx++;
        }
    }

    bool FloatSelectorToolsNode::DrawPinControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, VisualGraph::Flow::Pin const& pin )
    {
        FlowToolsNode::DrawPinControls( ctx, pUserContext, pin );

        // Add parameter value input field
        if ( pin.IsInputPin() && pin.m_type == GetPinTypeForValueType( GraphValueType::Bool ) )
        {
            int32_t const valueIdx = GetInputPinIndex( pin.m_ID );
            EE_ASSERT( valueIdx >= 0 && valueIdx < m_pinValues.size() );

            ImGui::PushID( &m_pinValues[valueIdx] );
            ImGui::SetNextItemWidth( 50 * ctx.m_viewScaleFactor );
            ImGui::InputFloat( "##parameter", &m_pinValues[valueIdx], 0.0f, 0.0f, "%.2f" );
            ImGui::PopID();

            return true;
        }

        return false;
    }
}