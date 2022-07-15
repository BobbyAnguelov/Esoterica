#pragma once
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Floats.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class FloatRemapEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( FloatRemapEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

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

    class FloatClampEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( FloatClampEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Float Clamp"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    public:

        EE_EXPOSE FloatRange                   m_clampRange = FloatRange( 0 );
    };

    //-------------------------------------------------------------------------

    class FloatAbsEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( FloatAbsEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Float Abs"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class FloatEaseEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( FloatEaseEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Float Ease"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    public:

        EE_EXPOSE Math::Easing::Type          m_easingType = Math::Easing::Type::Linear;
        EE_EXPOSE float                       m_easeTime = 1.0f;
        EE_EXPOSE float                       m_initialValue = -1.0f;
    };

    //-------------------------------------------------------------------------

    class FloatCurveEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( FloatCurveEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Float Curve"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    public:

        EE_EXPOSE FloatCurve                   m_curve;
    };

    //-------------------------------------------------------------------------

    class FloatMathEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( FloatMathEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Float Math"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    public:

        EE_EXPOSE bool                       m_returnAbsoluteResult = false;
        EE_EXPOSE FloatMathNode::Operator    m_operator = FloatMathNode::Operator::Add;
        EE_EXPOSE float                      m_valueB = 0.0f;
    };

    //-------------------------------------------------------------------------

    class FloatComparisonEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( FloatComparisonEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

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

    class FloatRangeComparisonEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( FloatRangeComparisonEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

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

    class FloatSwitchEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( FloatSwitchEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Float Switch"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class FloatReverseDirectionEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( FloatReverseDirectionEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Float Reverse Direction"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };
}