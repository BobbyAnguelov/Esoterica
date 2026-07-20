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
        m_defaultVariationData.CreateInstance( GetVariationDataTypeInfo() );

        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Effector Target", GraphValueType::Target );
        CreateInputPin( "Enable", GraphValueType::Bool );
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

            //-------------------------------------------------------------------------

            auto pEnableNode = GetConnectedInputNode<FlowToolsNode>( 2 );
            if ( pEnableNode != nullptr )
            {
                int16_t const compiledNodeIdx = pEnableNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_enableNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            pDefinition->m_effectorBoneID = pData->m_effectorBoneID;

            if ( !pData->m_effectorBoneID.IsValid() )
            {
                context.LogError( this, "No effector bone ID set for variation: %s!", context.GetVariationID().c_str() );
                return InvalidIndex;
            }

            pDefinition->m_isTargetInWorldSpace = m_isTargetInWorldSpace;
            pDefinition->m_blendTime = Math::Max( pData->m_blendTime.ToFloat(), 0.0f);
            pDefinition->m_blendMode = m_blendMode;
            pDefinition->m_referencePoseTwistWeight = m_referencePoseTwistWeight;
        }

        return pDefinition->m_nodeIdx;
    }

    void TwoBoneIKToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            int16_t const runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
            auto pRuntimeNode = static_cast<TwoBoneIKNode const*>( pGraphNodeContext->GetNodeDebugInstance( runtimeNodeIdx ) );
            if ( pRuntimeNode->IsInitialized() )
            {
                BeginDrawInternalRegion( ctx );
                ImGui::Text( "%.2f", pRuntimeNode->GetIKWeight() );
                EndDrawInternalRegion( ctx );
            }
        }

        //-------------------------------------------------------------------------

        DrawInternalSeparator( ctx );

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