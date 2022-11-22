#include "Animation_RuntimeGraph_Instance.h"
#include "Animation_RuntimeGraph_Node.h"
#include "Nodes/Animation_RuntimeGraphNode_ExternalGraph.h"
#include "System/Log.h"
#include "System/Profiling.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphInstance::GraphInstance( GraphVariation const* pGraphVariation, uint64_t userID, TaskSystem* pTaskSystem )
        : m_pGraphVariation( pGraphVariation )
        , m_userID( userID )
        , m_graphContext( userID, pGraphVariation->GetSkeleton() )
    {
        EE_ASSERT( pGraphVariation != nullptr );

        //-------------------------------------------------------------------------

        // If the supplied task system is null, then this is a standalone graph instance
        bool const isStandaloneGraphInstance = ( pTaskSystem == nullptr );
        if ( isStandaloneGraphInstance )
        {
            m_pTaskSystem = EE::New<TaskSystem>( m_pGraphVariation->GetSkeleton() );
        }

        // Allocate memory
        //-------------------------------------------------------------------------

        auto pGraphDef = m_pGraphVariation->m_pGraphDefinition.GetPtr();
        size_t const numNodes = pGraphDef->m_instanceNodeStartOffsets.size();
        EE_ASSERT( pGraphDef->m_nodeSettings.size() == numNodes );

        m_pAllocatedInstanceMemory = reinterpret_cast<uint8_t*>( EE::Alloc( pGraphDef->m_instanceRequiredMemory, pGraphDef->m_instanceRequiredAlignment ) );

        m_nodes.reserve( numNodes );

        for ( auto const& nodeOffset : pGraphDef->m_instanceNodeStartOffsets )
        {
            // Create ptrs to the future nodes!
            // The nodes are not actually created yet but the ptrs are valid
            m_nodes.emplace_back( reinterpret_cast<GraphNode*>( m_pAllocatedInstanceMemory + nodeOffset ) );
        }

        // Create child graph instances
        //-------------------------------------------------------------------------

        size_t const numChildGraphs = pGraphDef->m_childGraphSlots.size();
        m_childGraphs.reserve( numChildGraphs );

        TInlineVector<GraphInstance*, 20> createdChildGraphInstances;
        createdChildGraphInstances.reserve( numChildGraphs );

        for ( auto const& childGraphSlot : pGraphDef->m_childGraphSlots )
        {
            auto pChildGraphVariation = m_pGraphVariation->m_dataSet.GetResource<GraphVariation>( childGraphSlot.m_dataSlotIdx );
            if ( pChildGraphVariation != nullptr )
            {
                if ( pChildGraphVariation->GetSkeleton() == pGraphVariation->GetSkeleton() )
                {
                    ChildGraph cg;
                    cg.m_nodeIdx = childGraphSlot.m_nodeIdx;
                    cg.m_pInstance = new ( EE::Alloc( sizeof( GraphInstance ) ) ) GraphInstance( pChildGraphVariation, m_userID, isStandaloneGraphInstance ? m_pTaskSystem : pTaskSystem );
                    m_childGraphs.emplace_back( cg );

                    createdChildGraphInstances.emplace_back( cg.m_pInstance );
                }
                else
                {
                    createdChildGraphInstances.emplace_back( nullptr );
                    EE_LOG_ERROR( "Animation", "Graph Instance", "Different skeleton for child graph detected, this is not allowed. Trying to use '%s' within '%s'", pChildGraphVariation->GetResourceID().c_str(), pGraphVariation->GetResourceID().c_str() );
                }
            }
            else
            {
                createdChildGraphInstances.emplace_back( nullptr );
                m_childGraphs.emplace_back( ChildGraph() );
            }
        }

        // Instantiate individual nodes
        //-------------------------------------------------------------------------

        InstantiationContext instantiationContext = { (int16_t) InvalidIndex, m_nodes, createdChildGraphInstances, m_pGraphVariation->m_pGraphDefinition->m_parameterLookupMap, &m_pGraphVariation->m_dataSet, m_userID };

        #if EE_DEVELOPMENT_TOOLS
        instantiationContext.m_pLog = &m_log;
        #endif

        for ( int16_t i = 0; i < numNodes; i++ )
        {
            instantiationContext.m_currentNodeIdx = i;
            pGraphDef->m_nodeSettings[i]->InstantiateNode( instantiationContext, InstantiationOptions::CreateNode );
        }

        // Set up graph context
        //-------------------------------------------------------------------------

        // Initialize context
        m_graphContext.Initialize( isStandaloneGraphInstance ? m_pTaskSystem : pTaskSystem );
        EE_ASSERT( m_graphContext.IsValid() );

        #if EE_DEVELOPMENT_TOOLS
        m_graphContext.SetDebugSystems( &m_rootMotionDebugger, &m_activeNodes, &m_log );
        m_activeNodes.reserve( 50 );
        #endif

        // Initialize graph nodes
        //-------------------------------------------------------------------------

        // Initialize persistent graph nodes
        for ( auto nodeIdx : pGraphDef->m_persistentNodeIndices )
        {
            m_nodes[nodeIdx]->Initialize( m_graphContext );
        }

        // Set root node
        m_pRootNode = reinterpret_cast<PoseNode*>( m_nodes[pGraphDef->m_rootNodeIdx] );
        EE_ASSERT( !m_pRootNode->IsInitialized() );
    }

    GraphInstance::~GraphInstance()
    {
        // Ensure we dont have any connected external graphs
        EE_ASSERT( m_externalGraphs.empty() );

        // Shutdown persistent graph nodes
        auto pGraphDef = m_pGraphVariation->m_pGraphDefinition.GetPtr();
        for ( auto nodeIdx : pGraphDef->m_persistentNodeIndices )
        {
            m_nodes[nodeIdx]->Shutdown( m_graphContext );
        }

        // shutdown and clear root node
        if ( m_pRootNode->IsInitialized() )
        {
            m_pRootNode->Shutdown( m_graphContext );
        }

        m_pRootNode = nullptr;

        // Shutdown context
        m_graphContext.Shutdown();

        //-------------------------------------------------------------------------

        // Run graph node destructors
        for ( auto pNode : m_nodes )
        {
            pNode->~GraphNode();
        }

        // Destroy child graph instances
        for ( auto childGraph : m_childGraphs )
        {
            if ( childGraph.m_pInstance != nullptr )
            {
                childGraph.m_pInstance->~GraphInstance();
                EE::Free( childGraph.m_pInstance );
            }
        }
        m_childGraphs.clear();

        EE::Free( m_pAllocatedInstanceMemory );
        EE::Delete( m_pTaskSystem );
    }

    //-------------------------------------------------------------------------

    Pose const* GraphInstance::GetPose()
    {
        EE_ASSERT( m_pTaskSystem != nullptr );
        return m_pTaskSystem->GetPose();
    }

    bool GraphInstance::DoesTaskSystemNeedUpdate() const
    {
        return m_pTaskSystem->RequiresUpdate();
    }

    //-------------------------------------------------------------------------

    int32_t GraphInstance::GetExternalGraphSlotIndex( StringID slotID ) const
    {
        int32_t const numSlots = (int32_t) m_pGraphVariation->m_pGraphDefinition->m_externalGraphSlots.size();
        for ( int32_t i = 0; i < numSlots; i++ )
        {
            auto const& graphSlot = m_pGraphVariation->m_pGraphDefinition->m_externalGraphSlots[i];
            if ( graphSlot.m_slotID == slotID )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    int16_t GraphInstance::GetExternalGraphNodeIndex( StringID slotID ) const
    {
        int32_t const numSlots = (int32_t) m_pGraphVariation->m_pGraphDefinition->m_externalGraphSlots.size();
        for ( int32_t i = 0; i < numSlots; i++ )
        {
            auto const& graphSlot = m_pGraphVariation->m_pGraphDefinition->m_externalGraphSlots[i];
            if ( graphSlot.m_slotID == slotID )
            {
                return graphSlot.m_nodeIdx;
            }
        }

        return InvalidIndex;
    }

    int32_t GraphInstance::GetConnectedExternalGraphIndex( StringID slotID ) const
    {
        int32_t const numConnectedGraphs = (int32_t) m_externalGraphs.size();
        for ( int32_t i = 0; i < numConnectedGraphs; i++ )
        {
            if ( m_externalGraphs[i].m_slotID == slotID )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    GraphInstance* GraphInstance::ConnectExternalGraph( StringID slotID, GraphVariation const* pExternalGraphVariation )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance - Connect External Graph" );

        if ( pExternalGraphVariation->GetSkeleton() != m_pGraphVariation->GetSkeleton() )
        {
            // TODO: switch to internal error tracking
            EE_LOG_ERROR( "Animation", "Graph Instance", "Cannot insert extrenal graph since skeletons dont match! Expected: %s, supplied: %s", m_pGraphVariation->GetSkeleton()->GetResourceID().c_str(), pExternalGraphVariation->GetSkeleton()->GetResourceID().c_str() );
            return nullptr;
        }

        // Ensure we dont already have a graph connected for this slot
        int32_t connectedGraphIdx = GetConnectedExternalGraphIndex( slotID );
        EE_ASSERT( connectedGraphIdx == InvalidIndex );

        // Get the slot idx for the graph and ensure it is valid, users are expected to check slot validity before calling this function
        int32_t slotIdx = GetExternalGraphSlotIndex( slotID );
        EE_ASSERT( slotIdx != InvalidIndex );

        // Create the new connected graph record
        ExternalGraph& connectedGraph = m_externalGraphs.emplace_back();
        connectedGraph.m_slotID = slotID;
        connectedGraph.m_nodeIdx = m_pGraphVariation->m_pGraphDefinition->m_externalGraphSlots[slotIdx].m_nodeIdx;

        // Create graph instance
        //-------------------------------------------------------------------------

        connectedGraph.m_pInstance = new ( EE::Alloc( sizeof( GraphInstance ) ) ) GraphInstance( pExternalGraphVariation, m_userID, m_pTaskSystem );
        EE_ASSERT( connectedGraph.m_pInstance != nullptr );

        // Attach instance to the node
        //-------------------------------------------------------------------------

        auto pExternalGraphNode = reinterpret_cast<GraphNodes::ExternalGraphNode*> ( m_nodes[connectedGraph.m_nodeIdx] );
        pExternalGraphNode->AttachGraphInstance( m_graphContext, connectedGraph.m_pInstance );

        //-------------------------------------------------------------------------

        return connectedGraph.m_pInstance;
    }

    void GraphInstance::DisconnectExternalGraph( StringID slotID )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance - Disconnect External Graph" );

        // Ensure we have a graph connected for this slot
        int32_t connectedGraphIdx = GetConnectedExternalGraphIndex( slotID );
        EE_ASSERT( connectedGraphIdx != InvalidIndex );

        ExternalGraph& connectedGraph = m_externalGraphs[connectedGraphIdx];

        // Detach from node and destroy graph instance
        //-------------------------------------------------------------------------

        auto pExternalGraphNode = reinterpret_cast<GraphNodes::ExternalGraphNode*> ( m_nodes[connectedGraph.m_nodeIdx] );
        pExternalGraphNode->DetachExternalGraphInstance( m_graphContext );

        connectedGraph.m_pInstance->~GraphInstance();
        EE::Free( connectedGraph.m_pInstance );
        connectedGraph.m_pInstance = nullptr;

        // Remove record
        //-------------------------------------------------------------------------

        m_externalGraphs.erase_unsorted( m_externalGraphs.begin() + connectedGraphIdx );
    }

    //-------------------------------------------------------------------------

    GraphPoseNodeResult GraphInstance::EvaluateGraph( Seconds const deltaTime, Transform const& startWorldTransform, Physics::Scene* pPhysicsScene, bool resetGraphState )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance: Evaluate Graph" );
        #if EE_DEVELOPMENT_TOOLS
        m_activeNodes.clear();
        m_rootMotionDebugger.StartCharacterUpdate( startWorldTransform );
        RecordGraphUpdateData( deltaTime, startWorldTransform );
        #endif

        //-------------------------------------------------------------------------

        if ( m_pTaskSystem != nullptr )
        {
            m_pTaskSystem->Reset();
        }

        m_graphContext.Update( deltaTime, startWorldTransform, pPhysicsScene );

        //-------------------------------------------------------------------------

        if ( resetGraphState || !m_pRootNode->IsInitialized() )
        {
            if ( m_pRootNode->IsInitialized() )
            {
                m_pRootNode->Shutdown( m_graphContext );
            }

            m_pRootNode->Initialize( m_graphContext, SyncTrackTime() );
        }

        //-------------------------------------------------------------------------

        return m_pRootNode->Update( m_graphContext );
    }

    GraphPoseNodeResult GraphInstance::EvaluateGraph( Seconds const deltaTime, Transform const& startWorldTransform, Physics::Scene* pPhysicsScene, SyncTrackTimeRange const& updateRange, bool resetGraphState )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance: Evaluate Graph" );

        #if EE_DEVELOPMENT_TOOLS
        m_activeNodes.clear();
        m_rootMotionDebugger.StartCharacterUpdate( startWorldTransform );
        RecordGraphUpdateData( deltaTime, startWorldTransform );
        #endif

        //-------------------------------------------------------------------------

        if ( m_pTaskSystem != nullptr )
        {
            m_pTaskSystem->Reset();
        }

        m_graphContext.Update( deltaTime, startWorldTransform, pPhysicsScene );

        //-------------------------------------------------------------------------

        if ( resetGraphState || !m_pRootNode->IsInitialized() )
        {
            if ( m_pRootNode->IsInitialized() )
            {
                m_pRootNode->Shutdown( m_graphContext );
            }

            m_pRootNode->Initialize( m_graphContext, SyncTrackTime() );
        }

        //-------------------------------------------------------------------------

        return m_pRootNode->Update( m_graphContext, updateRange );
    }

    void GraphInstance::ExecutePrePhysicsPoseTasks( Transform const& endWorldTransform )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance: Pre-Physics Tasks" );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_rootMotionDebugger.EndCharacterUpdate( endWorldTransform );

        // Notify all connected external graphs
        for ( auto& externalGraph : m_externalGraphs )
        {
            externalGraph.m_pInstance->m_rootMotionDebugger.EndCharacterUpdate( endWorldTransform );
        }

        // Notify all child graphs
        for ( auto& childGraph : m_childGraphs )
        {
            if ( childGraph.m_pInstance != nullptr )
            {
                childGraph.m_pInstance->m_rootMotionDebugger.EndCharacterUpdate( endWorldTransform );
            }
        }
        #endif

        //-------------------------------------------------------------------------

        m_pTaskSystem->UpdatePrePhysics( m_graphContext.m_deltaTime, endWorldTransform, endWorldTransform.GetInverse() );
    }

    void GraphInstance::ExecutePostPhysicsPoseTasks()
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance: Post-Physics Tasks" );
        m_pTaskSystem->UpdatePostPhysics();
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    TaskSystemDebugMode GraphInstance::GetTaskSystemDebugMode() const
    {
        EE_ASSERT( m_pTaskSystem != nullptr );
        return m_pTaskSystem->GetDebugMode();
    }

    void GraphInstance::SetTaskSystemDebugMode( TaskSystemDebugMode mode )
    {
        EE_ASSERT( m_pTaskSystem != nullptr );
        m_pTaskSystem->SetDebugMode( mode );
    }

    Transform GraphInstance::GetTaskSystemDebugWorldTransform()
    {
        return m_pTaskSystem->GetCharacterWorldTransform();
    }

    void GraphInstance::GetChildGraphsForDebug( TVector<DebuggableChildGraph>& outChildGraphInstances, String const& pathPrefix ) const
    {
        for ( auto const& childGraph : m_childGraphs )
        {
            String const pathSoFar( String::CtorSprintf(), "%s/%s", pathPrefix.c_str(), m_pGraphVariation->GetDefinition()->m_nodePaths[childGraph.m_nodeIdx].c_str() );

            auto& debuggableGraph = outChildGraphInstances.emplace_back();
            debuggableGraph.m_pInstance = childGraph.m_pInstance;
            debuggableGraph.m_pathToInstance = pathSoFar;

            childGraph.m_pInstance->GetChildGraphsForDebug( outChildGraphInstances, pathSoFar );
        }
    }

    GraphInstance const* GraphInstance::GetChildGraphDebugInstance( int16_t nodeIdx ) const
    {
        for ( auto const& childGraph : m_childGraphs )
        {
            if ( childGraph.m_nodeIdx == nodeIdx )
            {
                return childGraph.m_pInstance;
            }
        }

        EE_UNREACHABLE_CODE();
        return nullptr;
    }

    GraphInstance const* GraphInstance::GetChildGraphDebugInstance( PointerID childGraphInstanceID ) const
    {
        TVector<DebuggableChildGraph> debuggableChildGraphs;
        GetChildGraphsForDebug( debuggableChildGraphs );

        for ( auto const& childGraph : m_childGraphs )
        {
            if ( PointerID( childGraph.m_pInstance ) == childGraphInstanceID )
            {
                return childGraph.m_pInstance;
            }
        }

        EE_UNREACHABLE_CODE();
        return nullptr;
    }

    GraphInstance const* GraphInstance::GetExternalGraphDebugInstance( StringID slotID ) const
    {
        int32_t const connectedGraphIdx = GetConnectedExternalGraphIndex( slotID );
        if ( connectedGraphIdx == InvalidIndex )
        {
            return nullptr;
        }

        return m_externalGraphs[connectedGraphIdx].m_pInstance;
    }

    void GraphInstance::DrawDebug( Drawing::DrawContext& drawContext )
    {
        // Graph
        //-------------------------------------------------------------------------

        if ( m_debugMode != GraphDebugMode::Off )
        {
            for ( auto graphNodeIdx : m_activeNodes )
            {
                // Filter nodes
                if ( !m_debugFilterNodes.empty() && !VectorContains( m_debugFilterNodes, graphNodeIdx ) )
                {
                    continue;
                }

                // Draw node debug
                auto pGraphNode = m_nodes[graphNodeIdx];
                if ( pGraphNode->GetValueType() == GraphValueType::Pose )
                {
                    auto pPoseNode = reinterpret_cast<PoseNode*>( pGraphNode );
                    pPoseNode->DrawDebug( m_graphContext, drawContext );
                }
            }
        }

        // Root Motion
        //-------------------------------------------------------------------------

        m_rootMotionDebugger.DrawDebug( drawContext );

        // Task System
        //-------------------------------------------------------------------------

        m_pTaskSystem->DrawDebug( drawContext );
    }

    //-------------------------------------------------------------------------

    void GraphInstance::StartRecording( RecordedGraphState& outState, GraphUpdateRecorder* pUpdateRecorder )
    {
        // Record initial state
        //-------------------------------------------------------------------------

        outState.m_graphID = GetDefinitionResourceID();
        outState.m_variationID = GetVariationID();
        outState.m_initializedNodeIndices.clear();

        for ( int16_t i = 0; i < m_nodes.size(); i++ )
        {
            if ( m_nodes[i]->IsInitialized() )
            {
                outState.m_initializedNodeIndices.emplace_back( i );
                m_nodes[i]->RecordGraphState( outState );
            }
        }

        // (Optional) Set the update recorder to track update data
        //-------------------------------------------------------------------------

        m_pUpdateRecorder = pUpdateRecorder;

        if ( m_pUpdateRecorder != nullptr )
        {
            m_pUpdateRecorder->m_graphID = GetDefinitionResourceID();
            m_pUpdateRecorder->m_variationID = GetVariationID();
        }
    }

    void GraphInstance::StopRecording()
    {
        m_pUpdateRecorder = nullptr;
    }

    void GraphInstance::RecordGraphUpdateData( Seconds const deltaTime, Transform const& startWorldTransform )
    {
        if ( m_pUpdateRecorder == nullptr )
        {
            return;
        }

        // Record time delta and world transform
        auto& frameData = m_pUpdateRecorder->m_recordedData.emplace_back();
        frameData.m_deltaTime = deltaTime;
        frameData.m_characterWorldTransform = startWorldTransform;

        // Record control parameter values
        for ( auto i = 0; i < GetNumControlParameters(); i++ )
        {
            auto pParameter = (ValueNode*) m_nodes[i];
            auto& paramData = frameData.m_parameterData.emplace_back();

            switch ( pParameter->GetValueType() )
            {
                case GraphValueType::Bool :
                {
                    paramData.m_bool = pParameter->GetValue<bool>( m_graphContext );
                }
                break;

                case GraphValueType::ID:
                {
                    paramData.m_ID = pParameter->GetValue<StringID>( m_graphContext );
                }
                break;

                case GraphValueType::Int:
                {
                    paramData.m_int = pParameter->GetValue<int32_t>( m_graphContext );
                }
                break;

                case GraphValueType::Float:
                {
                    paramData.m_float = pParameter->GetValue<float>( m_graphContext );
                }
                break;

                case GraphValueType::Vector:
                {
                    paramData.m_vector = pParameter->GetValue<Vector>( m_graphContext );
                }
                break;

                case GraphValueType::Target:
                {
                    paramData.m_target = pParameter->GetValue<Target>( m_graphContext );
                }
                break;

                default:
                EE_UNREACHABLE_CODE();
                break;
            }
        }
    }

    void GraphInstance::SetToRecordedState( RecordedGraphState const& recordedState )
    {
        EE_ASSERT( recordedState.m_graphID == GetDefinitionResourceID() );
        EE_ASSERT( recordedState.m_variationID == GetVariationID() );

        // Shutdown this graph if it was initialized
        if ( IsInitialized() )
        {
            m_pRootNode->Shutdown( m_graphContext );
        }

        //-------------------------------------------------------------------------

        // Set nodes array to the current instance array
        TVector<GraphNode*>* const pPreviousNodeArray = recordedState.m_pNodes;
        const_cast<TVector<GraphNode*>*&>( recordedState.m_pNodes ) = &m_nodes;

        // Restore the graph state
        for ( auto nodeIdx : recordedState.m_initializedNodeIndices )
        {
            m_nodes[nodeIdx]->RestoreGraphState( recordedState );
        }

        // Revert node array back to previous value
        const_cast<TVector<GraphNode*>*&>( recordedState.m_pNodes ) = pPreviousNodeArray;
    }

    void GraphInstance::SetRecordedUpdateData( RecordedGraphFrameData const& recordedUpdateData )
    {
        // Record control parameter values
        for ( auto i = 0; i < GetNumControlParameters(); i++ )
        {
            auto pParameter = (ValueNode*) m_nodes[i];
            auto const& paramData = recordedUpdateData.m_parameterData[i];

            switch ( pParameter->GetValueType() )
            {
                case GraphValueType::Bool:
                {
                    pParameter->SetValue<bool>( paramData.m_bool );
                }
                break;

                case GraphValueType::ID:
                {
                    pParameter->SetValue<StringID>( paramData.m_ID );
                }
                break;

                case GraphValueType::Int:
                {
                    pParameter->SetValue<int32_t>( paramData.m_int );
                }
                break;

                case GraphValueType::Float:
                {
                    pParameter->SetValue<float>( paramData.m_float );
                }
                break;

                case GraphValueType::Vector:
                {
                    pParameter->SetValue<Vector>( paramData.m_vector );
                }
                break;

                case GraphValueType::Target:
                {
                    pParameter->SetValue<Target>( paramData.m_target );
                }
                break;

                default:
                EE_UNREACHABLE_CODE();
                break;
            }
        }
    }
    #endif
}