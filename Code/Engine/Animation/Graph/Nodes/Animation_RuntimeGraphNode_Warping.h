#pragma once

#include "Animation_RuntimeGraphNode_AnimationClip.h"
#include "Engine/Animation/Events/AnimationEvent_Warp.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API OrientationWarpNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_clipReferenceNodeIdx, m_angleOffsetValueNodeIdx );

            virtual void InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const override;

            int16_t                     m_clipReferenceNodeIdx = InvalidIndex;
            int16_t                     m_angleOffsetValueNodeIdx = InvalidIndex;
        };

    private:

        virtual SyncTrack const& GetSyncTrack() const override { return m_pClipReferenceNode->GetSyncTrack(); }
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;

        void PerformWarp( GraphContext& context );
        Transform SampleWarpedRootMotion( GraphContext& context ) const;

    private:

        AnimationClipReferenceNode*     m_pClipReferenceNode = nullptr;
        FloatValueNode*                 m_pAngleOffsetValueNode = nullptr;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API TargetWarpNode final : public PoseNode
    {
    public:

        struct WarpSection
        {
            int32_t                             m_startFrame = 0;
            int32_t                             m_endFrame = 0;
            Transform                           m_deltaTransform;
            TVector<float>                      m_progressPerFrameAlongSection;
            float                               m_length = 0;
            WarpEvent::Type                     m_type;
            WarpEvent::TranslationWarpMode      m_translationWarpMode;

            #if EE_DEVELOPMENT_TOOLS
            mutable Vector                      m_debugPoints[4];
            #endif
        };

        enum class SamplingMode
        {
            EE_REGISTER_ENUM

            Inaccurate = 0, // Just returns the delta for each update from the warped root motion
            Accurate,       // Will return a delta that attempts to move the character to the expected world space position
        };

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_clipReferenceNodeIdx, m_targetValueNodeIdx, m_samplingPositionErrorThresholdSq, m_samplingMode, m_allowTargetUpdate );

            virtual void InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const override;

            int16_t                     m_clipReferenceNodeIdx = InvalidIndex;
            int16_t                     m_targetValueNodeIdx = InvalidIndex;
            float                       m_samplingPositionErrorThresholdSq = 0.0f; // The threshold at which we switch from accurate to inaccurate sampling
            SamplingMode                m_samplingMode = SamplingMode::Inaccurate;
            bool                        m_allowTargetUpdate = false;
        };

    private:

        virtual SyncTrack const& GetSyncTrack() const override { return m_pClipReferenceNode->GetSyncTrack(); }
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;
        void UpdateShared( GraphContext& context, GraphPoseNodeResult& result );

        bool TryReadTarget( GraphContext& context );
        bool CalculateWarpedRootMotion( GraphContext& context, Percentage startTime );
        void UpdateWarp( GraphContext& context );

        void WarpRotation( int32_t sectionIdx, Transform const& sectionStartTransform, Transform const& target );
        void WarpMotion( int32_t sectionIdx, Transform const& startTransform, Transform const& endTransform );

        #if EE_DEVELOPMENT_TOOLS
        virtual void DrawDebug( GraphContext& graphContext, Drawing::DrawContext& drawCtx ) override;
        #endif

        // Translation Warps
        void WarpTranslationBezier( WarpSection const& ws, Transform const& sectionStartTransform, Transform const& sectionEndTransform );
        void WarpTranslationHermite( WarpSection const& ws, Transform const& sectionStartTransform, Transform const& sectionEndTransform );
        void WarpTranslationFeaturePreserving( WarpSection const& ws, Transform const& sectionStartTransform, Transform const& sectionEndTransform );

    private:

        AnimationClipReferenceNode*     m_pClipReferenceNode = nullptr;
        TargetValueNode*                m_pTargetValueNode = nullptr;
        Transform                       m_warpTarget;
        Transform                       m_warpStartTransform;
        TVector<Transform>              m_deltaTransforms;
        TVector<Transform>              m_inverseDeltaTransforms;
        RootMotionData                  m_warpedRootMotion;
        TInlineVector<WarpSection, 3>   m_warpSections;
        SamplingMode                    m_samplingMode;

        #if EE_DEVELOPMENT_TOOLS
        Transform                       m_actualStartTransform;
        #endif
    };
}