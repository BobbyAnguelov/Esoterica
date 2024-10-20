#include "Animation_ToolsGraphNode_ChildGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ChildGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Variations.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    ChildGraphToolsNode::ChildGraphToolsNode()
        :DataSlotToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Pose );
    }

    int16_t ChildGraphToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ChildGraphNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<ChildGraphNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pDefinition->m_childGraphIdx = context.RegisterChildGraphNode( pDefinition->m_nodeIdx, GetID() );
        }
        return pDefinition->m_nodeIdx;
    }

    ResourceTypeID ChildGraphToolsNode::GetSlotResourceTypeID() const
    {
        return GraphVariation::GetStaticResourceTypeID();
    }

    void ChildGraphToolsNode::DrawContextMenuOptions( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, NodeGraph::Pin* pPin )
    {
        DataSlotToolsNode::DrawContextMenuOptions( ctx, pUserContext, mouseCanvasPos, pPin );

        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        ResourceID const resourceID = GetResolvedResourceID( *pGraphNodeContext->m_pVariationHierarchy, pGraphNodeContext->m_selectedVariationID );

        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( !resourceID.IsValid() );

        if ( ImGui::MenuItem( EE_ICON_PENCIL_BOX" Edit Child Graph" ) )
        {
            OpenChildGraphCommand cmd( this, OpenChildGraphCommand::OpenInNewEditor );
            pUserContext->RequestCustomCommand( &cmd );
        }

        if ( ImGui::MenuItem( EE_ICON_ARROW_RIGHT_BOLD" Reflect Parent Parameters to Child" ) )
        {
            ReflectParametersCommand cmd( this, ReflectParametersCommand::FromParent );
            pUserContext->RequestCustomCommand( &cmd );
        }

        if ( ImGui::MenuItem( EE_ICON_ARROW_LEFT_BOLD" Reflect Child Parameters to Parent" ) )
        {
            ReflectParametersCommand cmd( this, ReflectParametersCommand::FromChild );
            pUserContext->RequestCustomCommand( &cmd );
        }

        ImGui::EndDisabled();
    }
}