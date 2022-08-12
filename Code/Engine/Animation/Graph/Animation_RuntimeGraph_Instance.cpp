#include "Animation_RuntimeGraph_Instance.h"
#include "Animation_RuntimeGraph_Node.h"
#include "Animation_RuntimeGraph_Contexts.h"
#include "Nodes/Animation_RuntimeGraphNode_ExternalGraph.h"
#include "System/Log.h"
#include "System/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphInstance::GraphInstance( GraphVariation const* pGraphVariation, uint64_t userID )
        : m_pGraphVariation( pGraphVariation )
        , m_userID( userID )
        , m_graphContext( userID, pGraphVariation->GetSkeleton() )
    {
        EE_ASSERT( pGraphVariation != nullptr );

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

        // Instantiate individual nodes
        //-------------------------------------------------------------------------

        for ( auto i = 0; i < numNodes; i++ )
        {
            pGraphDef->m_nodeSettings[i]->InstantiateNode( m_nodes, m_pGraphVariation->m_pDataSet.GetPtr(), GraphNode::Settings::InstantiationOptions::CreateNode );
        }

        // Set up debugging
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_graphContext.SetDebugSystems( &m_rootMotionDebugger, &m_activeNodes );
        m_activeNodes.reserve( 50 );
        #endif
    }

    GraphInstance::~GraphInstance()
    {
        EE_ASSERT( m_pRootNode == nullptr );
        EE_ASSERT( m_connectedExternalGraphs.empty() );

        //-------------------------------------------------------------------------

        for ( auto pNode : m_nodes )
        {
            pNode->~GraphNode();
        }

        EE::Free( m_pAllocatedInstanceMemory );
    }

    void GraphInstance::Initialize( TaskSystem* pTaskSystem )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance - Init" );

        // Initialize context
        m_graphContext.Initialize( pTaskSystem );
        EE_ASSERT( m_graphContext.IsValid() );

        // Initialize persistent graph nodes
        auto pGraphDef = m_pGraphVariation->m_pGraphDefinition.GetPtr();
        for ( auto nodeIdx : pGraphDef->m_persistentNodeIndices )
        {
            m_nodes[nodeIdx]->Initialize( m_graphContext );
        }

        // Set root node
        m_pRootNode = static_cast<PoseNode*>( m_nodes[pGraphDef->m_rootNodeIdx ] );
        EE_ASSERT( m_pRootNode->IsInitialized() );
    }

    void GraphInstance::Shutdown()
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance - Init" );
        EE_ASSERT( m_graphContext.IsValid() );

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
        int32_t const numConnectedGraphs = (int32_t) m_connectedExternalGraphs.size();
        for ( int32_t i = 0; i < numConnectedGraphs; i++ )
        {
            if ( m_connectedExternalGraphs[i].m_slotID == slotID )
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
        ConnectedExternalGraph& connectedGraph = m_connectedExternalGraphs.emplace_back();
        connectedGraph.m_slotID = slotID;
        connectedGraph.m_nodeIdx = m_pGraphVariation->m_pGraphDefinition->m_externalGraphSlots[slotIdx].m_nodeIdx;

        // Create graph instance
        //-------------------------------------------------------------------------

        connectedGraph.m_pInstance = EE::New<GraphInstance>( pExternalGraphVariation, m_userID );
        EE_ASSERT( connectedGraph.m_pInstance != nullptr );

        // The task system is inherited from the parent, everything else is independent
        connectedGraph.m_pInstance->Initialize( m_graphContext.m_pTaskSystem );

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

        ConnectedExternalGraph& connectedGraph = m_connectedExternalGraphs[connectedGraphIdx];

        // Detach from node
        //-------------------------------------------------------------------------

        auto pExternalGraphNode = static_cast<GraphNodes::ExternalGraphNode*> ( m_nodes[connectedGraph.m_nodeIdx] );
        pExternalGraphNode->DetachExternalGraphInstance( m_graphContext );

        // Destroy graph instance
        //-------------------------------------------------------------------------

        connectedGraph.m_pInstance->Shutdown();
        EE::Delete( connectedGraph.m_pInstance );

        // Remove record
        //-------------------------------------------------------------------------

        m_connectedExternalGraphs.erase_unsorted( m_connectedExternalGraphs.begin() + connectedGraphIdx );
    }

    //-------------------------------------------------------------------------

    void GraphInstance::ResetGraphState()
    {
        m_pRootNode->Shutdown( m_graphContext );
        m_pRootNode->Initialize( m_graphContext, SyncTrackTime() );
    }

    void GraphInstance::StartUpdate( Seconds const deltaTime, Transform const& startWorldTransform, Physics::Scene* pPhysicsScene )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance - Start Update" );

        #if EE_DEVELOPMENT_TOOLS
        m_activeNodes.clear();
        m_rootMotionDebugger.StartCharacterUpdate( startWorldTransform );
        #endif

        m_graphContext.Update( deltaTime, startWorldTransform, pPhysicsScene );

        // Notify all connected external graphs
        for ( auto& externalGraph : m_connectedExternalGraphs )
        {
            externalGraph.m_pInstance->StartUpdate( deltaTime, startWorldTransform, pPhysicsScene );
        }
    }

    GraphPoseNodeResult GraphInstance::UpdateGraph()
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance - Update Graph" );
        return m_pRootNode->Update( m_graphContext );
    }

    GraphPoseNodeResult GraphInstance::UpdateGraph( SyncTrackTimeRange const& updateRange )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance - Update Graph" );
        return m_pRootNode->Update( m_graphContext, updateRange );
    }

    void GraphInstance::EndUpdate( Transform const& endWorldTransform )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance - End Update" );

        #if EE_DEVELOPMENT_TOOLS
        m_rootMotionDebugger.EndCharacterUpdate( endWorldTransform );
        #endif

        // Notify all connected external graphs
        for ( auto& externalGraph : m_connectedExternalGraphs )
        {
            externalGraph.m_pInstance->EndUpdate( endWorldTransform );
        }
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    GraphInstance const* GraphInstance::GetExternalGraphDebugInstance( StringID slotID ) const
    {
        int32_t const connectedGraphIdx = GetConnectedExternalGraphIndex( slotID );
        if ( connectedGraphIdx == InvalidIndex )
        {
            return nullptr;
        }

        return m_connectedExternalGraphs[connectedGraphIdx].m_pInstance;
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
    }

    

    #endif
}