#include "Animation_ToolsGraphNode_Events.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void GenericEventConditionToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Bool, true );
    }

    int16_t GenericEventConditionToolsNode::Compile( GraphCompilationContext& context ) const
    {
        GenericEventConditionNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<GenericEventConditionNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_sourceStateNodeIdx = context.IsCompilingConduit() ? context.GetConduitSourceStateIndex() : InvalidIndex;
            pSettings->m_operator = m_operator;
            pSettings->m_searchMode = m_searchMode;

            pSettings->m_eventIDs.clear();
            pSettings->m_eventIDs.insert( pSettings->m_eventIDs.begin(), m_eventIDs.begin(), m_eventIDs.end() );
        }
        return pSettings->m_nodeIdx;
    }

    void GenericEventConditionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        switch ( m_searchMode )
        {
            case EventSearchMode::OnlySearchStateEvents:
            {
                ImGui::Text( "Search: State Events" );
            }
            break;

            case EventSearchMode::OnlySearchAnimEvents:
            {
                ImGui::Text( "Search: Anim Events" );
            }
            break;

            case EventSearchMode::SearchAll:
            {
                ImGui::Text( "Search: All Events" );
            }
            break;
        }

        //-------------------------------------------------------------------------

        InlineString infoText;

        if ( m_operator == GenericEventConditionNode::Operator::Or )
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
            pSettings->m_preferHighestPercentageThrough = m_preferHighestPercentageThrough;
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

        if ( m_preferHighestPercentageThrough )
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
            pSettings->m_preferHighestPercentageThrough = m_preferHighestPercentageThrough;
        }
        return pSettings->m_nodeIdx;
    }

    void FootEventConditionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( FootEvent::GetPhaseConditionName( m_phaseCondition ) );

        if ( m_preferHighestPercentageThrough )
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
            pSettings->m_preferHighestPercentageThrough = m_preferHighestPercentageThrough;
        }
        return pSettings->m_nodeIdx;
    }

    void FootstepEventPercentageThroughToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( FootEvent::GetPhaseConditionName( m_phaseCondition ) );

        if ( m_preferHighestPercentageThrough )
        {
            ImGui::Text( "Prefer Highest Event %" );
        }
        else
        {
            ImGui::Text( "Prefer Highest Event Weight" );
        }
    }

    //-------------------------------------------------------------------------

    void SyncEventConditionToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
    }

    int16_t SyncEventConditionToolsNode::Compile( GraphCompilationContext& context ) const
    {
        SyncEventConditionNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<SyncEventConditionNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_sourceStateNodeIdx = context.IsCompilingConduit() ? context.GetConduitSourceStateIndex() : InvalidIndex;
            pSettings->m_triggerMode = m_triggerMode;
            pSettings->m_syncEventIdx = m_syncEventIdx;
        }
        return pSettings->m_nodeIdx;
    }

    void SyncEventConditionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        if ( m_triggerMode == SyncEventConditionNode::TriggerMode::ExactlyAtEventIndex )
        {
            ImGui::Text( "== %d", m_syncEventIdx );
        }
        else
        {
            ImGui::Text( ">= %d", m_syncEventIdx );
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
}