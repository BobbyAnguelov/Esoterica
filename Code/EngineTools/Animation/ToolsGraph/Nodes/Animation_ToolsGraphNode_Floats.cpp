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
        FloatRemapNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatRemapNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdx = compiledNodeIdx;
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

            pSettings->m_inputRange = m_inputRange;
            pSettings->m_outputRange = m_outputRange;
        }
        return pSettings->m_nodeIdx;
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
        FloatClampNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatClampNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdx = compiledNodeIdx;
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

            pSettings->m_clampRange = m_clampRange;
        }
        return pSettings->m_nodeIdx;
    }

    void FloatClampToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        if ( m_clampRange.IsSet() && m_clampRange.IsValid() )
        {
            ImGui::Text( "[%.3f, %.3f]", m_clampRange.m_begin, m_clampRange.m_end );
        }
        else
        {
            ImGui::TextColored( ImColor( 0xFF0000FF ), "Invalid" );
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
        FloatAbsNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatAbsNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdx = compiledNodeIdx;
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
        return pSettings->m_nodeIdx;
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
        FloatEaseNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatEaseNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdx = compiledNodeIdx;
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

            pSettings->m_easingType = m_easingType;
            pSettings->m_easeTime = m_easeTime;
            pSettings->m_useStartValue = m_useStartValue;
            pSettings->m_startValue = m_startValue;
        }
        return pSettings->m_nodeIdx;
    }

    void FloatEaseToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( Math::Easing::GetName( m_easingType ) );
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
        FloatCurveNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatCurveNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdx = compiledNodeIdx;
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

            pSettings->m_curve = m_curve;
        }
        return pSettings->m_nodeIdx;
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
        FloatMathNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatMathNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNodeA = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNodeA != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNodeA->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdxA = compiledNodeIdx;
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
                    pSettings->m_inputValueNodeIdxB = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pSettings->m_returnAbsoluteResult = m_returnAbsoluteResult;
            pSettings->m_operator = m_operator;
            pSettings->m_valueB = m_valueB;
        }
        return pSettings->m_nodeIdx;
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
        FloatComparisonNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatComparisonNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdx = compiledNodeIdx;
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
                    pSettings->m_comparandValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pSettings->m_comparison = m_comparison;
            pSettings->m_epsilon = m_epsilon;
            pSettings->m_comparisonValue = m_comparisonValue;
        }
        return pSettings->m_nodeIdx;
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
        FloatRangeComparisonNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatRangeComparisonNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdx = compiledNodeIdx;
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

            pSettings->m_range = m_range;
            pSettings->m_isInclusiveCheck = m_isInclusiveCheck;
        }
        return pSettings->m_nodeIdx;
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
        FloatSwitchNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatSwitchNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_switchValueNodeIdx = compiledNodeIdx;
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
                    pSettings->m_trueValueNodeIdx = compiledNodeIdx;
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
                    pSettings->m_falseValueNodeIdx = compiledNodeIdx;
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
        return pSettings->m_nodeIdx;
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
        FloatAngleMathNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatAngleMathNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdx = compiledNodeIdx;
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

        pSettings->m_operation = m_operation;

        return pSettings->m_nodeIdx;
    }
}