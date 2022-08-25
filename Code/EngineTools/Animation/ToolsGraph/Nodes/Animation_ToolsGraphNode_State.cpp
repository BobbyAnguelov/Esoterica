#include "Animation_ToolsGraphNode_State.h"
#include "Animation_ToolsGraphNode_Result.h"
#include "Animation_ToolsGraphNode_StateMachine.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "System/Imgui/ImguiStyle.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    static void DrawStateTypeWindow( VisualGraph::DrawContext const& ctx, Color fontColor, float width, char const* pLabel )
    {
        if ( width <= 0 )
        {
            width = 26;
        }

        ImVec2 const size( width, 20 );

        //-------------------------------------------------------------------------

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 2 );

        ImVec2 const rectMin = ctx.WindowToScreenPosition( ImGui::GetCursorPos() );
        ImVec2 const rectMax = rectMin + size;
        ctx.m_pDrawList->AddRectFilled( rectMin, rectMax, (uint32_t) ImGuiX::Style::s_colorGray6, 3.0f );

        ImGui::SetCursorPos( ImVec2( ImGui::GetCursorPosX() + 2, ImGui::GetCursorPosY() + 2 ) );

        {
            ImGuiX::ScopedFont font( ImGuiX::Font::Small, fontColor );
            ImGui::Text( pLabel );
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 8 );
    }

    //-------------------------------------------------------------------------

    void StateLayerDataToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateInputPin( "Layer Weight", GraphValueType::Float );
        CreateInputPin( "Layer Mask", GraphValueType::BoneMask );
    }

    //-------------------------------------------------------------------------

    void StateToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        VisualGraph::SM::State::Initialize( pParent );

        auto pBlendTree = EE::New<FlowGraph>( GraphType::BlendTree );
        pBlendTree->CreateNode<ResultToolsNode>( GraphValueType::Pose );
        SetChildGraph( pBlendTree );

        auto pValueTree = EE::New<FlowGraph>( GraphType::ValueTree );
        pValueTree->CreateNode<StateLayerDataToolsNode>();
        SetSecondaryGraph( pValueTree );

        // Create a state machine if this is a state machine state
        if ( m_type == StateType::StateMachineState )
        {
            auto pStateMachineNode = pBlendTree->CreateNode<StateMachineToolsNode>();
            
            auto resultNodes = GetChildGraph()->FindAllNodesOfType<ResultToolsNode>();
            EE_ASSERT( resultNodes.size() == 1 );
            auto pBlendTreeResultNode = resultNodes[0];

            pBlendTree->TryMakeConnection( pStateMachineNode, pStateMachineNode->GetOutputPin( 0 ), pBlendTreeResultNode, pBlendTreeResultNode->GetInputPin( 0 ) );
        }
    }

    void StateToolsNode::OnDoubleClick( VisualGraph::UserContext* pUserContext )
    {
        if ( IsBlendTreeState() )
        {
            auto pChildGraph = GetChildGraph();
            if ( pChildGraph != nullptr )
            {
                pUserContext->NavigateTo( pChildGraph );
            }
        }
        else // Skip the blend tree
        {
            auto resultNodes = GetChildGraph()->FindAllNodesOfType<StateMachineToolsNode>();
            EE_ASSERT( resultNodes.size() == 1 );
            auto pStateMachineNode = resultNodes[0];

            auto pChildGraph = pStateMachineNode->GetChildGraph();
            if ( pChildGraph != nullptr )
            {
                pUserContext->NavigateTo( pChildGraph );
            }
        }
    }

    void StateToolsNode::DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos )
    {
        if ( ImGui::BeginMenu( EE_ICON_INFORMATION_OUTLINE" Node Info" ) )
        {
            // UUID
            auto IDStr = GetID().ToString();
            InlineString label = InlineString( InlineString::CtorSprintf(), "UUID: %s", IDStr.c_str() );
            if ( ImGui::MenuItem( label.c_str() ) )
            {
                ImGui::SetClipboardText( IDStr.c_str() );
            }

            // Draw runtime node index
            auto pGraphNodeContext = reinterpret_cast<ToolsGraphUserContext*>( pUserContext );
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

    void StateToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        if ( IsBlendTreeState() )
        {
            DrawStateTypeWindow( ctx, Colors::White, GetWidth(), EE_ICON_FILE_TREE" Blend Tree" );
        }
        else
        {
            DrawStateTypeWindow( ctx, Colors::White, GetWidth(), EE_ICON_STATE_MACHINE" State Machine" );
        }

        ToolsState::DrawExtraControls( ctx, pUserContext );
    }

    ImColor StateToolsNode::GetTitleBarColor() const
    {
        if ( IsBlendTreeState() )
        {
            return ImGuiX::ConvertColor( Colors::DarkSlateBlue );
        }
        else
        {
            return ImGuiX::ConvertColor( Colors::DarkCyan );
        }
    }

    //-------------------------------------------------------------------------

    void OffStateToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        DrawStateTypeWindow( ctx, Colors::Red, GetWidth(), EE_ICON_CLOSE_CIRCLE" Off State" );
        ToolsState::DrawExtraControls( ctx, pUserContext );
    }
}