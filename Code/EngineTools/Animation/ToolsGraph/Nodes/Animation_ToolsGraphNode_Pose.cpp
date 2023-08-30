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
        ZeroPoseNode::Settings* pSettings = nullptr;
        context.GetSettings<ZeroPoseNode>( this, pSettings );
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    ReferencePoseToolsNode::ReferencePoseToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
    }

    int16_t ReferencePoseToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ReferencePoseNode::Settings* pSettings = nullptr;
        context.GetSettings<ReferencePoseNode>( this, pSettings );
        return pSettings->m_nodeIdx;
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

            //-------------------------------------------------------------------------

            pSettings->m_dataSlotIndex = context.RegisterDataSlotNode( GetID() );
            pSettings->m_inputTimeRemapRange = m_inputTimeRemapRange;
            pSettings->m_userSpecifiedTime = m_fixedTimeValue;
            pSettings->m_useFramesAsInput = m_useFramesAsInput;
        }
        return pSettings->m_nodeIdx;
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