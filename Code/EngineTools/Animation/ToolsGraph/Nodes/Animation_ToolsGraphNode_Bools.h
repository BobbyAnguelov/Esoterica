#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class AndToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( AndToolsNode );

    public:

        AndToolsNode();

        virtual char const* GetTypeName() const override { return "And"; }
        virtual char const* GetCategory() const override { return "Values/Bool"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "And"; }
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Bool ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class OrToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( OrToolsNode );

    public:

        OrToolsNode();

        virtual char const* GetTypeName() const override { return "Or"; }
        virtual char const* GetCategory() const override { return "Values/Bool"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override{ return "Or"; }
        virtual StringID GetDynamicInputPinValueType() const override{ return GetPinTypeForValueType( GraphValueType::Bool ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class NotToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( NotToolsNode );

    public:

        NotToolsNode();

        virtual char const* GetTypeName() const override { return "Not"; }
        virtual char const* GetCategory() const override { return "Values/Bool"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };
}