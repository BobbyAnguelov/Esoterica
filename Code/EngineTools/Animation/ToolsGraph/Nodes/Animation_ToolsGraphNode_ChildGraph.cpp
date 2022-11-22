#include "Animation_ToolsGraphNode_ChildGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ChildGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ChildGraphToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        DataSlotToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Pose, true );
    }

    int16_t ChildGraphToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ChildGraphNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ChildGraphNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_childGraphIdx = context.RegisterChildGraphNode( pSettings->m_nodeIdx, GetID() );
        }
        return pSettings->m_nodeIdx;
    }

    ResourceTypeID ChildGraphToolsNode::GetSlotResourceTypeID() const
    {
        return GraphVariation::GetStaticResourceTypeID();
    }

    void ChildGraphToolsNode::DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, VisualGraph::Flow::Pin* pPin )
    {
        DataSlotToolsNode::DrawContextMenuOptions( ctx, pUserContext, mouseCanvasPos, pPin );

        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        ResourceID const resourceID = GetResourceID( *pGraphNodeContext->m_pVariationHierarchy, pGraphNodeContext->m_selectedVariationID );

        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( !resourceID.IsValid() );
        if ( ImGui::MenuItem( EE_ICON_PENCIL_BOX" Edit Child Graph" ) )
        {
            pGraphNodeContext->OpenChildGraph( this, resourceID, true );
        }
        ImGui::EndDisabled();
    }

    void ChildGraphToolsNode::OnDoubleClick( VisualGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        ResourceID const resourceID = GetResourceID( *pGraphNodeContext->m_pVariationHierarchy, pGraphNodeContext->m_selectedVariationID );
        if ( resourceID.IsValid() )
        {
            pGraphNodeContext->OpenChildGraph( this, resourceID, pUserContext->m_isCtrlDown );
        }
    }
}