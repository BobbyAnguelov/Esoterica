#include "Animation_EditorGraphNode_Events.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void GenericEventConditionEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Bool, true );
    }

    int16_t GenericEventConditionEditorNode::Compile( GraphCompilationContext& context ) const
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

    void GenericEventConditionEditorNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
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

    void GenericEventPercentageThroughEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
    }

    int16_t GenericEventPercentageThroughEditorNode::Compile( GraphCompilationContext& context ) const
    {
        GenericEventPercentageThroughNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<GenericEventPercentageThroughNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_sourceStateNodeIdx = context.IsCompilingConduit() ? context.GetConduitSourceStateIndex() : InvalidIndex;
            pSettings->m_searchMode = m_searchMode;
            pSettings->m_preferHighestPercentageThrough = m_preferHighestPercentageThrough;
            pSettings->m_eventID = m_eventID;
        }
        return pSettings->m_nodeIdx;
    }

    void GenericEventPercentageThroughEditorNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {

    }

    //-------------------------------------------------------------------------

    void FootEventConditionEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Bool, true );
    }

    int16_t FootEventConditionEditorNode::Compile( GraphCompilationContext& context ) const
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

    //-------------------------------------------------------------------------

    void FootstepEventPercentageThroughEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
    }

    int16_t FootstepEventPercentageThroughEditorNode::Compile( GraphCompilationContext& context ) const
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

    //-------------------------------------------------------------------------

    void SyncEventConditionEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
    }

    int16_t SyncEventConditionEditorNode::Compile( GraphCompilationContext& context ) const
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

    //-------------------------------------------------------------------------

    void CurrentSyncEventEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
    }

    int16_t CurrentSyncEventEditorNode::Compile( GraphCompilationContext& context ) const
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