#include "Animation_ToolsGraphNode_State.h"
#include "Animation_ToolsGraphNode_Result.h"
#include "Animation_ToolsGraphNode_StateMachine.h"
#include "Animation_ToolsGraphNode_Parameters.h"
#include "EngineTools/NodeGraph/NodeGraph_Style.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Serialization/TypeSerialization.h"

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

    void StateToolsNode::UpdateAllClonedStates( TypeSystem::TypeRegistry const& typeRegistry, NodeGraph::BaseGraph *pGraph, bool isPartOfBulkUpdate )
    {
        TVector<UUID> updatedStates;

        int32_t lowestDepth = 0;

        auto FilterStateNodes = [&lowestDepth] ( NodeGraph::BaseNode const *pNode )
        {
            auto pStateNode = Cast<StateToolsNode>( pNode );
            if ( !pStateNode->IsClonedState() )
            {
                return false;
            }

            int32_t const pathDepth = pStateNode->GetPathDepthFromRoot();
            if ( pathDepth < lowestDepth )
            {
                return false;
            }

            return true;
        };

        // We need to progressively update all the cloned node from root to leaf, as cloning a node at one level might create new nodes on a level below it.
        TInlineVector<StateToolsNode*, 20> foundStateNodes = pGraph->FindAllNodesOfTypeAdvanced<StateToolsNode>( FilterStateNodes, NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );

        while ( !foundStateNodes.empty() )
        {
            int32_t lowestFoundDepth = INT32_MAX;
            for ( StateToolsNode *pStateNode : foundStateNodes )
            {
                int32_t const pathDepth = pStateNode->GetPathDepthFromRoot();
                lowestFoundDepth = Math::Min( lowestFoundDepth, pathDepth );
            }

            for ( StateToolsNode *pStateNode : foundStateNodes )
            {
                int32_t const pathDepth = pStateNode->GetPathDepthFromRoot();
                if ( pathDepth == lowestFoundDepth )
                {
                    pStateNode->UpdateClonedData( typeRegistry, true );
                }
            }

            lowestDepth = lowestFoundDepth + 1;
            foundStateNodes = pGraph->FindAllNodesOfTypeAdvanced<StateToolsNode>( FilterStateNodes, NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );
        }

        // Fix up any parameter references 
        //-------------------------------------------------------------------------

        if ( !isPartOfBulkUpdate )
        {
            ParameterBaseToolsNode::RefreshParameterReferences( Cast<Animation::FlowGraph>( pGraph->GetRootGraph() ) );
        }
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

    StateToolsNode::StateToolsNode( UUID const &sourceStateID )
        : NodeGraph::StateNode()
        , m_type( StateType::Clone )
        , m_cloneSourceStateID( sourceStateID )
    {
        EE_ASSERT( m_cloneSourceStateID.IsValid() );
        CreateChildGraph<FlowGraph>( GraphType::BlendTree );
        CreateSecondaryGraph<FlowGraph>( GraphType::ValueTree );
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

            case StateType::Clone:
            {
                return Colors::MediumVioletRed;
            }
            break;
        }

        return NodeGraph::StateNode::GetTitleBarColor();
    }

    NodeGraph::BaseGraph* StateToolsNode::GetNavigationTarget( NodeGraph::UserContext* pUserContext )
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
        else if ( IsClonedState() )
        {
            auto pToolsGraphContext = static_cast<ToolsGraphUserContext*>( pUserContext );

            if ( !pUserContext->IsReadOnly() )
            {
                UpdateClonedData( *pToolsGraphContext->m_pTypeRegistry );
            }
        }

        return pTargetGraph;
    }

    void StateToolsNode::DrawContextMenuOptions( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos )
    {
        if ( !ctx.m_isReadOnly )
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

            if ( !IsClonedState() )
            {
                if ( ImGui::MenuItem( EE_ICON_CONTENT_DUPLICATE" Clone State" ) )
                {
                    CreateClone( pUserContext );
                }
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

            case StateType::Clone:
            {
                DrawStateTypeWindow( ctx, Colors::MediumVioletRed, Colors::White, GetWidth(), EE_ICON_CONTENT_DUPLICATE, " Cloned State" );
            }
            break;
        }

        // State events
        //-------------------------------------------------------------------------

        InlineString string;
        auto CreateEventString = [&] ( TInlineVector<StringID, 5> const& IDs )
        {
            string.clear();

            for ( int32_t i = 0; i < (int32_t) IDs.size(); i++ )
            {
                if ( !IDs[i].IsValid() )
                {
                    continue;
                }

                if ( !string.empty() )
                {
                    string += ", ";
                }

                string += IDs[i].c_str();
            }
        };

        auto CreateTimedEventString = [&] ( TInlineVector<TimedStateEvent, 5> const& events )
        {
            string.clear();

            for ( int32_t i = 0; i < (int32_t) events.size(); i++ )
            {
                if ( !events[i].IsValid() )
                {
                    continue;
                }

                if ( !string.empty() )
                {
                    string += ", ";
                }

                string.append_sprintf( "%s (%.2fs)", events[i].m_ID.c_str(), events[i].m_timeValue.ToFloat() );
            }
        };

        bool hasStateEvents = false;
        {
            ImGuiX::ScopedFont const eventFont( ImGuiX::Font::Medium );

            TInlineVector<StringID, 5> const uniqueEntryEvents = GetUniqueEntryEvents();
            if ( !uniqueEntryEvents.empty() )
            {
                CreateEventString( uniqueEntryEvents );
                {
                    ImGuiX::ScopedFont const label( ImGuiX::Font::MediumBold, Colors::Green );
                    ImGui::Text( "Entry:", string.c_str() );
                }
                ImGui::SameLine();
                ImGui::Text( "%s", string.c_str() );
                hasStateEvents = true;
            }

            TInlineVector<StringID, 5> const uniqueExecuteEvents = GetUniqueFullyInStateEvents();
            if ( !uniqueExecuteEvents.empty() )
            {
                CreateEventString( uniqueExecuteEvents );
                {
                    ImGuiX::ScopedFont const label( ImGuiX::Font::MediumBold, Colors::Gold );
                    ImGui::Text( "Execute:", string.c_str() );
                }
                ImGui::SameLine();
                ImGui::Text( "%s", string.c_str() );
                hasStateEvents = true;
            }

            TInlineVector<StringID, 5> const uniqueExitEvents = GetUniqueExitEvents();
            if ( !uniqueExitEvents.empty() )
            {
                CreateEventString( uniqueExitEvents );
                {
                    ImGuiX::ScopedFont const label( ImGuiX::Font::MediumBold, Colors::Tomato );
                    ImGui::Text( "Exit:", string.c_str() );
                }
                ImGui::SameLine();
                ImGui::Text( "%s", string.c_str() );
                hasStateEvents = true;
            }

            TInlineVector<TimedStateEvent, 5> const uniqueTimeRemainingEvents = GetUniqueTimeRemainingEvents();
            if ( !uniqueTimeRemainingEvents.empty() )
            {
                CreateTimedEventString( uniqueTimeRemainingEvents );
                {
                    ImGuiX::ScopedFont const label( ImGuiX::Font::MediumBold );
                    ImGui::Text( "Time Left: %s", string.c_str() );
                }
                hasStateEvents = true;
            }

            TInlineVector<TimedStateEvent, 5> const uniqueTimeElapsedEvents = GetUniqueTimeElapsedEvents();
            if ( !uniqueTimeElapsedEvents.empty() )
            {
                CreateTimedEventString( uniqueTimeElapsedEvents );
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

    Color StateToolsNode::GetHighlightOutlineColor( NodeGraph::UserContext* pUserContext ) const
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            // Some nodes dont have runtime representations
            auto const runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
            if ( runtimeNodeIdx != InvalidIndex )
            {
                auto pDebugNode = pGraphNodeContext->GetNodeDebugInstance( runtimeNodeIdx );
                if ( pDebugNode->IsInitialized() && !pDebugNode->IsValid() )
                {
                    return NodeGraph::Style::s_invalidBorderColor;
                }
                else if( pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) )
                {
                    return NodeGraph::Style::s_activeBorderColor;
                }
            }
        }

        return Colors::Transparent;
    }

    UUID StateToolsNode::RegenerateIDs( THashMap<UUID, UUID>& IDMapping )
    {
        if ( !IsClonedState() )
        {
            UpdateCloneVersion();
        }

        return NodeGraph::StateNode::RegenerateIDs( IDMapping );
    }

    void StateToolsNode::PostModify()
    {
        if ( !IsClonedState() )
        {
            UpdateCloneVersion();
        }

        NodeGraph::StateNode::PostModify();
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

    void StateToolsNode::UpdateCloneVersion()
    {
        EE_ASSERT( !IsClonedState() );
        m_cloneVersion = UUID::GenerateID();
    }

    void StateToolsNode::UpdateCloneStateID( TypeSystem::TypeRegistry const& typeRegistry, THashMap<UUID, UUID> const& IDMapping )
    {
        m_cloneSourceStateID = IDMapping.at( m_cloneSourceStateID );
        UpdateClonedData( typeRegistry );
    }

    void StateToolsNode::UpdateClonedData( TypeSystem::TypeRegistry const& typeRegistry, bool isPartOfBulkUpdate )
    {
        EE_ASSERT( IsClonedState() && m_cloneSourceStateID.IsValid() );

        auto pSourceStateNode = Cast<StateToolsNode>( GetParentGraph()->FindNode( m_cloneSourceStateID ) );
        EE_ASSERT( pSourceStateNode != nullptr );

        // Early out if the versions match
        if ( pSourceStateNode->m_cloneVersion == m_cloneVersion )
        {
            return;
        }

        pSourceStateNode->PreCopy();

        // Copy Child Graph
        //-------------------------------------------------------------------------

        String serializedGraphStr;
        Serialization::WriteTypeToString( typeRegistry, pSourceStateNode->GetChildGraph(), serializedGraphStr );
        Log log;
        bool result = Serialization::ReadTypeFromString( typeRegistry, log, serializedGraphStr, GetChildGraph() );
        log.ReflectToSystemLogAndClear( LogCategory::Animation, "Serialize Clone State - Child Graph" );
        EE_ASSERT( result );

        // Copy Secondary Graph
        //-------------------------------------------------------------------------

        Serialization::WriteTypeToString( typeRegistry, pSourceStateNode->GetSecondaryGraph(), serializedGraphStr );
        result = Serialization::ReadTypeFromString( typeRegistry, log, serializedGraphStr, GetSecondaryGraph() );
        log.ReflectToSystemLogAndClear( LogCategory::Animation, "Serialize Clone State - Secondary Graph" );
        EE_ASSERT( result );

        // Fix parents
        //-------------------------------------------------------------------------

        UpdateChildGraphAndSecondaryGraphParents();

        // Fix all IDs
        //-------------------------------------------------------------------------

        THashMap<UUID, UUID> IDMapping;
        GetChildGraph()->RegenerateIDs( IDMapping );

        IDMapping.clear();
        GetSecondaryGraph()->RegenerateIDs( IDMapping );

        // Copy events
        //-------------------------------------------------------------------------

        m_stateEvents = pSourceStateNode->m_stateEvents;
        m_timedEvents = pSourceStateNode->m_timedEvents;

        //-------------------------------------------------------------------------

        UpdateAllClonedStates( typeRegistry, GetChildGraph(), isPartOfBulkUpdate );
    }

    StateToolsNode const *StateToolsNode::GetCloneStateSourceNode() const
    {
        EE_ASSERT( IsClonedState() );
        return Cast<StateToolsNode>( GetParentGraph()->FindNode( m_cloneSourceStateID ) );
    }

    char const* StateToolsNode::GetClonedStateName() const
    {
        EE_ASSERT( IsClonedState() );
        auto pSourceStateNode = Cast<StateToolsNode>( GetParentGraph()->FindNode( m_cloneSourceStateID ) );
        return pSourceStateNode->GetName();
    }

    void StateToolsNode::CreateClone( NodeGraph::UserContext* pUserContext )
    {
        auto pToolsGraphContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        auto pParentGraph = GetParentGraph();

        NodeGraph::ScopedGraphModification sgm( pParentGraph );
        auto pClonedStateNode = pParentGraph->CreateNode<StateToolsNode>( GetID() );
        pClonedStateNode->Rename( GetName() );
        pClonedStateNode->SetPosition( GetPosition() + Float2( 100, 100 ) );
        pClonedStateNode->UpdateClonedData( *pToolsGraphContext->m_pTypeRegistry );
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
        for ( auto const& evt : m_stateEvents )
        {
            if ( evt.m_ID.IsValid() )
            {
                outIDs.emplace_back( evt.m_ID );
            }
        }

        for ( auto const& evt : m_timedEvents )
        {
            if ( evt.m_ID.IsValid() )
            {
                outIDs.emplace_back( evt.m_ID );
            }
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

            for ( auto& evt : m_stateEvents )
            {
                if ( evt.m_ID == oldID )
                {
                    evt.m_ID = newID;
                }
            }

            for ( auto& evt : m_timedEvents )
            {
                if ( evt.m_ID == oldID )
                {
                    evt.m_ID = newID;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    TInlineVector<StringID, 5> StateToolsNode::GetUniqueEntryEvents() const
    {
        TInlineVector<StringID, 5> evts;

        for ( auto& evt : m_stateEvents )
        {
            if ( evt.m_isEntry )
            {
                VectorEmplaceBackUnique( evts, evt.m_ID );
            }
        }

        return evts;
    }

    TInlineVector<StringID, 5> StateToolsNode::GetUniqueFullyInStateEvents() const
    {
        TInlineVector<StringID, 5> evts;

        for ( auto& evt : m_stateEvents )
        {
            if ( evt.m_isFullyInState )
            {
                VectorEmplaceBackUnique( evts, evt.m_ID );
            }
        }

        return evts;
    }

    TInlineVector<StringID, 5> StateToolsNode::GetUniqueExitEvents() const
    {
        TInlineVector<StringID, 5> evts;

        for ( auto& evt : m_stateEvents )
        {
            if ( evt.m_isExit )
            {
                VectorEmplaceBackUnique( evts, evt.m_ID );
            }
        }

        return evts;
    }

    TInlineVector<StateToolsNode::TimedStateEvent, 5> StateToolsNode::GetUniqueTimeRemainingEvents() const
    {
        TInlineVector<TimedStateEvent, 5> evts;

        for ( TimedStateEvent const& evt : m_timedEvents )
        {
            if ( evt.IsTimeRemainingEvent() )
            {
                VectorEmplaceBackUnique( evts, evt );
            }
        }

        return evts;
    }

    TInlineVector<StateToolsNode::TimedStateEvent, 5> StateToolsNode::GetUniqueTimeElapsedEvents() const
    {
        TInlineVector<TimedStateEvent, 5> evts;

        for ( TimedStateEvent const& evt : m_timedEvents )
        {
            if ( evt.IsTimeElapsedEvent() )
            {
                VectorEmplaceBackUnique( evts, evt );
            }
        }

        return evts;
    }
}