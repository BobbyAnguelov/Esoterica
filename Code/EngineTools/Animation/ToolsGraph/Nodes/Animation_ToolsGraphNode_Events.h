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
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID ) override;

    private:

        EE_REFLECT();
        EventConditionOperator                          m_operator = EventConditionOperator::Or;

        EE_REFLECT();
        IDEventConditionNode::SearchRule                m_searchRule = IDEventConditionNode::SearchRule::SearchAll;

        EE_REFLECT();
        bool                                            m_onlyCheckEventsFromActiveBranch = false;

        EE_REFLECT( "Category" : "Conditions", "CustomEditor" : "AnimGraph_ID");
        TVector<StringID>                               m_eventIDs;
    };

    //-------------------------------------------------------------------------

    class StateEventConditionToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( StateEventConditionToolsNode );

        struct Condition : public IReflectedType
        {
            EE_REFLECT_TYPE( Condition );

            EE_REFLECT( "CustomEditor" : "AnimGraph_ID" );
            StringID                                    m_eventID;

            EE_REFLECT();
            StateEventTypeCondition                     m_type = StateEventTypeCondition::Any;
        };

    public:

        StateEventConditionToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "State Event Condition"; }
        virtual char const* GetCategory() const override { return "Events"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const override;
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID ) override;

    private:

        EE_REFLECT();
        EventConditionOperator                          m_operator = EventConditionOperator::Or;

        EE_REFLECT();
        bool                                            m_onlyCheckEventsFromActiveBranch = false;

        EE_REFLECT( "Category" : "Conditions" );
        TVector<Condition>                              m_conditions;
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
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID ) override { if ( m_eventID == oldID ) { VisualGraph::ScopedNodeModification snm( this ); m_eventID = newID; } }

    private:

        EE_REFLECT();
        EventPriorityRule                             m_priorityRule = EventPriorityRule::HighestWeight;

        EE_REFLECT();
        bool                                          m_onlyCheckEventsFromActiveBranch = false;

        EE_REFLECT( "CustomEditor" : "AnimGraph_ID" );
        StringID                                      m_eventID;
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

        EE_REFLECT();
        FootEvent::PhaseCondition                     m_phaseCondition = FootEvent::PhaseCondition::LeftFootDown;

        EE_REFLECT();
        bool                                          m_onlyCheckEventsFromActiveBranch = false;
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

        EE_REFLECT();
        FootEvent::PhaseCondition                     m_phaseCondition = FootEvent::PhaseCondition::LeftFootDown;

        EE_REFLECT();
        EventPriorityRule                             m_priorityRule = EventPriorityRule::HighestWeight;

        EE_REFLECT();
        bool                                          m_onlyCheckEventsFromActiveBranch = false;
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

        EE_REFLECT();
        SyncEventIndexConditionNode::TriggerMode      m_triggerMode = SyncEventIndexConditionNode::TriggerMode::ExactlyAtEventIndex;

        EE_REFLECT();
        int32_t                                       m_syncEventIdx = InvalidIndex;
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
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID ) override { if ( m_markerIDToMatch == oldID ) { VisualGraph::ScopedNodeModification snm( this ); m_markerIDToMatch = newID; } }

    private:

        EE_REFLECT();
        TransitionMarkerCondition                     m_markerCondition = TransitionMarkerCondition::AnyAllowed;

        EE_REFLECT();
        bool                                          m_onlyCheckEventsFromActiveBranch = false;

        EE_REFLECT();
        bool                                          m_matchOnlySpecificMarkerID = false;

        EE_REFLECT( "CustomEditor" : "AnimGraph_ID" );
        StringID                                      m_markerIDToMatch;
    };
}