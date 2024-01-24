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
    StateMachineToolsNode::StateMachineToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
    }

    void StateMachineToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );

        // Create graph
        auto pStateMachineGraph = EE::New<StateMachineGraph>();
        SetChildGraph( pStateMachineGraph );

        // Create conduits
        pStateMachineGraph->CreateNode<EntryStateOverrideConduitToolsNode>();
        pStateMachineGraph->CreateNode<GlobalTransitionConduitToolsNode>();

        // Create default state
        auto pDefaultStateNode = pStateMachineGraph->CreateNode<StateToolsNode>();
        pDefaultStateNode->SetPosition( ImVec2( 0, 150 ) );
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
        StateMachineNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<StateMachineNode>( this, pDefinition );
        if ( state != NodeCompilationState::NeedCompilation )
        {
            return pDefinition->m_nodeIdx;
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
            auto& stateDefinition = pDefinition->m_stateDefinition.emplace_back();
            stateDefinition.m_stateNodeIdx = CompileState( context, pStateNode );
            if ( stateDefinition.m_stateNodeIdx == InvalidIndex )
            {
                return InvalidIndex;
            }

            // Compile entry condition if it exists
            auto pEntryConditionNode = pEntryConditionsConduit->GetEntryConditionNodeForState( pStateNode->GetID() );
            if ( pEntryConditionNode != nullptr )
            {
                stateDefinition.m_entryConditionNodeIdx = pEntryConditionNode->Compile( context );
                if ( stateDefinition.m_entryConditionNodeIdx == InvalidIndex )
                {
                    return InvalidIndex;
                }
            }

            IDToStateIdxMap.insert( TPair<UUID, StateMachineNode::StateIndex>( pStateNode->GetID(), (StateMachineNode::StateIndex) i ) );
            IDToCompiledNodeIdxMap.insert( TPair<UUID, int16_t>( pStateNode->GetID(), stateDefinition.m_stateNodeIdx ) );
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
                    auto& transitionDefinition = pDefinition->m_stateDefinition[i].m_transitionDefinition.emplace_back();
                    transitionDefinition.m_targetStateIdx = IDToStateIdxMap[endStateID];

                    // Compile transition node
                    //-------------------------------------------------------------------------

                    transitionDefinition.m_transitionNodeIdx = CompileTransition( context, pTransitionNode, IDToCompiledNodeIdxMap[endStateID] );
                    transitionDefinition.m_canBeForced = pTransitionNode->m_canBeForced;
                    if ( transitionDefinition.m_transitionNodeIdx == InvalidIndex )
                    {
                        return false;
                    }

                    // Compile condition tree
                    //-------------------------------------------------------------------------

                    TransitionNode::Definition* pCompiledTransitionDefinition = nullptr;
                    NodeCompilationState const state = context.GetDefinition<TransitionNode>( pTransitionNode, pCompiledTransitionDefinition );
                    EE_ASSERT( state == NodeCompilationState::AlreadyCompiled );

                    context.BeginTransitionConditionsCompilation( pCompiledTransitionDefinition->m_duration, pCompiledTransitionDefinition->m_durationOverrideNodeIdx );
                    transitionDefinition.m_conditionNodeIdx = pConditionNode->Compile( context );
                    if ( transitionDefinition.m_conditionNodeIdx == InvalidIndex )
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

                if ( !TryCompileTransition( pGlobalTransition, pGlobalTransition->GetEndStateID() ) )
                {
                    return InvalidIndex;
                }
            }
        }

        //-------------------------------------------------------------------------

        pDefinition->m_defaultStateIndex = IDToStateIdxMap[pStateMachineGraph->GetDefaultEntryStateID()];

        return pDefinition->m_nodeIdx;
    }

    int16_t StateMachineToolsNode::CompileState( GraphCompilationContext& context, StateToolsNode const* pStateNode ) const
    {
        EE_ASSERT( pStateNode != nullptr );

        StateNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<StateNode>( pStateNode, pDefinition );
        EE_ASSERT( state == NodeCompilationState::NeedCompilation );

        //-------------------------------------------------------------------------

        auto ReflectStateEvents = [&] ( TVector<StringID> const& IDs, TInlineVector<StringID, 3>& outEvents )
        {
            for ( auto const& ID : IDs )
            {
                if ( ID.IsValid() )
                {
                    if ( !VectorContains( outEvents, ID ) )
                    {
                        outEvents.emplace_back( ID );
                    }
                }
                else
                {
                    context.LogWarning( this, "Invalid state event detected and ignored!" );
                }
            }
        };

        ReflectStateEvents( pStateNode->m_events, pDefinition->m_entryEvents );
        ReflectStateEvents( pStateNode->m_entryEvents, pDefinition->m_entryEvents );

        ReflectStateEvents( pStateNode->m_events, pDefinition->m_executeEvents );
        ReflectStateEvents( pStateNode->m_executeEvents, pDefinition->m_executeEvents );

        ReflectStateEvents( pStateNode->m_events, pDefinition->m_exitEvents );
        ReflectStateEvents( pStateNode->m_exitEvents, pDefinition->m_exitEvents );

        //-------------------------------------------------------------------------

        if ( pStateNode->IsOffState() )
        {
            pDefinition->m_childNodeIdx = InvalidIndex;
            pDefinition->m_isOffState = true;
        }
        else
        {
            // Compile Blend Tree
            //-------------------------------------------------------------------------

            auto resultNodes = pStateNode->GetChildGraph()->FindAllNodesOfType<ResultToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
            EE_ASSERT( resultNodes.size() == 1 );
            ResultToolsNode const* pBlendTreeRoot = resultNodes[0];
            EE_ASSERT( pBlendTreeRoot != nullptr );

            auto pBlendTreeNode = pBlendTreeRoot->GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pBlendTreeNode != nullptr )
            {
                pDefinition->m_childNodeIdx = pBlendTreeNode->Compile( context );
                if ( pDefinition->m_childNodeIdx == InvalidIndex )
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
                pDefinition->m_layerWeightNodeIdx = pLayerWeightNode->Compile( context );
                if ( pDefinition->m_layerWeightNodeIdx == InvalidIndex )
                {
                    return InvalidIndex;
                }
            }

            auto pLayerRootMotionWeightNode = pLayerData->GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pLayerRootMotionWeightNode != nullptr )
            {
                pDefinition->m_layerRootMotionWeightNodeIdx = pLayerRootMotionWeightNode->Compile( context );
                if ( pDefinition->m_layerRootMotionWeightNodeIdx == InvalidIndex )
                {
                    return InvalidIndex;
                }
            }

            auto pLayerMaskNode = pLayerData->GetConnectedInputNode<FlowToolsNode>( 2 );
            if ( pLayerMaskNode != nullptr )
            {
                pDefinition->m_layerBoneMaskNodeIdx = pLayerMaskNode->Compile( context );
                if ( pDefinition->m_layerBoneMaskNodeIdx == InvalidIndex )
                {
                    return InvalidIndex;
                }
            }

            // Transfer additional state events
            //-------------------------------------------------------------------------

            auto ReflectTimedStateEvents = [&] ( TVector<StateToolsNode::TimedStateEvent> const& timedEvents, TInlineVector<StateNode::TimedEvent, 1>& outEvents )
            {
                for ( auto const& evt : timedEvents )
                {
                    if ( evt.m_ID.IsValid() )
                    {
                        outEvents.emplace_back( StateNode::TimedEvent( evt.m_ID, evt.m_timeValue ) );
                    }
                    else
                    {
                        context.LogWarning( this, "Invalid state event detected and ignored!" );
                    }
                }
            };

            ReflectTimedStateEvents( pStateNode->m_timeRemainingEvents, pDefinition->m_timedRemainingEvents );
            ReflectTimedStateEvents( pStateNode->m_timeElapsedEvents, pDefinition->m_timedElapsedEvents );
        }

        //-------------------------------------------------------------------------

        return pDefinition->m_nodeIdx;
    }

    int16_t StateMachineToolsNode::CompileTransition( GraphCompilationContext& context, TransitionToolsNode const* pTransitionNode, int16_t targetStateNodeIdx ) const
    {
        EE_ASSERT( pTransitionNode != nullptr );
        TransitionNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<TransitionNode>( pTransitionNode, pDefinition );
        if ( state == NodeCompilationState::AlreadyCompiled )
        {
            return pDefinition->m_nodeIdx;
        }

        //-------------------------------------------------------------------------

        auto pDurationOverrideNode = pTransitionNode->GetConnectedInputNode<FlowToolsNode>( 1 );
        if ( pDurationOverrideNode != nullptr )
        {
            pDefinition->m_durationOverrideNodeIdx = pDurationOverrideNode->Compile( context );
            if ( pDefinition->m_durationOverrideNodeIdx == InvalidIndex )
            {
                return InvalidIndex;
            }
        }

        auto pSyncEventOffsetOverrideNode = pTransitionNode->GetConnectedInputNode<FlowToolsNode>( 2 );
        if ( pSyncEventOffsetOverrideNode != nullptr )
        {
            pDefinition->m_syncEventOffsetOverrideNodeIdx = pSyncEventOffsetOverrideNode->Compile( context );
            if ( pDefinition->m_syncEventOffsetOverrideNodeIdx == InvalidIndex )
            {
                return InvalidIndex;
            }
        }

        auto pStartBoneMaskNode = pTransitionNode->GetConnectedInputNode<FlowToolsNode>( 3 );
        if ( pStartBoneMaskNode != nullptr )
        {
            pDefinition->m_startBoneMaskNodeIdx = pStartBoneMaskNode->Compile( context );
            if ( pDefinition->m_startBoneMaskNodeIdx == InvalidIndex )
            {
                return InvalidIndex;
            }

            if ( pTransitionNode->m_boneMaskBlendInTimePercentage <= 0.0f )
            {
                context.LogError( "Bone mask blend time needs to be greater than zero!" );
                return InvalidIndex;
            }
        }

        auto pTargetSyncIDNode = pTransitionNode->GetConnectedInputNode<FlowToolsNode>( 4 );
        if ( pTargetSyncIDNode != nullptr )
        {
            if ( pTransitionNode->m_timeMatchMode >= TransitionToolsNode::TimeMatchMode::MatchSyncEventID && pTransitionNode->m_timeMatchMode <= TransitionToolsNode::TimeMatchMode::MatchClosestSyncEventIDAndPercentage )
            {
                pDefinition->m_targetSyncIDNodeIdx = pTargetSyncIDNode->Compile( context );
                if ( pDefinition->m_targetSyncIDNodeIdx == InvalidIndex )
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogWarning( "Target Sync Event ID set but we are not in a sync event ID time match mode - The supplied ID will be ignored!" );
            }
        }

        //-------------------------------------------------------------------------

        pDefinition->m_targetStateNodeIdx = targetStateNodeIdx;
        pDefinition->m_blendWeightEasingOp = pTransitionNode->m_blendWeightEasing;
        pDefinition->m_rootMotionBlend = pTransitionNode->m_rootMotionBlend;
        pDefinition->m_duration = Math::Max( pTransitionNode->m_duration.ToFloat(), 0.0f );
        pDefinition->m_syncEventOffset = pTransitionNode->m_syncEventOffset;
        pDefinition->m_boneMaskBlendInTimePercentage = pTransitionNode->m_boneMaskBlendInTimePercentage.GetClamped( false );

        //-------------------------------------------------------------------------

        pDefinition->m_transitionOptions.ClearAllFlags();
        pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::ClampDuration, pTransitionNode->m_clampDurationToSource );

        switch ( pTransitionNode->m_timeMatchMode )
        {
            case TransitionToolsNode::TimeMatchMode::None:
            break;

            case TransitionToolsNode::TimeMatchMode::Synchronized:
            {
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::Synchronized, true );
            }
            break;

            case TransitionToolsNode::TimeMatchMode::MatchSourceSyncEventIndex:
            {
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSourceTime, true );
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventIndex, true );
            }
            break;

            case TransitionToolsNode::TimeMatchMode::MatchSourceSyncEventPercentage:
            {
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSourceTime, true );
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventPercentage, true );
            }
            break;

            case TransitionToolsNode::TimeMatchMode::MatchSourceSyncEventIndexAndPercentage:
            {
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSourceTime, true );
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventIndex, true );
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventPercentage, true );
            }
            break;

            case TransitionToolsNode::TimeMatchMode::MatchSyncEventID:
            {
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSourceTime, true );
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventID, true );
            }
            break;

            case TransitionToolsNode::TimeMatchMode::MatchSyncEventIDAndPercentage:
            {
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSourceTime, true );
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventID, true );
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventPercentage, true );
            }
            break;

            case TransitionToolsNode::TimeMatchMode::MatchClosestSyncEventID:
            {
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSourceTime, true );
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventID, true );
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::PreferClosestSyncEventID, true );
            }
            break;

            case TransitionToolsNode::TimeMatchMode::MatchClosestSyncEventIDAndPercentage:
            {
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSourceTime, true );
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventID, true );
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::MatchSyncEventPercentage, true );
                pDefinition->m_transitionOptions.SetFlag( TransitionNode::TransitionOptions::PreferClosestSyncEventID, true );
            }
            break;
        }

        //-------------------------------------------------------------------------

        return pDefinition->m_nodeIdx;
    }

    void StateMachineToolsNode::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue )
    {
        FlowToolsNode::SerializeCustom( typeRegistry, graphObjectValue );
        GetEntryStateOverrideConduit()->UpdateConditionsNode();
        GetGlobalTransitionConduit()->UpdateTransitionNodes();
    }
}