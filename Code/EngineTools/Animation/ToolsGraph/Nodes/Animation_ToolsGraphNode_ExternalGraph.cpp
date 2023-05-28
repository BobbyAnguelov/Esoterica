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
        m_name = GetUniqueSlotName( "External Graph" );
    }

    int16_t ExternalGraphToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ExternalGraphNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ExternalGraphNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            context.RegisterExternalGraphSlotNode( pSettings->m_nodeIdx, StringID( GetName() ) );
        }
        return pSettings->m_nodeIdx;
    }

    void ExternalGraphToolsNode::SetName( String const& newName )
    {
        EE_ASSERT( IsRenameable() );
        m_name = GetUniqueSlotName( newName );
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

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 4 );

        //-------------------------------------------------------------------------

        DrawInternalSeparator( ctx );

        //-------------------------------------------------------------------------

        FlowToolsNode::DrawExtraControls( ctx, pUserContext );
    }

    String ExternalGraphToolsNode::GetUniqueSlotName( String const& desiredName )
    {
        // Ensure that the slot name is unique within the same graph
        int32_t cnt = 0;
        String uniqueName = desiredName;
        auto const externalSlotNodes = GetRootGraph()->FindAllNodesOfType<ExternalGraphToolsNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Exact );
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
        m_name = GetUniqueSlotName( m_name );
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
        m_name = GetUniqueSlotName( m_name );
    }
    #endif
}