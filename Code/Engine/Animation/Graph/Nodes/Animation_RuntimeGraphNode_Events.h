#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/Events/AnimationEvent_Foot.h"
#include "Engine/Animation/Events/AnimationEvent_Transition.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class StateNode;

    //-------------------------------------------------------------------------

    // Combined set of rules flags used in the event condition nodes
    enum class EventConditionRules
    {
        LimitSearchToSourceState = 0,
        IgnoreInactiveEvents,
        PreferHighestWeight,
        PreferHighestProgress,
        OperatorOr,
        OperatorAnd,
        SearchOnlyStateEvents,
        SearchOnlyAnimEvents,
        SearchBothStateAndAnimEvents,
    };

    //-------------------------------------------------------------------------

    // Check for a given ID - coming either from a state event or generic event
    class EE_ENGINE_API IDEventConditionNode : public BoolValueNode
    {

    public:

        struct EE_ENGINE_API Settings : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_sourceStateNodeIdx, m_rules, m_eventIDs );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
            TBitFlags<EventConditionRules>              m_rules;
            TInlineVector<StringID, 5>                  m_eventIDs;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

        bool TryMatchTags( GraphContext& context ) const;

    private:

        StateNode const*                                m_pSourceStateNode = nullptr;
        bool                                            m_result = false;
    };

    //-------------------------------------------------------------------------

    // Check for a given state event - coming either from a state event or generic event
    class EE_ENGINE_API StateEventConditionNode : public BoolValueNode
    {
    public:

        struct Condition
        {
            EE_SERIALIZE( m_eventID, m_eventTypeCondition );

            StringID                                    m_eventID;
            StateEventTypeCondition                     m_eventTypeCondition;
        };

    public:

        struct EE_ENGINE_API Settings : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_sourceStateNodeIdx, m_rules, m_conditions );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
            TBitFlags<EventConditionRules>              m_rules;
            TInlineVector<Condition, 5>                 m_conditions;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

        bool TryMatchTags( GraphContext& context ) const;

    private:

        StateNode const*                                m_pSourceStateNode = nullptr;
        bool                                            m_result = false;
    };

    //-------------------------------------------------------------------------

    // Get the percentage through a generic event with a specific ID
    class EE_ENGINE_API GenericEventPercentageThroughNode : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_sourceStateNodeIdx, m_rules, m_eventID );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
            TBitFlags<EventConditionRules>              m_rules;
            StringID                                    m_eventID;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        StateNode const*                                m_pSourceStateNode = nullptr;
        float                                           m_result = -1.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FootEventConditionNode : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Settings : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_sourceStateNodeIdx, m_rules );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
            FootEvent::PhaseCondition                   m_phaseCondition = FootEvent::PhaseCondition::LeftFootDown;
            TBitFlags<EventConditionRules>              m_rules;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        StateNode const*                                m_pSourceStateNode = nullptr;
        bool                                            m_result = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FootstepEventPercentageThroughNode : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_sourceStateNodeIdx, m_phaseCondition, m_rules );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
            FootEvent::PhaseCondition                   m_phaseCondition = FootEvent::PhaseCondition::LeftFootDown;
            TBitFlags<EventConditionRules>              m_rules;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        StateNode const*                                m_pSourceStateNode = nullptr;
        float                                           m_result = -1.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SyncEventIndexConditionNode : public BoolValueNode
    {
    public:

        enum class TriggerMode : uint8_t
        {
            EE_REFLECT_ENUM

            ExactlyAtEventIndex,
            GreaterThanEqualToEventIndex,
        };

        struct EE_ENGINE_API Settings : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_sourceStateNodeIdx, m_syncEventIdx, m_triggerMode );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
            TriggerMode                                 m_triggerMode = TriggerMode::ExactlyAtEventIndex;
            int32_t                                     m_syncEventIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        StateNode*                                      m_pSourceStateNode = nullptr;
        bool                                            m_result = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CurrentSyncEventNode : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_sourceStateNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        StateNode*                                      m_pSourceStateNode = nullptr;
        float                                           m_result = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API TransitionEventConditionNode : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Settings : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_sourceStateNodeIdx, m_markerCondition, m_markerIDToMatch, m_rules );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            StringID                                    m_markerIDToMatch = StringID();
            TBitFlags<EventConditionRules>              m_rules;
            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
            TransitionMarkerCondition                   m_markerCondition = TransitionMarkerCondition::AnyAllowed;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        StateNode const*                                m_pSourceStateNode = nullptr;
        bool                                            m_result = false;
    };
}