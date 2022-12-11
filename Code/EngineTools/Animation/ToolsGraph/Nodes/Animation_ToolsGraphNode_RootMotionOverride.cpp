#include "Animation_ToolsGraphNode_RootMotionOverride.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_RootMotionOverride.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void RootMotionOverrideToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Pose, true );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Desired Heading Velocity (Character)", GraphValueType::Vector );
        CreateInputPin( "Desired Facing Direction (Character)", GraphValueType::Vector );
        CreateInputPin( "Linear Velocity Limit (Optional)", GraphValueType::Float );
        CreateInputPin( "Angular Velocity Limit (Optional)", GraphValueType::Float );
    }

    int16_t RootMotionOverrideToolsNode::Compile( GraphCompilationContext& context ) const
    {
        RootMotionOverrideNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<RootMotionOverrideNode>( this, pSettings );
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

            auto pHeadingNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            auto pFacingNode = GetConnectedInputNode<FlowToolsNode>( 2 );

            if ( pHeadingNode == nullptr && pFacingNode == nullptr )
            {
                context.LogError( this, "You need to connect at least one of the heading/facing input pins!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            if ( pHeadingNode != nullptr )
            {
                int16_t const compiledNodeIdx = pHeadingNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_desiredHeadingVelocityNodeIdx = compiledNodeIdx;
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
                    pSettings->m_desiredFacingDirectionNodeIdx = compiledNodeIdx;
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
                    pSettings->m_linearVelocityLimitNodeIdx = compiledNodeIdx;
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
                    pSettings->m_angularVelocityLimitNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pSettings->m_maxLinearVelocity = m_maxLinearVelocity;
            pSettings->m_maxAngularVelocity = m_maxAngularVelocity;

            pSettings->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::AllowHeadingX, m_overrideHeadingX );
            pSettings->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::AllowHeadingY, m_overrideHeadingY );
            pSettings->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::AllowHeadingZ, m_overrideHeadingZ );
            pSettings->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::AllowFacingPitch, m_allowPitchForFacing );
            pSettings->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::ListenForEvents, m_listenForRootMotionEvents );
        }
        return pSettings->m_nodeIdx;
    }
}