#pragma once
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Vectors.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class VectorInfoEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( VectorInfoEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Vector Info"; }
        virtual char const* GetCategory() const override { return "Values/Vector"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE VectorInfoNode::Info            m_desiredInfo = VectorInfoNode::Info::X;
    };

    //-------------------------------------------------------------------------

    class VectorNegateEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( VectorNegateEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Vector Negate"; }
        virtual char const* GetCategory() const override { return "Values/Vector"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };
}