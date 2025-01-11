#pragma once

#include "Animation_RuntimeGraphNode_State.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "Engine/Animation/AnimationBlender.h"
#include "Engine/Animation/TaskSystem/Animation_TaskPosePool.h"
#include "Base/Math/Easing.h"

//-------------------------------------------------------------------------

namespace EE::Animation { struct GraphLayerContext; }

//-------------------------------------------------------------------------

namespace EE::Animation
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

            PreferClosestSyncEventID, // Only checked if MatchSyncEventID is set, will prefer the closest matching sync event rather than the first found
        };

        struct EE_ENGINE_API Definition : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_targetStateNodeIdx, m_durationOverrideNodeIdx, m_syncEventOffsetOverrideNodeIdx, m_blendWeightEasingOp, m_rootMotionBlend, m_duration, m_syncEventOffset, m_transitionOptions, m_targetSyncIDNodeIdx, m_startBoneMaskNodeIdx, m_boneMaskBlendInTimePercentage );

        public:

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            inline bool ShouldClampTransitionLength() const { return m_transitionOptions.IsFlagSet( TransitionOptions::ClampDuration ); }
            inline bool IsSynchronized() const { return m_transitionOptions.IsFlagSet( TransitionOptions::Synchronized ); }

            inline bool ShouldMatchSourceTime() const { return m_transitionOptions.IsFlagSet( TransitionOptions::MatchSourceTime ); }
            inline bool ShouldMatchSyncEventIndex() const { return m_transitionOptions.IsFlagSet( TransitionOptions::MatchSyncEventIndex ); }
            inline bool ShouldMatchSyncEventID() const { return m_transitionOptions.IsFlagSet( TransitionOptions::MatchSyncEventID ); }
            inline bool ShouldMatchSyncEventPercentage() const { return m_transitionOptions.IsFlagSet( TransitionOptions::MatchSyncEventPercentage ); }

            inline bool ShouldPreferClosestSyncEventID() const { return m_transitionOptions.IsFlagSet( TransitionOptions::PreferClosestSyncEventID ); }

        public:

            int16_t                             m_targetStateNodeIdx = InvalidIndex;
            int16_t                             m_durationOverrideNodeIdx = InvalidIndex;
            int16_t                             m_syncEventOffsetOverrideNodeIdx = InvalidIndex;
            int16_t                             m_startBoneMaskNodeIdx = InvalidIndex;
            Seconds                             m_duration = 0;
            Percentage                          m_boneMaskBlendInTimePercentage = 0.33f;
            float                               m_syncEventOffset = 0;
            TBitFlags<TransitionOptions>        m_transitionOptions;
            int16_t                             m_targetSyncIDNodeIdx = InvalidIndex;
            Math::Easing::Operation             m_blendWeightEasingOp = Math::Easing::Operation::Linear;
            RootMotionBlendMode                 m_rootMotionBlend = RootMotionBlendMode::Blend;
        };

        struct StartOptions
        {
            StartOptions( GraphPoseNodeResult const& sourceNodeResult )
                : m_sourceNodeResult( sourceNodeResult )
            {}

            GraphPoseNodeResult const&          m_sourceNodeResult;
            SyncTrackTimeRange const*           m_pUpdateRange = nullptr;
            int8_t                              m_sourceTasksStartMarker = InvalidIndex;
            PoseNode*                           m_pSourceNode = nullptr;
            bool                                m_isSourceATransition = false;
            bool                                m_startCachingSourcePose = false;
        };

    public:

        virtual SyncTrack const& GetSyncTrack() const override { return m_syncTrack; }
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        // Secondary initialization
        //-------------------------------------------------------------------------

        // Initialize internal state and update the target state - the update range is only set if the parent state machine was being updated via a sync'ed update
        GraphPoseNodeResult InitializeTargetStateAndUpdateTransition( GraphContext& context, StartOptions const& startOptions );

        // Forceable transitions
        //-------------------------------------------------------------------------

        // Called before we start a new transition to allow us to start caching poses and switch to a "cached pose" source if needed
        void NotifyNewTransitionStarting( GraphContext& context, StateNode* pTargetStateNode, TInlineVector<StateNode const *, 20> const& forceableFutureTargetStatesUsingCachedPoses );

        // Transition Info
        //-------------------------------------------------------------------------

        bool IsComplete( GraphContext& context ) const;
        inline float GetProgressPercentage() const { return m_transitionProgress; }

        inline bool IsSourceATransition() const { return m_sourceType == SourceType::Transition; }
        inline bool IsSourceAState() const { return m_sourceType == SourceType::State; }
        inline bool IsSourceACachedPose() const { return m_sourceType == SourceType::CachedPose; }

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        // Updates the layer context for this transition and any source states
        void UpdateLayerContext( GraphLayerContext* pSourceAndResultLayerContext, GraphLayerContext const* pTargetLayerContext );

        void StartCachingSourcePose( GraphContext& context );
        void RegisterPoseTasksAndUpdateRootMotion( GraphContext& context, GraphPoseNodeResult const& sourceResult, GraphPoseNodeResult const& targetResult, GraphPoseNodeResult& outResult );

        inline void CalculateBlendWeight()
        {
            auto pDefinition = GetDefinition<TransitionNode>();
            if ( m_transitionDuration == 0.0f )
            {
                m_blendWeight = 1.0f;
            }
            else
            {
                m_blendWeight = Math::Easing::Evaluate( pDefinition->m_blendWeightEasingOp, m_transitionProgress );
                m_blendWeight = Math::Clamp( m_blendWeight, 0.0f, 1.0f );
            }
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
        IDValueNode*                            m_pTargetSyncIDNode = nullptr;
        SyncTrack                               m_syncTrack;
        float                                   m_transitionProgress = 0;
        float                                   m_transitionDuration = 0; // This is either time in seconds, or percentage of the sync track
        float                                   m_syncEventOffset = 0;
        float                                   m_blendWeight = 0;
        Seconds                                 m_blendedDuration = 0.0f;

        CachedPoseID                            m_cachedPoseBufferID;
        SourceType                              m_sourceType = SourceType::State;
        BoneMaskTaskList                        m_boneMaskTaskList;

        #if EE_DEVELOPMENT_TOOLS
        bool                                    m_recreateCachedPoseBuffer = false; // Needed in the scenario where we are restoring from a recorded state
        int16_t                                 m_rootMotionActionIdxSource = InvalidIndex;
        int16_t                                 m_rootMotionActionIdxTarget = InvalidIndex;
        #endif
    };
}