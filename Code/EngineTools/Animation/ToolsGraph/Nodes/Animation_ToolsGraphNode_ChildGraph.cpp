#include "Animation_ToolsGraphNode_ChildGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ChildGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ChildGraphToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        DataSlotToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Pose, true );
    }

    int16_t ChildGraphToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ChildGraphNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ChildGraphNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_childGraphIdx = context.RegisterChildGraphNode( pSettings->m_nodeIdx, GetID() );
        }
        return pSettings->m_nodeIdx;
    }

    EE::ResourceTypeID ChildGraphToolsNode::GetSlotResourceTypeID() const
    {
        return GraphVariation::GetStaticResourceTypeID();
    }
}