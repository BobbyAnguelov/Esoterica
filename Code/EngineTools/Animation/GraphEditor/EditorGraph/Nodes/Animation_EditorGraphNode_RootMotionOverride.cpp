#include "Animation_EditorGraphNode_RootMotionOverride.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_RootMotionOverride.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void RootMotionOverrideEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Pose, true );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Desired Heading Velocity (Character)", GraphValueType::Vector );
        CreateInputPin( "Desired Facing Direction (Character)", GraphValueType::Vector );
        CreateInputPin( "Linear Velocity Limit (Optional)", GraphValueType::Float );
        CreateInputPin( "Angular Velocity Limit (Optional)", GraphValueType::Float );
    }

    int16_t RootMotionOverrideEditorNode::Compile( GraphCompilationContext& context ) const
    {
        RootMotionOverrideNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<RootMotionOverrideNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

            pInputNode = GetConnectedInputNode<EditorGraphNode>( 1 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_desiredHeadingVelocityNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected Heading Velocity pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pInputNode = GetConnectedInputNode<EditorGraphNode>( 2 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_desiredFacingDirectionNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected Facing Direction pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pInputNode = GetConnectedInputNode<EditorGraphNode>( 3 );
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

            pInputNode = GetConnectedInputNode<EditorGraphNode>( 4 );
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

            pSettings->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::HeadingX, m_overrideHeadingX );
            pSettings->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::HeadingY, m_overrideHeadingY );
            pSettings->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::HeadingZ, m_overrideHeadingZ );
            pSettings->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::FacingX, m_overrideFacingX );
            pSettings->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::FacingY, m_overrideFacingY );
            pSettings->m_overrideFlags.SetFlag( RootMotionOverrideNode::OverrideFlags::FacingZ, m_overrideFacingZ );
        }
        return pSettings->m_nodeIdx;
    }
}