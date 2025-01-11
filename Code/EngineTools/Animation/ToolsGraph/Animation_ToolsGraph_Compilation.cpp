#include "Animation_ToolsGraph_Compilation.h"
#include "Animation_ToolsGraph_Definition.h"
#include "Nodes/Animation_ToolsGraphNode_Parameters.h"
#include "Nodes/Animation_ToolsGraphNode_Result.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphCompilationContext::~GraphCompilationContext()
    {
        Reset();
    }

    void GraphCompilationContext::Reset()
    {
        m_pVariationHierarchy = nullptr;
        m_variationID.Clear();

        for ( auto pDefinition : m_nodeDefinitions )
        {
            EE::Delete( pDefinition );
        }

        m_nodeDefinitions.clear();

        //-------------------------------------------------------------------------

        m_log.clear();
        m_nodeIDToIndexMap.clear();
        m_nodeIndexToIDMap.clear();
        m_persistentNodeIndices.clear();
        m_compiledNodePaths.clear();
        m_currentNodeMemoryOffset = 0;
        m_graphInstanceRequiredAlignment = alignof( bool );

        m_registeredResources.clear();
        m_registeredExternalGraphSlots.clear();
        m_conduitSourceStateCompiledNodeIdx = InvalidIndex;
        m_transitionDuration = 0;
        m_transitionDurationOverrideIdx = InvalidIndex;

        m_nodeMemoryOffsets.clear();
    }

    void GraphCompilationContext::TryAddPersistentNode( NodeGraph::BaseNode const* pNode, GraphNode::Definition* pDefinition )
    {
        auto pFlowNode = TryCast<FlowToolsNode>( pNode );
        if ( pFlowNode != nullptr && pFlowNode->IsPersistentNode() )
        {
            m_persistentNodeIndices.emplace_back( pDefinition->m_nodeIdx );
        }
    }

    void GraphCompilationContext::SetVariationData( VariationHierarchy const* pVariationHierarchy, StringID ID )
    {
        EE_ASSERT( pVariationHierarchy != nullptr );
        EE_ASSERT( ID.IsValid() );
        EE_ASSERT( pVariationHierarchy->IsValidVariation( ID ) );

        m_pVariationHierarchy = pVariationHierarchy;
        m_variationID = ID;
    }

    //-------------------------------------------------------------------------

    bool GraphDefinitionCompiler::CompileGraph( ToolsGraphDefinition const& toolsGraph, StringID variationID )
    {
        EE_ASSERT( toolsGraph.IsValid() );
        EE_ASSERT( variationID.IsValid() );

        auto pRootGraph = toolsGraph.GetRootGraph();

        m_context.Reset();
        m_runtimeGraph = GraphDefinition();

        // Ensure the requested variation is valid
        //-------------------------------------------------------------------------

        auto const& variationHierarchy = toolsGraph.GetVariationHierarchy();
        Variation const* pVariation = variationHierarchy.GetVariation( variationID );
        if ( pVariation == nullptr )
        {
            m_context.LogError( "Invalid variation ID '%s'", variationID.c_str() );
            return false;
        }

        if ( !pVariation->m_skeleton.IsSet() )
        {
            m_context.LogError( "Variation '%s' has no skeleton set!", variationID.c_str() );
            return false;
        }

        m_context.SetVariationData( &variationHierarchy, variationID );

        // Always compile control parameters first
        //-------------------------------------------------------------------------

        auto const controlParameters = pRootGraph->FindAllNodesOfType<ControlParameterToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
        for ( ControlParameterToolsNode const* pParameter : controlParameters )
        {
            if ( pParameter->Compile( m_context ) == InvalidIndex )
            {
                return false;
            }

            m_runtimeGraph.m_controlParameterIDs.emplace_back( pParameter->GetParameterID() );
        }

        // Then compile virtual parameters
        //-------------------------------------------------------------------------

        auto const virtualParameters = pRootGraph->FindAllNodesOfType<VirtualParameterToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
        for ( VirtualParameterToolsNode const* pParameter : virtualParameters )
        {
            int16_t const parameterIdx = pParameter->Compile( m_context );
            if ( parameterIdx == InvalidIndex )
            {
                return false;
            }

            m_runtimeGraph.m_virtualParameterIDs.emplace_back( pParameter->GetParameterID() );
            m_runtimeGraph.m_virtualParameterNodeIndices.emplace_back( parameterIdx );
        }

        // Finally compile the actual graph
        //-------------------------------------------------------------------------

        auto const resultNodes = pRootGraph->FindAllNodesOfType<ResultToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
        EE_ASSERT( resultNodes.size() == 1 );
        int16_t const rootNodeIdx = resultNodes[0]->Compile( m_context );

        // Fill runtime definition
        //-------------------------------------------------------------------------

        m_runtimeGraph.m_variationID = variationID;
        m_runtimeGraph.m_skeleton = pVariation->m_skeleton;
        m_runtimeGraph.m_nodeDefinitions = m_context.m_nodeDefinitions;
        m_runtimeGraph.m_persistentNodeIndices = m_context.m_persistentNodeIndices;
        m_runtimeGraph.m_instanceNodeStartOffsets = m_context.m_nodeMemoryOffsets;
        m_runtimeGraph.m_instanceRequiredMemory = m_context.m_currentNodeMemoryOffset;
        m_runtimeGraph.m_instanceRequiredAlignment = m_context.m_graphInstanceRequiredAlignment;
        m_runtimeGraph.m_rootNodeIdx = rootNodeIdx;
        m_runtimeGraph.m_referencedGraphSlots = m_context.m_registeredReferencedGraphSlots;
        m_runtimeGraph.m_externalGraphSlots = m_context.m_registeredExternalGraphSlots;

        #if EE_DEVELOPMENT_TOOLS
        m_runtimeGraph.m_nodePaths.swap( m_context.m_compiledNodePaths );
        #endif

        m_runtimeGraph.m_resources.reserve( m_context.m_registeredResources.size() );
        for ( auto const& resourceID : m_context.m_registeredResources )
        {
            m_runtimeGraph.m_resources.emplace_back( resourceID );
        }

        //-------------------------------------------------------------------------

        return m_runtimeGraph.m_rootNodeIdx != InvalidIndex;
    }
}