#include "Animation_EditorGraphNode_Vectors.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void VectorInfoEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Vector", GraphValueType::Vector );
    }

    int16_t VectorInfoEditorNode::Compile( GraphCompilationContext& context ) const
    {
        VectorInfoNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<VectorInfoNode>( this, pSettings );
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

            pSettings->m_desiredInfo = m_desiredInfo;
        }
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    void VectorNegateEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Vector, true );
        CreateInputPin( "Vector", GraphValueType::Vector );
    }

    int16_t VectorNegateEditorNode::Compile( GraphCompilationContext& context ) const
    {
        VectorNegateNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<VectorNegateNode>( this, pSettings );
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