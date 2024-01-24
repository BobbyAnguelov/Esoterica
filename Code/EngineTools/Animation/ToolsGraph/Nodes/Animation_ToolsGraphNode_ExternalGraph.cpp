#include "Animation_ToolsGraphNode_ExternalGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ExternalGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    ExternalGraphToolsNode::ExternalGraphToolsNode()
        : FlowToolsNode()
        , m_name( "External Graph" )
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
    }

    void ExternalGraphToolsNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        FlowToolsNode::Initialize( pParentGraph );

        EE_ASSERT( pParentGraph != nullptr );
        m_name = GetUniqueSlotName( pParentGraph->GetRootGraph(), "External Graph" );
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

    void ExternalGraphToolsNode::SetName( String const& newName )
    {
        EE_ASSERT( IsRenameable() );
        if ( GetRootGraph() != nullptr )
        {
            m_name = GetUniqueSlotName( GetRootGraph(), newName );
        }
        else
        {
            m_name = newName;
        }
    }

    void ExternalGraphToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
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

    String ExternalGraphToolsNode::GetUniqueSlotName( VisualGraph::BaseGraph* pRootGraph, String const& desiredName )
    {
        EE_ASSERT( pRootGraph != nullptr && pRootGraph->IsRootGraph() );

        // Ensure that the slot name is unique within the same graph
        int32_t cnt = 0;
        String uniqueName = desiredName;
        auto const externalSlotNodes = pRootGraph->FindAllNodesOfType<ExternalGraphToolsNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Exact );
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
                i = 0;
            }
        }

        return uniqueName;
    }

    void ExternalGraphToolsNode::PostPaste()
    {
        FlowToolsNode::PostPaste();
        m_name = GetUniqueSlotName( GetRootGraph(), m_name );
    }

    void ExternalGraphToolsNode::OnDoubleClick( VisualGraph::UserContext* pUserContext )
    {
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
                    pGraphNodeContext->RequestOpenResource( pConnectedGraphInstance->GetDefinitionResourceID() );
                }
            }
        }
    }

    #if EE_DEVELOPMENT_TOOLS
    void ExternalGraphToolsNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        FlowToolsNode::PostPropertyEdit( pPropertyEdited );
        m_name = GetUniqueSlotName( GetRootGraph(), m_name );
    }
    #endif
}