#pragma once

#include "Animation_RuntimeGraphNode_Passthrough.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API StateNode final : public PoseNode
    {
        friend class TransitionNode;

    public:

        enum class TransitionState : uint8_t
        {
            None,
            TransitioningIn,
            TransitioningOut,
        };
        //-------------------------------------------------------------------------

        struct TimedEvent
        {
            EE_SERIALIZE( m_ID, m_timeValue );

            TimedEvent() = default;

            TimedEvent( StringID ID, Seconds value )
                : m_ID ( ID )
                , m_timeValue( value )
            {}

            StringID                                    m_ID;
            Seconds                                     m_timeValue;
        };

        //-------------------------------------------------------------------------

        struct EE_ENGINE_API Settings : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_childNodeIdx, m_entryEvents, m_executeEvents, m_exitEvents, m_timedRemainingEvents, m_timedElapsedEvents, m_layerBoneMaskNodeIdx, m_layerWeightNodeIdx, m_isOffState );

        public:

            virtual void InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const override;

        public:

            int16_t                              m_childNodeIdx = InvalidIndex;
            TInlineVector<StringID, 3>                  m_entryEvents;
            TInlineVector<StringID, 3>                  m_executeEvents;
            TInlineVector<StringID, 3>                  m_exitEvents;
            TInlineVector<TimedEvent, 1>                m_timedRemainingEvents;
            TInlineVector<TimedEvent, 1>                m_timedElapsedEvents;
            int16_t                              m_layerBoneMaskNodeIdx = InvalidIndex;
            int16_t                              m_layerWeightNodeIdx = InvalidIndex;
            bool                                        m_isOffState = false;
        };

    public:

        virtual bool IsValid() const override { return PoseNode::IsValid() && m_pChildNode != nullptr && m_pChildNode->IsValid(); }
        virtual SyncTrack const& GetSyncTrack() const override;

        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;
        virtual void DeactivateBranch( GraphContext& context ) override;

        // State info
        inline float GetElapsedTimeInState() const { return m_elapsedTimeInState; }
        inline SampledEventRange GetSampledGraphEventRange() const { return m_sampledEventRange; }
        inline bool IsOffState() const { return GetSettings<StateNode>()->m_isOffState; }

        // Transitions
        inline void SetTransitioningState( TransitionState newState ) { m_transitionState = newState; }
        inline bool IsTransitioning() const { return m_transitionState != TransitionState::None; }
        inline bool IsTransitioningIn() const { return m_transitionState == TransitionState::TransitioningIn; }
        inline bool IsTransitioningOut() const { return m_transitionState == TransitionState::TransitioningOut; }

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        void StartTransitionIn( GraphContext& context );
        void StartTransitionOut( GraphContext& context );
        void SampleStateEvents( GraphContext& context );
        void UpdateLayerContext( GraphContext& context );

    private:

        PoseNode*                                       m_pChildNode = nullptr;
        SampledEventRange                               m_sampledEventRange;
        BoneMaskValueNode*                              m_pBoneMaskNode = nullptr;
        FloatValueNode*                                 m_pLayerWeightNode = nullptr;
        Seconds                                         m_elapsedTimeInState = 0.0f;
        TransitionState                                 m_transitionState = TransitionState::None;
        bool                                            m_isFirstStateUpdate = false;
    };
}