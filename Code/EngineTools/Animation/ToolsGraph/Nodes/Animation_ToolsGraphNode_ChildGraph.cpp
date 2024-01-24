#include "Animation_ToolsGraphNode_ChildGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ChildGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Variations.h"
#include "EngineTools/Resource/ResourceDatabase.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    ChildGraphToolsNode::ChildGraphToolsNode()
        :DataSlotToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Pose, true );
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

    void ChildGraphToolsNode::DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, VisualGraph::Flow::Pin* pPin )
    {
        DataSlotToolsNode::DrawContextMenuOptions( ctx, pUserContext, mouseCanvasPos, pPin );

        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        ResourceID const resourceID = GetResourceID( *pGraphNodeContext->m_pVariationHierarchy, pGraphNodeContext->m_selectedVariationID );

        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( !resourceID.IsValid() );

        if ( ImGui::MenuItem( EE_ICON_PENCIL_BOX" Edit Child Graph" ) )
        {
            TSharedPtr<VisualGraph::AdvancedCommand> command = eastl::make_shared<OpenChildGraphCommand>( this, OpenChildGraphCommand::OpenInNewEditor );
            pUserContext->RequestAdvancedCommand( command );
        }

        if ( ImGui::MenuItem( EE_ICON_ARROW_RIGHT_BOLD" Reflect Parent Parameters to Child" ) )
        {
            TSharedPtr<VisualGraph::AdvancedCommand> command = eastl::make_shared<ReflectParametersCommand>( this, ReflectParametersCommand::FromParent );
            pUserContext->RequestAdvancedCommand( command );
        }

        if ( ImGui::MenuItem( EE_ICON_ARROW_LEFT_BOLD" Reflect Child Parameters to Parent" ) )
        {
            TSharedPtr<VisualGraph::AdvancedCommand> command = eastl::make_shared<ReflectParametersCommand>( this, ReflectParametersCommand::FromChild );
            pUserContext->RequestAdvancedCommand( command );
        }

        ImGui::EndDisabled();
    }

    void ChildGraphToolsNode::OnDoubleClick( VisualGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        ResourceID const resourceID = GetResourceID( *pGraphNodeContext->m_pVariationHierarchy, pGraphNodeContext->m_selectedVariationID );
        if ( resourceID.IsValid() )
        {
            TSharedPtr<VisualGraph::AdvancedCommand> command = eastl::make_shared<OpenChildGraphCommand>( this, pUserContext->m_isCtrlDown ? OpenChildGraphCommand::OpenInNewEditor : OpenChildGraphCommand::OpenInPlace );
            pUserContext->RequestAdvancedCommand( command );
        }
    }
}