#include "Animation_EditorGraphNode_Bools.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Bools.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void AndEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "And", GraphValueType::Bool );
        CreateInputPin( "And", GraphValueType::Bool );
    }

    int16_t AndEditorNode::Compile( GraphCompilationContext& context ) const
    {
        AndNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<AndNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numInputs = GetNumInputPins();
            for ( auto i = 0; i < numInputs; i++ )
            {
                // We allow some disconnected pins
                auto pConnectedNode = GetConnectedInputNode<EditorGraphNode>( i );
                if ( pConnectedNode != nullptr )
                {
                    auto compiledNodeIdx = pConnectedNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pSettings->m_conditionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( pSettings->m_conditionNodeIndices.empty() )
            {
                context.LogError( this, "All inputs on 'And' node disconnected" );
                return InvalidIndex;
            }
        }
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    void OrEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "Or", GraphValueType::Bool );
        CreateInputPin( "Or", GraphValueType::Bool );
    }

    int16_t OrEditorNode::Compile( GraphCompilationContext& context ) const
    {
        OrNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<OrNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numInputs = GetNumInputPins();
            for ( auto i = 0; i < numInputs; i++ )
            {
                // We allow some disconnected pins
                auto pConnectedNode = GetConnectedInputNode<EditorGraphNode>( i );
                if ( pConnectedNode != nullptr )
                {
                    auto compiledNodeIdx = pConnectedNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pSettings->m_conditionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( pSettings->m_conditionNodeIndices.empty() )
            {
                context.LogError( this, "All inputs on 'Or' node disconnected" );
                return InvalidIndex;
            }
        }
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    void NotEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "Not", GraphValueType::Bool );
    }

    int16_t NotEditorNode::Compile( GraphCompilationContext& context ) const
    {
        NotNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<NotNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            // Set input node
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
            if ( pInputNode != nullptr )
            {
                pSettings->m_inputValueNodeIdx = pInputNode->Compile( context );
                if ( pSettings->m_inputValueNodeIdx == InvalidIndex )
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
        return pSettings->m_nodeIdx;
    }
}