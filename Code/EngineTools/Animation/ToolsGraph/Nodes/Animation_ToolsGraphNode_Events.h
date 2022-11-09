#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_IDs.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Events.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class IDEventConditionToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( IDEventConditionToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Generic Event Condition"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE IDEventConditionNode::Operator                m_operator = IDEventConditionNode::Operator::Or;
        EE_EXPOSE IDEventConditionNode::SearchRule              m_searchRule = IDEventConditionNode::SearchRule::SearchAll;
        EE_EXPOSE bool                                          m_onlyCheckEventsFromActiveBranch = false;
        EE_EXPOSE TVector<StringID>                             m_eventIDs;
    };

    //-------------------------------------------------------------------------

    class GenericEventPercentageThroughToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( GenericEventPercentageThroughToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Generic Event Percentage Through"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE EventPriorityRule                             m_priorityRule = EventPriorityRule::HighestWeight;
        EE_EXPOSE bool                                          m_onlyCheckEventsFromActiveBranch = false;
        EE_EXPOSE StringID                                      m_eventID;
    };

    //-------------------------------------------------------------------------

    class FootEventConditionToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( FootEventConditionToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Footstep Condition"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE FootEvent::PhaseCondition                     m_phaseCondition = FootEvent::PhaseCondition::LeftFootDown;
        EE_EXPOSE bool                                          m_onlyCheckEventsFromActiveBranch = false;
    };

    //-------------------------------------------------------------------------

    class FootstepEventPercentageThroughToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( FootstepEventPercentageThroughToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Footstep Percentage Through"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE FootEvent::PhaseCondition                     m_phaseCondition = FootEvent::PhaseCondition::LeftFootDown;
        EE_EXPOSE EventPriorityRule                             m_priorityRule = EventPriorityRule::HighestWeight;
        EE_EXPOSE bool                                          m_onlyCheckEventsFromActiveBranch = false;
    };

    //-------------------------------------------------------------------------

    class SyncEventIndexConditionToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( SyncEventIndexConditionToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Sync Event Index Condition"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE SyncEventIndexConditionNode::TriggerMode      m_triggerMode = SyncEventIndexConditionNode::TriggerMode::ExactlyAtEventIndex;
        EE_EXPOSE int32_t                                       m_syncEventIdx = InvalidIndex;
    };

    //-------------------------------------------------------------------------

    class CurrentSyncEventToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( CurrentSyncEventToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Current Sync Event"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class TransitionEventConditionToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( TransitionEventConditionToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Transition Event Condition"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE TransitionMarkerCondition                     m_markerCondition = TransitionMarkerCondition::AnyAllowed;
        EE_EXPOSE bool                                          m_onlyCheckEventsFromActiveBranch = false;
        EE_EXPOSE bool                                          m_matchOnlySpecificMarkerID = false;
        EE_EXPOSE StringID                                      m_markerIDToMatch;
    };
}