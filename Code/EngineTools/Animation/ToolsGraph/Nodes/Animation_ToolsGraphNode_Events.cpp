#include "Animation_ToolsGraphNode_Events.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    IDEventConditionToolsNode::IDEventConditionToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
    }

    int16_t IDEventConditionToolsNode::Compile( GraphCompilationContext& context ) const
    {
        IDEventConditionNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<IDEventConditionNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( m_eventIDs.empty() )
            {
                context.LogError( this, "No event conditions specified for condition node!" );
                return InvalidIndex;
            }

            pSettings->m_sourceStateNodeIdx = context.IsCompilingConduit() ? context.GetConduitSourceStateIndex() : InvalidIndex;
            pSettings->m_operator = m_operator;
            pSettings->m_searchRule = m_searchRule;

            pSettings->m_eventIDs.clear();
            for ( auto const& ID : m_eventIDs )
            {
                if ( ID.IsValid() )
                {
                    pSettings->m_eventIDs.emplace_back( ID );
                }
                else
                {
                    context.LogError( this, "Invalid ID detected!" );
                    return InvalidIndex;
                }
            }
        }
        return pSettings->m_nodeIdx;
    }

    void IDEventConditionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        switch ( m_searchRule )
        {
            case IDEventConditionNode::SearchRule::OnlySearchStateEvents:
            {
                ImGui::Text( "Search: State Events" );
            }
            break;

            case IDEventConditionNode::SearchRule::OnlySearchAnimEvents:
            {
                ImGui::Text( "Search: Anim Events" );
            }
            break;

            case IDEventConditionNode::SearchRule::SearchAll:
            {
                ImGui::Text( "Search: All Events" );
            }
            break;
        }

        //-------------------------------------------------------------------------

        InlineString infoText;

        if ( m_operator == EventConditionOperator::Or )
        {
            infoText = "Any of These: \n";
        }
        else
        {
            infoText = "All of these: \n";
        }

        for ( auto i = 0; i < m_eventIDs.size(); i++ )
        {
            if ( m_eventIDs[i].IsValid() )
            {
                infoText.append( m_eventIDs[i].c_str() );
                if ( i != m_eventIDs.size() - 1 )
                {
                    infoText.append( "\n" );
                }
            }
        }

        ImGui::Text( infoText.c_str() );
    }

    void IDEventConditionToolsNode::GetLogicAndEventIDs( TVector<StringID>& outIDs ) const
    {
        for ( auto ID : m_eventIDs )
        {
            outIDs.emplace_back( ID );
        }
    }

    void IDEventConditionToolsNode::RenameLogicAndEventIDs( StringID oldID, StringID newID )
    {
        bool foundMatch = false;
        for ( auto ID : m_eventIDs )
        {
            if ( ID == oldID )
            {
                foundMatch = true;
                break;
            }
        }

        if ( foundMatch )
        {
            VisualGraph::ScopedNodeModification snm( this );
            for ( auto ID : m_eventIDs )
            {
                if ( ID == oldID )
                {
                    ID = newID;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    StateEventConditionToolsNode::StateEventConditionToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
    }

    int16_t StateEventConditionToolsNode::Compile( GraphCompilationContext& context ) const
    {
        StateEventConditionNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<StateEventConditionNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( m_conditions.empty() )
            {
                context.LogError( this, "No event conditions specified for condition node!" );
                return InvalidIndex;
            }

            pSettings->m_sourceStateNodeIdx = context.IsCompilingConduit() ? context.GetConduitSourceStateIndex() : InvalidIndex;
            pSettings->m_operator = m_operator;

            pSettings->m_conditions.clear();
            for ( auto const& condition : m_conditions )
            {
                if ( condition.m_eventID.IsValid() )
                {
                    StateEventConditionNode::Condition& runtimeCondition = pSettings->m_conditions.emplace_back();
                    runtimeCondition.m_eventID = condition.m_eventID;
                    runtimeCondition.m_eventTypeCondition = condition.m_type;
                }
                else
                {
                    context.LogError( this, "Invalid ID detected!" );
                    return InvalidIndex;
                }
            }

        }
        return pSettings->m_nodeIdx;
    }

    void StateEventConditionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        InlineString infoText;

        if ( m_operator == EventConditionOperator::Or )
        {
            infoText = "Any of these: \n";
        }
        else
        {
            infoText = "All of these: \n";
        }

        constexpr static char const* const typeLabels[] = { "Entry", "FullyInState","Exit", "Timed", "All" };

        for ( auto i = 0; i < m_conditions.size(); i++ )
        {
            if ( m_conditions[i].m_eventID.IsValid() )
            {
                infoText.append_sprintf( "[%s] %s", typeLabels[(uint8_t) m_conditions[i].m_type], m_conditions[i].m_eventID.c_str() );
                if ( i != m_conditions.size() - 1 )
                {
                    infoText.append( "\n" );
                }
            }
        }

        ImGui::Text( infoText.c_str() );
    }

    void StateEventConditionToolsNode::GetLogicAndEventIDs( TVector<StringID>& outIDs ) const
    {
        for ( auto const& condition : m_conditions )
        {
            outIDs.emplace_back( condition.m_eventID );
        }
    }

    void StateEventConditionToolsNode::RenameLogicAndEventIDs( StringID oldID, StringID newID )
    {
        bool foundMatch = false;
        for ( auto condition : m_conditions )
        {
            if ( condition.m_eventID == oldID )
            {
                foundMatch = true;
                break;
            }
        }

        if ( foundMatch )
        {
            VisualGraph::ScopedNodeModification snm( this );
            for ( auto condition : m_conditions )
            {
                if ( condition.m_eventID == oldID )
                {
                    condition.m_eventID = newID;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    GenericEventPercentageThroughToolsNode::GenericEventPercentageThroughToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
    }

    int16_t GenericEventPercentageThroughToolsNode::Compile( GraphCompilationContext& context ) const
    {
        GenericEventPercentageThroughNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<GenericEventPercentageThroughNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_sourceStateNodeIdx = context.IsCompilingConduit() ? context.GetConduitSourceStateIndex() : InvalidIndex;
            pSettings->m_onlyCheckEventsFromActiveBranch = m_onlyCheckEventsFromActiveBranch;
            pSettings->m_priorityRule = m_priorityRule;
            pSettings->m_eventID = m_eventID;
        }
        return pSettings->m_nodeIdx;
    }

    void GenericEventPercentageThroughToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        if ( m_eventID.IsValid() )
        {
            ImGui::Text( "Event ID: %s", m_eventID.c_str());
        }
        else
        {
            ImGui::TextColored( ImColor( 0xFF0000FF ), "Event ID: Invalid" );
        }

        if ( m_onlyCheckEventsFromActiveBranch )
        {
            ImGui::Text( "Prefer Highest Event %" );
        }
        else
        {
            ImGui::Text( "Prefer Highest Event Weight" );
        }
    }

    //-------------------------------------------------------------------------

    FootEventConditionToolsNode::FootEventConditionToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
    }

    int16_t FootEventConditionToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FootEventConditionNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FootEventConditionNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_sourceStateNodeIdx = context.IsCompilingConduit() ? context.GetConduitSourceStateIndex() : InvalidIndex;
            pSettings->m_phaseCondition = m_phaseCondition;
            pSettings->m_onlyCheckEventsFromActiveBranch = m_onlyCheckEventsFromActiveBranch;
        }
        return pSettings->m_nodeIdx;
    }

    void FootEventConditionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( FootEvent::GetPhaseConditionName( m_phaseCondition ) );

        if ( m_onlyCheckEventsFromActiveBranch )
        {
            ImGui::Text( "Prefer Highest Event %" );
        }
        else
        {
            ImGui::Text( "Prefer Highest Event Weight" );
        }
    }

    //-------------------------------------------------------------------------

    FootstepEventPercentageThroughToolsNode::FootstepEventPercentageThroughToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
    }

    int16_t FootstepEventPercentageThroughToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FootstepEventPercentageThroughNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FootstepEventPercentageThroughNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_sourceStateNodeIdx = context.IsCompilingConduit() ? context.GetConduitSourceStateIndex() : InvalidIndex;
            pSettings->m_phaseCondition = m_phaseCondition;
            pSettings->m_priorityRule = m_priorityRule;
            pSettings->m_onlyCheckEventsFromActiveBranch = m_onlyCheckEventsFromActiveBranch;
        }
        return pSettings->m_nodeIdx;
    }

    void FootstepEventPercentageThroughToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( FootEvent::GetPhaseConditionName( m_phaseCondition ) );

        if ( m_onlyCheckEventsFromActiveBranch )
        {
            ImGui::Text( "Prefer Highest Event %" );
        }
        else
        {
            ImGui::Text( "Prefer Highest Event Weight" );
        }
    }

    //-------------------------------------------------------------------------

    SyncEventIndexConditionToolsNode::SyncEventIndexConditionToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
    }

    int16_t SyncEventIndexConditionToolsNode::Compile( GraphCompilationContext& context ) const
    {
        SyncEventIndexConditionNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<SyncEventIndexConditionNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( m_syncEventIdx == InvalidIndex )
            {
                context.LogError( this, "Invalid sync event index" );
                return InvalidIndex;
            }

            pSettings->m_sourceStateNodeIdx = context.IsCompilingConduit() ? context.GetConduitSourceStateIndex() : InvalidIndex;
            pSettings->m_triggerMode = m_triggerMode;
            pSettings->m_syncEventIdx = m_syncEventIdx;
        }
        return pSettings->m_nodeIdx;
    }

    void SyncEventIndexConditionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        if ( m_triggerMode == SyncEventIndexConditionNode::TriggerMode::ExactlyAtEventIndex )
        {
            ImGui::Text( "Current index == %d", m_syncEventIdx );
        }
        else
        {
            ImGui::Text( "Current index >= %d", m_syncEventIdx );
        }
    }

    //-------------------------------------------------------------------------

    CurrentSyncEventToolsNode::CurrentSyncEventToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
    }

    int16_t CurrentSyncEventToolsNode::Compile( GraphCompilationContext& context ) const
    {
        CurrentSyncEventNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<CurrentSyncEventNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_sourceStateNodeIdx = context.IsCompilingConduit() ? context.GetConduitSourceStateIndex() : InvalidIndex;
        }
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    TransitionEventConditionToolsNode::TransitionEventConditionToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
    }

    int16_t TransitionEventConditionToolsNode::Compile( GraphCompilationContext& context ) const
    {
        TransitionEventConditionNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<TransitionEventConditionNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_sourceStateNodeIdx = context.IsCompilingConduit() ? context.GetConduitSourceStateIndex() : InvalidIndex;
            pSettings->m_markerCondition = m_markerCondition;
            pSettings->m_onlyCheckEventsFromActiveBranch = m_onlyCheckEventsFromActiveBranch;

            if ( m_matchOnlySpecificMarkerID )
            {
                if ( m_markerIDToMatch.IsValid() )
                {
                    pSettings->m_markerIDToMatch = m_markerIDToMatch;
                }
                else
                {
                    context.LogError( this, "Invalid event ID to match!" );
                    return InvalidIndex;
                }
            }
            else
            {
                pSettings->m_markerIDToMatch = StringID();
            }
        }
        return pSettings->m_nodeIdx;
    }

    void TransitionEventConditionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( GetTransitionMarkerConditionName( m_markerCondition ) );

        if ( m_onlyCheckEventsFromActiveBranch )
        {
            ImGui::Text( "Prefer Highest Event %" );
        }
        else
        {
            ImGui::Text( "Prefer Highest Event Weight" );
        }
    }
}