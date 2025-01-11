#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class TransitionNode;
    class StateNode;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API StateMachineNode final : public PoseNode
    {

    public:

        using StateIndex = int16_t;

        struct TransitionDefinition
        {
            EE_SERIALIZE( m_targetStateIdx, m_transitionNodeIdx, m_conditionNodeIdx, m_canBeForced );

            StateIndex                                              m_targetStateIdx = InvalidIndex;
            int16_t                                                 m_conditionNodeIdx = InvalidIndex;
            int16_t                                                 m_transitionNodeIdx = InvalidIndex;
            bool                                                    m_canBeForced = false;
        };

        struct StateDefinition
        {
            EE_SERIALIZE( m_stateNodeIdx, m_entryConditionNodeIdx, m_transitionDefinition );

            int16_t                                                 m_stateNodeIdx = InvalidIndex;
            int16_t                                                 m_entryConditionNodeIdx = InvalidIndex;
            TInlineVector<TransitionDefinition, 5>                  m_transitionDefinition;
        };

        //-------------------------------------------------------------------------

        struct EE_ENGINE_API Definition : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_stateDefinition, m_defaultStateIndex );

        public:

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            TInlineVector<StateDefinition, 5>                       m_stateDefinition;
            StateIndex                                              m_defaultStateIndex = InvalidIndex;
        };

        //-------------------------------------------------------------------------

        struct TransitionInfo
        {
            TransitionNode*                                         m_pTransitionNode = nullptr;
            BoolValueNode*                                          m_pConditionNode = nullptr;
            StateIndex                                              m_targetStateIdx = InvalidIndex;
            bool                                                    m_canBeForced = false;
        };

        struct StateInfo
        {
            StateNode*                                              m_pStateNode = nullptr;
            BoolValueNode*                                          m_pEntryConditionNode = nullptr;
            TInlineVector<TransitionInfo, 5>                        m_transitions;
            bool                                                    m_hasForceableTransitions = false;
        };

    public:

        virtual bool IsValid() const override;
        virtual SyncTrack const& GetSyncTrack() const override;

        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        StateIndex SelectDefaultState( GraphContext& context ) const;

        void InitializeTransitionConditions( GraphContext& context );
        void ShutdownTransitionConditions( GraphContext& context );

        void EvaluateTransitions( GraphContext& context, SyncTrackTimeRange const* pUpdateRange, GraphPoseNodeResult& NodeResult, int8_t sourceTasksStartMarker );

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        TInlineVector<StateInfo, 10>                                m_states;
        TransitionNode*                                             m_pActiveTransition = nullptr;
        StateIndex                                                  m_activeStateIndex = InvalidIndex;
    };
}