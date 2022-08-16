#include "Animation_EditorGraphNode_ChildGraph.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ChildGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ChildGraphEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        DataSlotEditorNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Pose, true );
    }

    int16_t ChildGraphEditorNode::Compile( GraphCompilationContext& context ) const
    {
        ChildGraphNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ChildGraphNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_childGraphIdx = context.RegisterChildGraphNode( pSettings->m_nodeIdx, GetID() );
        }
        return pSettings->m_nodeIdx;
    }

    EE::ResourceTypeID ChildGraphEditorNode::GetSlotResourceTypeID() const
    {
        return GraphVariation::GetStaticResourceTypeID();
    }
}