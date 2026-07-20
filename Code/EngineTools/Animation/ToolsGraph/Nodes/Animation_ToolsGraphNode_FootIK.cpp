#include "Animation_ToolsGraphNode_FootIK.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_UserContext.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_FootIK.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    FootIKToolsNode::FootIKToolsNode()
        : VariationDataToolsNode()
    {
        m_defaultVariationData.CreateInstance( GetVariationDataTypeInfo() );

        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Left Foot Target", GraphValueType::Target );
        CreateInputPin( "Right Foot Target", GraphValueType::Target );
        CreateInputPin( "Enable", GraphValueType::Bool );
    }

    int16_t FootIKToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FootIKNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<FootIKNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pData = GetResolvedVariationDataAs<Data>( context.GetVariationHierarchy(), context.GetVariationID() );
            if ( !pData->m_leftFootBoneID.IsValid() )
            {
                context.LogError( this, "Invalid left foot bone ID!" );
                return InvalidIndex;
            }

            if ( !pData->m_rightFootBoneID.IsValid() )
            {
                context.LogError( this, "Invalid right foot bone ID!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

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

            auto pLeftTargetNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pLeftTargetNode != nullptr )
            {
                int16_t const compiledNodeIdx = pLeftTargetNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_leftTargetNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected left foot target pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            auto pRightTargetNode = GetConnectedInputNode<FlowToolsNode>( 2 );
            if ( pRightTargetNode != nullptr )
            {
                int16_t const compiledNodeIdx = pRightTargetNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_rightTargetNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected right foot target pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            auto pEnableNode = GetConnectedInputNode<FlowToolsNode>( 3 );
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

            //-------------------------------------------------------------------------

            pDefinition->m_leftFootBoneID = pData->m_leftFootBoneID;
            pDefinition->m_rightFootBoneID = pData->m_rightFootBoneID;
            pDefinition->m_isTargetInWorldSpace = m_isTargetInWorldSpace;
            pDefinition->m_blendTime = Math::Max( pData->m_blendTime.ToFloat(), 0.0f);
            pDefinition->m_blendMode = m_blendMode;
        }

        return pDefinition->m_nodeIdx;
    }

    void FootIKToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            int16_t const runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
            auto pRuntimeNode = static_cast<FootIKNode const*>( pGraphNodeContext->GetNodeDebugInstance( runtimeNodeIdx ) );
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
        if ( !pData->m_leftFootBoneID.IsValid() || !pData->m_rightFootBoneID.IsValid() )
        {
            ImGui::TextColored( Colors::Red.ToFloat4(), "Invalid Effector Bone IDs" );
        }
        else
        {
            ImGui::Text( "Effectors - L: %s, R: %s", pData->m_leftFootBoneID.c_str(), pData->m_rightFootBoneID.c_str() );
        }
    }
}