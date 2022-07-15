#include "Animation_EditorGraphNode_ControlParameters.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void BoolControlParameterEditorNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        EditorGraphNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", GraphValueType::Bool, true );
    }

    void FloatControlParameterEditorNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        EditorGraphNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", GraphValueType::Float, true );
    }

    void IntControlParameterEditorNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        EditorGraphNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", GraphValueType::Int, true );
    }

    void IDControlParameterEditorNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        EditorGraphNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", GraphValueType::ID, true );
    }

    void VectorControlParameterEditorNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        EditorGraphNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", GraphValueType::Vector, true );
    }

    void TargetControlParameterEditorNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        EditorGraphNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", GraphValueType::Target, true );
    }

    void TargetControlParameterEditorNode::SetPreviewTargetTransform( Transform const& transform )
    {
        VisualGraph::ScopedNodeModification snm( this );
        m_previewStartTransform = transform;
    }
}