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

            // IDs
            //-------------------------------------------------------------------------

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

            // Set rules
            //-------------------------------------------------------------------------

            pSettings->m_rules.ClearAllFlags();
            pSettings->m_rules.SetFlag( EventConditionRules::LimitSearchToSourceState, m_limitSearchToSourceState );
            pSettings->m_rules.SetFlag( EventConditionRules::IgnoreInactiveEvents, m_ignoreInactiveBranchEvents );

            switch ( m_operator )
            {
                case EventConditionOperator::Or : 
                {
                    pSettings->m_rules.SetFlag( EventConditionRules::OperatorOr );
                }
                break;

                case EventConditionOperator::And :
                {
                    pSettings->m_rules.SetFlag( EventConditionRules::OperatorAnd );
                }
                break;
            }

            switch ( m_searchRule )
            {
                case SearchRule::OnlySearchAnimEvents:
                {
                    pSettings->m_rules.SetFlag( EventConditionRules::SearchOnlyAnimEvents, true );
                }
                break;

                case SearchRule::OnlySearchStateEvents:
                {
                    pSettings->m_rules.SetFlag( EventConditionRules::SearchOnlyStateEvents, true );
                }
                break;

                case SearchRule::SearchAll:
                {
                    pSettings->m_rules.SetFlag( EventConditionRules::SearchBothStateAndAnimEvents, true );
                }
                break;
            }
        }
        return pSettings->m_nodeIdx;
    }

    void IDEventConditionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        switch ( m_searchRule )
        {
            case SearchRule::OnlySearchStateEvents:
            {
                ImGui::Text( "Search: State Events" );
            }
            break;

            case SearchRule::OnlySearchAnimEvents:
            {
                ImGui::Text( "Search: Anim Events" );
            }
            break;

            case SearchRule::SearchAll:
            {
                ImGui::Text( "Search: All Events" );
            }
            break;
        }

        //-------------------------------------------------------------------------

        if ( m_operator == EventConditionOperator::Or )
        {
            ImGui::Text( "Any of These: " );
        }
        else
        {
            ImGui::Text( "All of these: " );
        }

        for ( auto i = 0; i < m_eventIDs.size(); i++ )
        {
            if ( m_eventIDs[i].IsValid() )
            {
                ImGui::BulletText( m_eventIDs[i].c_str() );
            }
            else
            {
                ImGui::BulletText( "INVALID ID" );
            }
        }

        //-------------------------------------------------------------------------

        if ( m_limitSearchToSourceState )
        {
            ImGui::Text( "Only checks source state" );
        }

        if ( m_ignoreInactiveBranchEvents )
        {
            ImGui::Text( "Inactive events ignored" );
        }
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
        for ( auto const ID : m_eventIDs )
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
            for ( auto& ID : m_eventIDs )
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

        m_conditions.emplace_back();
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

            // Conditions
            //-------------------------------------------------------------------------

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

            // Set rules
            //-------------------------------------------------------------------------

            pSettings->m_rules.ClearAllFlags();
            pSettings->m_rules.SetFlag( EventConditionRules::LimitSearchToSourceState, m_limitSearchToSourceState );
            pSettings->m_rules.SetFlag( EventConditionRules::IgnoreInactiveEvents, m_ignoreInactiveBranchEvents );

            switch ( m_operator )
            {
                case EventConditionOperator::Or:
                {
                    pSettings->m_rules.SetFlag( EventConditionRules::OperatorOr );
                }
                break;

                case EventConditionOperator::And:
                {
                    pSettings->m_rules.SetFlag( EventConditionRules::OperatorAnd );
                }
                break;
            }
        }
        return pSettings->m_nodeIdx;
    }

    void StateEventConditionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        if ( m_operator == EventConditionOperator::Or )
        {
            ImGui::Text( "Any of these:" );
        }
        else
        {
            ImGui::Text( "All of these:" );
        }

        constexpr static char const* const typeLabels[] = { "Entry", "FullyInState","Exit", "Timed", "All" };

        for ( auto i = 0; i < m_conditions.size(); i++ )
        {
            if ( m_conditions[i].m_eventID.IsValid() )
            {
                ImGui::BulletText( "[%s] %s", typeLabels[(uint8_t) m_conditions[i].m_type], m_conditions[i].m_eventID.c_str() );
            }
        }

        //-------------------------------------------------------------------------

        if ( m_limitSearchToSourceState )
        {
            ImGui::Text( "Only checks source state" );
        }

        if ( m_ignoreInactiveBranchEvents )
        {
            ImGui::Text( "Inactive events ignored" );
        }
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
        for ( auto const& condition : m_conditions )
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
            for ( auto& condition : m_conditions )
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
            pSettings->m_eventID = m_eventID;

            // Set rules
            //-------------------------------------------------------------------------

            pSettings->m_rules.ClearAllFlags();
            pSettings->m_rules.SetFlag( EventConditionRules::LimitSearchToSourceState, m_limitSearchToSourceState );
            pSettings->m_rules.SetFlag( EventConditionRules::IgnoreInactiveEvents, m_ignoreInactiveBranchEvents );

            switch ( m_priorityRule )
            {
                case EventPriorityRule::HighestWeight:
                {
                    pSettings->m_rules.SetFlag( EventConditionRules::PreferHighestWeight );
                }
                break;

                case EventPriorityRule::HighestPercentageThrough:
                {
                    pSettings->m_rules.SetFlag( EventConditionRules::PreferHighestProgress );
                }
                break;
            }
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
            ImGui::TextColored( Colors::Red.ToFloat4(), "Event ID: Invalid" );
        }

        if ( m_limitSearchToSourceState )
        {
            ImGui::Text( "Only checks source state" );
        }

        if ( m_ignoreInactiveBranchEvents )
        {
            ImGui::Text( "Inactive events ignored" );
        }

        if ( m_priorityRule == EventPriorityRule::HighestPercentageThrough )
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

            // Set rules
            //-------------------------------------------------------------------------

            pSettings->m_rules.ClearAllFlags();
            pSettings->m_rules.SetFlag( EventConditionRules::LimitSearchToSourceState, m_limitSearchToSourceState );
            pSettings->m_rules.SetFlag( EventConditionRules::IgnoreInactiveEvents, m_ignoreInactiveBranchEvents );
        }
        return pSettings->m_nodeIdx;
    }

    void FootEventConditionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( FootEvent::GetPhaseConditionName( m_phaseCondition ) );

        if ( m_limitSearchToSourceState )
        {
            ImGui::Text( "Only checks source state" );
        }

        if ( m_ignoreInactiveBranchEvents )
        {
            ImGui::Text( "Inactive events ignored" );
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

            // Set rules
            //-------------------------------------------------------------------------

            pSettings->m_rules.ClearAllFlags();
            pSettings->m_rules.SetFlag( EventConditionRules::LimitSearchToSourceState, m_limitSearchToSourceState );
            pSettings->m_rules.SetFlag( EventConditionRules::IgnoreInactiveEvents, m_ignoreInactiveBranchEvents );

            switch ( m_priorityRule )
            {
                case EventPriorityRule::HighestWeight:
                {
                    pSettings->m_rules.SetFlag( EventConditionRules::PreferHighestWeight );
                }
                break;

                case EventPriorityRule::HighestPercentageThrough:
                {
                    pSettings->m_rules.SetFlag( EventConditionRules::PreferHighestProgress );
                }
                break;
            }
        }
        return pSettings->m_nodeIdx;
    }

    void FootstepEventPercentageThroughToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( FootEvent::GetPhaseConditionName( m_phaseCondition ) );

        if ( m_limitSearchToSourceState )
        {
            ImGui::Text( "Only checks source state" );
        }

        if ( m_ignoreInactiveBranchEvents )
        {
            ImGui::Text( "Inactive events ignored" );
        }

        if ( m_priorityRule == EventPriorityRule::HighestPercentageThrough )
        {
            ImGui::Text( "Prefer Highest Event %" );
        }
        else
        {
            ImGui::Text( "Prefer Highest Event Weight" );
        }
    }

    //-------------------------------------------------------------------------

    FootstepEventIDToolsNode::FootstepEventIDToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "ID", GraphValueType::ID, true );
    }

    int16_t FootstepEventIDToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FootstepEventIDNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FootstepEventIDNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_sourceStateNodeIdx = context.IsCompilingConduit() ? context.GetConduitSourceStateIndex() : InvalidIndex;

            // Set rules
            //-------------------------------------------------------------------------

            pSettings->m_rules.ClearAllFlags();
            pSettings->m_rules.SetFlag( EventConditionRules::LimitSearchToSourceState, m_limitSearchToSourceState );
            pSettings->m_rules.SetFlag( EventConditionRules::IgnoreInactiveEvents, m_ignoreInactiveBranchEvents );

            switch ( m_priorityRule )
            {
                case EventPriorityRule::HighestWeight:
                {
                    pSettings->m_rules.SetFlag( EventConditionRules::PreferHighestWeight );
                }
                break;

                case EventPriorityRule::HighestPercentageThrough:
                {
                    pSettings->m_rules.SetFlag( EventConditionRules::PreferHighestProgress );
                }
                break;
            }
        }
        return pSettings->m_nodeIdx;
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

            EE_ASSERT( context.IsCompilingConduit() );

            pSettings->m_sourceStateNodeIdx = context.GetConduitSourceStateIndex();
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

    CurrentSyncEventIDToolsNode::CurrentSyncEventIDToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::ID, true );
    }

    int16_t CurrentSyncEventIDToolsNode::Compile( GraphCompilationContext& context ) const
    {
        CurrentSyncEventIDNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<CurrentSyncEventIDNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            EE_ASSERT( context.IsCompilingConduit() );
            pSettings->m_sourceStateNodeIdx = context.GetConduitSourceStateIndex();
        }
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    CurrentSyncEventIndexToolsNode::CurrentSyncEventIndexToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
    }

    int16_t CurrentSyncEventIndexToolsNode::Compile( GraphCompilationContext& context ) const
    {
        CurrentSyncEventIndexNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<CurrentSyncEventIndexNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            EE_ASSERT( context.IsCompilingConduit() );

            pSettings->m_sourceStateNodeIdx = context.GetConduitSourceStateIndex();
        }
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    CurrentSyncEventPercentageThroughToolsNode::CurrentSyncEventPercentageThroughToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
    }

    int16_t CurrentSyncEventPercentageThroughToolsNode::Compile( GraphCompilationContext& context ) const
    {
        CurrentSyncEventPercentageThroughNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<CurrentSyncEventPercentageThroughNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            EE_ASSERT( context.IsCompilingConduit() );

            pSettings->m_sourceStateNodeIdx = context.GetConduitSourceStateIndex();
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

            // Set rules
            //-------------------------------------------------------------------------

            pSettings->m_rules.ClearAllFlags();
            pSettings->m_rules.SetFlag( EventConditionRules::LimitSearchToSourceState, m_limitSearchToSourceState );
            pSettings->m_rules.SetFlag( EventConditionRules::IgnoreInactiveEvents, m_ignoreInactiveBranchEvents );
        }
        return pSettings->m_nodeIdx;
    }

    void TransitionEventConditionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( GetTransitionMarkerConditionName( m_markerCondition ) );

        if ( m_limitSearchToSourceState )
        {
            ImGui::Text( "Only checks source state" );
        }

        if ( m_ignoreInactiveBranchEvents )
        {
            ImGui::Text( "Inactive events ignored" );
        }
    }
}