#include "Animation_ToolsGraphNode_Pose.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Pose.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ZeroPoseToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Pose", GraphValueType::Pose );
    }

    int16_t ZeroPoseToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ZeroPoseNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ZeroPoseNode>( this, pSettings );
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    void ReferencePoseToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Pose", GraphValueType::Pose );
    }

    int16_t ReferencePoseToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ReferencePoseNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ReferencePoseNode>( this, pSettings );
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    void AnimationPoseToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        DataSlotToolsNode::Initialize( pParent );
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Time", GraphValueType::Float );
    }

    int16_t AnimationPoseToolsNode::Compile( GraphCompilationContext& context ) const
    {
        AnimationPoseNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<AnimationPoseNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pTimeParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pTimeParameterNode != nullptr )
            {
                auto compiledNodeIdx = pTimeParameterNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_poseTimeValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected time parameter pin on animation pose node!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pSettings->m_dataSlotIndex = context.RegisterDataSlotNode( GetID() );
            pSettings->m_inputTimeRemapRange = m_inputTimeRemapRange;
        }
        return pSettings->m_nodeIdx;
    }
}