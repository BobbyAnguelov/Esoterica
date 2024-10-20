#pragma once

#include "Animation_ToolsGraphNode_AnimationClip.h"
#include "Animation_ToolsGraphNode_Result.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class SelectorConditionToolsNode final : public ResultToolsNode
    {
        EE_REFLECT_TYPE( SelectorConditionToolsNode );

        friend class SelectorBaseToolsNode;

    public:

        SelectorConditionToolsNode();

        virtual char const* GetTypeName() const override { return "Conditions"; }
        virtual char const* GetCategory() const override { return "Conditions"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override { EE_UNREACHABLE_CODE(); return InvalidIndex; }

    private:

        virtual void RefreshDynamicPins() override;
    };

    //-------------------------------------------------------------------------

    class SelectorBaseToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( SelectorBaseToolsNode );

    public:

        SelectorBaseToolsNode();

        inline String const& GetOptionLabel( int32_t optionIdx ) const { return m_optionLabels[optionIdx]; }

    private:

        virtual bool IsRenameable() const override { return true; }

        virtual char const* GetCategory() const override { return "Animation/Selectors"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Option"; }
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Pose ); }
        virtual void OnDynamicPinCreation( UUID const& pinID ) override;
        virtual void PreDynamicPinDestruction( UUID const& pinID ) override;

    protected:

        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;

        virtual void RefreshDynamicPins() override;

    private:

        EE_REFLECT( ShowAsStaticArray );
        TVector<String>             m_optionLabels;
    };

    //-------------------------------------------------------------------------

    class SelectorToolsNode final : public SelectorBaseToolsNode
    {
        EE_REFLECT_TYPE( SelectorToolsNode );

     private:

        virtual char const* GetTypeName() const override { return "Selector"; }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class AnimationClipSelectorToolsNode final : public SelectorBaseToolsNode
    {
        EE_REFLECT_TYPE( AnimationClipSelectorToolsNode );

    private:

        virtual char const* GetTypeName() const override { return "Clip Selector"; }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual bool IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const override;
        virtual bool IsAnimationClipReferenceNode() const override { return true; }
    };

    //-------------------------------------------------------------------------

    class ParameterizedSelectorToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ParameterizedSelectorToolsNode );

    public:

        ParameterizedSelectorToolsNode();

    private:

        virtual bool IsRenameable() const override { return true; }

        virtual char const* GetTypeName() const override { return "Parameterized Selector"; }
        virtual char const* GetCategory() const override { return "Animation/Selectors"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Option"; }
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Pose ); }
        virtual void OnDynamicPinCreation( UUID const& pinID ) override;
        virtual void PreDynamicPinDestruction( UUID const& pinID ) override;

        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;
        virtual void RefreshDynamicPins() override;

    private:

        EE_REFLECT( ShowAsStaticArray );
        TVector<String>             m_optionLabels;
    };

    //-------------------------------------------------------------------------

    class ParameterizedAnimationClipSelectorToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ParameterizedAnimationClipSelectorToolsNode );

    public:

        ParameterizedAnimationClipSelectorToolsNode();

    public:

        virtual bool IsRenameable() const override { return true; }

        virtual bool IsAnimationClipReferenceNode() const override { return true; }
        virtual char const* GetTypeName() const override { return "Parameterized Clip Selector"; }
        virtual char const* GetCategory() const override { return "Animation/Selectors"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual bool IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const override;
        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Option"; }
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Pose ); }
        virtual void OnDynamicPinCreation( UUID const& pinID ) override;
        virtual void PreDynamicPinDestruction( UUID const& pinID ) override;

        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;
        virtual void RefreshDynamicPins() override;

    private:

        EE_REFLECT( ShowAsStaticArray );
        TVector<String>             m_optionLabels;
    };
}