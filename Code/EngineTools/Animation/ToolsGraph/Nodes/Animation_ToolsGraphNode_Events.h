#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_IDs.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Events.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class IDEventConditionToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( IDEventConditionToolsNode );

    public:

        IDEventConditionToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Generic Event Condition"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const override;

    private:

        EE_REFLECT() IDEventConditionNode::Operator                m_operator = IDEventConditionNode::Operator::Or;
        EE_REFLECT() IDEventConditionNode::SearchRule              m_searchRule = IDEventConditionNode::SearchRule::SearchAll;
        EE_REFLECT() bool                                          m_onlyCheckEventsFromActiveBranch = false;
        EE_REFLECT() TVector<StringID>                             m_eventIDs;
    };

    //-------------------------------------------------------------------------

    class GenericEventPercentageThroughToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( GenericEventPercentageThroughToolsNode );

    public:

        GenericEventPercentageThroughToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Generic Event Percentage Through"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const override { outIDs.emplace_back( m_eventID ); }

    private:

        EE_REFLECT() EventPriorityRule                             m_priorityRule = EventPriorityRule::HighestWeight;
        EE_REFLECT() bool                                          m_onlyCheckEventsFromActiveBranch = false;
        EE_REFLECT() StringID                                      m_eventID;
    };

    //-------------------------------------------------------------------------

    class FootEventConditionToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FootEventConditionToolsNode );

    public:

        FootEventConditionToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Footstep Condition"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_REFLECT() FootEvent::PhaseCondition                     m_phaseCondition = FootEvent::PhaseCondition::LeftFootDown;
        EE_REFLECT() bool                                          m_onlyCheckEventsFromActiveBranch = false;
    };

    //-------------------------------------------------------------------------

    class FootstepEventPercentageThroughToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FootstepEventPercentageThroughToolsNode );

    public:

        FootstepEventPercentageThroughToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Footstep Percentage Through"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_REFLECT() FootEvent::PhaseCondition                     m_phaseCondition = FootEvent::PhaseCondition::LeftFootDown;
        EE_REFLECT() EventPriorityRule                             m_priorityRule = EventPriorityRule::HighestWeight;
        EE_REFLECT() bool                                          m_onlyCheckEventsFromActiveBranch = false;
    };

    //-------------------------------------------------------------------------

    class SyncEventIndexConditionToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( SyncEventIndexConditionToolsNode );

    public:

        SyncEventIndexConditionToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Sync Event Index Condition"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_REFLECT() SyncEventIndexConditionNode::TriggerMode      m_triggerMode = SyncEventIndexConditionNode::TriggerMode::ExactlyAtEventIndex;
        EE_REFLECT() int32_t                                       m_syncEventIdx = InvalidIndex;
    };

    //-------------------------------------------------------------------------

    class CurrentSyncEventToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( CurrentSyncEventToolsNode );

    public:

        CurrentSyncEventToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Current Sync Event"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class TransitionEventConditionToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( TransitionEventConditionToolsNode );

    public:

        TransitionEventConditionToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Transition Event Condition"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const override { outIDs.emplace_back( m_markerIDToMatch ); }

    private:

        EE_REFLECT() TransitionMarkerCondition                     m_markerCondition = TransitionMarkerCondition::AnyAllowed;
        EE_REFLECT() bool                                          m_onlyCheckEventsFromActiveBranch = false;
        EE_REFLECT() bool                                          m_matchOnlySpecificMarkerID = false;
        EE_REFLECT() StringID                                      m_markerIDToMatch;
    };
}