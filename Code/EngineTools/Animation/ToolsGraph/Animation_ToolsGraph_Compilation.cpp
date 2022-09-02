#include "Animation_ToolsGraph_Compilation.h"
#include "Animation_ToolsGraph_Definition.h"
#include "Nodes/Animation_ToolsGraphNode_Parameters.h"
#include "Nodes/Animation_ToolsGraphNode_Result.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    using namespace GraphNodes;

    //-------------------------------------------------------------------------

    GraphCompilationContext::~GraphCompilationContext()
    {
        Reset();
    }

    void GraphCompilationContext::Reset()
    {
        for ( auto pSettings : m_nodeSettings )
        {
            EE::Delete( pSettings );
        }

        //-------------------------------------------------------------------------

        m_log.clear();
        m_nodeIDToIndexMap.clear();
        m_persistentNodeIndices.clear();
        m_compiledNodePaths.clear();
        m_currentNodeMemoryOffset = 0;
        m_graphInstanceRequiredAlignment = alignof( bool );

        m_registeredDataSlots.clear();
        m_registeredExternalGraphSlots.clear();
        m_conduitSourceStateCompiledNodeIdx = InvalidIndex;
        m_transitionDuration = 0;
        m_transitionDurationOverrideIdx = InvalidIndex;

        m_nodeMemoryOffsets.clear();
    }

    void GraphCompilationContext::TryAddPersistentNode( VisualGraph::BaseNode const* pNode, GraphNode::Settings* pSettings )
    {
        auto pFlowNode = TryCast<GraphNodes::FlowToolsNode>( pNode );
        if ( pFlowNode != nullptr && pFlowNode->IsPersistentNode() )
        {
            m_persistentNodeIndices.emplace_back( pSettings->m_nodeIdx );
        }
    }

    //-------------------------------------------------------------------------

    bool GraphDefinitionCompiler::CompileGraph( ToolsGraphDefinition const& toolsGraph )
    {
        EE_ASSERT( toolsGraph.IsValid() );
        auto pRootGraph = toolsGraph.GetRootGraph();

        m_context.Reset();
        m_runtimeGraph = GraphDefinition();

        // Ensure that all variations have skeletons set
        //-------------------------------------------------------------------------

        auto const& variationHierarchy = toolsGraph.GetVariationHierarchy();
        for ( auto const& variation : variationHierarchy.GetAllVariations() )
        {
            EE_ASSERT( variation.m_ID.IsValid() );
            if ( !variation.m_skeleton.IsSet() )
            {
                String const message( String::CtorSprintf(), "Variation '%s' has no skeleton set!", variation.m_ID.c_str() );
                m_context.LogError( message );
                return false;
            }
        }

        // Always compile control parameters first
        //-------------------------------------------------------------------------

        auto const controlParameters = pRootGraph->FindAllNodesOfType<ControlParameterToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        uint32_t const numControlParameters = (uint32_t) controlParameters.size();
        for ( auto pParameter : controlParameters )
        {
            if ( pParameter->Compile( m_context ) == InvalidIndex )
            {
                return false;
            }

            m_runtimeGraph.m_controlParameterIDs.emplace_back( pParameter->GetParameterID() );
        }

        // Then compile virtual parameters
        //-------------------------------------------------------------------------

        auto const virtualParameters = pRootGraph->FindAllNodesOfType<VirtualParameterToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );
        uint32_t const numVirtualParameters = (uint32_t) controlParameters.size();
        for ( auto pParameter : virtualParameters )
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

        auto const resultNodes = pRootGraph->FindAllNodesOfType<ResultToolsNode>();
        EE_ASSERT( resultNodes.size() == 1 );
        int16_t const rootNodeIdx = resultNodes[0]->Compile( m_context );
        m_context.m_persistentNodeIndices.emplace_back( rootNodeIdx );

        // Fill runtime definition
        //-------------------------------------------------------------------------

        m_runtimeGraph.m_nodeSettings = m_context.m_nodeSettings;
        m_runtimeGraph.m_persistentNodeIndices = m_context.m_persistentNodeIndices;
        m_runtimeGraph.m_instanceNodeStartOffsets = m_context.m_nodeMemoryOffsets;
        m_runtimeGraph.m_instanceRequiredMemory = m_context.m_currentNodeMemoryOffset;
        m_runtimeGraph.m_instanceRequiredAlignment = m_context.m_graphInstanceRequiredAlignment;
        m_runtimeGraph.m_rootNodeIdx = rootNodeIdx;
        m_runtimeGraph.m_childGraphSlots = m_context.m_registeredChildGraphSlots;
        m_runtimeGraph.m_externalGraphSlots = m_context.m_registeredExternalGraphSlots;

        #if EE_DEVELOPMENT_TOOLS
        m_runtimeGraph.m_nodePaths.swap( m_context.m_compiledNodePaths );
        #endif

        //-------------------------------------------------------------------------

        return m_runtimeGraph.m_rootNodeIdx != InvalidIndex;
    }
}