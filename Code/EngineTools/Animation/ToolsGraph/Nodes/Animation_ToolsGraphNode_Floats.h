#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Floats.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class FloatRemapToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatRemapToolsNode );

    public:

        FloatRemapToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Remap"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

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

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Clamp"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    public:

        EE_REFLECT() FloatRange                   m_clampRange = FloatRange( 0 );
    };

    //-------------------------------------------------------------------------

    class FloatAbsToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatAbsToolsNode );

    public:

        FloatAbsToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Abs"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class FloatEaseToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatEaseToolsNode );

    public:

        FloatEaseToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Ease"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    public:

        EE_REFLECT() Math::Easing::Type          m_easingType = Math::Easing::Type::Linear;
        EE_REFLECT() float                       m_easeTime = 1.0f;
        EE_REFLECT() bool                        m_useStartValue = true; // Should we initialize this node to the input value or to the specified start value
        EE_REFLECT() float                       m_startValue = 0.0f; // Optional initialization value for this node
    };

    //-------------------------------------------------------------------------

    class FloatCurveToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatCurveToolsNode );

    public:

        FloatCurveToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Curve"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
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

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Math"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

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

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Float Comparison"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

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

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Float Range Check"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

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

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Switch"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class FloatAngleMathToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatAngleMathToolsNode );

    public:

        FloatAngleMathToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Angle Math"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_REFLECT() FloatAngleMathNode::Operation m_operation = FloatAngleMathNode::Operation::ClampTo180;
    };

    //-------------------------------------------------------------------------

    class FloatSelectorToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FloatSelectorToolsNode );

    public:

        FloatSelectorToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Selector"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual bool DrawPinControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, VisualGraph::Flow::Pin const& pin ) override;
        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Bool ); }
        virtual void OnDynamicPinCreation( UUID pinID ) override;
        virtual void OnDynamicPinDestruction( UUID pinID ) override;

    private:

        EE_REFLECT( "ShowAsStaticArray" : true );
        TVector<float>              m_pinValues;

        EE_REFLECT();
        float                       m_defaultValue = 0.0f;

        EE_REFLECT( "Category" : "Easing" );
        Math::Easing::Type          m_easingType = Math::Easing::Type::None;

        EE_REFLECT( "Category" : "Easing" );
        float                       m_easeTime = 0.3f;
    };
}