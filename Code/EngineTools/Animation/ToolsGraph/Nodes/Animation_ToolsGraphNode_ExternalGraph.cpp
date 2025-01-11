#include "Animation_ToolsGraphNode_ExternalGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ExternalGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    ExternalGraphToolsNode::ExternalGraphToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        Rename( "External Graph" );
    }

    int16_t ExternalGraphToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ExternalGraphNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<ExternalGraphNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            context.RegisterExternalGraphSlotNode( pDefinition->m_nodeIdx, StringID( GetName() ) );
        }
        return pDefinition->m_nodeIdx;
    }

    void ExternalGraphToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
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
                GraphInstance const* pConnectedGraphInstance = pGraphNodeContext->m_pGraphInstance->GetExternalGraphDebugInstance( SlotID );
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

    String ExternalGraphToolsNode::CreateUniqueNodeName( String const& desiredName ) const
    {
        String uniqueName = desiredName;

        if ( HasParentGraph() )
        {
            // Ensure that the slot name is unique within the same graph
            int32_t cnt = 0;
            auto const externalSlotNodes = GetRootGraph()->FindAllNodesOfType<ExternalGraphToolsNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Exact );
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
}