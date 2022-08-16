#pragma once
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ChildGraphEditorNode final : public DataSlotEditorNode
    {
        EE_REGISTER_TYPE( ChildGraphEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Child Graph"; }
        virtual char const* GetCategory() const override { return ""; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual char const* const GetDefaultSlotName() const override { return "Graph"; }
        virtual ResourceTypeID GetSlotResourceTypeID() const override;
    };
}