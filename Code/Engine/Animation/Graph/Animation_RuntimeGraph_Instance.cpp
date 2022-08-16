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
        bool const isStandaloneGraphInstance = pTaskSystem == nullptr;
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
            auto pChildGraphVariation = m_pGraphVariation->m_pDataSet->GetResource<GraphVariation>( childGraphSlot.m_dataSlotIdx );
            if ( pChildGraphVariation != nullptr )
            {
                if ( pChildGraphVariation->GetSkeleton() == pGraphVariation->GetSkeleton() )
                {
                    ChildGraph cg;
                    cg.m_nodeIdx = childGraphSlot.m_nodeIdx;
                    cg.m_pInstance = new ( EE::Alloc( sizeof( GraphInstance ) ) ) GraphInstance( pChildGraphVariation, m_userID, m_pTaskSystem );
                    m_childGraphs.emplace_back( cg );

                    createdChildGraphInstances.emplace_back( cg.m_pInstance );
                }
                else
                {
                    EE_LOG_ERROR( "Animation", "Graph Instance", "Different skeleton for child graph detected, this is not allowed. Trying to use '%s' within '%s'", pChildGraphVariation->GetResourceID().c_str(), pGraphVariation->GetResourceID().c_str() );
                }
            }
            else
            {
                m_childGraphs.emplace_back( ChildGraph() );
            }
        }

        // Instantiate individual nodes
        //-------------------------------------------------------------------------

        InstantiationContext context = { m_nodes, createdChildGraphInstances, m_pGraphVariation->m_pGraphDefinition->m_parameterLookupMap, m_pGraphVariation->m_pDataSet.GetPtr(), m_userID };

        for ( auto i = 0; i < numNodes; i++ )
        {
            pGraphDef->m_nodeSettings[i]->InstantiateNode( context, InstantiationOptions::CreateNode );
        }

        // Set up graph context
        //-------------------------------------------------------------------------

        // Initialize context
        m_graphContext.Initialize( isStandaloneGraphInstance ? m_pTaskSystem : pTaskSystem );
        EE_ASSERT( m_graphContext.IsValid() );

        #if EE_DEVELOPMENT_TOOLS
        m_graphContext.SetDebugSystems( &m_rootMotionDebugger, &m_activeNodes );
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
        m_pRootNode = static_cast<PoseNode*>( m_nodes[pGraphDef->m_rootNodeIdx] );
        EE_ASSERT( m_pRootNode->IsInitialized() );
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

        // Clear root node
        EE_ASSERT( !m_pRootNode->IsInitialized() );
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
            childGraph.m_pInstance->~GraphInstance();
            EE::Free( childGraph.m_pInstance );
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

        auto pExternalGraphNode = static_cast<GraphNodes::ExternalGraphNode*> ( m_nodes[connectedGraph.m_nodeIdx] );
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

        auto pExternalGraphNode = static_cast<GraphNodes::ExternalGraphNode*> ( m_nodes[connectedGraph.m_nodeIdx] );
        pExternalGraphNode->DetachExternalGraphInstance( m_graphContext );

        connectedGraph.m_pInstance->~GraphInstance();
        EE::Free( connectedGraph.m_pInstance );
        connectedGraph.m_pInstance = nullptr;

        // Remove record
        //-------------------------------------------------------------------------

        m_externalGraphs.erase_unsorted( m_externalGraphs.begin() + connectedGraphIdx );
    }

    //-------------------------------------------------------------------------

    void GraphInstance::ResetGraphState()
    {
        if ( m_pRootNode->IsInitialized() )
        {
            m_pRootNode->Shutdown( m_graphContext );
        }

        m_pRootNode->Initialize( m_graphContext, SyncTrackTime() );
    }

    GraphPoseNodeResult GraphInstance::EvaluateGraph( Seconds const deltaTime, Transform const& startWorldTransform, Physics::Scene* pPhysicsScene )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance: Evaluate Graph" );
        #if EE_DEVELOPMENT_TOOLS
        m_activeNodes.clear();
        m_rootMotionDebugger.StartCharacterUpdate( startWorldTransform );
        #endif

        //-------------------------------------------------------------------------

        if ( m_pTaskSystem != nullptr )
        {
            m_pTaskSystem->Reset();
        }

        m_graphContext.Update( deltaTime, startWorldTransform, pPhysicsScene );
        return m_pRootNode->Update( m_graphContext );
    }

    GraphPoseNodeResult GraphInstance::EvaluateGraph( Seconds const deltaTime, Transform const& startWorldTransform, Physics::Scene* pPhysicsScene, SyncTrackTimeRange const& updateRange )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance: Evaluate Graph" );

        #if EE_DEVELOPMENT_TOOLS
        m_activeNodes.clear();
        m_rootMotionDebugger.StartCharacterUpdate( startWorldTransform );
        #endif

        //-------------------------------------------------------------------------

        if ( m_pTaskSystem != nullptr )
        {
            m_pTaskSystem->Reset();
        }

        m_graphContext.Update( deltaTime, startWorldTransform, pPhysicsScene );
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
            childGraph.m_pInstance->m_rootMotionDebugger.EndCharacterUpdate( endWorldTransform );
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

    GraphInstance const* GraphInstance::GetChildGraphDebugInstance( int16_t nodeIdx ) const
    {
        for ( auto const& childGraph : m_childGraphs )
        {
            if ( childGraph.m_nodeIdx == nodeIdx )
            {
                return childGraph.m_pInstance;
            }
        }

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
                    auto pPoseNode = static_cast<PoseNode*>( pGraphNode );
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
    #endif
}