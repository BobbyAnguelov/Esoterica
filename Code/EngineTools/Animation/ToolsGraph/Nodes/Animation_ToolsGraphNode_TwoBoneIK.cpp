#include "Animation_ToolsGraphNode_TwoBoneIK.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_TwoBoneIK.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    TwoBoneIKToolsNode::TwoBoneIKToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Effector Target", GraphValueType::Target );
    }

    int16_t TwoBoneIKToolsNode::Compile( GraphCompilationContext& context ) const
    {
        TwoBoneIKNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<TwoBoneIKNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( !m_effectorBoneID.IsValid() )
            {
                context.LogError( this, "Invalid effector bone ID!" );
                return InvalidIndex;
            }

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
                    pDefinition->m_effectorTargetNodeIdx = compiledNodeIdx;
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

        pDefinition->m_effectorBoneID = m_effectorBoneID;
        pDefinition->m_allowedStretchPercentage = Math::Clamp( m_allowedStretchPercentage.ToFloat(), 0.0f, 0.1f );
        pDefinition->m_isTargetInWorldSpace = m_isTargetInWorldSpace;

        return pDefinition->m_nodeIdx;
    }
}