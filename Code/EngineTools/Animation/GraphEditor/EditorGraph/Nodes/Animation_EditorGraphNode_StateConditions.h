#pragma once
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_StateConditions.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class StateCompletedConditionEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( StateCompletedConditionEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "State Completed"; }
        virtual char const* GetCategory() const override { return "Transitions"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class TimeConditionEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( TimeConditionEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Time Condition"; }
        virtual char const* GetCategory() const override { return "Transitions"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE float                                    m_comparand = 0.0f;
        EE_EXPOSE TimeConditionNode::ComparisonType        m_type = TimeConditionNode::ComparisonType::ElapsedTime;
        EE_EXPOSE TimeConditionNode::Operator              m_operator = TimeConditionNode::Operator::LessThan;
    };
}