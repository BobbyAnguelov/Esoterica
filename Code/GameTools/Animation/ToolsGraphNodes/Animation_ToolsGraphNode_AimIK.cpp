#include "Animation_ToolsGraphNode_AimIK.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Game/Animation/GraphNodes/Animation_RuntimeGraphNode_AimIK.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    AimIKToolsNode::AimIKToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "World Target", GraphValueType::Vector );
    }

    int16_t AimIKToolsNode::Compile( GraphCompilationContext& context ) const
    {
        AimIKNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<AimIKNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_childNodeIdx = compiledNodeIdx;
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
                    pDefinition->m_targetNodeIdx = compiledNodeIdx;
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

        return pDefinition->m_nodeIdx;
    }
}