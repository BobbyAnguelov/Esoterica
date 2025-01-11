#include "Animation_ToolsGraphNode_ReferencedGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ReferencedGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Variations.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    ReferencedGraphToolsNode::ReferencedGraphToolsNode()
        :VariationDataToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Fallback", GraphValueType::Pose );

        m_defaultVariationData.CreateInstance( GetVariationDataTypeInfo() );
    }

    int16_t ReferencedGraphToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ReferencedGraphNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<ReferencedGraphNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pData = GetResolvedVariationDataAs<ReferencedGraphToolsNode::Data>( context.GetVariationHierarchy(), context.GetVariationID() );
            pDefinition->m_referencedGraphIdx = context.RegisterReferencedGraphNode( pDefinition->m_nodeIdx, GetID(), pData->m_graphDefinition.GetResourceID() );

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

    void ReferencedGraphToolsNode::DrawContextMenuOptions( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, NodeGraph::Pin* pPin )
    {
        VariationDataToolsNode::DrawContextMenuOptions( ctx, pUserContext, mouseCanvasPos, pPin );

        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );

        auto pVariationData = GetResolvedVariationDataAs<Data>( *pGraphNodeContext->m_pVariationHierarchy, pGraphNodeContext->m_selectedVariationID );
        ResourceID const resourceID = pVariationData->m_graphDefinition.GetResourceID();

        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( !resourceID.IsValid() );

        if ( ImGui::MenuItem( EE_ICON_PENCIL_BOX" Edit Child Graph" ) )
        {
            OpenReferencedGraphCommand cmd( this, OpenReferencedGraphCommand::OpenInNewEditor );
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

	ResourceID ReferencedGraphToolsNode::GetReferencedGraphResourceID( VariationHierarchy const& variationHierarchy, StringID variationID ) const
	{
        auto pData = GetResolvedVariationDataAs<Data>( variationHierarchy, variationID );
        return pData->m_graphDefinition.GetResourceID();
	}
}