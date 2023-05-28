#include "Animation_ToolsGraphNode_StateConditions.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    StateCompletedConditionToolsNode::StateCompletedConditionToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
    }

    int16_t StateCompletedConditionToolsNode::Compile( GraphCompilationContext& context ) const
    {
        StateCompletedConditionNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<StateCompletedConditionNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_sourceStateNodeIdx = context.GetConduitSourceStateIndex();
            pSettings->m_transitionDuration = context.GetCompiledTransitionDuration();
            pSettings->m_transitionDurationOverrideNodeIdx = context.GetCompiledTransitionDurationOverrideIdx();
        }
        else // Encountered this node twice during compilation
        {
            context.LogError( this, "State Completed Conditions are only allowed to be connected to a single transition" );
            return InvalidIndex;
        }

        return pSettings->m_nodeIdx;
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
        TimeConditionNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<TimeConditionNode>( this, pSettings );
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

            //-------------------------------------------------------------------------

            pSettings->m_sourceStateNodeIdx = context.GetConduitSourceStateIndex();
            pSettings->m_comparand = m_comparand;
            pSettings->m_type = m_type;
            pSettings->m_operator = m_operator;
        }
        return pSettings->m_nodeIdx;
    }

    void TimeConditionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
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