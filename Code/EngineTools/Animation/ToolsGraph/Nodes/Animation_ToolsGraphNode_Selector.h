#pragma once

#include "Animation_ToolsGraphNode_AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class SelectorConditionToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( SelectorConditionToolsNode );

        friend class SelectorBaseToolsNode;

    public:

        SelectorConditionToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Conditions"; }
        virtual char const* GetCategory() const override { return "Conditions"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree ); }

        virtual bool DrawPinControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, VisualGraph::Flow::Pin const& pin ) override;
    };

    //-------------------------------------------------------------------------

    class SelectorBaseToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( SelectorBaseToolsNode );

    public:

        SelectorBaseToolsNode();
        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        inline String const& GetPinLabel( int32_t pinIdx ) const { return m_optionLabels[pinIdx]; }

    private:

        virtual char const* GetName() const override { return m_name.c_str(); }

        virtual bool IsRenameable() const override { return true; }
        virtual void SetName( String const& newName ) override { newName.empty() ? m_name = GetTypeName() : m_name.sprintf( "Selector: %s", newName.c_str() ); }

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetCategory() const override { return "Animation/Selectors"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }

        virtual bool DrawPinControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, VisualGraph::Flow::Pin const& pin ) override;

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Pose ); }
        virtual void OnDynamicPinCreation( UUID pinID ) override;
        virtual void OnDynamicPinDestruction( UUID pinID ) override;

        void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& nodeObjectValue ) override;

    private:

        EE_REFLECT( "IsToolsReadOnly" : true );
        String                      m_name = "Selector";

        EE_REFLECT( "IsToolsReadOnly" : true );
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
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;
        virtual bool IsAnimationClipReferenceNode() const override { return true; }
    };

    //-------------------------------------------------------------------------

    class ParameterizedSelectorToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ParameterizedSelectorToolsNode );

    public:

        ParameterizedSelectorToolsNode();

    private:

        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual bool IsRenameable() const override { return true; }
        virtual void SetName( String const& newName ) override { newName.empty() ? m_name = GetTypeName() : m_name.sprintf( "Selector: %s", newName.c_str() ); }

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Parameterized Selector"; }
        virtual char const* GetCategory() const override { return "Animation/Selectors"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Pose ); }
        virtual void OnDynamicPinCreation( UUID pinID ) override;
        virtual void OnDynamicPinDestruction( UUID pinID ) override;

    private:

        EE_REFLECT( "IsToolsReadOnly" : true );
        String                      m_name = "Parameterized Selector";
    };

    //-------------------------------------------------------------------------

    class ParameterizedAnimationClipSelectorToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ParameterizedAnimationClipSelectorToolsNode );

    public:

        ParameterizedAnimationClipSelectorToolsNode();

    public:

        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual bool IsRenameable() const override { return true; }
        virtual void SetName( String const& newName ) override { newName.empty() ? m_name = GetTypeName() : m_name.sprintf( "Selector: %s", newName.c_str() ); }

        virtual bool IsAnimationClipReferenceNode() const override { return true; }
        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Parameterized Clip Selector"; }
        virtual char const* GetCategory() const override { return "Animation/Selectors"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Pose ); }
        virtual void OnDynamicPinCreation( UUID pinID ) override;
        virtual void OnDynamicPinDestruction( UUID pinID ) override;
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;

    private:

        EE_REFLECT( "IsToolsReadOnly" : true );
        String                      m_name = "Parameterized Clip Selector";
    };
}