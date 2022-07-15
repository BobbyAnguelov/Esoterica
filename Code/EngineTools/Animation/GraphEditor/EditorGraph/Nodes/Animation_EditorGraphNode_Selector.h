#pragma once

#include "Animation_EditorGraphNode_AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class SelectorConditionEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( SelectorConditionEditorNode );

        friend class SelectorEditorNode;
        friend class AnimationClipSelectorEditorNode;

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Conditions"; }
        virtual char const* GetCategory() const override { return "Conditions"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree ); }
    };

    //-------------------------------------------------------------------------

    class SelectorEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( SelectorEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

    private:

        virtual char const* GetTypeName() const override { return "Selector"; }
        virtual char const* GetCategory() const override { return "Selectors"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual bool SupportsDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual uint32_t GetDynamicInputPinValueType() const override { return (uint32_t) GraphValueType::Pose; }
        virtual void OnDynamicPinCreation( UUID pinID ) override;
        virtual void OnDynamicPinDestruction( UUID pinID ) override;
    };

    //-------------------------------------------------------------------------

    class AnimationClipSelectorEditorNode final : public AnimationClipReferenceEditorNode
    {
        EE_REGISTER_TYPE( AnimationClipSelectorEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

    private:

        virtual char const* GetTypeName() const override { return "Animation Clip Selector"; }
        virtual char const* GetCategory() const override { return "Selectors"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;
        virtual bool SupportsDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual uint32_t GetDynamicInputPinValueType() const override { return (uint32_t) GraphValueType::Pose; }
        virtual void OnDynamicPinCreation( UUID pinID ) override;
        virtual void OnDynamicPinDestruction( UUID pinID ) override;
    };
}