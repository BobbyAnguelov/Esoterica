#include "Animation_EditorGraphNode_Floats.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void FloatRemapEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Float", GraphValueType::Float );
    }

    int16_t FloatRemapEditorNode::Compile( GraphCompilationContext& context ) const
    {
        FloatRemapNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatRemapNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

    void FloatRemapEditorNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( "[%.2f, %.2f] to [%.2f, %.2f]", m_inputRange.m_begin, m_inputRange.m_end, m_outputRange.m_begin, m_outputRange.m_end );
    }

    //-------------------------------------------------------------------------

    void FloatClampEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Float", GraphValueType::Float );
    }

    int16_t FloatClampEditorNode::Compile( GraphCompilationContext& context ) const
    {
        FloatClampNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatClampNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

    //-------------------------------------------------------------------------

    void FloatAbsEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Float", GraphValueType::Float );
    }

    int16_t FloatAbsEditorNode::Compile( GraphCompilationContext& context ) const
    {
        FloatAbsNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatAbsNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

    void FloatEaseEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Float", GraphValueType::Float );
    }

    int16_t FloatEaseEditorNode::Compile( GraphCompilationContext& context ) const
    {
        FloatEaseNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatEaseNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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
            pSettings->m_initalValue = m_initialValue;
        }
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    void FloatCurveEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Float", GraphValueType::Float );
    }

    int16_t FloatCurveEditorNode::Compile( GraphCompilationContext& context ) const
    {
        FloatCurveNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatCurveNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

    void FloatMathEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "A", GraphValueType::Float );
        CreateInputPin( "B (Optional)", GraphValueType::Float );
    }

    int16_t FloatMathEditorNode::Compile( GraphCompilationContext& context ) const
    {
        FloatMathNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatMathNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNodeA = GetConnectedInputNode<EditorGraphNode>( 0 );
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

            auto pInputNodeB = GetConnectedInputNode<EditorGraphNode>( 1 );
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

    //-------------------------------------------------------------------------

    void FloatComparisonEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "Float", GraphValueType::Float );
    }

    int16_t FloatComparisonEditorNode::Compile( GraphCompilationContext& context ) const
    {
        FloatComparisonNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatComparisonNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

            pSettings->m_comparison = m_comparison;
            pSettings->m_epsilon = m_epsilon;
            pSettings->m_comparisonValue = m_comparisonValue;
        }
        return pSettings->m_nodeIdx;
    }

    void FloatComparisonEditorNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        static char const* comparisionStr[] =
        {
            ">=",
            "<=",
            "~=",
            ">",
            "<",
        };

        ImGui::Text( "%s %.2f", comparisionStr[(int32_t)m_comparison], m_comparisonValue );
    }

    //-------------------------------------------------------------------------

    void FloatRangeComparisonEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "Float", GraphValueType::Float );
    }

    int16_t FloatRangeComparisonEditorNode::Compile( GraphCompilationContext& context ) const
    {
        FloatRangeComparisonNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatRangeComparisonNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

    void FloatRangeComparisonEditorNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
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

    void FloatSwitchEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Bool", GraphValueType::Bool );
        CreateInputPin( "If True", GraphValueType::Float );
        CreateInputPin( "If False", GraphValueType::Float );
    }

    int16_t FloatSwitchEditorNode::Compile( GraphCompilationContext& context ) const
    {
        FloatSwitchNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatSwitchNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

            pInputNode = GetConnectedInputNode<EditorGraphNode>( 1 );
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

            pInputNode = GetConnectedInputNode<EditorGraphNode>( 2 );
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

    void FloatReverseDirectionEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Float", GraphValueType::Float );
    }

    int16_t FloatReverseDirectionEditorNode::Compile( GraphCompilationContext& context ) const
    {
        FloatReverseDirectionNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FloatReverseDirectionNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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
}