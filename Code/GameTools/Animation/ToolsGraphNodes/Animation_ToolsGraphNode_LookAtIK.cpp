#include "Animation_ToolsGraphNode_LookAtIK.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Game/Animation/GraphNodes/Animation_RuntimeGraphNode_LookAtIK.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    LookAtIKToolsNode::LookAtIKToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Pose, true );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "World Target Point", GraphValueType::Vector );
    }

    int16_t LookAtIKToolsNode::Compile( GraphCompilationContext& context ) const
    {
        LookAtIKNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<LookAtIKNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_childNodeIdx = compiledNodeIdx;
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

            auto pTargetNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pTargetNode != nullptr )
            {
                int16_t const compiledNodeIdx = pTargetNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_targetNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected target pin!" );
                return InvalidIndex;
            }
        }
        return pSettings->m_nodeIdx;
    }
}