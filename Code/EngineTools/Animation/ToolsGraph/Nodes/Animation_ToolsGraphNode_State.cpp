#include "Animation_ToolsGraphNode_State.h"
#include "Animation_ToolsGraphNode_Result.h"
#include "Animation_ToolsGraphNode_StateMachine.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    StateLayerDataToolsNode::StateLayerDataToolsNode()
        : ResultToolsNode()
    {
        CreateInputPin( "Layer Weight", GraphValueType::Float );
        CreateInputPin( "Root Motion Weight", GraphValueType::Float );
        CreateInputPin( "Layer Mask", GraphValueType::BoneMask );
    }

    //-------------------------------------------------------------------------

    StateToolsNode::StateToolsNode()
        : NodeGraph::StateNode()
        , m_type( StateType::BlendTreeState )
    {
        auto pBlendTree = CreateChildGraph<FlowGraph>( GraphType::BlendTree );
        pBlendTree->CreateNode<PoseResultToolsNode>();

        auto pValueTree = CreateSecondaryGraph<FlowGraph>( GraphType::ValueTree );
        pValueTree->CreateNode<StateLayerDataToolsNode>();
    }

    StateToolsNode::StateToolsNode( StateType type )
        : StateToolsNode::StateToolsNode()
    {
        m_type = type;

        // Create a state machine if this is a state machine state
        if ( m_type == StateType::StateMachineState )
        {
            auto pBlendTree = GetChildGraphAs<FlowGraph>();
            auto pStateMachineNode = pBlendTree->CreateNode<StateMachineToolsNode>();

            auto resultNodes = pBlendTree->FindAllNodesOfType<ResultToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
            EE_ASSERT( resultNodes.size() == 1 );
            auto pBlendTreeResultNode = resultNodes[0];

            pBlendTree->TryMakeConnection( pStateMachineNode, pStateMachineNode->GetOutputPin( 0 ), pBlendTreeResultNode, pBlendTreeResultNode->GetInputPin( 0 ) );
        }
        // Change the name for off states
        else if ( m_type == StateType::OffState )
        {
            m_name = "Off";
        }
    }

    Color StateToolsNode::GetTitleBarColor() const
    {
        switch ( m_type )
        {
            case StateType::OffState:
            {
                return Colors::DarkRed;
            }
            break;

            case StateType::BlendTreeState:
            {
                return Colors::MediumSlateBlue;
            }
            break;

            case StateType::StateMachineState:
            {
                return Colors::DarkCyan;
            }
            break;
        }

        return NodeGraph::StateNode::GetTitleBarColor();
    }

    NodeGraph::BaseGraph* StateToolsNode::GetNavigationTarget()
    {
        NodeGraph::BaseGraph* pTargetGraph = GetChildGraph();

        if ( IsStateMachineState() )
        {
            auto stateMachineNodes = GetChildGraph()->FindAllNodesOfType<StateMachineToolsNode>();
            if ( stateMachineNodes.size() == 1 )
            {
                pTargetGraph = stateMachineNodes[0]->GetChildGraph();
            }
        }

        return pTargetGraph;
    }

    void StateToolsNode::DrawContextMenuOptions( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos )
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

    void StateToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        float const scaledVerticalGap = 6.0f * ctx.m_viewScaleFactor;
        float const scaledExtraSpacing = 2.0f * ctx.m_viewScaleFactor;
        float const scaledMinimumStateWidth = s_minimumStateNodeUnscaledWidth * ctx.m_viewScaleFactor;
        float const scaledMinimumStateHeight = s_minimumStateNodeUnscaledHeight * ctx.m_viewScaleFactor;
        float const scaledRounding = 3.0f * ctx.m_viewScaleFactor;

        //-------------------------------------------------------------------------

        auto DrawStateTypeWindow = [&] ( NodeGraph::DrawContext const& ctx, Color iconColor, Color fontColor, float width, char const* pIcon, char const* pLabel )
        {
            BeginDrawInternalRegion( ctx );

            {
                ImGuiX::ScopedFont font( ImGuiX::Font::Small, iconColor );
                ImGui::Text( pIcon );
            }

            {
                ImGuiX::ScopedFont font( ImGuiX::Font::Small, fontColor );
                ImGui::SameLine();
                ImGui::Text( pLabel );
            }

            EndDrawInternalRegion( ctx );
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
        auto CreateEventString = [&] ( TVector<StringID> const& stateIDs, TVector<StringID> const& specificIDs )
        {
            TInlineVector<StringID, 10> finalIDs;
            finalIDs.insert( finalIDs.end(), stateIDs.begin(), stateIDs.end() );

            for ( StringID specificID : specificIDs )
            {
                if ( !VectorContains( finalIDs, specificID ) )
                {
                    finalIDs.emplace_back( specificID );
                }
            }

            //-------------------------------------------------------------------------

            string.clear();
            for ( int32_t i = 0; i < (int32_t) finalIDs.size(); i++ )
            {
                if ( !finalIDs[i].IsValid() )
                {
                    continue;
                }

                string += finalIDs[i].c_str();

                if ( i != finalIDs.size() - 1 )
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
        {
            ImGuiX::ScopedFont const eventFont( ImGuiX::Font::Medium );

            if ( !m_entryEvents.empty() || !m_events.empty() )
            {
                CreateEventString( m_events, m_entryEvents );
                {
                    ImGuiX::ScopedFont const label( ImGuiX::Font::MediumBold, Colors::Green );
                    ImGui::Text( "Entry:", string.c_str() );
                }
                ImGui::SameLine();
                ImGui::Text( "%s", string.c_str() );
                hasStateEvents = true;
            }

            if ( !m_executeEvents.empty() || !m_events.empty() )
            {
                CreateEventString( m_events, m_executeEvents );
                {
                    ImGuiX::ScopedFont const label( ImGuiX::Font::MediumBold, Colors::Gold );
                    ImGui::Text( "Execute:", string.c_str() );
                }
                ImGui::SameLine();
                ImGui::Text( "%s", string.c_str() );
                hasStateEvents = true;
            }

            if ( !m_exitEvents.empty() || !m_events.empty() )
            {
                CreateEventString( m_events, m_exitEvents );
                {
                    ImGuiX::ScopedFont const label( ImGuiX::Font::MediumBold, Colors::Tomato );
                    ImGui::Text( "Exit:", string.c_str() );
                }
                ImGui::SameLine();
                ImGui::Text( "%s", string.c_str() );
                hasStateEvents = true;
            }

            if ( !m_timeRemainingEvents.empty() )
            {
                CreateTimedEventString( m_timeRemainingEvents );
                {
                    ImGuiX::ScopedFont const label( ImGuiX::Font::MediumBold );
                    ImGui::Text( "Time Left: %s", string.c_str() );
                }
                hasStateEvents = true;
            }

            if ( !m_timeElapsedEvents.empty() )
            {
                CreateTimedEventString( m_timeElapsedEvents );
                {
                    ImGuiX::ScopedFont const label( ImGuiX::Font::MediumBold );
                    ImGui::Text( "Time Elapsed: %s", string.c_str() );
                }
                hasStateEvents = true;
            }

            if ( !hasStateEvents )
            {
                ImGui::Text( "No State Events" );
            }
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + scaledVerticalGap );

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
                DrawPoseNodeDebugInfo( ctx, GetWidth(), &debugInfo );
                shouldDrawEmptyDebugInfoBlock = false;
            }

            //-------------------------------------------------------------------------

            if ( pGraphNodeContext->m_showRuntimeIndices && runtimeNodeIdx != InvalidIndex )
            {
                DrawRuntimeNodeIndex( ctx, pGraphNodeContext, this, runtimeNodeIdx );
            }
        }

        if ( shouldDrawEmptyDebugInfoBlock )
        {
            DrawPoseNodeDebugInfo( ctx, GetWidth(), nullptr );
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + scaledVerticalGap );
    }

    bool StateToolsNode::IsActive( NodeGraph::UserContext* pUserContext ) const
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

        NodeGraph::ScopedNodeModification const snm( this );
        m_type = StateType::BlendTreeState;
    }

    bool StateToolsNode::CanConvertToStateMachineState()
    {
        EE_ASSERT( m_type == StateType::BlendTreeState );

        TInlineVector<BaseNode const*, 30> const childNodes = GetChildGraph()->GetNodes();
        if ( childNodes.size() != 2 )
        {
            return false;
        }

        auto resultNodes = GetChildGraph()->FindAllNodesOfType<ResultToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
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

        NodeGraph::ScopedNodeModification const snm( this );
        m_type = StateType::StateMachineState;
    }

    void StateToolsNode::OnShowNode()
    {
        NodeGraph::StateNode::OnShowNode();

        // State machine states, never show the state machine node so we need to call that function here, to ensure that any code needing to be run is!
        if ( m_type == StateType::StateMachineState )
        {
            auto childSMs = GetChildGraph()->FindAllNodesOfType<StateMachineToolsNode>();
            EE_ASSERT( childSMs.size() == 1 );
            childSMs[0]->OnShowNode();
        }
    }

    void StateToolsNode::GetLogicAndEventIDs( TVector<StringID>& outIDs ) const
    {
        for ( auto const ID : m_events )
        {
            outIDs.emplace_back( ID );
        }

        for ( auto const ID : m_entryEvents )
        {
            outIDs.emplace_back( ID );
        }

        for ( auto const ID : m_executeEvents )
        {
            outIDs.emplace_back( ID );
        }

        for ( auto const ID : m_exitEvents )
        {
            outIDs.emplace_back( ID );
        }

        for ( auto const& evt : m_timeRemainingEvents )
        {
            outIDs.emplace_back( evt.m_ID );
        }

        for ( auto const& evt : m_timeElapsedEvents )
        {
            outIDs.emplace_back( evt.m_ID );
        }
    }

    void StateToolsNode::RenameLogicAndEventIDs( StringID oldID, StringID newID )
    {
        bool foundMatch = false;

        //-------------------------------------------------------------------------

        TVector<StringID> IDs;
        GetLogicAndEventIDs( IDs );
        for ( auto const ID : IDs )
        {
            if ( ID == oldID )
            {
                foundMatch = true;
                break;
            }
        }

        //-------------------------------------------------------------------------

        if ( foundMatch )
        {
            NodeGraph::ScopedNodeModification snm( this );

            for ( auto& ID : m_events )
            {
                if ( ID == oldID )
                {
                    ID = newID;
                }
            }

            for ( auto& ID : m_entryEvents )
            {
                if ( ID == oldID )
                {
                    ID = newID;
                }
            }

            for ( auto& ID : m_executeEvents )
            {
                if ( ID == oldID )
                {
                    ID = newID;
                }
            }

            for ( auto& ID : m_exitEvents )
            {
                if ( ID == oldID )
                {
                    ID = newID;
                }
            }

            for ( auto& evt : m_timeRemainingEvents )
            {
                if ( evt.m_ID == oldID )
                {
                    evt.m_ID = newID;
                }
            }

            for ( auto& evt : m_timeElapsedEvents )
            {
                if ( evt.m_ID == oldID )
                {
                    evt.m_ID = newID;
                }
            }
        }
    }
}