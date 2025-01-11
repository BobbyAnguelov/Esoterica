#include "Animation_RuntimeGraph_Instance.h"
#include "Animation_RuntimeGraph_Node.h"
#include "Nodes/Animation_RuntimeGraphNode_ExternalGraph.h"
#include "Nodes/Animation_RuntimeGraphNode_ReferencedGraph.h"
#include "Nodes/Animation_RuntimeGraphNode_Layers.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphInstance::GraphInstance( GraphDefinition const* pGraphDefinition, uint64_t ownerID, TaskSystem* pTaskSystem, SampledEventsBuffer* pSampledEventsBuffer, RootMotionDebugger* pRootMotionDebugger )
        : m_pGraphDefinition( pGraphDefinition )
        , m_ownerID( ownerID )
        , m_graphContext( ownerID, pGraphDefinition->GetPrimarySkeleton() )
    {
        EE_ASSERT( pGraphDefinition != nullptr );

        //-------------------------------------------------------------------------

        TaskSystem* pFinalTaskSystem = pTaskSystem;
        SampledEventsBuffer* pFinalSampledEventsBuffer = pSampledEventsBuffer;
        RootMotionDebugger* pFinalRootMotionDebugger = pRootMotionDebugger;

        // If the supplied task system is null, then this is a standalone graph instance
        m_isStandaloneGraph = ( pTaskSystem == nullptr );
        if ( m_isStandaloneGraph )
        {
            m_pTaskSystem = EE::New<TaskSystem>( m_pGraphDefinition->GetPrimarySkeleton() );
            m_pSampledEventsBuffer = EE::New<SampledEventsBuffer>();

            #if EE_DEVELOPMENT_TOOLS
            m_pRootMotionDebugger = EE::New<RootMotionDebugger>();
            #endif

            //-------------------------------------------------------------------------

            pFinalTaskSystem = m_pTaskSystem;
            pFinalSampledEventsBuffer = m_pSampledEventsBuffer;
            
            #if EE_DEVELOPMENT_TOOLS
            pFinalRootMotionDebugger = m_pRootMotionDebugger;
            #endif
        }

        EE_ASSERT( pFinalTaskSystem != nullptr );
        EE_ASSERT( pFinalSampledEventsBuffer != nullptr );

        #if EE_DEVELOPMENT_TOOLS
        EE_ASSERT( pFinalRootMotionDebugger != nullptr );
        #endif

        // Allocate memory
        //-------------------------------------------------------------------------

        size_t const numNodes = m_pGraphDefinition->m_instanceNodeStartOffsets.size();
        EE_ASSERT( m_pGraphDefinition->m_nodeDefinitions.size() == numNodes );

        m_pAllocatedInstanceMemory = reinterpret_cast<uint8_t*>( EE::Alloc( m_pGraphDefinition->m_instanceRequiredMemory, m_pGraphDefinition->m_instanceRequiredAlignment ) );

        m_nodes.reserve( numNodes );

        for ( auto const& nodeOffset : m_pGraphDefinition->m_instanceNodeStartOffsets )
        {
            // Create ptrs to the future nodes!
            // The nodes are not actually created yet but the ptrs are valid
            m_nodes.emplace_back( reinterpret_cast<GraphNode*>( m_pAllocatedInstanceMemory + nodeOffset ) );
        }

        // Create referenced graph instances
        //-------------------------------------------------------------------------

        size_t const numReferencedGraphs = m_pGraphDefinition->m_referencedGraphSlots.size();
        m_referencedGraphs.reserve( numReferencedGraphs );

        TInlineVector<GraphInstance*, 20> createdReferencedGraphInstances;
        createdReferencedGraphInstances.reserve( numReferencedGraphs );

        for ( auto const& referencedGraphSlot : m_pGraphDefinition->m_referencedGraphSlots )
        {
            GraphDefinition const* pReferencedGraphDefinition = m_pGraphDefinition->GetResource<GraphDefinition>( referencedGraphSlot.m_dataSlotIdx );
            if ( pReferencedGraphDefinition != nullptr && pReferencedGraphDefinition->IsValid() )
            {
                if ( pReferencedGraphDefinition->GetPrimarySkeleton() == pGraphDefinition->GetPrimarySkeleton() )
                {
                    ReferencedGraph cg;
                    cg.m_nodeIdx = referencedGraphSlot.m_nodeIdx;
                    cg.m_pInstance = EE::New<GraphInstance>( pReferencedGraphDefinition, m_ownerID, pFinalTaskSystem, pFinalSampledEventsBuffer, pFinalRootMotionDebugger );
                    m_referencedGraphs.emplace_back( cg );

                    createdReferencedGraphInstances.emplace_back( cg.m_pInstance );
                }
                else
                {
                    createdReferencedGraphInstances.emplace_back( nullptr );
                    EE_LOG_ERROR( "Animation", "Graph Instance", "Different skeleton for referenced graph detected, this is not allowed. Trying to use '%s' within '%s'", pReferencedGraphDefinition->GetResourceID().c_str(), pGraphDefinition->GetResourceID().c_str() );
                }
            }
            else
            {
                createdReferencedGraphInstances.emplace_back( nullptr );
                m_referencedGraphs.emplace_back( ReferencedGraph() );
            }
        }

        // Instantiate individual nodes
        //-------------------------------------------------------------------------

        InstantiationContext instantiationContext = { (int16_t) InvalidIndex, m_nodes, createdReferencedGraphInstances, m_pGraphDefinition->m_skeleton.GetPtr(), m_pGraphDefinition->m_parameterLookupMap, &m_pGraphDefinition->m_resources, m_ownerID };

        #if EE_DEVELOPMENT_TOOLS
        instantiationContext.m_pLog = &m_log;
        #endif

        for ( int16_t i = 0; i < numNodes; i++ )
        {
            instantiationContext.m_currentNodeIdx = i;
            m_pGraphDefinition->m_nodeDefinitions[i]->InstantiateNode( instantiationContext, InstantiationOptions::CreateNode );
        }

        // Set up graph context
        //-------------------------------------------------------------------------

        // Initialize context
        m_graphContext.Initialize( pFinalTaskSystem, pFinalSampledEventsBuffer );
        EE_ASSERT( m_graphContext.IsValid() );

        #if EE_DEVELOPMENT_TOOLS
        m_graphContext.SetDebugSystems( pFinalRootMotionDebugger, &m_activeNodes, &m_log );
        m_activeNodes.reserve( 50 );
        #endif

        // Initialize graph nodes
        //-------------------------------------------------------------------------

        // Initialize persistent graph nodes
        for ( auto nodeIdx : m_pGraphDefinition->m_persistentNodeIndices )
        {
            m_nodes[nodeIdx]->Initialize( m_graphContext );
        }

        // Set root node
        m_pRootNode = reinterpret_cast<PoseNode*>( m_nodes[m_pGraphDefinition->m_rootNodeIdx] );
        EE_ASSERT( !m_pRootNode->IsInitialized() );

        // Resource Mappings needed for serialization
        //-------------------------------------------------------------------------

        if ( m_isStandaloneGraph )
        {
            GenerateResourceMappings();
        }
    }

    GraphInstance::~GraphInstance()
    {
        // Ensure we dont have any connected external graphs
        EE_ASSERT( m_externalGraphs.empty() );

        // Shutdown persistent graph nodes
        for ( int16_t nodeIdx : m_pGraphDefinition->m_persistentNodeIndices )
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
        for ( GraphNode* pNode : m_nodes )
        {
            pNode->~GraphNode();
        }
        m_nodes.clear();

        // Destroy referenced graph instances
        for ( ReferencedGraph& referencedGraph : m_referencedGraphs )
        {
            if ( referencedGraph.m_pInstance != nullptr )
            {
                EE::Delete( referencedGraph.m_pInstance );
            }
        }
        m_referencedGraphs.clear();

        EE::Free( m_pAllocatedInstanceMemory );
        EE::Delete( m_pTaskSystem );
        EE::Delete( m_pSampledEventsBuffer );

        #if EE_DEVELOPMENT_TOOLS
        EE::Delete( m_pRootMotionDebugger );
        #endif
    }

    //-------------------------------------------------------------------------

    void GraphInstance::GetResourceLookupTables( TInlineVector<ResourceLUT const*, 10>& LUTs ) const
    {
        LUTs.emplace_back( &m_pGraphDefinition->m_resourceLUT );

        for ( auto const& externalGraph : m_externalGraphs )
        {
            externalGraph.m_pInstance->GetResourceLookupTables( LUTs );
        }
    }

    void GraphInstance::GenerateResourceMappings()
    {
        m_resourceMappings.Clear();
        TInlineVector<ResourceLUT const*, 10> LUTs;
        GetResourceLookupTables( LUTs );
        m_resourceMappings.Generate( LUTs );
    }

    bool GraphInstance::DoesTaskSystemNeedUpdate() const
    {
        EE_ASSERT( m_isStandaloneGraph );
        return m_pTaskSystem->RequiresUpdate();
    }

    void GraphInstance::SerializeTaskList( Blob& outBlob ) const
    {
        EE_ASSERT( !DoesTaskSystemNeedUpdate() );
        m_pTaskSystem->SerializeTasks( m_resourceMappings, outBlob );
    }

    //-------------------------------------------------------------------------

    int32_t GraphInstance::GetExternalGraphSlotIndex( StringID slotID ) const
    {
        int32_t const numSlots = (int32_t) m_pGraphDefinition->m_externalGraphSlots.size();
        for ( int32_t i = 0; i < numSlots; i++ )
        {
            auto const& graphSlot = m_pGraphDefinition->m_externalGraphSlots[i];
            if ( graphSlot.m_slotID == slotID )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    int16_t GraphInstance::GetExternalGraphNodeIndex( StringID slotID ) const
    {
        int32_t const numSlots = (int32_t) m_pGraphDefinition->m_externalGraphSlots.size();
        for ( int32_t i = 0; i < numSlots; i++ )
        {
            auto const& graphSlot = m_pGraphDefinition->m_externalGraphSlots[i];
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

    GraphInstance* GraphInstance::ConnectExternalGraph( StringID slotID, GraphDefinition const* pExternalGraphDefinition )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance - Connect External Graph" );

        if ( pExternalGraphDefinition->GetPrimarySkeleton() != m_pGraphDefinition->GetPrimarySkeleton() )
        {
            EE_LOG_ERROR( "Animation", "Graph Instance", "Cannot insert external graph (%s) since skeletons dont match! Expected: %s, supplied: %s", pExternalGraphDefinition->GetResourceID().c_str(), m_pGraphDefinition->GetPrimarySkeleton()->GetResourceID().c_str(), pExternalGraphDefinition->GetPrimarySkeleton()->GetResourceID().c_str() );
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
        connectedGraph.m_nodeIdx = m_pGraphDefinition->m_externalGraphSlots[slotIdx].m_nodeIdx;

        // Create graph instance
        //-------------------------------------------------------------------------

        connectedGraph.m_pInstance = new ( EE::Alloc( sizeof( GraphInstance ) ) ) GraphInstance( pExternalGraphDefinition, m_ownerID, m_pTaskSystem, m_pSampledEventsBuffer );
        EE_ASSERT( connectedGraph.m_pInstance != nullptr );

        // Attach instance to the node
        //-------------------------------------------------------------------------

        auto pExternalGraphNode = reinterpret_cast<ExternalGraphNode*> ( m_nodes[connectedGraph.m_nodeIdx] );
        pExternalGraphNode->AttachExternalGraphInstance( m_graphContext, connectedGraph.m_pInstance );

        // Update resource mappings
        //-------------------------------------------------------------------------

        // TODO: re-evaluate this whole thing with the context of referenced graphs and how we ensure that the resource mappings are 100% consistent between client and server. 
        GenerateResourceMappings();

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

        auto pExternalGraphNode = reinterpret_cast<ExternalGraphNode*> ( m_nodes[connectedGraph.m_nodeIdx] );
        pExternalGraphNode->DetachExternalGraphInstance( m_graphContext );

        connectedGraph.m_pInstance->~GraphInstance();
        EE::Free( connectedGraph.m_pInstance );
        connectedGraph.m_pInstance = nullptr;

        // Remove record
        //-------------------------------------------------------------------------

        m_externalGraphs.erase_unsorted( m_externalGraphs.begin() + connectedGraphIdx );

        // Update resource mappings
        //-------------------------------------------------------------------------

        GenerateResourceMappings();
    }

    //-------------------------------------------------------------------------

    void GraphInstance::ResetGraphState( SyncTrackTime initTime, TVector<GraphLayerUpdateState> const* pLayerInitInfo )
    {
        EE_ASSERT( initTime.m_percentageThrough >= 0.0f && initTime.m_percentageThrough <= 1.0f );

        if ( m_pRootNode->IsInitialized() )
        {
            m_pRootNode->Shutdown( m_graphContext );
        }

        m_graphContext.m_updateID++; // Bump the update ID to ensure that any initialization code that relies on it is dirtied.
        m_graphContext.m_pLayerInitializationInfo = pLayerInitInfo;
        m_pRootNode->Initialize( m_graphContext, initTime );
        m_graphContext.m_pLayerInitializationInfo = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        m_log.clear();
        #endif
    }

    GraphPoseNodeResult GraphInstance::EvaluateGraph( Seconds const deltaTime, Transform const& startWorldTransform, Physics::PhysicsWorld* pPhysicsWorld, SyncTrackTimeRange const* pUpdateRange, bool resetGraphState )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance: Evaluate Graph" );

        #if EE_DEVELOPMENT_TOOLS
        m_activeNodes.clear();
        RecordPreGraphEvaluateState( deltaTime, startWorldTransform );
        #endif

        //-------------------------------------------------------------------------

        if ( m_isStandaloneGraph )
        {
            m_pTaskSystem->Reset();
            m_pSampledEventsBuffer->Clear();

            #if EE_DEVELOPMENT_TOOLS
            m_pRootMotionDebugger->StartCharacterUpdate( startWorldTransform );
            #endif
        }

        m_graphContext.Update( deltaTime, startWorldTransform, pPhysicsWorld );

        //-------------------------------------------------------------------------

        if ( resetGraphState || !m_pRootNode->IsInitialized() )
        {
            ResetGraphState();
        }

        //-------------------------------------------------------------------------

        auto result = m_pRootNode->Update( m_graphContext, pUpdateRange );

        #if EE_DEVELOPMENT_TOOLS
        RecordPostGraphEvaluateState( nullptr );

        if ( m_isStandaloneGraph )
        {
            EE_ASSERT( !m_pSampledEventsBuffer->HasDebugBasePathSet() );
            EE_ASSERT( !m_pTaskSystem->HasDebugBasePathSet() );
            EE_ASSERT( !m_pRootMotionDebugger->HasDebugBasePathSet() );
        }
        #endif

        return result;
    }

    GraphPoseNodeResult GraphInstance::EvaluateReferencedGraph( Seconds const deltaTime, Transform const& startWorldTransform, Physics::PhysicsWorld* pPhysicsWorld, SyncTrackTimeRange const* pUpdateRange, GraphLayerContext* pLayerContext )
    {
        m_graphContext.m_pLayerContext = pLayerContext;
        GraphPoseNodeResult result = EvaluateGraph( deltaTime, startWorldTransform, pPhysicsWorld, pUpdateRange );
        m_graphContext.m_pLayerContext = nullptr;

        return result;
    }

    void GraphInstance::ExecutePrePhysicsPoseTasks( Transform const& endWorldTransform )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance: Pre-Physics Tasks" );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_pRootMotionDebugger->EndCharacterUpdate( endWorldTransform );

        // Notify all connected external graphs
        for ( auto& externalGraph : m_externalGraphs )
        {
            externalGraph.m_pInstance->m_pRootMotionDebugger->EndCharacterUpdate( endWorldTransform );
        }
        #endif

        //-------------------------------------------------------------------------

        m_pTaskSystem->UpdatePrePhysics( m_graphContext.m_deltaTime, endWorldTransform, endWorldTransform.GetInverse() );
    }

    void GraphInstance::ExecutePostPhysicsPoseTasks()
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance: Post-Physics Tasks" );
        m_pTaskSystem->UpdatePostPhysics();

        #if EE_DEVELOPMENT_TOOLS
        RecordTasks();
        #endif
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void GraphInstance::GetUpdateStateForActiveLayers( TVector<GraphLayerUpdateState>& outRanges )
    {
        for ( auto activeNodeIdx : m_activeNodes )
        {
            auto pNodeDefinition = m_pGraphDefinition->m_nodeDefinitions[activeNodeIdx];
            if ( IsOfType<LayerBlendNode::Definition>( pNodeDefinition ) )
            {
                auto& layerUpdateRange = outRanges.emplace_back();
                layerUpdateRange.m_nodeIdx = activeNodeIdx;

                auto pNode = static_cast<LayerBlendNode*>( m_nodes[activeNodeIdx] );
                pNode->GetSyncUpdateRangesForUnsynchronizedLayers( layerUpdateRange.m_updateRanges );
            }
        }
    }

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

    void GraphInstance::GetReferencedGraphsForDebug( TVector<DebuggableReferencedGraph>& outReferencedGraphInstances, DebugPath const& parentPath ) const
    {
        for ( auto const& referencedGraph : m_referencedGraphs )
        {
            DebugPath currentPath = parentPath;
            currentPath.m_path.emplace_back( referencedGraph.m_nodeIdx, m_pGraphDefinition->m_nodePaths[referencedGraph.m_nodeIdx].c_str() );

            auto& debuggableGraph = outReferencedGraphInstances.emplace_back();
            debuggableGraph.m_pInstance = referencedGraph.m_pInstance;
            debuggableGraph.m_path = currentPath;

            //-------------------------------------------------------------------------

            referencedGraph.m_pInstance->GetReferencedGraphsForDebug( outReferencedGraphInstances, currentPath );
        }
    }

    GraphInstance const* GraphInstance::GetReferencedGraphDebugInstance( int16_t nodeIdx ) const
    {
        for ( auto const& referencedGraph : m_referencedGraphs )
        {
            if ( referencedGraph.m_nodeIdx == nodeIdx )
            {
                return referencedGraph.m_pInstance;
            }
        }

        EE_UNREACHABLE_CODE();
        return nullptr;
    }

    GraphInstance const* GraphInstance::GetReferencedGraphDebugInstance( PointerID referencedGraphInstanceID ) const
    {
        TVector<DebuggableReferencedGraph> debuggableReferencedGraphs;
        GetReferencedGraphsForDebug( debuggableReferencedGraphs );

        for ( auto const& referencedGraph : debuggableReferencedGraphs )
        {
            if ( PointerID( referencedGraph.m_pInstance ) == referencedGraphInstanceID )
            {
                return referencedGraph.m_pInstance;
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

    GraphInstance const* GraphInstance::GetReferencedOrExternalGraphDebugInstance( int16_t nodeIdx ) const
    {
        EE_ASSERT( IsValidNodeIndex( nodeIdx ) );

        for ( auto const& referencedGraphRecord : m_referencedGraphs )
        {
            if ( referencedGraphRecord.m_nodeIdx == nodeIdx )
            {
                return referencedGraphRecord.m_pInstance;
            }
        }

        for ( auto const& externalGraphRecord : m_externalGraphs )
        {
            if ( externalGraphRecord.m_nodeIdx == nodeIdx )
            {
                return externalGraphRecord.m_pInstance;
            }
        }

        return nullptr;
    }

    DebugPath GraphInstance::GetDebugPathForReferencedGraphInstance( PointerID instanceID ) const
    {
        TVector<DebuggableReferencedGraph> debuggableReferencedGraphs;
        GetReferencedGraphsForDebug( debuggableReferencedGraphs );

        for ( auto const& referencedGraph : debuggableReferencedGraphs )
        {
            if ( PointerID( referencedGraph.m_pInstance ) == instanceID )
            {
                return referencedGraph.m_path;
            }
        }

        return DebugPath();
    }

    DebugPath GraphInstance::GetDebugPathForReferencedOrExternalGraphDebugInstance( PointerID instanceID ) const
    {
        TVector<DebuggableReferencedGraph> debuggableReferencedGraphs;
        GetReferencedGraphsForDebug( debuggableReferencedGraphs );

        for ( auto const& referencedGraph : debuggableReferencedGraphs )
        {
            if ( PointerID( referencedGraph.m_pInstance ) == instanceID )
            {
                return referencedGraph.m_path;
            }
        }

        //-------------------------------------------------------------------------

        for ( auto const& externalGraphRecord : m_externalGraphs )
        {
            if ( PointerID( externalGraphRecord.m_pInstance ) == instanceID )
            {
                DebugPath path;
                path.m_path.emplace_back( externalGraphRecord.m_nodeIdx, m_pGraphDefinition->m_nodePaths[externalGraphRecord.m_nodeIdx].c_str() );
                return path;
            }
        }

        //-------------------------------------------------------------------------

        return DebugPath();
    }

    DebugPath GraphInstance::ResolveSourcePath( TInlineVector<int64_t, 5> const& sourcePath ) const
    {
        EE_ASSERT( IsStandaloneInstance() );

        //-------------------------------------------------------------------------

        DebugPath debugPath;

        GraphInstance const* pFinalGraphInstance = this;
        int32_t const numElements = (int32_t) sourcePath.size();
        for ( int32_t i = 0; i < numElements; i++ )
        {
            auto& element = debugPath.m_path.emplace_back();

            // Resolve node path
            if ( i == ( numElements - 1 ) )
            {
                element.m_itemID = sourcePath[i];
                element.m_pathString = pFinalGraphInstance->m_pGraphDefinition->GetNodePath( (int16_t) sourcePath[i] ).c_str();
            }
            else // Resolve to referenced or external graph
            {
                element.m_itemID = sourcePath[i];
                element.m_pathString = pFinalGraphInstance->m_pGraphDefinition->GetNodePath( (int16_t) sourcePath[i] ).c_str();
                pFinalGraphInstance = pFinalGraphInstance->GetReferencedOrExternalGraphDebugInstance( (int16_t) sourcePath[i] );
            }

            // This should only ever happen if we detach an external graph directly after updating a graph instance and before drawing this debug display
            if ( pFinalGraphInstance == nullptr )
            {
                debugPath.m_path.clear();
                return debugPath;
            }
        }

        return debugPath;
    }

    void GraphInstance::DrawNodeDebug( GraphContext& graphContext, Drawing::DrawContext& drawContext )
    {
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
    }

    void GraphInstance::DrawDebug( Drawing::DrawContext& drawContext )
    {
        // Graph
        //-------------------------------------------------------------------------

        DrawNodeDebug( m_graphContext, drawContext );

        // Root Motion
        //-------------------------------------------------------------------------

        m_pRootMotionDebugger->DrawDebug( drawContext );

        // Task System
        //-------------------------------------------------------------------------

        m_pTaskSystem->DrawDebug( drawContext );
    }

    void GraphInstance::OutputLog()
    {
        for ( m_lastOutputtedLogItemIdx; m_lastOutputtedLogItemIdx < m_log.size(); m_lastOutputtedLogItemIdx++ )
        {
            auto const& item = m_log[m_lastOutputtedLogItemIdx];
            InlineString const source( InlineString::CtorSprintf(), "%s (%d)", m_pGraphDefinition->GetNodePath( item.m_nodeIdx ).c_str(), item.m_nodeIdx );
            SystemLog::AddEntry( item.m_severity, "Animation", source.c_str(), "", 0, item.m_message.c_str() );
        }
    }

    //-------------------------------------------------------------------------

    void GraphInstance::StartRecording( GraphRecorder* pRecorder )
    {
        EE_ASSERT( pRecorder != nullptr );
        EE_ASSERT( m_pRecorder == nullptr );

        // Record initial state
        //-------------------------------------------------------------------------

        RecordGraphState( pRecorder->m_initialState );

        // (Optional) Set the update recorder to track update data
        //-------------------------------------------------------------------------

        m_pRecorder = pRecorder;

        if ( m_pRecorder != nullptr )
        {
            m_pRecorder->m_graphID = m_pGraphDefinition->GetResourceID();
            m_pRecorder->m_variationID = m_pGraphDefinition->m_variationID;
            m_pRecorder->m_recordedResourceHash = GetGraphDefinition()->GetSourceResourceHash();
        }
    }

    void GraphInstance::RecordGraphState( RecordedGraphState& recordedState )
    {
        recordedState.m_graphID = m_pGraphDefinition->GetResourceID();
        recordedState.m_variationID = m_pGraphDefinition->m_variationID;
        recordedState.m_recordedResourceHash = GetGraphDefinition()->GetSourceResourceHash();
        recordedState.m_initializedNodeIndices.clear();

        for ( int16_t i = 0; i < m_nodes.size(); i++ )
        {
            if ( m_nodes[i]->IsInitialized() )
            {
                recordedState.m_initializedNodeIndices.emplace_back( i );
                m_nodes[i]->RecordGraphState( recordedState );
            }
        }
    }

    void GraphInstance::StopRecording()
    {
        EE_ASSERT( m_pRecorder != nullptr );
        m_pRecorder = nullptr;
    }

    void GraphInstance::RecordPreGraphEvaluateState( Seconds const deltaTime, Transform const& startWorldTransform )
    {
        if ( m_pRecorder == nullptr )
        {
            return;
        }

        // Record time delta and world transform
        auto& frameData = m_pRecorder->m_recordedData.emplace_back();
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

                case GraphValueType::Float:
                {
                    paramData.m_float = pParameter->GetValue<float>( m_graphContext );
                }
                break;

                case GraphValueType::Vector:
                {
                    paramData.m_vector = pParameter->GetValue<Float3>( m_graphContext );
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

        // Calculate sync start time
        frameData.m_updateRange.m_startTime = m_pRootNode->GetSyncTrack().GetTime( m_pRootNode->GetCurrentTime() );
    }

    void GraphInstance::RecordPostGraphEvaluateState( SyncTrackTimeRange const* pRange )
    {
        if ( m_pRecorder == nullptr )
        {
            return;
        }

        auto& frameData = m_pRecorder->m_recordedData.back();

        // Record the global update range info
        //-------------------------------------------------------------------------

        if ( pRange == nullptr )
        {
            frameData.m_updateRange.m_endTime = m_pRootNode->GetSyncTrack().GetTime( m_pRootNode->GetCurrentTime() );
        }
        else // Directly overwrite the sync update
        {
            frameData.m_updateRange = *pRange;
        }

        // Record the layer update range info 
        //-------------------------------------------------------------------------

        GetUpdateStateForActiveLayers( frameData.m_layerUpdateStates );
    }

    void GraphInstance::SetToRecordedState( RecordedGraphState const& recordedState )
    {
        EE_ASSERT( !IsRecording() );
        EE_ASSERT( recordedState.m_graphID == m_pGraphDefinition->GetResourceID() );
        EE_ASSERT( recordedState.m_variationID == m_pGraphDefinition->m_variationID );

        // Shutdown this graph if it was initialized
        if ( WasInitialized() )
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

        #if EE_DEVELOPMENT_TOOLS
        if ( m_pRootMotionDebugger != nullptr )
        {
            m_pRootMotionDebugger->ResetRecordedPositions();
        }
        #endif
    }

    void GraphInstance::SetRecordedFrameUpdateData( RecordedGraphFrameData const& recordedUpdateData )
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

                case GraphValueType::Float:
                {
                    pParameter->SetValue<float>( paramData.m_float );
                }
                break;

                case GraphValueType::Vector:
                {
                    pParameter->SetValue<Float3>( paramData.m_vector );
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

    void GraphInstance::RecordTasks()
    {
        if ( m_pRecorder == nullptr )
        {
            return;
        }

        // All the resources in use by this graph instance
        TInlineVector<ResourceLUT const*, 10> LUTs;
        GetResourceLookupTables( LUTs );

        // Record task
        auto& frameData = m_pRecorder->m_recordedData.back();
        m_pTaskSystem->SerializeTasks( LUTs, frameData.m_serializedTaskData );
    }
    #endif
}