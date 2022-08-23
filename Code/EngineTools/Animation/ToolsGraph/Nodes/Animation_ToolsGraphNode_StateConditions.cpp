#include "Animation_ToolsGraphNode_StateConditions.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void StateCompletedConditionToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
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

    void TimeConditionToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateOutputPin( "Time Value (optional)", GraphValueType::Float, true );
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
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pSettings->m_sourceStateNodeIdx = context.GetConduitSourceStateIndex();
            pSettings->m_comparand = m_comparand;
            pSettings->m_type = m_type;
            pSettings->m_operator = m_operator;
        }
        return pSettings->m_nodeIdx;
    }
}