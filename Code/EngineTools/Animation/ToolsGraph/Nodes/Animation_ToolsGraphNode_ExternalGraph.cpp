#include "Animation_ToolsGraphNode_ExternalGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ReferencedGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    ExternalReferencedGraphToolsNode::ExternalReferencedGraphToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Fallback", GraphValueType::Pose );

        Rename( "External Graph" );
    }

    int16_t ExternalReferencedGraphToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ReferencedGraphNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<ReferencedGraphNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            context.RegisterExternalGraphSlotNode( pDefinition->m_nodeIdx, StringID( GetName() ) );

            // Optionally compile fallback node
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledFallbackNodeIdx = pInputNode->Compile( context );
                if ( compiledFallbackNodeIdx != InvalidIndex )
                {
                    pDefinition->m_fallbackNodeIdx = compiledFallbackNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
        }
        return pDefinition->m_nodeIdx;
    }

    void ExternalReferencedGraphToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        DrawInternalSeparator( ctx );

        //-------------------------------------------------------------------------

        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            int16_t runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
            if ( runtimeNodeIdx != InvalidIndex )
            {
                StringID const SlotID( GetName() );
                GraphInstance const* pConnectedGraphInstance = pGraphNodeContext->m_pGraphInstance->GetDebuggableGraphInstance( SlotID );
                if ( pConnectedGraphInstance != nullptr )
                {
                    ImGui::Text( EE_ICON_NEEDLE" %s", pConnectedGraphInstance->GetDefinitionResourceID().c_str() + 7 );
                }
                else
                {
                    ImGui::Text( EE_ICON_NEEDLE_OFF " Empty" );
                }
            }
        }
        else // Simple label when editing
        {
            ImGui::Text( EE_ICON_NEEDLE" External Graph" );
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y );

        //-------------------------------------------------------------------------

        DrawInternalSeparator( ctx );

        //-------------------------------------------------------------------------

        FlowToolsNode::DrawExtraControls( ctx, pUserContext );
    }

    String ExternalReferencedGraphToolsNode::CreateUniqueNodeName( String const& desiredName ) const
    {
        String uniqueName = desiredName;

        if ( HasParentGraph() )
        {
            // Ensure that the slot name is unique within the same graph
            int32_t cnt = 0;
            auto const externalSlotNodes = GetRootGraph()->FindAllNodesOfType<ExternalReferencedGraphToolsNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Exact );
            for ( int32_t i = 0; i < (int32_t) externalSlotNodes.size(); i++ )
            {
                if ( externalSlotNodes[i] == this )
                {
                    continue;
                }

                if ( externalSlotNodes[i]->GetName() == uniqueName )
                {
                    uniqueName = String( String::CtorSprintf(), "%s_%d", desiredName.c_str(), cnt );
                    cnt++;
                    i = -1;
                }
            }
        }

        return uniqueName;
    }

    //-------------------------------------------------------------------------

    IsExternalGraphSlotFilledToolsNode::IsExternalGraphSlotFilledToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
    }

    int16_t IsExternalGraphSlotFilledToolsNode::Compile( GraphCompilationContext &context ) const
    {
        IsExternalGraphSlotFilledNode::Definition *pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<IsExternalGraphSlotFilledNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( !m_slotID.IsValid() )
            {
                context.LogError( "Invalid external slot ID" );
                return false;
            }

            bool externalSlotNodeFound = false;

            auto const externalSlotNodes = GetRootGraph()->FindAllNodesOfType<ExternalReferencedGraphToolsNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Exact );
            for ( int32_t i = 0; i < externalSlotNodes.size(); i++ )
            {
                if ( StringID( externalSlotNodes[i]->GetName() ) == m_slotID )
                {
                    int16_t const compiledNodeIdx = externalSlotNodes[i]->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_externalGraphNodeIdx = compiledNodeIdx;
                    }
                    else
                    {
                        return InvalidIndex;
                    }

                    externalSlotNodeFound = true;
                    break;
                }
            }

            if ( !externalSlotNodeFound )
            {
                context.LogError( "Invalid external slot ID: %s", m_slotID.c_str() );
                return InvalidIndex;
            }
        }

        return pDefinition->m_nodeIdx;
    }

}