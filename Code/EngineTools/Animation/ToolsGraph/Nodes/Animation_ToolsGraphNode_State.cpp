#include "Animation_ToolsGraphNode_State.h"
#include "Animation_ToolsGraphNode_Result.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void StateLayerDataToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateInputPin( "Layer Weight", GraphValueType::Float );
        CreateInputPin( "Layer Mask", GraphValueType::BoneMask );
    }

    //-------------------------------------------------------------------------

    void BlendTreeStateToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        VisualGraph::SM::State::Initialize( pParent );

        auto pBlendTree = EE::New<FlowGraph>( GraphType::BlendTree );
        pBlendTree->CreateNode<ResultToolsNode>( GraphValueType::Pose );
        SetChildGraph( pBlendTree );

        auto pValueTree = EE::New<FlowGraph>( GraphType::ValueTree );
        pValueTree->CreateNode<StateLayerDataToolsNode>();
        SetSecondaryGraph( pValueTree );
    }

    ResultToolsNode const* BlendTreeStateToolsNode::GetBlendTreeRootNode() const
    {
        auto resultNodes = GetChildGraph()->FindAllNodesOfType<ResultToolsNode>();
        EE_ASSERT( resultNodes.size() == 1 );
        return resultNodes[0];
    }

    StateLayerDataToolsNode const* BlendTreeStateToolsNode::GetLayerDataNode() const
    {
        auto dataNodes = GetSecondaryGraph()->FindAllNodesOfType<StateLayerDataToolsNode>();
        EE_ASSERT( dataNodes.size() == 1 );
        return dataNodes[0];
    }

    void BlendTreeStateToolsNode::DrawExtraContextMenuOptions( VisualGraph::DrawContext const& ctx, Float2 const& mouseCanvasPos )
    {
        ImGui::Separator();

        if ( ImGui::MenuItem( "Make Default Entry State" ) )
        {
            auto pParentStateMachineGraph = Cast<VisualGraph::StateMachineGraph>( GetParentGraph() );
            pParentStateMachineGraph->SetDefaultEntryState( GetID() );
        }

        if ( ImGui::BeginMenu( "Node Info" ) )
        {
            // UUID
            auto IDStr = GetID().ToString();
            InlineString label = InlineString( InlineString::CtorSprintf(), "UUID: %s", IDStr.c_str() );
            if ( ImGui::MenuItem( label.c_str() ) )
            {
                ImGui::SetClipboardText( IDStr.c_str() );
            }

            // Draw runtime node index
            auto pGraphNodeContext = reinterpret_cast<ToolsNodeContext*>( ctx.m_pUserContext );
            if ( pGraphNodeContext->HasDebugData() )
            {
                int16_t runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
                if ( runtimeNodeIdx != InvalidIndex )
                {
                    label = InlineString( InlineString::CtorSprintf(), "Runtime Index: %d", runtimeNodeIdx );
                    if ( ImGui::MenuItem( label.c_str() ) )
                    {
                        InlineString const value( InlineString::CtorSprintf(), "%d", runtimeNodeIdx );
                        ImGui::SetClipboardText( value.c_str() );
                    }
                }
            }

            ImGui::EndMenu();
        }
    }

    //-------------------------------------------------------------------------

    void OffStateToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx )
    {
        {
            ImGuiX::ScopedFont font( ImGuiX::Font::Large, Colors::Red );
            ImGui::Text( EE_ICON_CLOSE_CIRCLE "OFF" );
        }

        ToolsState::DrawExtraControls( ctx );
    }
}