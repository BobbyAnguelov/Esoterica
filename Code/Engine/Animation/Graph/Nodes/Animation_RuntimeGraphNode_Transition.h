#pragma once

#include "Animation_RuntimeGraphNode_State.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "Engine/Animation/AnimationBlender.h"
#include "Engine/Math/Easing.h"

//-------------------------------------------------------------------------

namespace EE::Animation { struct GraphLayerContext; }

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    //-------------------------------------------------------------------------
    // State Machine Transition Node
    //-------------------------------------------------------------------------
    // Note: The duration of this node is always set to the target state's duration to ensure that any subsequent queries on this transition return the correct duration

    class EE_ENGINE_API TransitionNode final : public PoseNode
    {
        enum class SourceType
        {
            State,
            Transition,
            CachedPose
        };

    public:

        // Set of options for this transition, they are stored as flag since we want to save space
        // Note: not all options can be used together, the tools node will provide validation of the options
        enum class TransitionOptions : uint8_t
        {
            ClampDuration,

            Synchronized, // The time control mode: either sync, match or none
            MatchSourceTime, // The time control mode: either sync, match or none

            MatchSyncEventIndex, // Only checked if MatchSourceTime is set
            MatchSyncEventID, // Only checked if MatchSourceTime is set
            MatchSyncEventPercentage, // Only checked if MatchSourceTime is set
        };

        struct EE_ENGINE_API Settings : public PoseNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_targetStateNodeIdx, m_durationOverrideNodeIdx, m_syncEventOffsetOverrideNodeIdx, m_blendWeightEasingType, m_rootMotionBlend, m_duration, m_syncEventOffset, m_transitionOptions, m_startBoneMaskNodeIdx, m_boneMaskBlendInTimePercentage );

        public:

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            inline bool ShouldClampTransitionLength() const { return m_transitionOptions.IsFlagSet( TransitionOptions::ClampDuration ); }
            inline bool IsSynchronized() const { return m_transitionOptions.IsFlagSet( TransitionOptions::Synchronized ); }

            inline bool ShouldMatchSourceTime() const { return m_transitionOptions.IsFlagSet( TransitionOptions::MatchSourceTime ); }
            inline bool ShouldMatchSyncEventIndex() const { return m_transitionOptions.IsFlagSet( TransitionOptions::MatchSyncEventIndex ); }
            inline bool ShouldMatchSyncEventID() const { return m_transitionOptions.IsFlagSet( TransitionOptions::MatchSyncEventID ); }
            inline bool ShouldMatchSyncEventPercentage() const { return m_transitionOptions.IsFlagSet( TransitionOptions::MatchSyncEventPercentage ); }

        public:

            int16_t                             m_targetStateNodeIdx = InvalidIndex;
            int16_t                             m_durationOverrideNodeIdx = InvalidIndex;
            int16_t                             m_syncEventOffsetOverrideNodeIdx = InvalidIndex;
            int16_t                             m_startBoneMaskNodeIdx = InvalidIndex;
            Seconds                             m_duration = 0;
            Percentage                          m_boneMaskBlendInTimePercentage = 0.33f;
            float                               m_syncEventOffset = 0;
            TBitFlags<TransitionOptions>        m_transitionOptions;
            Math::Easing::Type                  m_blendWeightEasingType = Math::Easing::Type::Linear;
            RootMotionBlendMode                 m_rootMotionBlend = RootMotionBlendMode::Blend;
        };

    public:

        virtual SyncTrack const& GetSyncTrack() const override { return m_syncTrack; }
        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;

        // Secondary initialization
        //-------------------------------------------------------------------------

        // Start a transition from a state source node - will initialize the target state. Pass by value of the source node result is intentional
        GraphPoseNodeResult StartTransitionFromState( GraphContext& context, GraphPoseNodeResult const& sourceNodeResult, StateNode* SourceState, bool startCachingSourcePose );

        // Start a transition from a transition source node - will initialize the target state. Pass by value of the source node result is intentional
        GraphPoseNodeResult StartTransitionFromTransition( GraphContext& context, GraphPoseNodeResult const& sourceNodeResult, TransitionNode* SourceTransition, bool startCachingSourcePose );

        // Forceable transitions
        //-------------------------------------------------------------------------

        // Called before we start a new transition to allow us to start caching poses and switch to a "cached pose" source if needed
        void NotifyNewTransitionStarting( GraphContext& context, StateNode* pTargetStateNode, TInlineVector<StateNode const *, 20> const& forceableFutureTargetStates );

        // Transition Info
        //-------------------------------------------------------------------------

        inline bool IsComplete( GraphContext& context ) const;
        inline float GetProgressPercentage() const { return m_transitionProgress; }
        inline SourceType GetSourceType() const { return m_sourceType; }
        inline bool IsSourceATransition() const { return m_sourceType == SourceType::Transition; }
        inline bool IsSourceAState() const { return m_sourceType == SourceType::State; }
        inline bool IsSourceACachedPose() const { return m_sourceType == SourceType::CachedPose; }

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        // Initialize and update the target state. Note: the source node result pass-by-value is intentional
        GraphPoseNodeResult InitializeTargetStateAndUpdateTransition( GraphContext& context, GraphPoseNodeResult sourceNodeResult );

        void UpdateProgress( GraphContext& context, bool isInitializing = false );
        void UpdateProgressClampedSynchronized( GraphContext& context, SyncTrackTimeRange const& updateRange, bool isInitializing = false );
        void UpdateLayerContext( GraphContext& context, GraphLayerContext const& sourceLayerContext );

        GraphPoseNodeResult UpdateUnsynchronized( GraphContext& context );
        GraphPoseNodeResult UpdateSynchronized( GraphContext& context, SyncTrackTimeRange const& updateRange );

        void StartCachingSourcePose( GraphContext& context );
        void RegisterSourceCachePoseTask( GraphContext& context, GraphPoseNodeResult& sourceNodeResult );
        void RegisterPoseTasksAndUpdateRootMotion( GraphContext& context, GraphPoseNodeResult const& sourceResult, GraphPoseNodeResult const& targetResult, GraphPoseNodeResult& outResult );

        inline void CalculateBlendWeight()
        {
            auto pSettings = GetSettings<TransitionNode>();
            m_blendWeight = EvaluateEasingFunction( pSettings->m_blendWeightEasingType, m_transitionProgress );
            m_blendWeight = Math::Clamp( m_blendWeight, 0.0f, 1.0f );
        }

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

        // Source Node
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE TransitionNode* GetSourceTransitionNode() { EE_ASSERT( IsSourceATransition() ); return reinterpret_cast<TransitionNode*>( m_pSourceNode ); }

        EE_FORCE_INLINE StateNode* GetSourceStateNode() { EE_ASSERT( IsSourceAState() ); return reinterpret_cast<StateNode*>( m_pSourceNode ); }

        void EndSourceTransition( GraphContext& context );
    private:

        PoseNode*                               m_pSourceNode = nullptr;
        StateNode*                              m_pTargetNode = nullptr;
        FloatValueNode*                         m_pDurationOverrideNode = nullptr;
        FloatValueNode*                         m_pEventOffsetOverrideNode = nullptr;
        BoneMaskValueNode*                      m_pStartBoneMaskNode = nullptr;
        BoneMask                                m_boneMask;
        SyncTrack                               m_syncTrack;
        float                                   m_transitionProgress = 0;
        float                                   m_transitionLength = 0; // This is either time in seconds, or percentage of the sync track
        float                                   m_syncEventOffset = 0;
        float                                   m_blendWeight = 0;

        UUID                                    m_cachedPoseBufferID;
        SourceType                              m_sourceType = SourceType::State;
        BoneMaskTaskList                        m_boneMaskTaskList;

        #if EE_DEVELOPMENT_TOOLS
        int16_t                                 m_rootMotionActionIdxSource = InvalidIndex;
        int16_t                                 m_rootMotionActionIdxTarget = InvalidIndex;
        #endif
    };
}