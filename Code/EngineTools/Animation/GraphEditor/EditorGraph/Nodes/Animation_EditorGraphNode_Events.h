#pragma once
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_IDs.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Events.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class GenericEventConditionEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( GenericEventConditionEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Generic Event Condition"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE GenericEventConditionNode::Operator         m_operator = GenericEventConditionNode::Operator::Or;
        EE_EXPOSE EventSearchMode                             m_searchMode = EventSearchMode::SearchAll;
        EE_EXPOSE TVector<StringID>                           m_eventIDs;
    };

    //-------------------------------------------------------------------------

    class GenericEventPercentageThroughEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( GenericEventPercentageThroughEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Generic Event Percentage Through"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE EventSearchMode                             m_searchMode = EventSearchMode::SearchAll;
        EE_EXPOSE bool                                        m_preferHighestPercentageThrough = false;
        EE_EXPOSE StringID                                    m_eventID;
    };

    //-------------------------------------------------------------------------

    class FootEventConditionEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( FootEventConditionEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Footstep Condition"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE FootEvent::PhaseCondition                   m_phaseCondition = FootEvent::PhaseCondition::LeftFootDown;
        EE_EXPOSE bool                                        m_preferHighestPercentageThrough = false;
    };

    //-------------------------------------------------------------------------

    class FootstepEventPercentageThroughEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( FootstepEventPercentageThroughEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Footstep Percentage Through"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE FootEvent::PhaseCondition                   m_phaseCondition = FootEvent::PhaseCondition::LeftFootDown;
        EE_EXPOSE bool                                        m_preferHighestPercentageThrough = false;
    };

    //-------------------------------------------------------------------------

    class SyncEventConditionEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( SyncEventConditionEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Sync Event Condition"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE SyncEventConditionNode::TriggerMode          m_triggerMode = SyncEventConditionNode::TriggerMode::ExactlyAtEventIndex;
        EE_EXPOSE int32_t                                        m_syncEventIdx = InvalidIndex;
    };

    //-------------------------------------------------------------------------

    class CurrentSyncEventEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( CurrentSyncEventEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Current Sync Event"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };
}