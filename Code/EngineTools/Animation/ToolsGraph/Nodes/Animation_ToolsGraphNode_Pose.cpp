#include "Animation_ToolsGraphNode_Pose.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Pose.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    ZeroPoseToolsNode::ZeroPoseToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
    }

    int16_t ZeroPoseToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ZeroPoseNode::Definition* pDefinition = nullptr;
        context.GetDefinition<ZeroPoseNode>( this, pDefinition );
        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    ReferencePoseToolsNode::ReferencePoseToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
    }

    int16_t ReferencePoseToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ReferencePoseNode::Definition* pDefinition = nullptr;
        context.GetDefinition<ReferencePoseNode>( this, pDefinition );
        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    AnimationPoseToolsNode::AnimationPoseToolsNode()
        : DataSlotToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Time", GraphValueType::Float );
    }

    int16_t AnimationPoseToolsNode::Compile( GraphCompilationContext& context ) const
    {
        AnimationPoseNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<AnimationPoseNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pTimeParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pTimeParameterNode != nullptr )
            {
                auto compiledNodeIdx = pTimeParameterNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_poseTimeValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pDefinition->m_dataSlotIndex = context.RegisterDataSlotNode( GetID() );
            pDefinition->m_inputTimeRemapRange = m_inputTimeRemapRange;
            pDefinition->m_userSpecifiedTime = m_fixedTimeValue;
            pDefinition->m_useFramesAsInput = m_useFramesAsInput;
        }
        return pDefinition->m_nodeIdx;
    }

    bool AnimationPoseToolsNode::DrawPinControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, VisualGraph::Flow::Pin const& pin )
    {
        DataSlotToolsNode::DrawPinControls( ctx, pUserContext, pin );

        // Add parameter value input field
        if ( pin.IsInputPin() && pin.m_type == GetPinTypeForValueType( GraphValueType::Float ) )
        {
            int32_t const pinIdx = GetInputPinIndex( pin.m_ID );

            ImGui::SetNextItemWidth( 50 * ctx.m_viewScaleFactor );
            ImGui::InputFloat( "##parameter", &m_fixedTimeValue, 0.0f, 0.0f, "%.2f" );

            return true;
        }

        return false;
    }
}