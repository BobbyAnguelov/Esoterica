#pragma once
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class AndEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( AndEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "And"; }
        virtual char const* GetCategory() const override { return "Values/Bool"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual bool SupportsDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "And"; }
        virtual uint32_t GetDynamicInputPinValueType() const override { return (uint32_t) GraphValueType::Bool; }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class OrEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( OrEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Or"; }
        virtual char const* GetCategory() const override { return "Values/Bool"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual bool SupportsDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override{ return "Or"; }
        virtual uint32_t GetDynamicInputPinValueType() const override{ return (uint32_t) GraphValueType::Bool; }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class NotEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( NotEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Not"; }
        virtual char const* GetCategory() const override { return "Values/Bool"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };
}