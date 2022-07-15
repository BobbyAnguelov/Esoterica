#include "Animation_EditorGraphNode_SpeedScale.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_SpeedScale.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void SpeedScaleEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Pose, true );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Scale", GraphValueType::Float );
    }

    int16_t SpeedScaleEditorNode::Compile( GraphCompilationContext& context ) const
    {
        SpeedScaleNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<SpeedScaleNode>( this, pSettings );
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
                    pSettings->m_scaleValueNodeIdx = compiledNodeIdx;
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

            pSettings->m_scaleLimits = m_scaleLimits;
            pSettings->m_blendTime = m_blendTime;
        }
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    void VelocityBasedSpeedScaleEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Pose, true );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Scale", GraphValueType::Float );
    }

    int16_t VelocityBasedSpeedScaleEditorNode::Compile( GraphCompilationContext& context ) const
    {
        VelocityBasedSpeedScaleNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<VelocityBasedSpeedScaleNode>( this, pSettings );
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
                    pSettings->m_desiredVelocityValueNodeIdx = compiledNodeIdx;
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

            pSettings->m_blendTime = m_blendTime;
        }
        return pSettings->m_nodeIdx;
    }
}