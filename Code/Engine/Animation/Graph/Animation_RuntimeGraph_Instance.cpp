#include "Animation_RuntimeGraph_Instance.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Contexts.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphInstance::GraphInstance( GraphVariation const* pGraphVariation )
        : m_pGraphVariation( pGraphVariation )
    {
        EE_ASSERT( pGraphVariation != nullptr );

        // Allocate memory
        //-------------------------------------------------------------------------

        auto pGraphDef = m_pGraphVariation->m_pGraphDefinition.GetPtr();
        size_t const numNodes = pGraphDef->m_instanceNodeStartOffsets.size();
        EE_ASSERT( pGraphDef->m_nodeSettings.size() == numNodes );

        m_pAllocatedInstanceMemory = reinterpret_cast<uint8_t*>( EE::Alloc( pGraphDef->m_instanceRequiredMemory, pGraphDef->m_instanceRequiredAlignment ) );

        m_nodes.reserve( numNodes );
        for( auto const& nodeOffset : pGraphDef->m_instanceNodeStartOffsets )
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
    }

    GraphInstance::~GraphInstance()
    {
        EE_ASSERT( m_pRootNode == nullptr );

        for ( auto pNode : m_nodes )
        {
            pNode->~GraphNode();
        }

        EE::Free( m_pAllocatedInstanceMemory );
    }

    void GraphInstance::Initialize( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        auto pGraphDef = m_pGraphVariation->m_pGraphDefinition.GetPtr();
        for ( auto nodeIdx : pGraphDef->m_persistentNodeIndices )
        {
            m_nodes[nodeIdx]->Initialize( context );
        }

        m_pRootNode = static_cast<PoseNode*>( m_nodes[pGraphDef->m_rootNodeIdx ] );
        EE_ASSERT( m_pRootNode->IsInitialized() );
    }

    void GraphInstance::Shutdown( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        auto pGraphDef = m_pGraphVariation->m_pGraphDefinition.GetPtr();
        for ( auto nodeIdx : pGraphDef->m_persistentNodeIndices )
        {
            m_nodes[nodeIdx]->Shutdown( context );
        }

        EE_ASSERT( !m_pRootNode->IsInitialized() );
        m_pRootNode = nullptr;
    }

    //-------------------------------------------------------------------------

    void GraphInstance::Reset( GraphContext& context )
    {
        m_pRootNode->Shutdown( context );
        m_pRootNode->Initialize( context, SyncTrackTime() );
    }

    GraphPoseNodeResult GraphInstance::UpdateGraph( GraphContext& context )
    {
        auto result = m_pRootNode->Update( context );

        #if EE_DEVELOPMENT_TOOLS
        m_activeNodes = context.GetActiveNodes();
        #endif

        return result;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void GraphInstance::DrawDebug( GraphContext& context, Drawing::DrawContext& drawContext )
    {
        if ( m_debugMode == GraphDebugMode::Off )
        {
            return;
        }

        //-------------------------------------------------------------------------

        for ( auto graphNodeIdx : m_activeNodes )
        {
            // Filter nodes
            if( !m_debugFilterNodes.empty() && !VectorContains( m_debugFilterNodes, graphNodeIdx ) )
            {
                continue;
            }

            // Draw debug
            auto pGraphNode = m_nodes[graphNodeIdx];
            if ( pGraphNode->GetValueType() == GraphValueType::Pose )
            {
                auto pPoseNode = static_cast<PoseNode*>( pGraphNode );
                pPoseNode->DrawDebug( context, drawContext );
            }
        }
    }
    #endif
}