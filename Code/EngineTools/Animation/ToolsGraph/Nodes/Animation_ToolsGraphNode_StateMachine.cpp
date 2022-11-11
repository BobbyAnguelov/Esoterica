#include "Animation_ToolsGraphNode_StateMachine.h"
#include "Animation_ToolsGraphNode_EntryStates.h"
#include "Animation_ToolsGraphNode_GlobalTransitions.h"
#include "Animation_ToolsGraphNode_State.h"
#include "Animation_ToolsGraphNode_Result.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_StateMachineGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_StateMachine.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void StateMachineToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Pose", GraphValueType::Pose );

        // Create graph
        auto pStateMachineGraph = EE::New<StateMachineGraph>();
        SetChildGraph( pStateMachineGraph );

        // Create conduits
        pStateMachineGraph->CreateNode<EntryStateOverrideConduitToolsNode>();
        pStateMachineGraph->CreateNode<GlobalTransitionConduitToolsNode>();

        // Create default state
        auto pDefaultStateNode = pStateMachineGraph->CreateNode<StateToolsNode>();
        pDefaultStateNode->SetCanvasPosition( ImVec2( 0, 150 ) );
        pStateMachineGraph->SetDefaultEntryState( pDefaultStateNode->GetID() );

        // Update dependent nodes
        GetEntryStateOverrideConduit()->UpdateConditionsNode();
        GetGlobalTransitionConduit()->UpdateTransitionNodes();
    }

    void StateMachineToolsNode::OnShowNode()
    {
        FlowToolsNode::OnShowNode();
        GetEntryStateOverrideConduit()->UpdateConditionsNode();
        GetGlobalTransitionConduit()->UpdateTransitionNodes();
    }

    EntryStateOverrideConduitToolsNode const* StateMachineToolsNode::GetEntryStateOverrideConduit() const
    {
        auto pStateMachineGraph = Cast<StateMachineGraph>( GetChildGraph() );
        auto const foundNodes = pStateMachineGraph->FindAllNodesOfType<EntryStateOverrideConduitToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );
        EE_ASSERT( foundNodes.size() == 1 );
        return foundNodes[0];
    }

    GlobalTransitionConduitToolsNode const* StateMachineToolsNode::GetGlobalTransitionConduit() const
    {
        auto pStateMachineGraph = Cast<StateMachineGraph>( GetChildGraph() );
        auto const foundNodes = pStateMachineGraph->FindAllNodesOfType<GlobalTransitionConduitToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );
        EE_ASSERT( foundNodes.size() == 1 );
        return foundNodes[0];
    }

    int16_t StateMachineToolsNode::Compile( GraphCompilationContext& context ) const
    {
        StateMachineNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<StateMachineNode>( this, pSettings );
        if ( state != NodeCompilationState::NeedCompilation )
        {
            return pSettings->m_nodeIdx;
        }

        // Get all necessary nodes for compilation
        //-------------------------------------------------------------------------

        auto pStateMachineGraph = Cast<StateMachineGraph>( GetChildGraph() );
        auto stateNodes = pStateMachineGraph->FindAllNodesOfType<StateToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        int32_t const numStateNodes = (int32_t) stateNodes.size();
        EE_ASSERT( numStateNodes >= 1 );

        auto conduitNodes = pStateMachineGraph->FindAllNodesOfType<TransitionConduitToolsNode>();
        int32_t const numConduitNodes = (int32_t) conduitNodes.size();

        auto globalTransitionNodes = GetGlobalTransitionConduit()->GetSecondaryGraph()->FindAllNodesOfType<GlobalTransitionToolsNode>();

        // Compile all states
        //-------------------------------------------------------------------------

        auto pEntryConditionsConduit = GetEntryStateOverrideConduit();

        THashMap<UUID, StateMachineNode::StateIndex> IDToStateIdxMap;
        THashMap<UUID, int16_t> IDToCompiledNodeIdxMap;

        for ( auto i = 0; i < numStateNodes; i++ )
        {
            auto pStateNode = stateNodes[i];

            // Compile state node
            auto& stateSettings = pSettings->m_stateSettings.emplace_back();
            stateSettings.m_stateNodeIdx = CompileState( context, pStateNode );
            if ( stateSettings.m_stateNodeIdx == InvalidIndex )
            {
                return InvalidIndex;
            }

            // Compile entry condition if it exists
            auto pEntryConditionNode = pEntryConditionsConduit->GetEntryConditionNodeForState( pStateNode->GetID() );
            if ( pEntryConditionNode != nullptr )
            {
                stateSettings.m_entryConditionNodeIdx = pEntryConditionNode->Compile( context );
                if ( stateSettings.m_entryConditionNodeIdx == InvalidIndex )
                {
                    return InvalidIndex;
                }
            }

            IDToStateIdxMap.insert( TPair<UUID, StateMachineNode::StateIndex>( pStateNode->GetID(), (StateMachineNode::StateIndex) i ) );
            IDToCompiledNodeIdxMap.insert( TPair<UUID, int16_t>( pStateNode->GetID(), stateSettings.m_stateNodeIdx ) );
        }

        // Compile all transitions
        //-------------------------------------------------------------------------

        for ( auto i = 0; i < numStateNodes; i++ )
        {
            auto pStartStateNode = stateNodes[i];

            // Check all conduits for any starting at the specified state
            //-------------------------------------------------------------------------

            // We need to ignore any global transitions that we have an explicit conduit for
            TInlineVector<GlobalTransitionToolsNode const*, 20> globalTransitionNodesCopy = globalTransitionNodes;
            auto RemoveFromGlobalTransitions = [&globalTransitionNodesCopy] ( UUID const& endStateID )
            {
                for ( auto iter = globalTransitionNodesCopy.begin(); iter != globalTransitionNodesCopy.end(); ++iter )
                {
                    if ( ( *iter )->GetEndStateID() == endStateID )
                    {
                        globalTransitionNodesCopy.erase( iter );
                        break;
                    }
                }
            };

            auto TryCompileTransition = [&] ( TransitionToolsNode const* pTransitionNode, UUID const& endStateID )
            {
                // Transitions are only enabled if a condition is connected to them
                auto pConditionNode = pTransitionNode->GetConnectedInputNode<FlowToolsNode>( 0 );
                if ( pConditionNode != nullptr )
                {
                    auto& transitionSettings = pSettings->m_stateSettings[i].m_transitionSettings.emplace_back();
                    transitionSettings.m_targetStateIdx = IDToStateIdxMap[endStateID];

                    // Compile transition node
                    //-------------------------------------------------------------------------

                    transitionSettings.m_transitionNodeIdx = CompileTransition( context, pTransitionNode, IDToCompiledNodeIdxMap[endStateID] );
                    if ( transitionSettings.m_transitionNodeIdx == InvalidIndex )
                    {
                        return false;
                    }

                    // Compile condition tree
                    //-------------------------------------------------------------------------

                    TransitionNode::Settings* pCompiledTransitionSettings = nullptr;
                    NodeCompilationState const state = context.GetSettings<TransitionNode>( pTransitionNode, pCompiledTransitionSettings );
                    EE_ASSERT( state == NodeCompilationState::AlreadyCompiled );

                    context.BeginTransitionConditionsCompilation( pCompiledTransitionSettings->m_duration, pCompiledTransitionSettings->m_durationOverrideNodeIdx );
                    transitionSettings.m_conditionNodeIdx = pConditionNode->Compile( context );
                    if ( transitionSettings.m_conditionNodeIdx == InvalidIndex )
                    {
                        return false;
                    }
                    context.EndTransitionConditionsCompilation();
                }

                return true;
            };

            // Remove ourselves state from the global transitions copy
            RemoveFromGlobalTransitions( pStartStateNode->GetID() );

            // Compile conduits
            for ( auto c = 0; c < numConduitNodes; c++ )
            {
                auto pConduit = conduitNodes[c];
                if ( pConduit->GetStartStateID() != pStartStateNode->GetID() )
                {
                    continue;
                }

                RemoveFromGlobalTransitions( pConduit->GetEndStateID() );

                auto const foundSourceStateIter = IDToCompiledNodeIdxMap.find( pConduit->GetStartStateID() );
                EE_ASSERT( foundSourceStateIter != IDToCompiledNodeIdxMap.end() );
                context.BeginConduitCompilation( foundSourceStateIter->second );

                // Compile transitions in conduit
                auto transitionNodes = pConduit->GetSecondaryGraph()->FindAllNodesOfType<TransitionToolsNode>();
                for ( auto pTransitionNode : transitionNodes )
                {
                    if ( !TryCompileTransition( pTransitionNode, pConduit->GetEndStateID() ) )
                    {
                        return InvalidIndex;
                    }
                }

                context.EndConduitCompilation();
            }

            // Global transitions
            //-------------------------------------------------------------------------
            // Compile all global transitions from this state to others

            for ( auto pGlobalTransition : globalTransitionNodesCopy )
            {
                auto const foundSourceStateIter = IDToCompiledNodeIdxMap.find( pStartStateNode->GetID() );
                EE_ASSERT( foundSourceStateIter != IDToCompiledNodeIdxMap.end() );
                context.BeginConduitCompilation( foundSourceStateIter->second );

                if ( !TryCompileTransition( pGlobalTransition, pGlobalTransition->GetEndStateID() ) )
                {
                    return InvalidIndex;
                }
                context.EndConduitCompilation();
            }
        }

        //-------------------------------------------------------------------------

        pSettings->m_defaultStateIndex = IDToStateIdxMap[pStateMachineGraph->GetDefaultEntryStateID()];

        return pSettings->m_nodeIdx;
    }

    int16_t StateMachineToolsNode::CompileState( GraphCompilationContext& context, StateToolsNode const* pStateNode ) const
    {
        EE_ASSERT( pStateNode != nullptr );

        StateNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<StateNode>( pStateNode, pSettings );
        EE_ASSERT( state == NodeCompilationState::NeedCompilation );

        //-------------------------------------------------------------------------

        for ( auto const& ID : pStateNode->m_entryEvents ) { pSettings->m_entryEvents.emplace_back( ID ); }
        for ( auto const& ID : pStateNode->m_executeEvents ) { pSettings->m_executeEvents.emplace_back( ID ); }
        for ( auto const& ID : pStateNode->m_exitEvents ) { pSettings->m_exitEvents.emplace_back( ID ); }

        //-------------------------------------------------------------------------

        if ( pStateNode->IsOffState() )
        {
            pSettings->m_childNodeIdx = InvalidIndex;
            pSettings->m_isOffState = true;
        }
        else
        {
            // Compile Blend Tree
            //-------------------------------------------------------------------------

            auto resultNodes = pStateNode->GetChildGraph()->FindAllNodesOfType<ResultToolsNode>();
            EE_ASSERT( resultNodes.size() == 1 );
            ResultToolsNode const* pBlendTreeRoot = resultNodes[0];
            EE_ASSERT( pBlendTreeRoot != nullptr );

            auto pBlendTreeNode = pBlendTreeRoot->GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pBlendTreeNode != nullptr )
            {
                pSettings->m_childNodeIdx = pBlendTreeNode->Compile( context );
                if ( pSettings->m_childNodeIdx == InvalidIndex )
                {
                    return InvalidIndex;
                }
            }

            // Compile Layer Data
            //-------------------------------------------------------------------------

            auto dataNodes = pStateNode->GetSecondaryGraph()->FindAllNodesOfType<StateLayerDataToolsNode>();
            EE_ASSERT( dataNodes.size() == 1 );
            auto pLayerData = dataNodes[0];
            EE_ASSERT( pLayerData != nullptr );

            auto pLayerWeightNode = pLayerData->GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pLayerWeightNode != nullptr )
            {
                pSettings->m_layerWeightNodeIdx = pLayerWeightNode->Compile( context );
                if ( pSettings->m_layerWeightNodeIdx == InvalidIndex )
                {
                    return InvalidIndex;
                }
            }

            auto pLayerMaskNode = pLayerData->GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pLayerMaskNode != nullptr )
            {
                pSettings->m_layerBoneMaskNodeIdx = pLayerMaskNode->Compile( context );
                if ( pSettings->m_layerBoneMaskNodeIdx == InvalidIndex )
                {
                    return InvalidIndex;
                }
            }

            // Transfer additional state events
            //-------------------------------------------------------------------------

            for ( auto const& evt : pStateNode->m_timeRemainingEvents ) { pSettings->m_timedRemainingEvents.emplace_back( StateNode::TimedEvent( evt.m_ID, evt.m_timeValue ) ); }
            for ( auto const& evt : pStateNode->m_timeElapsedEvents ) { pSettings->m_timedElapsedEvents.emplace_back( StateNode::TimedEvent( evt.m_ID, evt.m_timeValue ) ); }
        }

        //-------------------------------------------------------------------------

        return pSettings->m_nodeIdx;
    }

    int16_t StateMachineToolsNode::CompileTransition( GraphCompilationContext& context, TransitionToolsNode const* pTransitionNode, int16_t targetStateNodeIdx ) const
    {
        EE_ASSERT( pTransitionNode != nullptr );
        TransitionNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<TransitionNode>( pTransitionNode, pSettings );
        if ( state == NodeCompilationState::AlreadyCompiled )
        {
            return pSettings->m_nodeIdx;
        }

        //-------------------------------------------------------------------------

        auto pDurationOverrideNode = pTransitionNode->GetConnectedInputNode<FlowToolsNode>( 1 );
        if ( pDurationOverrideNode != nullptr )
        {
            pSettings->m_durationOverrideNodeIdx = pDurationOverrideNode->Compile( context );
            if ( pSettings->m_durationOverrideNodeIdx == InvalidIndex )
            {
                return InvalidIndex;
            }
        }

        auto pSyncEventOffsetOverrideNode = pTransitionNode->GetConnectedInputNode<FlowToolsNode>( 2 );
        if ( pSyncEventOffsetOverrideNode != nullptr )
        {
            pSettings->m_syncEventOffsetOverrideNodeIdx = pSyncEventOffsetOverrideNode->Compile( context );
            if ( pSettings->m_syncEventOffsetOverrideNodeIdx == InvalidIndex )
            {
                return InvalidIndex;
            }
        }

        //-------------------------------------------------------------------------

        pSettings->m_targetStateNodeIdx = targetStateNodeIdx;
        pSettings->m_blendWeightEasingType = pTransitionNode->m_blendWeightEasingType;
        pSettings->m_rootMotionBlend = pTransitionNode->m_rootMotionBlend;
        pSettings->m_duration = pTransitionNode->m_duration;
        pSettings->m_syncEventOffset = pTransitionNode->m_syncEventOffset;

        //-------------------------------------------------------------------------
        
        pSettings->m_transitionOptions.ClearAllFlags();
        pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::ClampDuration, pTransitionNode->m_clampDurationToSource );
        pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::ForcedTransitionAllowed, pTransitionNode->m_canBeForced );

        switch ( pTransitionNode->m_timeMatchMode )
        {
            case TransitionToolsNode::TimeMatchMode::None:
            break;

            case TransitionToolsNode::TimeMatchMode::Synchronized:
            {
                pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::Synchronized, true );
            }
            break;

            case TransitionToolsNode::TimeMatchMode::MatchSourceSyncEventIndexOnly:
            {
                pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSourceTime, true );
                pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventIndex, true );
            }
            break;

            case TransitionToolsNode::TimeMatchMode::MatchSourceSyncEventIndexAndPercentage:
            {
                pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSourceTime, true );
                pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventIndex, true );
                pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventPercentage, true );
            }
            break;

            case TransitionToolsNode::TimeMatchMode::MatchSourceSyncEventID:
            {
                pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSourceTime, true );
                pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventID, true );
            }
            break;

            case TransitionToolsNode::TimeMatchMode::MatchSourceSyncEventIDAndPercentage:
            {
                pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSourceTime, true );
                pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventID, true );
                pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventPercentage, true );
            }
            break;

            case TransitionToolsNode::TimeMatchMode::MatchSourceSyncEventPercentage:
            {
                pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSourceTime, true );
                pSettings->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventPercentage, true );
            }
            break;
        }

        //-------------------------------------------------------------------------

        return pSettings->m_nodeIdx;
    }

    void StateMachineToolsNode::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue )
    {
        FlowToolsNode::SerializeCustom( typeRegistry, graphObjectValue );
        GetEntryStateOverrideConduit()->UpdateConditionsNode();
        GetGlobalTransitionConduit()->UpdateTransitionNodes();
    }
}