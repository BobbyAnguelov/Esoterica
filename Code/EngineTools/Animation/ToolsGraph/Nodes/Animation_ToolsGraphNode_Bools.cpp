#include "Animation_ToolsGraphNode_Bools.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Bools.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
     AndToolsNode::AndToolsNode()
         : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "And", GraphValueType::Bool );
        CreateInputPin( "And", GraphValueType::Bool );
    }

    int16_t AndToolsNode::Compile( GraphCompilationContext& context ) const
    {
        AndNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<AndNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numInputs = GetNumInputPins();
            for ( auto i = 0; i < numInputs; i++ )
            {
                // We allow some disconnected pins
                auto pConnectedNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pConnectedNode != nullptr )
                {
                    auto compiledNodeIdx = pConnectedNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_conditionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( pDefinition->m_conditionNodeIndices.empty() )
            {
                context.LogError( this, "All inputs on 'And' node disconnected" );
                return InvalidIndex;
            }
        }
        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    OrToolsNode::OrToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "Or", GraphValueType::Bool );
        CreateInputPin( "Or", GraphValueType::Bool );
    }

    int16_t OrToolsNode::Compile( GraphCompilationContext& context ) const
    {
        OrNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<OrNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numInputs = GetNumInputPins();
            for ( auto i = 0; i < numInputs; i++ )
            {
                // We allow some disconnected pins
                auto pConnectedNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pConnectedNode != nullptr )
                {
                    auto compiledNodeIdx = pConnectedNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_conditionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( pDefinition->m_conditionNodeIndices.empty() )
            {
                context.LogError( this, "All inputs on 'Or' node disconnected" );
                return InvalidIndex;
            }
        }
        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    NotToolsNode::NotToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "Not", GraphValueType::Bool );
    }

    int16_t NotToolsNode::Compile( GraphCompilationContext& context ) const
    {
        NotNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<NotNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            // Set input node
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                pDefinition->m_inputValueNodeIdx = pInputNode->Compile( context );
                if ( pDefinition->m_inputValueNodeIdx == InvalidIndex )
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Input not connected!" );
                return InvalidIndex;
            }
        }
        return pDefinition->m_nodeIdx;
    }
}