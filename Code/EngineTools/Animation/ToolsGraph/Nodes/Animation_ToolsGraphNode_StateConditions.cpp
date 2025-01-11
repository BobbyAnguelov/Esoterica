#include "Animation_ToolsGraphNode_StateConditions.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    StateCompletedConditionToolsNode::StateCompletedConditionToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
    }

    int16_t StateCompletedConditionToolsNode::Compile( GraphCompilationContext& context ) const
    {
        StateCompletedConditionNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<StateCompletedConditionNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pDefinition->m_sourceStateNodeIdx = context.GetConduitSourceStateIndex();
            pDefinition->m_transitionDuration = context.GetCompiledTransitionDuration();
            pDefinition->m_transitionDurationOverrideNodeIdx = context.GetCompiledTransitionDurationOverrideIdx();
        }
        else // Encountered this node twice during compilation
        {
            context.LogError( this, "State Completed Conditions are only allowed to be connected to a single transition" );
            return InvalidIndex;
        }

        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    TimeConditionToolsNode::TimeConditionToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "Time Value (optional)", GraphValueType::Float );
    }

    int16_t TimeConditionToolsNode::Compile( GraphCompilationContext& context ) const
    {
        TimeConditionNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<TimeConditionNode>( this, pDefinition );
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

            //-------------------------------------------------------------------------

            pDefinition->m_sourceStateNodeIdx = context.GetConduitSourceStateIndex();
            pDefinition->m_comparand = m_comparand;
            pDefinition->m_type = m_type;
            pDefinition->m_operator = m_operator;
        }
        return pDefinition->m_nodeIdx;
    }

    void TimeConditionToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        InlineString text;

        switch ( m_type )
        {
            case TimeConditionNode::ComparisonType::PercentageThroughState:
            text = "Percentage Through State";
            break;

            case TimeConditionNode::ComparisonType::PercentageThroughSyncEvent:
            text = "Percentage Through Sync Event";
            break;

            case TimeConditionNode::ComparisonType::ElapsedTime:
            text = "Elapsed Time In State";
            break;

            default:
            break;
        }

        switch ( m_operator )
        {
            case TimeConditionNode::Operator::LessThan:
            text += " < ";
            break;

            case TimeConditionNode::Operator::LessThanEqual:
            text += " <= ";
            break;

            case TimeConditionNode::Operator::GreaterThan:
            text += " > ";
            break;

            case TimeConditionNode::Operator::GreaterThanEqual:
            text += " >= ";
            break;

            default:
            break;
        }

        text += InlineString( InlineString::CtorSprintf(), "%.3f", m_comparand);

        ImGui::Text( text.c_str() );
    }
}