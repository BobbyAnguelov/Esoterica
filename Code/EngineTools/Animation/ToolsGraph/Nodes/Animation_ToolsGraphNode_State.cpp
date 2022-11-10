#include "Animation_ToolsGraphNode_State.h"
#include "Animation_ToolsGraphNode_Result.h"
#include "Animation_ToolsGraphNode_StateMachine.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "System/Imgui/ImguiStyle.h"
#include "System/Imgui/ImguiX.h"

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

    ImColor StateToolsNode::GetTitleBarColor() const
    {
        switch ( m_type )
        {
            case StateType::OffState:
            {
                return ImGuiX::ConvertColor( Colors::DarkRed );
            }
            break;

            case StateType::BlendTreeState:
            {
                return ImGuiX::ConvertColor( Colors::DarkSlateBlue );
            }
            break;

            case StateType::StateMachineState:
            {
                return ImGuiX::ConvertColor( Colors::DarkCyan );
            }
            break;
        }

        return VisualGraph::SM::State::GetTitleBarColor();
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
            if ( resultNodes.size() == 0 )
            {
                // Off-state so do nothing!
            }
            else
            {
                EE_ASSERT( resultNodes.size() == 1 );
                auto pStateMachineNode = resultNodes[0];

                auto pChildGraph = pStateMachineNode->GetChildGraph();
                if ( pChildGraph != nullptr )
                {
                    pUserContext->NavigateTo( pChildGraph );
                }
            }
        }
    }

    void StateToolsNode::DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos )
    {
        if ( m_type == StateType::BlendTreeState && CanConvertToStateMachineState() )
        {
            if ( ImGui::MenuItem( EE_ICON_STATE_MACHINE" Convert To State Machine State" ) )
            {
                ConvertToStateMachineState();
            }
        }

        if ( m_type == StateType::StateMachineState && CanConvertToBlendTreeState() )
        {
            if ( ImGui::MenuItem( EE_ICON_FILE_TREE" Convert to Blend Tree State" ) )
            {
                ConvertToBlendTreeState();
            }
        }

        //-------------------------------------------------------------------------

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
            auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
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
        auto DrawStateTypeWindow = [] ( VisualGraph::DrawContext const& ctx, Color iconColor, Color fontColor, float width, char const* pIcon, char const* pLabel )
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
                ImGuiX::ScopedFont font( ImGuiX::Font::Small, iconColor );
                ImGui::Text( pIcon );
            }

            {
                ImGuiX::ScopedFont font( ImGuiX::Font::Small, fontColor );
                ImGui::SameLine();
                ImGui::Text( pLabel );
            }

            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 8 );
        };

        //-------------------------------------------------------------------------

        switch ( m_type )
        {
            case StateType::OffState:
            {
                DrawStateTypeWindow( ctx, Colors::Red, Colors::White, GetWidth(), EE_ICON_CLOSE_CIRCLE, " Off State" );
            }
            break;

            case StateType::BlendTreeState:
            {
                DrawStateTypeWindow( ctx, Colors::MediumPurple, Colors::White, GetWidth(), EE_ICON_FILE_TREE, " Blend Tree" );
            }
            break;

            case StateType::StateMachineState:
            {
                DrawStateTypeWindow( ctx, Colors::Turquoise, Colors::White, GetWidth(), EE_ICON_STATE_MACHINE, " State Machine" );
            }
            break;
        }

        // State events
        //-------------------------------------------------------------------------

        InlineString string;
        auto CreateEventString = [&] ( TVector<StringID> const& IDs )
        {
            string.clear();
            for ( int32_t i = 0; i < (int32_t) IDs.size(); i++ )
            {
                if ( !IDs[i].IsValid() )
                {
                    continue;
                }

                string += IDs[i].c_str();

                if ( i != IDs.size() - 1 )
                {
                    string += ", ";
                }
            }
        };

        auto CreateTimedEventString = [&] ( TVector<TimedStateEvent> const& events )
        {
            string.clear();
            for ( int32_t i = 0; i < (int32_t) events.size(); i++ )
            {
                if ( !events[i].m_ID.IsValid() )
                {
                    continue;
                }

                InlineString const eventStr( InlineString::CtorSprintf(), "%s (%.2fs)", events[i].m_ID.c_str(), events[i].m_timeValue.ToFloat() );
                string += eventStr.c_str();

                if ( i != events.size() - 1 )
                {
                    string += ", ";
                }
            }
        };

        bool hasStateEvents = false;

        if ( !m_entryEvents.empty() )
        {
            CreateEventString( m_entryEvents );
            ImGui::Text( "Entry: %s", string.c_str() );
            hasStateEvents = true;
        }

        if ( !m_executeEvents.empty() )
        {
            CreateEventString( m_executeEvents );
            ImGui::Text( "Execute: %s", string.c_str() );
            hasStateEvents = true;
        }

        if ( !m_exitEvents.empty() )
        {
            CreateEventString( m_exitEvents );
            ImGui::Text( "Exit: %s", string.c_str() );
            hasStateEvents = true;
        }

        if ( !m_timeRemainingEvents.empty() )
        {
            CreateTimedEventString( m_timeRemainingEvents );
            ImGui::Text( "Time Left: %s", string.c_str() );
            hasStateEvents = true;
        }

        if ( !m_timeElapsedEvents.empty() )
        {
            CreateTimedEventString( m_timeElapsedEvents );
            ImGui::Text( "Time Elapsed: %s", string.c_str() );
            hasStateEvents = true;
        }

        if ( !hasStateEvents )
        {
            ImGui::Text( "No State Events" );
        }

        // Draw separator
        //-------------------------------------------------------------------------

        DrawInternalSeparator( ctx );

        // Draw runtime debug info
        //-------------------------------------------------------------------------

        bool shouldDrawEmptyDebugInfoBlock = true;
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            int16_t runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
            if ( runtimeNodeIdx != InvalidIndex && pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) )
            {
                PoseNodeDebugInfo const debugInfo = pGraphNodeContext->GetPoseNodeDebugInfo( runtimeNodeIdx );
                DrawPoseNodeDebugInfo( ctx, GetWidth(), debugInfo );
                shouldDrawEmptyDebugInfoBlock = false;
            }
        }

        if ( shouldDrawEmptyDebugInfoBlock )
        {
            DrawEmptyPoseNodeDebugInfo( ctx, GetWidth() );
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 4 );
    }

    bool StateToolsNode::IsActive( VisualGraph::UserContext* pUserContext ) const
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            // Some nodes dont have runtime representations
            auto const runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
            if ( runtimeNodeIdx != InvalidIndex )
            {
                return pGraphNodeContext->IsNodeActive( runtimeNodeIdx );
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    bool StateToolsNode::CanConvertToBlendTreeState()
    {
        EE_ASSERT( m_type == StateType::StateMachineState );

        return true;
    }

    void StateToolsNode::ConvertToBlendTreeState()
    {
        EE_ASSERT( m_type == StateType::StateMachineState );

        VisualGraph::ScopedNodeModification const snm( this );
        m_type = StateType::BlendTreeState;
    }

    bool StateToolsNode::CanConvertToStateMachineState()
    {
        EE_ASSERT( m_type == StateType::BlendTreeState );

        auto const& childNodes = GetChildGraph()->GetNodes();
        if ( childNodes.size() != 2 )
        {
            return false;
        }

        auto resultNodes = GetChildGraph()->FindAllNodesOfType<ResultToolsNode>();
        if ( resultNodes.size() != 1 )
        {
            return false;
        }

        auto pInputNode = resultNodes[0]->GetConnectedInputNode( 0 );
        if ( pInputNode == nullptr || !IsOfType<StateMachineToolsNode>( pInputNode ) )
        {
            return false;
        }

        return true;
    }

    void StateToolsNode::ConvertToStateMachineState()
    {
        EE_ASSERT( m_type == StateType::BlendTreeState );
        EE_ASSERT( CanConvertToStateMachineState() );

        VisualGraph::ScopedNodeModification const snm( this );
        m_type = StateType::StateMachineState;
    }
}