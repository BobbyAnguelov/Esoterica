#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Floats.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class FloatRemapToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( FloatRemapToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Remap"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    public:

        EE_EXPOSE FloatRemapNode::RemapRange  m_inputRange;
        EE_EXPOSE FloatRemapNode::RemapRange  m_outputRange;
    };

    //-------------------------------------------------------------------------

    class FloatClampToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( FloatClampToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Clamp"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    public:

        EE_EXPOSE FloatRange                   m_clampRange = FloatRange( 0 );
    };

    //-------------------------------------------------------------------------

    class FloatAbsToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( FloatAbsToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Abs"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class FloatEaseToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( FloatEaseToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Ease"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    public:

        EE_EXPOSE Math::Easing::Type          m_easingType = Math::Easing::Type::Linear;
        EE_EXPOSE float                       m_easeTime = 1.0f;
        EE_EXPOSE bool                        m_useStartValue = true; // Should we initialize this node to the input value or to the specified start value
        EE_EXPOSE float                       m_startValue = 0.0f; // Optional initialization value for this node
    };

    //-------------------------------------------------------------------------

    class FloatCurveToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( FloatCurveToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Curve"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    public:

        EE_EXPOSE FloatCurve                   m_curve;
    };

    //-------------------------------------------------------------------------

    class FloatMathToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( FloatMathToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Math"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    public:

        EE_EXPOSE bool                       m_returnAbsoluteResult = false;
        EE_EXPOSE FloatMathNode::Operator    m_operator = FloatMathNode::Operator::Add;
        EE_EXPOSE float                      m_valueB = 0.0f;
    };

    //-------------------------------------------------------------------------

    class FloatComparisonToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( FloatComparisonToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Float Comparison"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE FloatComparisonNode::Comparison         m_comparison = FloatComparisonNode::Comparison::GreaterThanEqual;
        EE_EXPOSE float                                   m_comparisonValue = 0.0f;
        EE_EXPOSE float                                   m_epsilon = 0.0f;
    };

    //-------------------------------------------------------------------------

    class FloatRangeComparisonToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( FloatRangeComparisonToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Float Range Check"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE  FloatRange                              m_range = FloatRange( 0, 1 );
        EE_EXPOSE  bool                                    m_isInclusiveCheck = true;
    };

    //-------------------------------------------------------------------------

    class FloatSwitchToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( FloatSwitchToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float Switch"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class FloatAngleMathToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( FloatAngleMathToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Angle Math"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE FloatAngleMathNode::Operation m_operation = FloatAngleMathNode::Operation::ClampTo180;
    };
}