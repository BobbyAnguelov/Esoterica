#pragma once

#include "Animation_ToolsGraphNode_AnimationClip.h"
#include "Animation_ToolsGraphNode_Result.h"

//-------------------------------------------------------------------------

namespace EE::Animation
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
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree, GraphType::EntryOverrideTree ); }
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

    class ParameterizedSelectorToolsNode final : public VariationDataToolsNode
    {
        EE_REFLECT_TYPE( ParameterizedSelectorToolsNode );

    public:

        struct Data final : public VariationDataToolsNode::Data
        {
            EE_REFLECT_TYPE( Data );

        public:

            EE_REFLECT( Category = "Advanced", ShowAsStaticArray );
            TVector<uint8_t>        m_optionWeights;
        };

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

        virtual TypeSystem::TypeInfo const* GetVariationDataTypeInfo() const override { return ParameterizedSelectorToolsNode::Data::s_pTypeInfo; }
        virtual void OnVariationOverrideCreated( VariationDataToolsNode::Data* pCreatedData ) override;

    private:

        EE_REFLECT( ShowAsStaticArray );
        TVector<String>             m_optionLabels;

        EE_REFLECT( Category = "Advanced" );
        bool                        m_ignoreInvalidOptions = false;
    };

    //-------------------------------------------------------------------------

    class ParameterizedAnimationClipSelectorToolsNode final : public VariationDataToolsNode
    {
        EE_REFLECT_TYPE( ParameterizedAnimationClipSelectorToolsNode );

    public:

        struct Data final : public VariationDataToolsNode::Data
        {
            EE_REFLECT_TYPE( Data );

        public:

            EE_REFLECT( Category = "Advanced", ShowAsStaticArray );
            TVector<uint8_t>        m_optionWeights;
        };

    public:

        ParameterizedAnimationClipSelectorToolsNode();

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

        virtual TypeSystem::TypeInfo const* GetVariationDataTypeInfo() const override { return ParameterizedAnimationClipSelectorToolsNode::Data::s_pTypeInfo; }
        virtual void OnVariationOverrideCreated( VariationDataToolsNode::Data* pCreatedData ) override;

    private:

        EE_REFLECT( ShowAsStaticArray );
        TVector<String>             m_optionLabels;

        EE_REFLECT( Category = "Advanced" );
        bool                        m_ignoreInvalidOptions = false;
    };

    //-------------------------------------------------------------------------

    class IDBasedSelectorToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( IDBasedSelectorToolsNode );

    public:

        IDBasedSelectorToolsNode();

    private:

        virtual bool IsRenameable() const override { return true; }

        virtual char const* GetTypeName() const override { return "ID Based Selector"; }
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

        EE_REFLECT( Category = "Advanced" );
        bool                        m_ignoreInvalidOptions = false;
    };

    //-------------------------------------------------------------------------

    class IDBasedAnimationClipSelectorToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( IDBasedAnimationClipSelectorToolsNode );

    public:

        IDBasedAnimationClipSelectorToolsNode();

        virtual bool IsRenameable() const override { return true; }

        virtual bool IsAnimationClipReferenceNode() const override { return true; }
        virtual char const* GetTypeName() const override { return "ID Based Clip Selector"; }
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

        EE_REFLECT( Category = "Advanced" );
        bool                        m_ignoreInvalidOptions = false;
    };
}