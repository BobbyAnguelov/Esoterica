#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/Events/AnimationEvent_Foot.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class StateNode;

    //-------------------------------------------------------------------------

    enum class EventSearchMode : uint8_t
    {
        EE_REGISTER_ENUM

        SearchAll = 0,
        OnlySearchStateEvents,
        OnlySearchAnimEvents,
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API GenericEventConditionNode : public BoolValueNode
    {
    public:

        enum class Operator : uint8_t
        {
            EE_REGISTER_ENUM

            Or = 0,
            And,
        };

    public:

        struct EE_ENGINE_API Settings : public BoolValueNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_sourceStateNodeIdx, m_eventIDs, m_operator, m_searchMode );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
            Operator                                    m_operator = Operator::Or;
            EventSearchMode                             m_searchMode = EventSearchMode::SearchAll;
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

    class EE_ENGINE_API GenericEventPercentageThroughNode : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings : public BoolValueNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_sourceStateNodeIdx, m_eventID, m_preferHighestPercentageThrough );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
            bool                                        m_preferHighestPercentageThrough = false;
            StringID                                    m_eventID;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
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
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_sourceStateNodeIdx, m_phaseCondition, m_preferHighestPercentageThrough );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
            FootEvent::PhaseCondition                   m_phaseCondition = FootEvent::PhaseCondition::LeftFootDown;
            bool                                        m_preferHighestPercentageThrough = false;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
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
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_sourceStateNodeIdx, m_phaseCondition, m_preferHighestPercentageThrough );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
            FootEvent::PhaseCondition                   m_phaseCondition = FootEvent::PhaseCondition::LeftFootDown;
            bool                                        m_preferHighestPercentageThrough = false;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        StateNode const*                                m_pSourceStateNode = nullptr;
        float                                           m_result = -1.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SyncEventConditionNode : public BoolValueNode
    {
    public:

        enum class TriggerMode : uint8_t
        {
            EE_REGISTER_ENUM

            ExactlyAtEventIndex,
            GreaterThanEqualToEventIndex,
        };

        struct EE_ENGINE_API Settings : public BoolValueNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_sourceStateNodeIdx, m_syncEventIdx, m_triggerMode );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
            TriggerMode                                 m_triggerMode = TriggerMode::ExactlyAtEventIndex;
            int32_t                                     m_syncEventIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

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
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_sourceStateNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                                     m_sourceStateNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        StateNode*                                      m_pSourceStateNode = nullptr;
        float                                           m_result = 0.0f;
    };
}