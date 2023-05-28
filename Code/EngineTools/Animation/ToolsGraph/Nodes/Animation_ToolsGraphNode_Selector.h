#pragma once

#include "Animation_ToolsGraphNode_AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class SelectorConditionToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( SelectorConditionToolsNode );

        friend class SelectorToolsNode;
        friend class AnimationClipSelectorToolsNode;

    public:

        SelectorConditionToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Conditions"; }
        virtual char const* GetCategory() const override { return "Conditions"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree ); }
    };

    //-------------------------------------------------------------------------

    class SelectorToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( SelectorToolsNode );

    public:

        SelectorToolsNode();
        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

    private:

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Selector"; }
        virtual char const* GetCategory() const override { return "Animation/Selectors"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual uint32_t GetDynamicInputPinValueType() const override { return (uint32_t) GraphValueType::Pose; }
        virtual void OnDynamicPinCreation( UUID pinID ) override;
        virtual void OnDynamicPinDestruction( UUID pinID ) override;
    };

    //-------------------------------------------------------------------------

    class AnimationClipSelectorToolsNode final : public AnimationClipReferenceToolsNode
    {
        EE_REFLECT_TYPE( AnimationClipSelectorToolsNode );

    public:

        AnimationClipSelectorToolsNode();
        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

    private:

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Animation Clip Selector"; }
        virtual char const* GetCategory() const override { return "Animation/Selectors"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;
        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual uint32_t GetDynamicInputPinValueType() const override { return (uint32_t) GraphValueType::Pose; }
        virtual void OnDynamicPinCreation( UUID pinID ) override;
        virtual void OnDynamicPinDestruction( UUID pinID ) override;
    };
}