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
            ForcedTransitionAllowed,

            Synchronized, // The time control mode: either sync, match or none
            MatchSourceTime, // The time control mode: either sync, match or none

            MatchSyncEventIndex, // Only checked if MatchSourceTime is set
            MatchSyncEventID, // Only checked if MatchSourceTime is set
            MatchSyncEventPercentage, // Only checked if MatchSourceTime is set
        };

        struct InitializationOptions
        {
            GraphPoseNodeResult                 m_sourceNodeResult;
            bool                                m_shouldCachePose = false;
        };

        struct EE_ENGINE_API Settings : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_targetStateNodeIdx, m_durationOverrideNodeIdx, m_syncEventOffsetOverrideNodeIdx, m_blendWeightEasingType, m_rootMotionBlend, m_duration, m_syncEventOffset, m_transitionOptions );

        public:

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            inline bool ShouldClampDuration() const { return m_transitionOptions.IsFlagSet( TransitionOptions::ClampDuration ); }
            inline bool IsForcedTransitionAllowed() const { return m_transitionOptions.IsFlagSet( TransitionOptions::ForcedTransitionAllowed ); }
            inline bool IsSynchronized() const { return m_transitionOptions.IsFlagSet( TransitionOptions::Synchronized ); }

            inline bool ShouldMatchSourceTime() const { return m_transitionOptions.IsFlagSet( TransitionOptions::MatchSourceTime ); }
            inline bool ShouldMatchSyncEventIndex() const { return m_transitionOptions.IsFlagSet( TransitionOptions::MatchSyncEventIndex ); }
            inline bool ShouldMatchSyncEventID() const { return m_transitionOptions.IsFlagSet( TransitionOptions::MatchSyncEventID ); }
            inline bool ShouldMatchSyncEventPercentage() const { return m_transitionOptions.IsFlagSet( TransitionOptions::MatchSyncEventPercentage ); }

        public:

            int16_t                             m_targetStateNodeIdx = InvalidIndex;
            int16_t                             m_durationOverrideNodeIdx = InvalidIndex;
            int16_t                             m_syncEventOffsetOverrideNodeIdx = InvalidIndex;
            Math::Easing::Type                  m_blendWeightEasingType = Math::Easing::Type::Linear;
            RootMotionBlendMode                 m_rootMotionBlend = RootMotionBlendMode::Blend;

            Seconds                             m_duration = 0;
            float                               m_syncEventOffset = 0;

            TBitFlags<TransitionOptions>        m_transitionOptions;
        };

    public:

        virtual SyncTrack const& GetSyncTrack() const override { return m_syncTrack; }
        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;

        // Transition Info
        GraphPoseNodeResult StartTransitionFromState( GraphContext& context, InitializationOptions const& options, StateNode* SourceState );
        GraphPoseNodeResult StartTransitionFromTransition( GraphContext& context, InitializationOptions const& options, TransitionNode* SourceTransition, bool bForceTransition );
        inline bool IsComplete( GraphContext& context ) const;
        inline SourceType GetSourceType() const { return m_sourceType; }

        inline bool IsForcedTransitionAllowed() const { return GetSettings<TransitionNode>()->IsForcedTransitionAllowed(); }
        inline bool IsSourceATransition() const { return m_sourceType == SourceType::Transition; }
        inline bool IsSourceAState() const { return m_sourceType == SourceType::State; }
        inline bool IsSourceACachedPose() const { return m_sourceType == SourceType::CachedPose; }

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        GraphPoseNodeResult InitializeTargetStateAndUpdateTransition( GraphContext& context, InitializationOptions const& options );

        void UpdateProgress( GraphContext& context, bool isInitializing = false );
        void UpdateProgressClampedSynchronized( GraphContext& context, SyncTrackTimeRange const& updateRange, bool isInitializing = false );
        void UpdateLayerContext( GraphContext& context, GraphLayerContext const& sourceLayerContext, GraphLayerContext const& targetLayerContext );

        GraphPoseNodeResult UpdateUnsynchronized( GraphContext& context );
        GraphPoseNodeResult UpdateSynchronized( GraphContext& context, SyncTrackTimeRange const& updateRange );

        void UpdateCachedPoseBufferIDState( GraphContext& context );
        void TransferAdditionalPoseBufferIDs( TInlineVector<UUID, 2>& outInheritedCachedPoseBufferIDs );
        void RegisterPoseTasksAndUpdateRootMotion( GraphContext& context, GraphPoseNodeResult const& sourceResult, GraphPoseNodeResult const& targetResult, GraphPoseNodeResult& outResult );

        inline void CalculateBlendWeight()
        {
            auto pSettings = GetSettings<TransitionNode>();
            m_blendWeight = EvaluateEasingFunction( pSettings->m_blendWeightEasingType, m_transitionProgress );
            m_blendWeight = Math::Clamp( m_blendWeight, 0.0f, 1.0f );
        }

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( GraphStateRecorder& recorder ) override;
        virtual void RestoreGraphState( GraphStateRecording const& recording ) override;
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
        BoneMask                                m_boneMask;
        SyncTrack                               m_syncTrack;
        float                                   m_transitionProgress = 0;
        float                                   m_transitionDuration = 0; // This is either time in seconds, or percentage of the sync track
        float                                   m_syncEventOffset = 0;
        float                                   m_blendWeight = 0;

        UUID                                    m_cachedPoseBufferID;                   // The buffer we are currently caching to
        UUID                                    m_sourceCachedPoseBufferID;             // The buffer we are reading from (in the case of an interrupted transition)
        TInlineVector<UUID, 2>                  m_inheritedCachedPoseBufferIDs;         // All cached buffers we have inherited when canceling a transition (need to be kept alive for one frame)
        float                                   m_sourceCachedPoseBlendWeight = 0.0f;

        SourceType                              m_sourceType = SourceType::State;

        #if EE_DEVELOPMENT_TOOLS
        int16_t                                 m_rootMotionActionIdxSource = InvalidIndex;
        int16_t                                 m_rootMotionActionIdxTarget = InvalidIndex;
        #endif
    };
}