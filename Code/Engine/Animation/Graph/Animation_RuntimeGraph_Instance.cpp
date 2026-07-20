#include "Animation_RuntimeGraph_Instance.h"
#include "Animation_RuntimeGraph_Node.h"
#include "Nodes/Animation_RuntimeGraphNode_ReferencedGraph.h"
#include "Nodes/Animation_RuntimeGraphNode_Layers.h"
#include "Nodes/Animation_RuntimeGraphNode_ExternalPose.h"
#include "Base/Profiling.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphInstance::GraphInstance( GraphDefinition const* pGraphDefinition, GraphContext *pParentContext )
        : m_pGraphDefinition( pGraphDefinition )
        , m_isStandaloneGraphInstance( pParentContext == nullptr )
        , m_graphContext( pGraphDefinition->GetPrimarySkeleton() )
    {
        EE_ASSERT( pGraphDefinition != nullptr );

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

        // Set up graph context
        //-------------------------------------------------------------------------

        m_graphContext.Initialize( pParentContext );
        EE_ASSERT( m_graphContext.IsValid() );

        // Create referenced graph instances
        //-------------------------------------------------------------------------

        size_t const numReferencedGraphs = m_pGraphDefinition->m_internalGraphSlots.size();

        TInlineVector<GraphInstance*, 20> createdReferencedGraphInstances;
        createdReferencedGraphInstances.reserve( numReferencedGraphs );

        for ( auto const& referencedGraphSlot : m_pGraphDefinition->m_internalGraphSlots )
        {
            GraphDefinition const* pReferencedGraphDefinition = m_pGraphDefinition->GetResourceForSlot<GraphDefinition>( referencedGraphSlot.m_dataSlotIdx );
            if ( pReferencedGraphDefinition != nullptr && pReferencedGraphDefinition->IsValid() )
            {
                if ( pReferencedGraphDefinition->GetPrimarySkeleton() == pGraphDefinition->GetPrimarySkeleton() )
                {
                    m_graphContext.PushBasePath( referencedGraphSlot.m_nodeIdx );
                    auto pReferencedGraphInstance = EE::New<GraphInstance>( pReferencedGraphDefinition, &m_graphContext );
                    m_graphContext.PopBasePath();
                    createdReferencedGraphInstances.emplace_back( pReferencedGraphInstance );
                }
                else
                {
                    createdReferencedGraphInstances.emplace_back( nullptr );
                    EE_LOG_ERROR( LogCategory::Animation, "Graph Instance", "Different skeleton for referenced graph detected, this is not allowed. Trying to use '%s' within '%s'", pReferencedGraphDefinition->GetResourceID().c_str(), pGraphDefinition->GetResourceID().c_str() );
                }
            }
            else
            {
                createdReferencedGraphInstances.emplace_back( nullptr );
            }
        }

        // Instantiate individual nodes
        //-------------------------------------------------------------------------

        InstantiationContext instantiationContext = { (int16_t) InvalidIndex, m_nodes, createdReferencedGraphInstances, m_pGraphDefinition->m_skeleton.GetPtr(), m_pGraphDefinition->m_parameterLookupMap, &m_pGraphDefinition->m_resources };

        #if EE_DEVELOPMENT_TOOLS
        if ( IsStandaloneInstance() )
        {
            instantiationContext.m_pLog = m_graphContext.m_pLog;
            instantiationContext.m_basePath = m_graphContext.m_basePath;
        }
        else
        {
            instantiationContext.m_pLog = pParentContext->m_pLog;
            instantiationContext.m_basePath = pParentContext->m_basePath;
        }
        #endif

        for ( int16_t i = 0; i < numNodes; i++ )
        {
            instantiationContext.m_currentNodeIdx = i;
            m_pGraphDefinition->m_nodeDefinitions[i]->InstantiateNode( instantiationContext, InstantiationOptions::CreateNode );
        }

        // Set root node and initialize graph nodes
        //-------------------------------------------------------------------------

        m_pRootNode = reinterpret_cast<PoseNode *>( m_nodes[m_pGraphDefinition->m_rootNodeIdx] );

        Initialize();

        // Resource Mappings needed for serialization
        //-------------------------------------------------------------------------

        if ( m_isStandaloneGraphInstance )
        {
            UpdateTaskSerializationContext();
        }
    }

    GraphInstance::~GraphInstance()
    {
        Shutdown();

        m_pRootNode = nullptr;

        DisconnectAllExternalGraphs();

        for ( GraphDefinition::InternalGraphSlot const &internalGraphSlot : m_pGraphDefinition->m_internalGraphSlots )
        {
            auto pInternalGraphNode = static_cast<ReferencedGraphNode *>( m_nodes[internalGraphSlot.m_nodeIdx] );
            EE_ASSERT( !pInternalGraphNode->IsInitialized() );

            if ( !pInternalGraphNode->HasInstance() )
            {
                continue;
            }

            EE::Delete( pInternalGraphNode->m_pGraphInstance );
        }

        // Shutdown context
        if ( m_graphContext.IsInitialized() )
        {
            m_graphContext.Shutdown();
        }

        //-------------------------------------------------------------------------

        // Run graph node destructors
        for ( GraphNode* pNode : m_nodes )
        {
            pNode->~GraphNode();
        }
        m_nodes.clear();

        EE::Free( m_pAllocatedInstanceMemory );
    }

    void GraphInstance::Initialize()
    {
        // Initialize persistent graph nodes
        for ( auto nodeIdx : m_pGraphDefinition->m_persistentNodeIndices )
        {
            EE_ASSERT( !m_nodes[nodeIdx]->IsInitialized() );
            m_nodes[nodeIdx]->Initialize( m_graphContext );
        }

        EE_ASSERT( m_pRootNode != nullptr );

        // Output any instantiation warnings/errors
        #if EE_DEVELOPMENT_TOOLS
        if ( IsStandaloneInstance() )
        {
            OutputLog();
        }
        #endif
    }

    void GraphInstance::Shutdown()
    {
        if ( m_graphContext.IsInitialized() )
        {
            // Shutdown and clear root node
            if ( m_pRootNode->IsInitialized() )
            {
                m_pRootNode->Shutdown( m_graphContext );
            }

            // Shutdown persistent graph nodes
            for ( int16_t nodeIdx : m_pGraphDefinition->m_persistentNodeIndices )
            {
                EE_ASSERT( m_nodes[nodeIdx]->IsInitialized() );
                m_nodes[nodeIdx]->Shutdown( m_graphContext );
            }
        }
        else // Verify that graph is correctly shutdown
        {
            EE_ASSERT( !m_pRootNode->IsInitialized() );

            for ( int16_t nodeIdx : m_pGraphDefinition->m_persistentNodeIndices )
            {
                EE_ASSERT( !m_nodes[nodeIdx]->IsInitialized() );
            }
        }
    }

    //-------------------------------------------------------------------------

    void GraphInstance::SetSecondarySkeletons( SecondarySkeletonList const& secondarySkeletons )
    {
        EE_ASSERT( m_isStandaloneGraphInstance );
        m_secondarySkeletons = secondarySkeletons;
        m_graphContext.m_pTaskSystem->SetSecondarySkeletons( m_secondarySkeletons );
    }

    Pose const* GraphInstance::GetSecondaryPose( Skeleton const* pSkeleton ) const
    {
        EE_ASSERT( pSkeleton != nullptr );

        TInlineVector<Pose const*, 1> secondaryPoses = m_graphContext.m_pTaskSystem->GetSecondaryPoses();
        for ( int32_t i = 0; i < secondaryPoses.size(); i++ )
        {
            if ( secondaryPoses[i]->GetSkeleton() == pSkeleton )
            {
                return secondaryPoses[i];
            }
        }

        return nullptr;
    }

    Pose const* GraphInstance::GetSecondaryPoseForPreviousUpdate( Skeleton const* pSkeleton ) const
    {
        EE_ASSERT( pSkeleton != nullptr );

        TInlineVector<Pose const*, 1> secondaryPoses = m_graphContext.m_pTaskSystem->GetSecondaryPosesForPreviousUpdate();
        for ( int32_t i = 0; i < secondaryPoses.size(); i++ )
        {
            if ( secondaryPoses[i]->GetSkeleton() == pSkeleton )
            {
                return secondaryPoses[i];
            }
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------

    void GraphInstance::GetResourceLookupTables( TInlineVector<ResourceLUT const*, 10>& LUTs ) const
    {
        LUTs.emplace_back( &m_pGraphDefinition->m_resourceLUT );

        TInlineVector<GraphDefinition*, 3> externalGraphDefs;
        for ( auto const &eg : m_pGraphDefinition->m_externalGraphSlots )
        {
            auto pReferencedGraphNode = reinterpret_cast<ReferencedGraphNode*>( m_nodes[eg.m_nodeIdx] );
            if ( pReferencedGraphNode->HasInstance() )
            {
                pReferencedGraphNode->m_pGraphInstance->GetResourceLookupTables( LUTs );
            }
        }
    }

    TaskSerializationContext& GraphInstance::UpdateTaskSerializationContext()
    {
        //m_taskSerializationContext.Clear();
        //TInlineVector<ResourceLUT const*, 10> LUTs;
        //GetResourceLookupTables( LUTs );

        ////-------------------------------------------------------------------------

        ////EE_UNIMPLEMENTED_FUNCTION();

        ////-------------------------------------------------------------------------

        //for ( auto const &eg : m_pGraphDefinition->m_externalGraphSlots )
        //{
        //    //EE_UNIMPLEMENTED_FUNCTION();
        //}

        if ( m_taskSerializationRequiresUpdate )
        {
            EE_ASSERT( IsStandaloneInstance() );

            TInlineVector<GraphDefinition const*, 3> externalGraphDefs;
            for ( auto const &eg : m_pGraphDefinition->m_externalGraphSlots )
            {
                auto pReferenceGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[eg.m_nodeIdx] );
                if ( pReferenceGraphNode->HasInstance() )
                {
                    externalGraphDefs.emplace_back( pReferenceGraphNode->m_pGraphInstance->GetGraphDefinition() );
                }
            }

            m_taskSerializationContext = TaskSerializationContext( *m_graphContext.GetTaskSystem()->GetTaskTypesList(), m_pGraphDefinition, externalGraphDefs, m_secondarySkeletons );
            m_taskSerializationRequiresUpdate = false;
        }

        return m_taskSerializationContext;
    }

    bool GraphInstance::DoesTaskSystemNeedUpdate() const
    {
        EE_ASSERT( m_isStandaloneGraphInstance );
        return m_graphContext.m_pTaskSystem->RequiresUpdate();
    }

    void GraphInstance::SerializeTaskList( Blob& outTopologyData, Blob& outTaskData )
    {
        EE_ASSERT( !DoesTaskSystemNeedUpdate() );

        // This will update the serialization context if needed
        m_graphContext.m_pTaskSystem->SerializeTasks( GetTaskSerializationContext(), outTopologyData, outTaskData );
    }

    //-------------------------------------------------------------------------

    int32_t GraphInstance::GetNumExternalGraphSlots() const
    {
        return (int32_t) m_pGraphDefinition->m_externalGraphSlots.size();
    }

    StringID GraphInstance::GetExternalGraphSlotID( int32_t slotIdx ) const
    {
        EE_ASSERT( slotIdx >= 0 && slotIdx < m_pGraphDefinition->m_externalGraphSlots.size() );
        return m_pGraphDefinition->m_externalGraphSlots[slotIdx].m_slotID;
    }

    int32_t GraphInstance::GetExternalGraphSlotIndex( StringID slotID ) const
    {
        EE_ASSERT( slotID.IsValid() );

        int32_t const numSlots = (int32_t) m_pGraphDefinition->m_externalGraphSlots.size();
        for ( int32_t i = 0; i < numSlots; i++ )
        {
            auto const &graphSlot = m_pGraphDefinition->m_externalGraphSlots[i];
            if ( graphSlot.m_slotID == slotID )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    int16_t GraphInstance::GetExternalGraphNodeIndex( StringID slotID ) const
    {
        EE_ASSERT( slotID.IsValid() );

        int32_t const numSlots = (int32_t) m_pGraphDefinition->m_externalGraphSlots.size();
        for ( int32_t i = 0; i < numSlots; i++ )
        {
            auto const &graphSlot = m_pGraphDefinition->m_externalGraphSlots[i];
            if ( graphSlot.m_slotID == slotID )
            {
                return graphSlot.m_nodeIdx;
            }
        }

        return InvalidIndex;
    }

    ReferencedGraphNode *GraphInstance::GetExternalGraphNode( StringID slotID ) const
    {
        EE_ASSERT( slotID.IsValid() );

        int32_t const numSlots = (int32_t) m_pGraphDefinition->m_externalGraphSlots.size();
        for ( int32_t i = 0; i < numSlots; i++ )
        {
            auto const &graphSlot = m_pGraphDefinition->m_externalGraphSlots[i];
            if ( graphSlot.m_slotID == slotID )
            {
                auto pNode = reinterpret_cast<ReferencedGraphNode*>( m_nodes[graphSlot.m_nodeIdx] );
                EE_ASSERT( pNode->IsExternalGraphSlot() );
                return pNode;
            }
        }

        return nullptr;
    }

    bool GraphInstance::IsExternalGraphSlotFilled( StringID slotID ) const
    {
        EE_ASSERT( slotID.IsValid() );

        for ( auto const &eg : m_pGraphDefinition->m_externalGraphSlots )
        {
            if ( eg.m_slotID == slotID )
            {
                auto pReferencedGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[eg.m_nodeIdx] );
                EE_ASSERT( pReferencedGraphNode->IsExternalGraphSlot() );
                return pReferencedGraphNode->HasInstance();
            }
        }

        EE_UNREACHABLE_CODE();
        return false;
    }

    void GraphInstance::InitializeReferencedGraphContexts( GraphContext *pParentContext, bool initializeGraphInstances )
    {
        EE_ASSERT( !m_isStandaloneGraphInstance );
        EE_ASSERT( !m_graphContext.IsInitialized() );

        //-------------------------------------------------------------------------

        m_graphContext.Initialize( pParentContext );

        if ( initializeGraphInstances )
        {
            Initialize();
        }

        //-------------------------------------------------------------------------

        int32_t const nNumInternalGraphSlots = (int32_t) m_pGraphDefinition->m_internalGraphSlots.size();
        for ( int32_t i = 0; i < nNumInternalGraphSlots; i++ )
        {
            auto const &graphSlot = m_pGraphDefinition->m_internalGraphSlots[i];
            auto pReferencedGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[graphSlot.m_nodeIdx] );
            EE_ASSERT( !pReferencedGraphNode->IsExternalGraphSlot() );

            if ( pReferencedGraphNode->HasInstance() )
            {
                pReferencedGraphNode->m_pGraphInstance->InitializeReferencedGraphContexts( &m_graphContext, initializeGraphInstances );
            }
        }

        //-------------------------------------------------------------------------

        int32_t const nNumExternalGraphSlots = (int32_t) m_pGraphDefinition->m_externalGraphSlots.size();
        for ( int32_t i = 0; i < nNumExternalGraphSlots; i++ )
        {
            auto const &graphSlot = m_pGraphDefinition->m_externalGraphSlots[i];
            auto pReferencedGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[graphSlot.m_nodeIdx] );
            EE_ASSERT( pReferencedGraphNode->IsExternalGraphSlot() );

            if ( pReferencedGraphNode->HasInstance() )
            {
                pReferencedGraphNode->m_pGraphInstance->InitializeReferencedGraphContexts( &m_graphContext, initializeGraphInstances );
            }
        }
    }

    void GraphInstance::ShutdownReferencedGraphContexts( bool shutdownGraphInstances )
    {
        EE_ASSERT( !m_isStandaloneGraphInstance );
        EE_ASSERT( m_graphContext.IsInitialized() );

        //-------------------------------------------------------------------------

        int32_t const numExternalGraphSlots = (int32_t) m_pGraphDefinition->m_externalGraphSlots.size();
        for ( int32_t i = 0; i < numExternalGraphSlots; i++ )
        {
            auto const &graphSlot = m_pGraphDefinition->m_externalGraphSlots[i];
            auto pReferencedGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[graphSlot.m_nodeIdx] );
            EE_ASSERT( pReferencedGraphNode->IsExternalGraphSlot() );

            if ( pReferencedGraphNode->HasInstance() )
            {
                pReferencedGraphNode->m_pGraphInstance->ShutdownReferencedGraphContexts( shutdownGraphInstances );
            }
        }

        //-------------------------------------------------------------------------

        int32_t const nNumInternalGraphSlots = (int32_t) m_pGraphDefinition->m_internalGraphSlots.size();
        for ( int32_t i = 0; i < nNumInternalGraphSlots; i++ )
        {
            auto const &graphSlot = m_pGraphDefinition->m_internalGraphSlots[i];
            auto pReferencedGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[graphSlot.m_nodeIdx] );
            EE_ASSERT( !pReferencedGraphNode->IsExternalGraphSlot() );

            if ( pReferencedGraphNode->HasInstance() )
            {
                pReferencedGraphNode->m_pGraphInstance->ShutdownReferencedGraphContexts( shutdownGraphInstances );
            }
        }

        //-------------------------------------------------------------------------

        if ( shutdownGraphInstances )
        {
            Shutdown();
        }

        m_graphContext.Shutdown();
    }

    bool GraphInstance::ConnectExternalGraph( StringID slotID, GraphInstance* pExternalGraphInstance )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance - Connect External Graph" );

        EE_ASSERT( slotID.IsValid() );
        EE_ASSERT( pExternalGraphInstance != nullptr );

        if ( pExternalGraphInstance->GetPrimarySkeleton() != m_pGraphDefinition->GetPrimarySkeleton() )
        {
            EE_LOG_ERROR( LogCategory::Animation, "Graph Instance", "Cannot insert external graph (%s) since skeletons dont match! Expected: %s, supplied: %s", pExternalGraphInstance->GetGraphDefinition()->GetResourceID().c_str(), m_pGraphDefinition->GetPrimarySkeleton()->GetResourceID().c_str(), pExternalGraphInstance->GetPrimarySkeleton()->GetResourceID().c_str() );
            return false;
        }

        // Get the node idx for the graph and ensure it is valid, users are expected to check slot validity before calling this function
        ReferencedGraphNode *pReferencedGraphNode = GetExternalGraphNode( slotID );
        EE_ASSERT( pReferencedGraphNode != nullptr && pReferencedGraphNode->IsExternalGraphSlot() );
        if ( pReferencedGraphNode == nullptr )
        {
            return false;
        }

        EE_ASSERT( pReferencedGraphNode->m_pGraphInstance == nullptr );
        if ( pReferencedGraphNode->m_pGraphInstance != nullptr )
        {
            return false;
        }

        // Convert instance to not-standalone, this is unrecoverable operation
        //-------------------------------------------------------------------------

        if ( pExternalGraphInstance->IsStandaloneInstance() )
        {
            EE_ASSERT( pExternalGraphInstance->m_graphContext.IsInitialized() );

            pExternalGraphInstance->m_isStandaloneGraphInstance = false;
            pExternalGraphInstance->ShutdownReferencedGraphContexts( false );
            pExternalGraphInstance->InitializeReferencedGraphContexts( &m_graphContext, false );
            pExternalGraphInstance->ResetGraphState();
        }
        else
        {
            EE_ASSERT( !pExternalGraphInstance->IsInitialized() );
            EE_ASSERT( pExternalGraphInstance->m_isExternalGraph );

            pExternalGraphInstance->InitializeReferencedGraphContexts( &m_graphContext, true );
        }

        // Connect graph instance
        //-------------------------------------------------------------------------

        pReferencedGraphNode->ConnectExternalGraphInstance( m_graphContext, pExternalGraphInstance );
        pReferencedGraphNode->m_pGraphInstance->m_isExternalGraph = true;

        m_taskSerializationRequiresUpdate = true;
        m_externalGraphStateChanged = true;

        return true;
    }

    void GraphInstance::DisconnectExternalGraph( StringID slotID )
    {
        EE_ASSERT( slotID.IsValid() );

        // Get the node idx for the graph and ensure it is valid, users are expected to check slot validity before calling this function
        ReferencedGraphNode *pReferencedGraphNode = GetExternalGraphNode( slotID );
        EE_ASSERT( pReferencedGraphNode != nullptr && pReferencedGraphNode->IsExternalGraphSlot() );
        if ( pReferencedGraphNode == nullptr )
        {
            return;
        }

        EE_ASSERT( pReferencedGraphNode->m_pGraphInstance != nullptr );
        if ( pReferencedGraphNode->m_pGraphInstance == nullptr )
        {
            return;
        }

        // Clear all execution state!
        //-------------------------------------------------------------------------

        pReferencedGraphNode->m_pGraphInstance->ShutdownReferencedGraphContexts( true );

        // Detach graph
        //-------------------------------------------------------------------------

        pReferencedGraphNode->DisconnectExternalGraphInstance( m_graphContext );
        m_taskSerializationRequiresUpdate = true;
        m_externalGraphStateChanged = true;
    }

    void GraphInstance::DisconnectAllExternalGraphs()
    {
        for ( auto const &eg : m_pGraphDefinition->m_externalGraphSlots )
        {
            auto pReferencedGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[eg.m_nodeIdx] );
            EE_ASSERT( pReferencedGraphNode->IsExternalGraphSlot() );
            if ( pReferencedGraphNode->HasInstance() )
            {
                DisconnectExternalGraph( eg.m_slotID );
            }
        }
    }

    //-------------------------------------------------------------------------

    int32_t GraphInstance::GetNumExternalPoseSlots() const
    {
        return (int32_t) m_pGraphDefinition->m_externalPoseSlots.size();
    }

    StringID GraphInstance::GetExternalPoseSlotID( int32_t nSlotIdx ) const
    {
        EE_ASSERT( nSlotIdx >= 0 && nSlotIdx < GetNumExternalPoseSlots() );

        if ( nSlotIdx < 0 || nSlotIdx >= GetNumExternalPoseSlots() )
        {
            return StringID();
        }

        return m_pGraphDefinition->m_externalPoseSlots[nSlotIdx].m_slotID;
    }

    bool GraphInstance::IsExternalPoseSet( StringID slotID ) const
    {
        EE_ASSERT( slotID.IsValid() );

        for ( auto const &eg : m_pGraphDefinition->m_externalPoseSlots )
        {
            if ( eg.m_slotID == slotID )
            {
                auto pExternalPoseNode = reinterpret_cast<ExternalPoseNode *>( m_nodes[eg.m_nodeIdx] );
                return pExternalPoseNode->IsPoseSet();
            }
        }

        EE_UNREACHABLE_CODE();
        return false;
    }

    bool GraphInstance::GetExternalPoseState( StringID slotID, ExternalPoseData &outState )
    {
        EE_ASSERT( slotID.IsValid() );

        for ( auto const &eg : m_pGraphDefinition->m_externalPoseSlots )
        {
            if ( eg.m_slotID == slotID )
            {
                auto pExternalPoseNode = reinterpret_cast<ExternalPoseNode *>( m_nodes[eg.m_nodeIdx] );
                if ( pExternalPoseNode->IsPoseSet() )
                {
                    outState = pExternalPoseNode->m_poseData;
                    return true;
                }
            }
        }

        //-------------------------------------------------------------------------

        outState.Reset();
        return false;
    }

    bool GraphInstance::SetExternalPose( StringID slotID, ExternalPoseData const& poseData )
    {
        EE_ASSERT( slotID.IsValid() );

        Skeleton const* pSkeleton = GetPrimarySkeleton();

        // Get the node idx for the graph and ensure it is valid, users are expected to check slot validity before calling this function
        ExternalPoseNode *pExternalPoseNode = GetExternalPoseNode( slotID );
        EE_ASSERT( pExternalPoseNode != nullptr );
        if ( pExternalPoseNode == nullptr )
        {
            EE_LOG_WARNING( LogCategory::Animation, "Graph Instance", "Cannot insert external pose into slot '%s' since the provide slot ID is INVALID!\n", slotID.c_str() );
            return false;
        }

        //-------------------------------------------------------------------------

        pExternalPoseNode->m_poseData = poseData;
        pExternalPoseNode->m_isPoseSet = true;

        return true;
    }

    void GraphInstance::ClearExternalPose( StringID slotID )
    {
        // Get the node idx for the graph and ensure it is valid, users are expected to check slot validity before calling this function
        ExternalPoseNode *pExternalPoseNode = GetExternalPoseNode( slotID );
        EE_ASSERT( pExternalPoseNode != nullptr );
        if ( pExternalPoseNode == nullptr )
        {
            EE_LOG_WARNING( LogCategory::Animation, "Graph Instance", "Cannot clear external pose for slot '%s' since the provide slot ID is INVALID!\n", slotID.c_str() );
            return;
        }

        //-------------------------------------------------------------------------

        pExternalPoseNode->m_poseData.Reset();
        pExternalPoseNode->m_isPoseSet = false;
        m_taskSerializationRequiresUpdate = true;
    }

    void GraphInstance::ClearAllExternalPoses()
    {
        for ( auto const &ep : m_pGraphDefinition->m_externalPoseSlots )
        {
            ClearExternalPose( ep.m_slotID );
        }
    }

    int32_t GraphInstance::GetExternalPoseSlotIndex( StringID slotID ) const
    {
        EE_ASSERT( slotID.IsValid() );

        int32_t const numSlots = (int32_t) m_pGraphDefinition->m_externalPoseSlots.size();
        for ( int32_t slotIdx = 0; slotIdx < numSlots; slotIdx++ )
        {
            auto const &poseSlot = m_pGraphDefinition->m_externalPoseSlots[slotIdx];
            if ( poseSlot.m_slotID == slotID )
            {
                return slotIdx;
            }
        }

        return InvalidIndex;
    }

    int16_t GraphInstance::GetExternalPoseNodeIndex( StringID slotID ) const
    {
        int32_t const numSlots = (int32_t) m_pGraphDefinition->m_externalPoseSlots.size();
        for ( int32_t i = 0; i < numSlots; i++ )
        {
            auto const &poseSlot = m_pGraphDefinition->m_externalPoseSlots[i];
            if ( poseSlot.m_slotID == slotID )
            {
                return poseSlot.m_nodeIdx;
            }
        }

        return InvalidIndex;
    }

    ExternalPoseNode *GraphInstance::GetExternalPoseNode( StringID slotID ) const
    {
        int32_t const numSlots = (int32_t) m_pGraphDefinition->m_externalPoseSlots.size();
        for ( int32_t i = 0; i < numSlots; i++ )
        {
            auto const &poseSlot = m_pGraphDefinition->m_externalPoseSlots[i];
            if ( poseSlot.m_slotID == slotID )
            {
                return reinterpret_cast<ExternalPoseNode *>( m_nodes[poseSlot.m_nodeIdx] );
            }
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------

    void GraphInstance::ResetGraphState( GraphTimeInfo const *pGraphTimeInfo, SyncTrackTime initTime )
    {
        EE_ASSERT( initTime.m_percentageThrough >= 0.0f && initTime.m_percentageThrough <= 1.0f );

        if ( m_pRootNode->IsInitialized() )
        {
            m_pRootNode->Shutdown( m_graphContext );
        }

        m_rootMotionDelta = Transform::Identity;

        m_graphContext.m_updateID++; // Bump the update ID to ensure that any initialization code that relies on it is dirtied.
        m_graphContext.m_pInitializationTimeInfo = pGraphTimeInfo;
        m_pRootNode->Initialize( m_graphContext, initTime );

        #if EE_DEVELOPMENT_TOOLS
        if ( IsStandaloneInstance() )
        {
            m_graphContext.ClearLog();
        }

        if ( IsRecording() )
        {
            GraphTimeInfo graphTimeInfo;
            graphTimeInfo.m_baseUpdateRange = SyncTrackTimeRange( initTime );
            RecordGraphState( RecordedUpdateType::Reset, graphTimeInfo, m_graphContext.m_worldTransform );
        }
        #endif

        m_graphContext.m_pInitializationTimeInfo = nullptr;
    }

    void GraphInstance::ResetGraphState( SyncTrackTime initTime )
    {
        ResetGraphState( nullptr, initTime );
    }

    void GraphInstance::ResetGraphState( GraphTimeInfo const &graphTimeInfo )
    {
        ResetGraphState( &graphTimeInfo, graphTimeInfo.m_baseUpdateRange.m_endTime );
    }

    void GraphInstance::ResetReferencedGraphState( GraphContext const &parentContext, SyncTrackTime initTime )
    {
        m_graphContext.TransferContextDataFromParent( parentContext );
        ResetGraphState( nullptr, initTime );
        m_graphContext.m_pLayerContext = nullptr;
    }

    void GraphInstance::ResetReferencedGraphState( GraphContext const &parentContext, GraphTimeInfo const &graphTimeInfo )
    {
        m_graphContext.TransferContextDataFromParent( parentContext );
        ResetGraphState( &graphTimeInfo, graphTimeInfo.m_baseUpdateRange.m_endTime );
        m_graphContext.m_pLayerContext = nullptr;
    }

    GraphPoseNodeResult GraphInstance::EvaluateGraph( Seconds const deltaTime, Transform const& startWorldTransform, Physics::PhysicsWorld* pPhysicsWorld, BranchState const* pBranchState, SyncTrackTimeRange const* pUpdateRange )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance: Evaluate Graph" );

        #if EE_DEVELOPMENT_TOOLS
        ScopedTimer<PlatformClock, Microseconds> st( m_executionTime );
        #endif

        // Update the serialization context
        //-------------------------------------------------------------------------

        UpdateTaskSerializationContext();

        // Record changes to external graph state
        //-------------------------------------------------------------------------

        if ( IsRecording() && m_externalGraphStateChanged )
        {
            RecordGraphState( RecordedUpdateType::ExternalGraphChanged, GraphTimeInfo(), m_graphContext.m_worldTransform );
            m_externalGraphStateChanged = false;
        }

        // Pre-Update
        //-------------------------------------------------------------------------

        m_graphContext.m_activeNodes.clear();

        #if EE_DEVELOPMENT_TOOLS
        RecordPreGraphEvaluateState( deltaTime, startWorldTransform );
        #endif

        if ( m_isStandaloneGraphInstance )
        {
            m_graphContext.m_pTaskSystem->ResetForNewUpdate();
            m_graphContext.m_pSampledEventsBuffer->ResetForNewUpdate();

            #if EE_DEVELOPMENT_TOOLS
            m_graphContext.m_pRootMotionDebugger->StartCharacterUpdate( startWorldTransform );
            #endif
        }

        // Update
        //-------------------------------------------------------------------------

        m_graphContext.Update( deltaTime, startWorldTransform, pPhysicsWorld );

        if ( !m_pRootNode->IsInitialized() )
        {
            ResetGraphState();
        }

        if ( pBranchState != nullptr )
        {
            m_graphContext.m_branchState = *pBranchState;
        }

        auto result = m_pRootNode->Update( m_graphContext, pUpdateRange );
        m_rootMotionDelta = result.m_rootMotionDelta;

        // Post Update
        //-------------------------------------------------------------------------

        if ( m_isStandaloneGraphInstance )
        {
            EE_ASSERT( m_graphContext.GetBasePath().IsEmpty() );
            m_graphContext.m_pSampledEventsBuffer->UpdateEventTracking();
        }

        #if EE_DEVELOPMENT_TOOLS
        RecordPostGraphEvaluateState( nullptr );

        if ( m_isStandaloneGraphInstance )
        {
            OutputLog();
        }
        #endif

        return result;
    }

    GraphPoseNodeResult GraphInstance::EvaluateGraph( Seconds const deltaTime, Transform const& startWorldTransform, Physics::PhysicsWorld* pPhysicsWorld, SyncTrackTimeRange const* pUpdateRange )
    {
        GraphPoseNodeResult result = EvaluateGraph( deltaTime, startWorldTransform, pPhysicsWorld, nullptr, pUpdateRange );
        return result;
    }

    GraphPoseNodeResult GraphInstance::EvaluateReferencedGraph( GraphContext const& parentContext, SyncTrackTimeRange const* pUpdateRange )
    {
        m_graphContext.TransferContextDataFromParent( parentContext );
        GraphPoseNodeResult result = EvaluateGraph( parentContext.m_deltaTime, parentContext.m_worldTransform, parentContext.m_pPhysicsWorld, &parentContext.m_branchState, pUpdateRange );
        m_graphContext.m_pLayerContext = nullptr;
        return result;
    }

    void GraphInstance::ExecutePrePhysicsPoseTasks( Transform const& endWorldTransform )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance: Pre-Physics Tasks" );

        #if EE_DEVELOPMENT_TOOLS
        m_graphContext.m_pRootMotionDebugger->EndCharacterUpdate( endWorldTransform );
        #endif

        m_graphContext.m_pTaskSystem->UpdatePrePhysics( m_graphContext.m_deltaTime, endWorldTransform, endWorldTransform.GetInverse() );
    }

    void GraphInstance::ExecutePostPhysicsPoseTasks()
    {
        EE_PROFILE_SCOPE_ANIMATION( "Graph Instance: Post-Physics Tasks" );
        m_graphContext.m_pTaskSystem->UpdatePostPhysics();

        #if EE_DEVELOPMENT_TOOLS
        RecordTasks();
        OutputTaskSystemLog();
        #endif
    }

    void GraphInstance::GetCurrentGraphTimingInfo( GraphTimeInfo &outGraphTimeInfo ) const
    {
        EE_ASSERT( IsValid() );

        outGraphTimeInfo.Reset();

        SyncTrack const &syncTrack = m_pRootNode->GetSyncTrack();
        outGraphTimeInfo.m_baseUpdateRange = SyncTrackTimeRange( syncTrack.GetTime( m_pRootNode->GetPreviousTime() ), syncTrack.GetTime( m_pRootNode->GetCurrentTime() ) );

        for ( int16_t activeNodeIdx : m_graphContext.m_activeNodes )
        {
            auto pNodeDefinition = m_pGraphDefinition->m_nodeDefinitions[activeNodeIdx];
            if ( TryCast<LayerBlendNode::Definition>( pNodeDefinition ) )
            {
                GraphLayerUpdateState &layerUpdateRange = outGraphTimeInfo.m_layerTimes.emplace_back();
                layerUpdateRange.m_nodeIdx = activeNodeIdx;

                auto pNode = static_cast<LayerBlendNode *>( m_nodes[activeNodeIdx] );
                pNode->GetSyncUpdateRangesForUnsynchronizedLayers( layerUpdateRange.m_updateRanges );
            }
            else if ( TryCast<ReferencedGraphNode::Definition>( pNodeDefinition ) )
            {
                for ( auto &referencedGraphSlot : m_pGraphDefinition->m_internalGraphSlots )
                {
                    if ( referencedGraphSlot.m_nodeIdx == activeNodeIdx )
                    {
                        auto pReferenceGraphNode = reinterpret_cast<ReferencedGraphNode*>( m_nodes[referencedGraphSlot.m_nodeIdx] );
                        if ( pReferenceGraphNode->m_pGraphInstance != nullptr && pReferenceGraphNode->m_pGraphInstance->IsValid() )
                        {
                            GraphTimeInfo::ReferencedGraphState &rgs = outGraphTimeInfo.m_referencedGraphStates.emplace_back();
                            rgs.m_referencedGraphNodeIdx = activeNodeIdx;
                            rgs.m_pTimeInfo = EE::New<GraphTimeInfo>();
                            pReferenceGraphNode->m_pGraphInstance->GetCurrentGraphTimingInfo( *rgs.m_pTimeInfo );
                        }
                        break;
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    void GraphInstance::StartRecording( GraphRecording* pRecording )
    {
        EE_ASSERT( pRecording != nullptr );
        EE_ASSERT( m_pRecording == nullptr );

        m_pRecording = pRecording;
        m_pRecording->m_graphID = m_pGraphDefinition->GetResourceID();
        m_pRecording->m_variationID = m_pGraphDefinition->m_variationID;

        // Record initial state
        //-------------------------------------------------------------------------

        RecordGraphState( RecordedUpdateType::FirstRecording, GraphTimeInfo(), m_graphContext.m_worldTransform );
    }

    void GraphInstance::StopRecording()
    {
        EE_ASSERT( m_pRecording != nullptr );
        m_pRecording = nullptr;
    }

    void GraphInstance::RecordPreGraphEvaluateState( Seconds const deltaTime, Transform const& startWorldTransform )
    {
        if ( m_pRecording == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto RecordGraphData = [] ( GraphInstance *pInstance, Seconds const flDeltaTime, Transform const &startWorldTransform, RecordedGraphUpdateData &outData )
        {
            outData.m_deltaTime = flDeltaTime;
            outData.m_characterWorldTransform = startWorldTransform;
            outData.m_updateType = RecordedUpdateType::TimeStep;

            // Record control parameter values
            for ( auto i = 0; i < pInstance->GetNumControlParameters(); i++ )
            {
                auto pParameter = (ValueNode *) pInstance->m_nodes[i];
                RecordedGraphUpdateData::ParameterData &paramData = outData.m_parameterData.emplace_back();

                switch ( pParameter->GetValueType() )
                {
                    case GraphValueType::Bool:
                    {
                        paramData.m_bool = pParameter->GetValue<bool>( pInstance->m_graphContext );
                    }
                    break;

                    case GraphValueType::ID:
                    {
                        paramData.m_ID = pParameter->GetValue<StringID>( pInstance->m_graphContext );
                    }
                    break;

                    case GraphValueType::Float:
                    {
                        paramData.m_float = pParameter->GetValue<float>( pInstance->m_graphContext );
                    }
                    break;

                    case GraphValueType::Vector:
                    {
                        paramData.m_vector = pParameter->GetValue<Float3>( pInstance->m_graphContext );
                    }
                    break;

                    case GraphValueType::Target:
                    {
                        paramData.m_target = pParameter->GetValue<Target>( pInstance->m_graphContext );
                    }
                    break;

                    default:
                    EE_UNREACHABLE_CODE();
                    break;
                }
            }

            // Calculate sync start time
            outData.m_updateRange.m_startTime = pInstance->m_pRootNode->GetSyncTrack().GetTime( pInstance->m_pRootNode->GetCurrentTime() );
        };

        // Record time delta and world transform
        RecordedGraphUpdateData *pUpdateData = m_pRecording->CreateUpdateData();
        RecordGraphData( this, deltaTime, startWorldTransform, *pUpdateData );

        // Record all external pose nodes
        //-------------------------------------------------------------------------

        for ( auto const &ep : m_pGraphDefinition->m_externalPoseSlots )
        {
            auto pExternalPoseNode = reinterpret_cast<ExternalPoseNode *>( m_nodes[ep.m_nodeIdx] );
            if ( pExternalPoseNode->IsPoseSet() )
            {
                /*RecordedExternalPoseData *pEPD = pUpdateData->CreateExternalPoseData();
                pEPD->m_externalPoseNodeIdx = ep.m_nodeIdx;
                pEPD->m_slotID = ep.m_slotID;

                pEPD->m_clipResourceID0 = ( pExternalPoseNode->m_poseData.m_pClip0 != nullptr ) ? pExternalPoseNode->m_poseData.m_pClip0->GetResourceID() : ResourceID();
                pEPD->m_startTime0 = pExternalPoseNode->m_poseData.m_startTime0;
                pEPD->m_endTime0 = pExternalPoseNode->m_poseData.m_endTime0;

                pEPD->m_clipResourceID1 = ( pExternalPoseNode->m_poseData.m_pClip1 != nullptr ) ? pExternalPoseNode->m_poseData.m_pClip1->GetResourceID() : ResourceID();
                pEPD->m_startTime1 = pExternalPoseNode->m_poseData.m_startTime1;
                pEPD->m_endTime1 = pExternalPoseNode->m_poseData.m_endTime1;

                pEPD->m_blendWeight = pExternalPoseNode->m_poseData.m_blendWeight;*/
            }
        }

        // Record all external graphs
        //-------------------------------------------------------------------------

        for ( auto const &eg : m_pGraphDefinition->m_externalGraphSlots )
        {
            auto pReferencedGraphNode = reinterpret_cast<ReferencedGraphNode*>( m_nodes[eg.m_nodeIdx] );
            if ( pReferencedGraphNode->HasInstance() )
            {
                RecordedExternalGraphData *pEGD = pUpdateData->CreateExternalGraphData();
                pEGD->m_externalGraphNodeIdx = eg.m_nodeIdx;
                pEGD->m_graphResourceID = pReferencedGraphNode->m_pGraphInstance->GetGraphDefinition()->GetResourceID();
                RecordGraphData( pReferencedGraphNode->m_pGraphInstance, deltaTime, startWorldTransform, pEGD->m_updateData );
            }
        }
    }

    void GraphInstance::RecordPostGraphEvaluateState( SyncTrackTimeRange const* pRange )
    {
        if ( m_pRecording == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto RecordGraphTimingInfo = [this, pRange] ( GraphInstance *pInstance, RecordedGraphUpdateData &outData )
        {
            // Record the global update range info
            //-------------------------------------------------------------------------

            if ( pRange == nullptr )
            {
                outData.m_updateRange.m_endTime = pInstance->m_pRootNode->GetSyncTrack().GetTime( pInstance->m_pRootNode->GetCurrentTime() );
            }
            else // Directly overwrite the sync update
            {
                outData.m_updateRange = *pRange;
            }

            // Record the layer update range info 
            //-------------------------------------------------------------------------

            GetCurrentGraphTimingInfo( outData.m_timingInfo );
        };

        RecordedGraphUpdateData *pUpdateData = m_pRecording->m_recordedUpdateData.back();
        RecordGraphTimingInfo( this, *pUpdateData );

        // Secondary Skeletons
        //-------------------------------------------------------------------------

        if ( IsStandaloneInstance() )
        {
            pUpdateData->SetSecondarySkeletons( m_secondarySkeletons );
        }
        else
        {
            pUpdateData->SetSecondarySkeletons( m_graphContext.GetTaskSystem()->GetSecondarySkeletons() );
        }

        // Record all external graphs
        //-------------------------------------------------------------------------

        for ( auto const &eg : m_pGraphDefinition->m_externalGraphSlots )
        {
            auto pReferencedGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[eg.m_nodeIdx] );
            if ( pReferencedGraphNode->HasInstance() )
            {
                RecordedExternalGraphData *pExternalGraphData = pUpdateData->TryGetExternalGraphData( eg.m_nodeIdx );
                EE_ASSERT( pExternalGraphData != nullptr );
                RecordGraphTimingInfo( pReferencedGraphNode->m_pGraphInstance, pExternalGraphData->m_updateData );
            }
        }
    }

    void GraphInstance::RecordReferencedGraphState( RecordedGraphState &recordedState )
    {
        EE_ASSERT( !m_isStandaloneGraphInstance );
        RecordGraphNodeState( recordedState );
    }

    void GraphInstance::RecordGraphState( RecordedUpdateType updateType, GraphTimeInfo const &timingInfo, Transform const& worldTransform )
    {
        EE_ASSERT( m_pRecording != nullptr );
        EE_ASSERT( updateType != RecordedUpdateType::Unknown && updateType != RecordedUpdateType::TimeStep );

        RecordedGraphUpdateData *pUpdateData = m_pRecording->CreateUpdateData();
        pUpdateData->m_updateType = updateType;
        pUpdateData->m_characterWorldTransform = worldTransform;

        // Record Graph State
        //-------------------------------------------------------------------------

        if ( updateType != RecordedUpdateType::ExternalGraphChanged )
        {
            pUpdateData->m_timingInfo = timingInfo;

            pUpdateData->m_recordedGraphStateIdx = (int32_t) m_pRecording->m_recordedGraphStates.size();
            RecordedGraphState *pGraphState = m_pRecording->m_recordedGraphStates.emplace_back( EE::New<RecordedGraphState>() );
            RecordGraphNodeState( *pGraphState );
        }

        // External Graphs
        //-------------------------------------------------------------------------

        if ( updateType == RecordedUpdateType::FirstRecording || updateType == RecordedUpdateType::ExternalGraphChanged )
        {
            // Record all external pose nodes
            //-------------------------------------------------------------------------

            for ( auto const &ep : m_pGraphDefinition->m_externalPoseSlots )
            {
               /* auto pExternalPoseNode = reinterpret_cast<ExternalPoseNode *>( m_nodes[ep.m_nodeIdx] );
                if ( pExternalPoseNode->IsPoseSet() )
                {
                    RecordedExternalPoseData *pEPD = pUpdateData->CreateExternalPoseData();
                    pEPD->m_externalPoseNodeIdx = ep.m_nodeIdx;
                    pEPD->m_slotID = ep.m_slotID;

                    pEPD->m_clipResourceID0 = ( pExternalPoseNode->m_poseData.m_pClip0 != nullptr ) ? pExternalPoseNode->m_poseData.m_pClip0->GetResourceID() : ResourceID();
                    pEPD->m_startTime0 = pExternalPoseNode->m_poseData.m_startTime0;
                    pEPD->m_endTime0 = pExternalPoseNode->m_poseData.m_endTime0;

                    pEPD->m_clipResourceID1 = ( pExternalPoseNode->m_poseData.m_pClip1 != nullptr ) ? pExternalPoseNode->m_poseData.m_pClip1->GetResourceID() : ResourceID();
                    pEPD->m_startTime1 = pExternalPoseNode->m_poseData.m_startTime1;
                    pEPD->m_endTime1 = pExternalPoseNode->m_poseData.m_endTime1;

                    pEPD->m_blendWeight = pExternalPoseNode->m_poseData.m_blendWeight;
                }*/
            }

            // Record all external graph nodes
            for ( auto const &eg : m_pGraphDefinition->m_externalGraphSlots )
            {
                auto pReferencedGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[eg.m_nodeIdx] );
                if ( pReferencedGraphNode->HasInstance() )
                {
                    RecordedExternalGraphData *pEGD = pUpdateData->CreateExternalGraphData();
                    pEGD->m_externalGraphNodeIdx = eg.m_nodeIdx;
                    pEGD->m_graphResourceID = pReferencedGraphNode->m_pGraphInstance->GetGraphDefinition()->GetResourceID();
                    pEGD->m_updateData.m_updateType = RecordedUpdateType::Unknown;
                    pEGD->m_updateData.m_recordedGraphStateIdx = (int32_t) m_pRecording->m_recordedGraphStates.size();

                    RecordedGraphState *pGraphState = m_pRecording->m_recordedGraphStates.emplace_back( EE::New<RecordedGraphState>() );
                    pReferencedGraphNode->m_pGraphInstance->RecordGraphNodeState( *pGraphState );
                }
            }
        }
    }

    void GraphInstance::RecordGraphNodeState( RecordedGraphState &recordedState )
    {
        recordedState.m_graphResourceID = m_pGraphDefinition->GetResourceID();
        recordedState.m_variationID = m_pGraphDefinition->m_variationID;
        recordedState.m_initializedNodeIndices.clear();
        recordedState.m_isStandaloneGraph = m_isStandaloneGraphInstance;

        for ( int16_t i = 0; i < m_nodes.size(); i++ )
        {
            if ( m_nodes[i]->IsInitialized() )
            {
                recordedState.m_initializedNodeIndices.emplace_back( i );
                m_nodes[i]->RecordGraphState( recordedState );
            }
        }
    }

    void GraphInstance::RecordTasks()
    {
        if ( m_pRecording == nullptr )
        {
            return;
        }

        // All the resources in use by this graph instance
        TInlineVector<ResourceLUT const*, 10> LUTs;
        GetResourceLookupTables( LUTs );

        // Record task
        auto& pFrameData = m_pRecording->m_recordedUpdateData.back();
        m_graphContext.m_pTaskSystem->SerializeTasks( UpdateTaskSerializationContext(), pFrameData->m_serializedTopologyData, pFrameData->m_serializedTaskData );
    }

    bool GraphInstance::SetToRecordedState( RecordedGraphUpdateData const &updateData, TVector<RecordedGraphState *> const &recordedGraphStates )
    {
        EE_ASSERT( updateData.m_updateType != RecordedUpdateType::Unknown );

        // Should we restore the main graph state?
        //-------------------------------------------------------------------------

        if ( updateData.m_updateType == RecordedUpdateType::FirstRecording || updateData.m_updateType == RecordedUpdateType::Reset )
        {
            auto pGraphState = recordedGraphStates[updateData.m_recordedGraphStateIdx];
            pGraphState->PrepareForReading();

            if ( !RestoreRecordedGraphState( *pGraphState ) )
            {
                return false;
            }
        }

        // Should we update parameters
        //-------------------------------------------------------------------------

        if ( updateData.m_updateType == RecordedUpdateType::TimeStep )
        {
            RestoreRecordedGraphParameters( updateData );
        }

        return true;
    }

    bool GraphInstance::SetToRecordedReferencedGraphState( RecordedGraphState const &recordedState )
    {
        EE_ASSERT( !m_isStandaloneGraphInstance );
        return RestoreRecordedGraphState( recordedState );
    }

    bool GraphInstance::RestoreRecordedGraphState( RecordedGraphState const &recordedState )
    {
        EE_ASSERT( !IsRecording() );
        EE_ASSERT( recordedState.m_graphResourceID == m_pGraphDefinition->GetResourceID() );
        EE_ASSERT( recordedState.m_variationID == m_pGraphDefinition->m_variationID );

        // Shutdown this graph if it was initialized
        if ( IsInitialized() )
        {
            m_pRootNode->Shutdown( m_graphContext );
        }

        //-------------------------------------------------------------------------

        // Set nodes array to the current instance array
        TVector<GraphNode *> *const pPreviousNodeArray = recordedState.m_pNodes;
        const_cast<TVector<GraphNode *> *&>( recordedState.m_pNodes ) = &m_nodes;

        // Restore the graph state
        for ( auto nodeIdx : recordedState.m_initializedNodeIndices )
        {
            if ( !m_nodes[nodeIdx]->RestoreGraphState( recordedState ) )
            {
                return false;
            }
        }

        // Revert node array back to previous value
        const_cast<TVector<GraphNode*>*&>( recordedState.m_pNodes ) = pPreviousNodeArray;

        #if EE_DEVELOPMENT_TOOLS
        if ( m_graphContext.m_pRootMotionDebugger != nullptr )
        {
            m_graphContext.m_pRootMotionDebugger->ResetRecordedPositions();
        }
        #endif

        return true;
    }

    void GraphInstance::RestoreRecordedGraphParameters( RecordedGraphUpdateData const &updateData )
    {
        EE_ASSERT( GetNumControlParameters() == updateData.m_parameterData.size() );

        // Record control parameter values
        for ( auto i = 0; i < GetNumControlParameters(); i++ )
        {
            auto pParameter = (ValueNode *) m_nodes[i];
            auto const &paramData = updateData.m_parameterData[i];

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
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }
        }
    }

    void GraphInstance::GetUpdateStateForActiveLayers( TVector<GraphLayerUpdateState>& outRanges ) const
    {
        for ( int16_t activeNodeIdx : m_graphContext.m_activeNodes )
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

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    TaskSystemDebugMode GraphInstance::GetTaskSystemDebugMode() const
    {
        EE_ASSERT( m_graphContext.m_pTaskSystem != nullptr );
        return m_graphContext.m_pTaskSystem->GetDebugMode();
    }

    void GraphInstance::SetTaskSystemDebugMode( TaskSystemDebugMode mode )
    {
        EE_ASSERT( m_graphContext.m_pTaskSystem != nullptr );
        m_graphContext.m_pTaskSystem->SetDebugMode( mode );
    }

    Transform GraphInstance::GetTaskSystemDebugWorldTransform()
    {
        return m_graphContext.m_pTaskSystem->GetCharacterWorldTransform();
    }

    void GraphInstance::GetReferencedGraphsForDebug( TVector<DebuggableGraphInstance>& outReferencedGraphInstances, SourcePath const& parentPath ) const
    {
        for ( auto &internalGraphSlot : m_pGraphDefinition->m_internalGraphSlots )
        {
            auto pReferenceGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[internalGraphSlot.m_nodeIdx] );
            EE_ASSERT( pReferenceGraphNode->IsInternalGraphSlot() );
            if ( pReferenceGraphNode->m_pGraphInstance != nullptr && pReferenceGraphNode->m_pGraphInstance->IsValid() )
            {
                SourcePath currentPath = parentPath;
                currentPath.m_path.emplace_back( internalGraphSlot.m_nodeIdx );

                DebuggableGraphInstance &debuggableGraph = outReferencedGraphInstances.emplace_back();
                debuggableGraph.m_pInstance = pReferenceGraphNode->m_pGraphInstance;
                debuggableGraph.m_path = currentPath;

                //-------------------------------------------------------------------------

                pReferenceGraphNode->m_pGraphInstance->GetReferencedGraphsForDebug( outReferencedGraphInstances, currentPath );
            }
        }

        for ( auto &externalGraphSlot : m_pGraphDefinition->m_externalGraphSlots )
        {
            auto pReferenceGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[externalGraphSlot.m_nodeIdx] );
            EE_ASSERT( pReferenceGraphNode->IsExternalGraphSlot() );
            if ( pReferenceGraphNode->m_pGraphInstance != nullptr )
            {
                SourcePath currentPath = parentPath;
                currentPath.m_path.emplace_back( externalGraphSlot.m_nodeIdx );

                DebuggableGraphInstance &debuggableGraph = outReferencedGraphInstances.emplace_back();
                debuggableGraph.m_pInstance = pReferenceGraphNode->m_pGraphInstance;
                debuggableGraph.m_path = currentPath;

                //-------------------------------------------------------------------------

                pReferenceGraphNode->m_pGraphInstance->GetReferencedGraphsForDebug( outReferencedGraphInstances, currentPath );
            }
        }
    }

    void GraphInstance::GetDebuggableGraphInstances( TVector<DebuggableGraphInstance> &outGraphInstances ) const
    {
        GetReferencedGraphsForDebug( outGraphInstances, SourcePath() );

        for ( auto &externalGraphSlot : m_pGraphDefinition->m_externalGraphSlots )
        {
            auto pReferencedGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[externalGraphSlot.m_nodeIdx] );
            if ( pReferencedGraphNode->m_pGraphInstance != nullptr && pReferencedGraphNode->m_pGraphInstance->IsValid() )
            {
                SourcePath currentPath;
                currentPath.m_path.emplace_back( externalGraphSlot.m_nodeIdx );

                DebuggableGraphInstance &debuggableGraph = outGraphInstances.emplace_back();
                debuggableGraph.m_pInstance = pReferencedGraphNode->m_pGraphInstance;
                debuggableGraph.m_externalSlotID = externalGraphSlot.m_slotID;
                debuggableGraph.m_path = currentPath;
            }
        }
    }

    SourcePath GraphInstance::GetSourcePathForDebuggableGraphInstance( PointerID instanceID ) const
    {
        TVector<DebuggableGraphInstance> debuggableReferencedGraphs;
        GetReferencedGraphsForDebug( debuggableReferencedGraphs, SourcePath() );

        for ( auto const& referencedGraph : debuggableReferencedGraphs )
        {
            if ( PointerID( referencedGraph.m_pInstance ) == instanceID )
            {
                return referencedGraph.m_path;
            }
        }

        EE_UNREACHABLE_CODE();
        return SourcePath();
    }

    GraphInstance const* GraphInstance::GetDebuggableGraphInstance( PointerID referencedGraphInstanceID ) const
    {
        TVector<DebuggableGraphInstance> debuggableReferencedGraphs;
        GetDebuggableGraphInstances( debuggableReferencedGraphs );

        for ( auto const& debuggableInstance : debuggableReferencedGraphs )
        {
            if ( PointerID( debuggableInstance.m_pInstance ) == referencedGraphInstanceID )
            {
                return debuggableInstance.m_pInstance;
            }
        }

        EE_UNREACHABLE_CODE();
        return nullptr;
    }

    GraphInstance const* GraphInstance::GetDebuggableGraphInstance( int16_t nodeIdx ) const
    {
        EE_ASSERT( IsValidNodeIndex( nodeIdx ) );

        for ( auto &referencedGraphSlot : m_pGraphDefinition->m_internalGraphSlots )
        {
            if ( referencedGraphSlot.m_nodeIdx == nodeIdx )
            {
                auto pReferenceGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[referencedGraphSlot.m_nodeIdx] );
                return pReferenceGraphNode->m_pGraphInstance;
            }
        }

        for ( auto const &eg : m_pGraphDefinition->m_externalGraphSlots )
        {
            if ( eg.m_nodeIdx == nodeIdx )
            {
                auto pReferencedGraphNode = reinterpret_cast<ReferencedGraphNode *>( m_nodes[eg.m_nodeIdx] );
                return pReferencedGraphNode->m_pGraphInstance;
            }
        }

        return nullptr;
    }

    GraphInstance const* GraphInstance::GetDebuggableGraphInstance( StringID externalSlotID ) const
    {
        auto pReferencedGraphNode = GetExternalGraphNode( externalSlotID );
        if ( pReferencedGraphNode != nullptr && pReferencedGraphNode->IsExternalGraphSlot() && pReferencedGraphNode->HasInstance() )
        {
            return pReferencedGraphNode->m_pGraphInstance;
        }

        return nullptr;
    }

    InlineString GraphInstance::ResolveSourcePath( SourcePath const& sourcePath ) const
    {
        EE_ASSERT( IsStandaloneInstance() );

        //-------------------------------------------------------------------------

        InlineString pathStr;

        GraphInstance const* pFinalGraphInstance = this;
        int32_t const numElements = (int32_t) sourcePath.Size();
        for ( int32_t i = 0; i < numElements; i++ )
        {
            // Resolve node path
            if ( i == ( numElements - 1 ) )
            {
                if ( !pathStr.empty() )
                {
                    pathStr.append( "/" );
                }

                pathStr.append( pFinalGraphInstance->m_pGraphDefinition->GetNodePath( (int16_t) sourcePath[i] ).c_str() );
            }
            else // Resolve to referenced or external graph
            {
                if ( !pathStr.empty() )
                {
                    pathStr.append( "/" );
                }

                pathStr.append( pFinalGraphInstance->m_pGraphDefinition->GetNodePath( (int16_t) sourcePath[i] ).c_str() );
                pFinalGraphInstance = pFinalGraphInstance->GetDebuggableGraphInstance( (int16_t) sourcePath[i] );
            }

            // This should only ever happen if we detach an external graph directly after updating a graph instance and before drawing this debug display
            if ( pFinalGraphInstance == nullptr )
            {
                pathStr.clear();
                return pathStr;
            }
        }

        return pathStr;
    }

    void GraphInstance::DrawDebug( DebugDrawContext& drawContext, TVector<SourcePath> const* pNodesToDebug )
    {
        // Graph
        //-------------------------------------------------------------------------

        if ( m_debugMode != GraphDebugMode::Off || m_isStandaloneGraphInstance == false )
        {
            m_graphContext.m_pNodesToDebug = pNodesToDebug;

            for ( int16_t graphNodeIdx : m_graphContext.m_activeNodes )
            {
                auto pGraphNode = m_nodes[graphNodeIdx];
                if ( pGraphNode->GetValueType() == GraphValueType::Pose )
                {
                    bool shouldDrawDebugNode = true;
                    if ( pNodesToDebug )
                    {
                        shouldDrawDebugNode = false;

                        SourcePath const nodePath = pGraphNode->GetNodePath( m_graphContext );
                        for ( SourcePath const &nodeToDebugPath : *pNodesToDebug )
                        {
                            if ( nodeToDebugPath.IsUnderOrEqual( nodePath ) )
                            {
                                shouldDrawDebugNode = true;
                            }
                        }
                    }

                    if ( shouldDrawDebugNode )
                    {
                        auto pPoseNode = static_cast<PoseNode *>( pGraphNode );
                        pPoseNode->DrawDebug( m_graphContext, drawContext );
                    }
                }
            }

            m_graphContext.m_pNodesToDebug = nullptr;
        }

        // Root Motion
        //-------------------------------------------------------------------------

        if ( IsStandaloneInstance() )
        {
            m_graphContext.m_pRootMotionDebugger->DrawDebug( drawContext );
        }

        // Task System
        //-------------------------------------------------------------------------

        if ( IsStandaloneInstance() )
        {
            m_graphContext.m_pTaskSystem->DrawDebug( drawContext );
        }
    }

    void GraphInstance::OutputLog()
    {
        EE_ASSERT( IsStandaloneInstance() );

        for ( m_graphContext.m_lastOutputtedLogItemIdx; m_graphContext.m_lastOutputtedLogItemIdx < m_graphContext.m_pLog->size(); m_graphContext.m_lastOutputtedLogItemIdx++ )
        {
            GraphLogEntry const& item = ( *m_graphContext.m_pLog )[m_graphContext.m_lastOutputtedLogItemIdx];

            InlineString const pathStr = ResolveSourcePath( item.m_sourcePath );
            InlineString const source( InlineString::CtorSprintf(), "%s - %s", m_pGraphDefinition->GetResourceID().c_str(), pathStr.c_str() );
            SystemLog::AddEntry( item.m_severity, "Animation", source.c_str(), "", 0, item.m_message.c_str() );
        }
    }

    void GraphInstance::OutputTaskSystemLog()
    {
        EE_ASSERT( IsStandaloneInstance() );

        TVector<TaskLogEntry> const &taskSystemLog = m_graphContext.m_pTaskSystem->GetLog();
        for ( TaskLogEntry const &entry : taskSystemLog )
        {
            InlineString const pathStr = ResolveSourcePath( entry.m_sourcePath );
            InlineString const source( InlineString::CtorSprintf(), "%s - %s", m_pGraphDefinition->GetResourceID().c_str(), pathStr.c_str() );
            SystemLog::AddEntry( entry.m_severity, "Animation", source.c_str(), "", 0, entry.m_message.c_str() );
        }
    }
    #endif
}