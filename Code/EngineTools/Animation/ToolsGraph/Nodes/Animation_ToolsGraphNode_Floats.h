#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Floats.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class FloatRemapToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatRemapToolsNode );

    public:

        FloatRemapToolsNode();

        virtual char const* GetTypeName() const override { return "Float Remap"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    public:

        EE_REFLECT() FloatRemapNode::RemapRange  m_inputRange;
        EE_REFLECT() FloatRemapNode::RemapRange  m_outputRange;
    };

    //-------------------------------------------------------------------------

    class FloatClampToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatClampToolsNode );

    public:

        FloatClampToolsNode();

        virtual char const* GetTypeName() const override { return "Float Clamp"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    public:

        EE_REFLECT() FloatRange                   m_clampRange = FloatRange( 0 );
    };

    //-------------------------------------------------------------------------

    class FloatAbsToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatAbsToolsNode );

    public:

        FloatAbsToolsNode();

        virtual char const* GetTypeName() const override { return "Float Abs"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class FloatEaseToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatEaseToolsNode );

    public:

        FloatEaseToolsNode();

        virtual char const* GetTypeName() const override { return "Float Ease"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    public:

        EE_REFLECT();
        Math::Easing::Operation     m_easing = Math::Easing::Operation::Linear;

        EE_REFLECT();
        float                       m_easeTime = 1.0f; // How long should the ease take

        EE_REFLECT();
        bool                        m_useStartValue = true; // Should we initialize this node to the input value or to the specified start value

        EE_REFLECT();
        float                       m_startValue = 0.0f; // Optional initialization value for this node
    };

    //-------------------------------------------------------------------------

    class FloatCurveToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatCurveToolsNode );

    public:

        FloatCurveToolsNode();

        virtual char const* GetTypeName() const override { return "Float Curve"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    public:

        EE_REFLECT() FloatCurve                   m_curve;
    };

    //-------------------------------------------------------------------------

    class FloatMathToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatMathToolsNode );

    public:

        FloatMathToolsNode();

        virtual char const* GetTypeName() const override { return "Float Math"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    public:

        EE_REFLECT() bool                       m_returnAbsoluteResult = false;
        EE_REFLECT() FloatMathNode::Operator    m_operator = FloatMathNode::Operator::Add;
        EE_REFLECT() float                      m_valueB = 0.0f;
    };

    //-------------------------------------------------------------------------

    class FloatComparisonToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatComparisonToolsNode );

    public:

        FloatComparisonToolsNode();

        virtual char const* GetTypeName() const override { return "Float Comparison"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() FloatComparisonNode::Comparison         m_comparison = FloatComparisonNode::Comparison::GreaterThanEqual;
        EE_REFLECT() float                                   m_comparisonValue = 0.0f;
        EE_REFLECT() float                                   m_epsilon = 0.0f;
    };

    //-------------------------------------------------------------------------

    class FloatRangeComparisonToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatRangeComparisonToolsNode );

    public:

        FloatRangeComparisonToolsNode();

        virtual char const* GetTypeName() const override { return "Float Range Check"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT()  FloatRange                              m_range = FloatRange( 0, 1 );
        EE_REFLECT()  bool                                    m_isInclusiveCheck = true;
    };

    //-------------------------------------------------------------------------

    class FloatSwitchToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatSwitchToolsNode );

    public:

        FloatSwitchToolsNode();

        virtual char const* GetTypeName() const override { return "Float Switch"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class FloatAngleMathToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatAngleMathToolsNode );

    public:

        FloatAngleMathToolsNode();

        virtual char const* GetTypeName() const override { return "Angle Math"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_REFLECT() FloatAngleMathNode::Operation m_operation = FloatAngleMathNode::Operation::ClampTo180;
    };

    //-------------------------------------------------------------------------

    class FloatSelectorToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatSelectorToolsNode );

        struct Option : public IReflectedType
        {
            EE_REFLECT_TYPE( Option );

        public:

            Option() = default;
            Option( String const& name, float value ) : m_name( name ), m_value( value ) {}

        public:

            EE_REFLECT();
            String                  m_name;

            EE_REFLECT();
            float                   m_value = 0.0f;
        };

    public:

        FloatSelectorToolsNode();

        virtual char const* GetTypeName() const override { return "Dynamic Float Selector"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Option"; }
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Bool ); }
        virtual void OnDynamicPinCreation( UUID const& pinID ) override;
        virtual void PreDynamicPinDestruction( UUID const& pinID ) override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;
        virtual void RefreshDynamicPins() override;

    private:

        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;

    private:

        EE_REFLECT( ShowAsStaticArray );
        TVector<Option>             m_options;

        EE_REFLECT();
        float                       m_defaultValue = 0.0f; // The default value if no option is valid

        EE_REFLECT( Category = "Easing" );
        Math::Easing::Operation     m_easing = Math::Easing::Operation::None;  // The type of ease to perform (or directly pop to new value if easing is set to none)

        EE_REFLECT( Category = "Easing" );
        float                       m_easeTime = 0.3f; // How long should the ease take?
    };
}