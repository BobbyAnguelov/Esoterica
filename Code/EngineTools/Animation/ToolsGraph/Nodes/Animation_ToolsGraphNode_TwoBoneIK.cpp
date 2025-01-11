#include "Animation_ToolsGraphNode_TwoBoneIK.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_UserContext.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_TwoBoneIK.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    TwoBoneIKToolsNode::TwoBoneIKToolsNode()
        : VariationDataToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Effector Target", GraphValueType::Target );

        m_defaultVariationData.CreateInstance( GetVariationDataTypeInfo() );
    }

    int16_t TwoBoneIKToolsNode::Compile( GraphCompilationContext& context ) const
    {
        TwoBoneIKNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<TwoBoneIKNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pData = GetResolvedVariationDataAs<Data>( context.GetVariationHierarchy(), context.GetVariationID() );
            if ( !pData->m_effectorBoneID.IsValid() )
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

            pDefinition->m_effectorBoneID = pData->m_effectorBoneID;
            pDefinition->m_isTargetInWorldSpace = m_isTargetInWorldSpace;
        }

        return pDefinition->m_nodeIdx;
    }

    void TwoBoneIKToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        DrawInternalSeparator( ctx );

        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        auto pData = GetResolvedVariationDataAs<Data>( *pGraphNodeContext->m_pVariationHierarchy, pGraphNodeContext->m_selectedVariationID );
        if ( !pData->m_effectorBoneID.IsValid() )
        {
            ImGui::TextColored( Colors::Red.ToFloat4(), "Invalid Effector Bone ID" );
        }
        else
        {
            ImGui::Text( "Effector: %s", pData->m_effectorBoneID.c_str() );
        }
    }
}