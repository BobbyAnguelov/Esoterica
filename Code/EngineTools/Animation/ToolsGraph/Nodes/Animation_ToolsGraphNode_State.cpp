#include "Animation_ToolsGraphNode_State.h"
#include "Animation_ToolsGraphNode_Result.h"
#include "Animation_ToolsGraphNode_StateMachine.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    StateLayerDataToolsNode::StateLayerDataToolsNode()
        : FlowToolsNode()
    {
        CreateInputPin( "Layer Weight", GraphValueType::Float );
        CreateInputPin( "Root Motion Weight", GraphValueType::Float );
        CreateInputPin( "Layer Mask", GraphValueType::BoneMask );
    }

    //-------------------------------------------------------------------------

    StateToolsNode::StateToolsNode( StateType type )
        : m_type( type )
    {
        if ( m_type == StateType::OffState )
        {
            m_name = "Off";
        }
    }

    void StateToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        VisualGraph::SM::State::Initialize( pParent );

        auto pBlendTree = EE::New<FlowGraph>( GraphType::BlendTree );
        pBlendTree->CreateNode<PoseResultToolsNode>();
        SetChildGraph( pBlendTree );

        auto pValueTree = EE::New<FlowGraph>( GraphType::ValueTree );
        pValueTree->CreateNode<StateLayerDataToolsNode>();
        SetSecondaryGraph( pValueTree );

        // Create a state machine if this is a state machine state
        if ( m_type == StateType::StateMachineState )
        {
            auto pStateMachineNode = pBlendTree->CreateNode<StateMachineToolsNode>();
            
            auto resultNodes = GetChildGraph()->FindAllNodesOfType<ResultToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
            EE_ASSERT( resultNodes.size() == 1 );
            auto pBlendTreeResultNode = resultNodes[0];

            pBlendTree->TryMakeConnection( pStateMachineNode, pStateMachineNode->GetOutputPin( 0 ), pBlendTreeResultNode, pBlendTreeResultNode->GetInputPin( 0 ) );
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
            case StateType::StateMachineState:
            {
                return Colors::DarkCyan;
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
        float const scaledVerticalGap = 6.0f * ctx.m_viewScaleFactor;
        float const scaledExtraSpacing = 2.0f * ctx.m_viewScaleFactor;
        float const scaledMinimumStateWidth = s_minimumStateNodeUnscaledWidth * ctx.m_viewScaleFactor;
        float const scaledMinimumStateHeight = s_minimumStateNodeUnscaledHeight * ctx.m_viewScaleFactor;
        float const scaledRounding = 3.0f * ctx.m_viewScaleFactor;

        //-------------------------------------------------------------------------

        auto DrawStateTypeWindow = [&] ( VisualGraph::DrawContext const& ctx, Color iconColor, Color fontColor, float width, char const* pIcon, char const* pLabel )
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
                    ImGuiX::ScopedFont const label( ImGuiX::Font::MediumBold, Colors::Lime );
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
                ImGui::SameLine();
                ImGui::Text( "%s", string.c_str() );
                hasStateEvents = true;
            }

            if ( !m_timeElapsedEvents.empty() )
            {
                CreateTimedEventString( m_timeElapsedEvents );
                {
                    ImGuiX::ScopedFont const label( ImGuiX::Font::MediumBold );
                    ImGui::Text( "Time Elapsed: %s", string.c_str() );
                }
                ImGui::SameLine();
                ImGui::Text( "%s", string.c_str() );
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

        auto resultNodes = GetChildGraph()->FindAllNodesOfType<ResultToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
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

    void StateToolsNode::OnShowNode()
    {
        VisualGraph::SM::State::OnShowNode();

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
            VisualGraph::ScopedNodeModification snm( this );

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