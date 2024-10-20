#include "Animation_ToolsGraphNode_RootMotionOverride.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_RootMotionOverride.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    RootMotionOverrideToolsNode::RootMotionOverrideToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Desired Moving Velocity (Character)", GraphValueType::Vector );
        CreateInputPin( "Desired Facing Direction (Character)", GraphValueType::Vector );
        CreateInputPin( "Linear Velocity Limit (Optional)", GraphValueType::Float );
        CreateInputPin( "Angular Velocity Limit (Optional)", GraphValueType::Float );
    }

    int16_t RootMotionOverrideToolsNode::Compile( GraphCompilationContext& context ) const
    {
        RootMotionOverrideNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<RootMotionOverrideNode>( this, pDefinition );
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

            auto pMovingVelocityNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            auto pFacingNode = GetConnectedInputNode<FlowToolsNode>( 2 );

            if ( pMovingVelocityNode == nullptr && pFacingNode == nullptr )
            {
                context.LogError( this, "You need to connect at least one of the movement/facing input pins!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            if ( pMovingVelocityNode != nullptr )
            {
                int16_t const compiledNodeIdx = pMovingVelocityNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_desiredMovingVelocityNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            if ( pFacingNode != nullptr )
            {
                int16_t const compiledNodeIdx = pFacingNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_desiredFacingDirectionNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 3 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_linearVelocityLimitNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 4 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_angularVelocityLimitNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pDefinition->m_maxLinearVelocity = m_maxLinearVelocity;
            pDefinition->m_maxAngularVelocity = m_maxAngularVelocity;

            pDefinition->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::AllowMoveX, m_overrideMoveDirX );
            pDefinition->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::AllowMoveY, m_overrideMoveDirY );
            pDefinition->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::AllowMoveZ, m_overrideMoveDirZ );
            pDefinition->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::AllowFacingPitch, m_allowPitchForFacing );
            pDefinition->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::ListenForEvents, m_listenForRootMotionEvents );
        }
        return pDefinition->m_nodeIdx;
    }
}