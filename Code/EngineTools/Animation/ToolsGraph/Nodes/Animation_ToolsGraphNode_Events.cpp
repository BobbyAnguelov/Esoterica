#include "Animation_ToolsGraphNode_Events.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void IDEventConditionToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Bool, true );
    }

    int16_t IDEventConditionToolsNode::Compile( GraphCompilationContext& context ) const
    {
        IDEventConditionNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<IDEventConditionNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_sourceStateNodeIdx = context.IsCompilingConduit() ? context.GetConduitSourceStateIndex() : InvalidIndex;
            pSettings->m_operator = m_operator;
            pSettings->m_searchRule = m_searchRule;

            pSettings->m_eventIDs.clear();
            pSettings->m_eventIDs.insert( pSettings->m_eventIDs.begin(), m_eventIDs.begin(), m_eventIDs.end() );
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

        if ( m_operator == IDEventConditionNode::Operator::Or )
        {
            infoText = "Any: ";
        }
        else
        {
            infoText = "All: ";
        }

        for ( auto i = 0; i < m_eventIDs.size(); i++ )
        {
            if ( m_eventIDs[i].IsValid() )
            {
                infoText.append( m_eventIDs[i].c_str() );
                if ( i != m_eventIDs.size() - 1 )
                {
                    infoText.append( ", " );
                }
            }
        }

        ImGui::Text( infoText.c_str() );
    }

    //-------------------------------------------------------------------------

    void GenericEventPercentageThroughToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
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

    void FootEventConditionToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
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

    void FootstepEventPercentageThroughToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
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

    void SyncEventIndexConditionToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
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

    void CurrentSyncEventToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
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

    void TransitionEventConditionToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
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